// Share parsing rules so a breaking PR title also works after squash merge.
export const commitParser = {
  headerPattern: /^(\w+)(?:\(([^)]+)\))?!?: (.+)$/,
  breakingHeaderPattern: /^(\w+)(?:\(([^)]+)\))?!: (.+)$/,
  headerCorrespondence: ["type", "scope", "subject"],
  noteKeywords: ["BREAKING CHANGE", "BREAKING-CHANGE"]
};

export default {
  branches: ["main", { name: "develop", prerelease: "dev", channel: "dev" }],
  tagFormat: "v${version}",
  plugins: [
    ["@semantic-release/commit-analyzer", { parserOpts: commitParser }],
    ["@semantic-release/release-notes-generator", { parserOpts: commitParser }],
    "./tools/release_guard.mjs",
    ["@semantic-release/github", {
      // Avoid posting comments or opening issues from release automation.
      successComment: false,
      failComment: false,
      failTitle: false,
      releasedLabels: false,
      assets: [
        { path: "release-assets/firmware.tar.gz", label: "ESP32-S3 firmware and flash instructions" },
        { path: "release-assets/SHA256SUMS", label: "SHA-256 checksums" },
        { path: "release-assets/manifest.json", label: "Build provenance" }
      ]
    }]
  ]
};
