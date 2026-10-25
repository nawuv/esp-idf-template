/** Plan a release against a disposable local Git remote, without publication credentials. */
import { execFileSync } from "node:child_process";
import { appendFileSync, mkdtempSync, mkdirSync, rmSync, writeFileSync } from "node:fs";
import { tmpdir } from "node:os";
import path from "node:path";
import { fileURLToPath, pathToFileURL } from "node:url";
import { Writable } from "node:stream";
import semanticRelease from "semantic-release";
import config, { commitParser } from "../release.config.mjs";

const git = (cwd, ...args) => execFileSync("git", args, { cwd, encoding: "utf8", stdio: ["ignore", "pipe", "pipe"] }).trim();
export function validateVersion(version) {
  // esp_app_desc_t.version has 32 bytes including the terminating NUL.
  if (!/^(?:(0|[1-9]\d*)\.(0|[1-9]\d*)\.(0|[1-9]\d*)(?:-dev\.[1-9]\d*)?|0\.0\.0-dev\+g[0-9a-f]{7}(?:\.dirty)?)$/.test(version) || Buffer.byteLength(version) > 31) {
    throw new Error("Firmware version must be supported SemVer and fit in 31 bytes");
  }
  return version;
}

export async function planRelease({ cwd = process.cwd(), branch, enabled = false, event = "push", repository = "" }) {
  const sha = git(cwd, "rev-parse", "HEAD");
  const plan = { schema_version: 1, source_sha: sha, repository, branch, publish: false,
    version: `0.0.0-dev+g${sha.slice(0, 7)}`, tag: null, channel: "snapshot" };
  validateVersion(plan.version);
  if (!enabled || event !== "push" || !["main", "develop"].includes(branch)) return plan;
  const temporary = mkdtempSync(path.join(tmpdir(), "espkit-release-"));
  const remote = path.join(temporary, "snapshot.git");
  const work = path.join(temporary, "work");
  try {
    const bundle = path.join(temporary, "history.bundle");
    git(cwd, "bundle", "create", bundle, "--all", "HEAD");
    git(cwd, "clone", "--mirror", bundle, remote);
    for (const name of ["main", "develop"]) {
      let ref;
      try { ref = git(cwd, "rev-parse", "--verify", `refs/remotes/origin/${name}`); }
      catch { try { ref = git(cwd, "rev-parse", "--verify", `refs/heads/${name}`); } catch { continue; } }
      git(remote, "update-ref", `refs/heads/${name}`, ref);
    }
    git(remote, "update-ref", `refs/heads/${branch}`, sha);
    if (branch === "develop") {
      try { git(remote, "merge-base", "--is-ancestor", "refs/heads/main", sha); }
      catch { throw new Error("Merge main into develop before a dev release so stable tags and fixes remain in its history"); }
    }
    git(cwd, "clone", "--no-hardlinks", "--branch", branch, remote, work);
    const quiet = new Writable({ write(_chunk, _encoding, done) { done(); } });
    // No GitHub plugin, tokens, or remote GitHub writes. semantic-release's push
    // permission probe and branch fetches target only the disposable file remote.
    const result = await semanticRelease({ ...config, dryRun: true, ci: false,
      repositoryUrl: pathToFileURL(remote).href,
      plugins: [[fileURLToPath(import.meta.resolve("@semantic-release/commit-analyzer")), { parserOpts: commitParser }]] },
    { cwd: work, env: { PATH: process.env.PATH, HOME: temporary }, stdout: quiet, stderr: quiet });
    if (result?.nextRelease) {
      plan.version = validateVersion(result.nextRelease.version);
      plan.publish = true;
      plan.tag = result.nextRelease.gitTag;
      plan.channel = branch === "develop" ? "dev" : "stable";
    }
    return plan;
  } finally { rmSync(temporary, { recursive: true, force: true }); }
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  const plan = await planRelease({ branch: process.env.GITHUB_REF_NAME,
    enabled: process.env.RELEASE_ENABLED === "true", event: process.env.GITHUB_EVENT_NAME,
    repository: process.env.GITHUB_REPOSITORY ?? "" });
  if (process.env.GITHUB_SHA && plan.source_sha !== process.env.GITHUB_SHA) throw new Error("Checkout/source SHA mismatch");
  validateVersion(plan.version);
  mkdirSync("local/ci", { recursive: true });
  writeFileSync("local/ci/release-plan.json", `${JSON.stringify(plan, null, 2)}\n`);
  if (process.env.GITHUB_OUTPUT) appendFileSync(process.env.GITHUB_OUTPUT,
    `version=${plan.version}\npublish=${plan.publish}\n`);
  console.log(`Firmware ${plan.version}; ${plan.publish ? "release planned" : "snapshot only"}`);
}
