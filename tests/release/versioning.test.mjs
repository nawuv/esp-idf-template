import assert from "node:assert/strict";
import { execFileSync } from "node:child_process";
import { mkdtempSync, mkdirSync, rmSync, writeFileSync } from "node:fs";
import { tmpdir } from "node:os";
import path from "node:path";
import { test } from "node:test";
import { createHash } from "node:crypto";
import { fileURLToPath, pathToFileURL } from "node:url";
import { Writable } from "node:stream";
import semanticRelease from "semantic-release";
import config, { commitParser } from "../../release.config.mjs";
import { planRelease, validateVersion } from "../../tools/release_plan.mjs";
import { checkRelease, verifyRelease } from "../../tools/release_guard.mjs";

function repository(t) {
  const cwd = mkdtempSync(path.join(tmpdir(), "espkit-version-test-"));
  t.after(() => rmSync(cwd, { recursive: true, force: true }));
  const git = (...args) => execFileSync("git", args, { cwd, encoding: "utf8", stdio: ["ignore", "pipe", "pipe"] }).trim();
  git("init", "-b", "main"); git("config", "user.name", "Version Test"); git("config", "user.email", "test@example.invalid");
  git("commit", "--allow-empty", "-m", "feat: initial template"); git("tag", "v1.0.0");
  git("branch", "develop");
  return { cwd, git };
}

test("PRs and disabled publication produce snapshots without release decisions", async t => {
  const { cwd, git } = repository(t);
  git("commit", "--allow-empty", "-m", "feat: new feature");
  for (const options of [{ enabled: false, event: "push" }, { enabled: true, event: "pull_request" }]) {
    const plan = await planRelease({ cwd, branch: "main", ...options });
    assert.equal(plan.publish, false);
    validateVersion(plan.version);
    assert.equal(plan.version, `0.0.0-dev+g${git("rev-parse", "HEAD").slice(0, 7)}`);
  }
});

test("docs-only commits do not release and features bump stable minor", async t => {
  const { cwd, git } = repository(t);
  git("commit", "--allow-empty", "-m", "docs: improve guide");
  assert.equal((await planRelease({ cwd, branch: "main", enabled: true })).publish, false);
  git("commit", "--allow-empty", "-m", "feat: add a sensor");
  assert.equal((await planRelease({ cwd, branch: "main", enabled: true })).version, "1.1.0");
  assert.equal(git("tag", "--list"), "v1.0.0", "planning never tags the source repository");
});

test("develop increments dev versions and promotion preserves the breaking change", async t => {
  const { cwd, git } = repository(t);
  git("checkout", "develop");
  git("commit", "--allow-empty", "-m", "refactor!: change API");
  assert.equal((await planRelease({ cwd, branch: "develop", enabled: true })).version, "2.0.0-dev.1");
  git("tag", "v2.0.0-dev.1");
  git("notes", "--ref=refs/notes/semantic-release-v2.0.0-dev.1", "add", "-m", '{"channels":["dev"]}', "v2.0.0-dev.1");
  git("commit", "--allow-empty", "-m", "fix: preserve settings");
  assert.equal((await planRelease({ cwd, branch: "develop", enabled: true })).version, "2.0.0-dev.2");
  git("checkout", "main"); git("merge", "--no-ff", "develop", "-m", "chore: promote develop");
  assert.equal((await planRelease({ cwd, branch: "main", enabled: true })).version, "2.0.0");
});

test("a fresh template repository starts its own release history", async t => {
  const { cwd, git } = repository(t);
  git("tag", "-d", "v1.0.0");
  assert.equal((await planRelease({ cwd, branch: "main", enabled: true })).version, "1.0.0");
});

test("develop must include the stable branch history before planning", async t => {
  const { cwd, git } = repository(t);
  git("commit", "--allow-empty", "-m", "fix: stable hotfix");
  git("checkout", "develop"); git("commit", "--allow-empty", "-m", "feat: new sensing");
  await assert.rejects(planRelease({ cwd, branch: "develop", enabled: true }), /Merge main into develop/);
});

test("version metadata rejects truncation and unsupported formats", () => {
  assert.equal(validateVersion("2.0.0-dev.12"), "2.0.0-dev.12");
  for (const invalid of ["v2.0.0", "2.0", "2.0.0-dev.01", "2.0.0-dev.0", "2.0.0\n", "12345678901234567890.1.0-dev.123456789"]) {
    assert.throws(() => validateVersion(invalid));
  }
});

