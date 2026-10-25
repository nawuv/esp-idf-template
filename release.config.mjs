export default {
  branches: ["main"],
  tagFormat: "v${version}",
  plugins: [
    "@semantic-release/commit-analyzer",
    "@semantic-release/release-notes-generator",
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
