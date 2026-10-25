import assert from "node:assert/strict";
import { analyzeCommits } from "@semantic-release/commit-analyzer";
import config, { commitParser } from "../release.config.mjs";

const logger = { log() {} };
const cases = [
  ["docs: improve quick start", null],
  ["fix: reject stale readings", "patch"],
  ["feat: add a sensor adapter", "minor"],
  ["refactor!: change namespaces", "major"],
  ["feat(mqtt)!: change topics", "major"],
  ["feat: change output contract\n\nBREAKING CHANGE: output adapters now return errors", "major"]
];
for (const [message, expected] of cases) {
  const actual = await analyzeCommits({ parserOpts: commitParser }, { commits: [{ message }], logger });
  assert.equal(actual, expected, message);
}
assert.deepEqual(config.branches, ["main", { name: "develop", prerelease: "dev", channel: "dev" }]);
assert.ok(!config.plugins.some(plugin => String(Array.isArray(plugin) ? plugin[0] : plugin).includes("npm")));
console.log("Release rules verified: docs skip, fix patch, feature minor, breaking major.");