function candidate() {
  return { plan: { schema_version: 1, publish: true, version: "2.0.0-dev.1", tag: "v2.0.0-dev.1",
    source_sha: "a".repeat(40), branch: "develop", repository: "owner/repo", channel: "dev" },
  manifest: { application_version: "2.0.0-dev.1", source_sha: "a".repeat(40) },
  version: "2.0.0-dev.1", sha: "a".repeat(40), branch: "develop", repository: "owner/repo" };
}

test("release guard rejects stale version, different source, and cross-repository artifacts", () => {
  checkRelease(candidate());
  for (const key of ["version", "sha", "branch", "repository"]) {
    const value = candidate(); value[key] = "changed";
    assert.throws(() => checkRelease(value));
  }
  const value = candidate(); value.manifest.application_version = "0.1.0";
  assert.throws(() => checkRelease(value));
});

test("release guard refuses a tampered archive before publication", t => {
  const cwd = mkdtempSync(path.join(tmpdir(), "espkit-guard-test-"));
  t.after(() => rmSync(cwd, { recursive: true, force: true }));
  const { plan, manifest } = candidate();
  mkdirSync(path.join(cwd, "release-assets"));
  for (const [name, value] of [["release-plan.json", plan], ["manifest.json", manifest]]) {
    writeFileSync(path.join(cwd, "release-assets", name), JSON.stringify(value));
  }
  writeFileSync(path.join(cwd, "release-assets/SHA256SUMS"), `${"0".repeat(64)}  firmware.tar.gz\n`);
  writeFileSync(path.join(cwd, "release-assets/firmware.tar.gz"), "tampered");
  assert.throws(() => verifyRelease({}, { cwd, env: { GITHUB_REPOSITORY: "owner/repo", GITHUB_SHA: plan.source_sha },
    nextRelease: { version: plan.version, gitHead: plan.source_sha }, branch: { name: "develop" } }), /integrity/);
});


test("real local publication checks the plan before creating any new tag", async t => {
  const { cwd, git } = repository(t);
  const remote = path.join(cwd, "remote.git");
  git("clone", "--bare", cwd, remote); git("remote", "add", "origin", remote);
  git("checkout", "develop"); git("commit", "--allow-empty", "-m", "feat!: change protocol");
  git("push", "origin", "develop");
  const plan = await planRelease({ cwd, branch: "develop", enabled: true, repository: "owner/repo" });
  assert.equal(plan.version, "2.0.0-dev.1");
  const directory = path.join(cwd, "release-assets"); mkdirSync(directory);
  const manifest = JSON.stringify({ source_sha: plan.source_sha, application_version: plan.version });
  const archive = "test archive";
  writeFileSync(path.join(directory, "manifest.json"), manifest);
  writeFileSync(path.join(directory, "firmware.tar.gz"), archive);
  writeFileSync(path.join(directory, "SHA256SUMS"),
    [["firmware.tar.gz", archive], ["manifest.json", manifest]].map(([name, content]) =>
      `${createHash("sha256").update(content).digest("hex")}  ${name}`).join("\n") + "\n");
  const options = { ...config, ci: false, repositoryUrl: pathToFileURL(remote).href,
    plugins: [[fileURLToPath(import.meta.resolve("@semantic-release/commit-analyzer")), { parserOpts: commitParser }],
      fileURLToPath(new URL("../../tools/release_guard.mjs", import.meta.url))] };
  const quiet = new Writable({ write(_chunk, _encoding, done) { done(); } });
  const context = { cwd, env: { PATH: process.env.PATH, HOME: cwd,
    GITHUB_REPOSITORY: "owner/repo", GITHUB_SHA: plan.source_sha }, stdout: quiet, stderr: quiet };
  writeFileSync(path.join(directory, "release-plan.json"), JSON.stringify({ ...plan, version: "1.9.0" }));
  await assert.rejects(semanticRelease(options, context));
  assert.equal(git("tag", "--list", "v2.0.0-dev.*"), "", "a mismatch must not create a tag");
  writeFileSync(path.join(directory, "release-plan.json"), JSON.stringify(plan));
  assert.equal((await semanticRelease(options, context)).nextRelease.version, plan.version);
  assert.equal(git("ls-remote", "--tags", "origin", plan.tag).split("\t")[0], plan.source_sha);
});
