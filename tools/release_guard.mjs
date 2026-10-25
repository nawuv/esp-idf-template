/** Fail before tag creation if the publish decision differs from the tested firmware. */
import assert from "node:assert/strict";
import { createHash } from "node:crypto";
import { readFileSync } from "node:fs";
import path from "node:path";

export function checkRelease({ plan, manifest, version, sha, branch, repository }) {
  assert.equal(plan.schema_version, 1, "Unknown release plan schema");
  assert.equal(plan.publish, true, "This build is not a release candidate");
  assert.ok(["main", "develop"].includes(branch), "Not a release branch");
  assert.equal(plan.branch, branch, "Release branch changed");
  assert.equal(plan.repository, repository, "Release repository changed");
  assert.equal(plan.channel, branch === "develop" ? "dev" : "stable", "Release channel changed");
  assert.equal(plan.version, version, "Version changed since build; rerun the entire workflow");
  assert.equal(plan.tag, `v${version}`, "Release tag mismatch");
  assert.equal(plan.source_sha, sha, "Release source changed");
  assert.equal(manifest.source_sha, sha, "Firmware source changed");
  assert.equal(manifest.application_version, version, "Firmware version differs from release");
}

export function verifyRelease(_options, { cwd, env, nextRelease, branch }) {
  const directory = path.join(cwd, "release-assets");
  const plan = JSON.parse(readFileSync(path.join(directory, "release-plan.json")));
  const manifest = JSON.parse(readFileSync(path.join(directory, "manifest.json")));
  checkRelease({ plan, manifest, version: nextRelease.version, sha: nextRelease.gitHead,
    branch: branch.name, repository: env.GITHUB_REPOSITORY });
  assert.equal(nextRelease.gitHead, env.GITHUB_SHA, "Workflow SHA differs from release source");
  const expectedFiles = new Set(["firmware.tar.gz", "manifest.json"]);
  for (const line of readFileSync(path.join(directory, "SHA256SUMS"), "utf8").trim().split("\n")) {
    const match = /^([0-9a-f]{64})  (firmware\.tar\.gz|manifest\.json)$/.exec(line);
    assert.ok(match && expectedFiles.delete(match[2]), "Unexpected checksum entry");
    const actual = createHash("sha256").update(readFileSync(path.join(directory, match[2]))).digest("hex");
    assert.equal(actual, match[1], "Firmware bundle integrity failure");
  }
  assert.equal(expectedFiles.size, 0, "Missing artifact checksums");
}
