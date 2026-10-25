/** Publish only the current tested branch tip; the guard plugin checks before tagging. */
import assert from "node:assert/strict";
import { execFileSync } from "node:child_process";
import { readFileSync } from "node:fs";
import semanticRelease from "semantic-release";

const branch = process.env.GITHUB_REF_NAME;
assert.ok(["main", "develop"].includes(branch), "Not a release branch");
assert.equal(process.env.GITHUB_EVENT_NAME, "push", "Only push builds publish");
const git = (...args) => execFileSync("git", args, { encoding: "utf8", stdio: ["ignore", "pipe", "pipe"] }).trim();
git("fetch", "origin", `+refs/heads/${branch}:refs/remotes/origin/${branch}`, "+refs/notes/*:refs/notes/*", "--tags");
if (git("rev-parse", "HEAD") !== git("rev-parse", `origin/${branch}`)) {
  console.log("Skipping superseded branch build.");
} else {
  const plan = JSON.parse(readFileSync("release-assets/release-plan.json"));
  const result = await semanticRelease();
  assert.equal(result?.nextRelease?.version, plan.version, "Release decision changed; inspect tags and rerun the entire workflow");
}
