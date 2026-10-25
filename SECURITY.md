# Security

Do not publish credentials, private keys, or exploitable device-specific details in
public issues. Until a private reporting channel is configured, use the repository
owner's established private contact. Do not assume this prototype is production-ready.

Provision unique device credentials, verify TLS peers and firmware authenticity,
restrict MQTT topics per device, and define credential rotation before deployment.
Secure boot and flash encryption require board-specific provisioning and recovery
planning; they are not enabled by this template.

## Release-tool dependency audit

The configured release plugins do not publish npm packages. However, semantic-release
also installs its default npm plugin as a dependency, which brings a bundled npm CLI.
The current lockfile reports two development-tool audit findings inside that bundle:
[undici](https://github.com/advisories/GHSA-4qv8-2p9j-2685) (high) and
[ip-address](https://github.com/advisories/GHSA-rxp7-6hxx-x8q5) (moderate).
The npm publishing plugin is not enabled, and Node packages are not linked into
firmware. Standard audit fixes cannot replace npm's bundled copies. Track upstream
patched bundles when updating tooling; do not enable npm publishing without review.
