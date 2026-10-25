# Releases and branches

```text
feature/* -> pull request -> develop -> release pull request -> main
```

`develop` is the integration branch. `main` is the stable branch and recommended
repository default so template users start from reviewed code. After an urgent
main-branch fix, bring the change back into develop. Branches are created locally;
remote branch protection is a separate administrator setting.

## What CI does

Pull requests, pushes, and manual runs check formatting, host tests, documentation,
root firmware, the Unity test application, and all examples. Each firmware build
is packaged with a source commit, build description, and checksums.

On a push to `main`, after all required jobs succeed:

- `ENABLE_PAGES=true` deploys the built documentation through GitHub Pages.
- `ENABLE_RELEASES=true` evaluates semantic-release and publishes when needed.

The variables are opt-in so a new template copy can run CI without deployment setup.
Develop pushes do not publish stable releases. Automatic pre-releases are not enabled.

## Version policy

semantic-release analyzes Conventional Commits. Features cause a minor release,
fixes cause a patch, and breaking changes cause a major release under the default
rules. Documentation-only changes do not normally publish a release. The first
automatic release normally starts at 1.0.0. Use `docs:` or `chore:` deliberately.
The workflow still evaluates each main push; it does not force a new version every time.

Release tags version the template source. The sample firmware's CMake application
version is separate and recorded in its manifest. Consumers choose their own
firmware product versioning policy.

## Assets and provenance

The release attaches the root ESP32-S3 firmware archive, its checksum file, and a
JSON manifest. Packaging preserves bootloader/partition paths and flash arguments.
The manifest records the exact source commit and SDK target. Generated sdkconfig is
not attached. Verify the archive checksum before extracting, then read
`FLASHING.md`. Only flash compatible hardware with outputs made safe.

Assets come from the same workflow run that passed CI. A stale main run is skipped
if the branch has advanced. Release jobs are serialized, and the firmware manifest
must match the checked-out source commit. Tags do not depend on a second workflow
triggered by the release token. OTA deployment is not part of GitHub Releases.

## Configuration

`package.json`, its lockfile, and `release.config.mjs` pin the Node-based automation.
Only commit analysis, release notes, and GitHub publishing plugins are used: no npm
package is published. CI uses Node 24. See [repository setup](repository-setup.md).
