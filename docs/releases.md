# Releases, versions, and branches

```text
feature/* -> PR -> develop -> release PR -> main
                    dev                    stable
```

Feature PRs may squash with a Conventional Commit title. Promote develop to main
with **Create a merge commit**, preserving the commits that release analysis uses.
After a stable release or hotfix, merge main back into develop. Planning fails if
develop does not contain the current main tip, preventing a missing stable baseline. Do not squash the
entire integration history into an uninformative promotion commit. Linear-history
rules must remain disabled because release promotion uses merge commits.

## One build version

| Build | Example | Publication |
| --- | --- | --- |
| PR, feature branch, manual run, or releases disabled | `0.0.0-dev+gabc1234` | CI artifacts only |
| Local modified tracked files | `0.0.0-dev+gabc1234.dirty` | Local build only |
| Relevant develop push with releases enabled | `2.0.0-dev.1` | GitHub pre-release on the dev channel |
| Relevant main push with releases enabled | `2.0.0` | GitHub stable release |

Examples are illustrative. The next number is computed from actual tags and commits;
no script hardcodes the next major. Docs-only changes normally produce snapshots and
no release. Existing `v1.0.0` history is preserved. Breaking namespace changes after
that tag imply a major increment under the configured policy. A source release is
not certification of physical hardware, networking timing, or production safety.

There is no single C++ equivalent of pyproject.toml. Here `CMakeLists.txt` owns the
build, `idf_component.yml` describes component dependencies, `dependencies.lock`
records dependency resolution, and `release.config.mjs` owns source release policy.
`package.json` is Node-based release tooling only; nothing is published to npm.

The template source and the sample firmware in a release use the same computed
version. A project created with Use this template starts its own Git history and
product versioning policy; it does not inherit upstream release tags automatically.
A new repository's first automatic release normally starts at 1.0.0.

## Pipeline

1. The read-only version job fetches branch/tag/channel-note history. Release planning
   clones it into a disposable local Git remote and runs semantic-release in dry-run
   mode with only the configured commit analyzer. No publication token or GitHub
   publish plugin is used. The source checkout and remote tags are not modified.
2. `release-plan.json` records source SHA, repository, branch, channel, version, and
   whether publication is expected. Non-release events always get a snapshot version.
3. Each firmware matrix build receives `-DPROJECT_VER=<planned version>`. Packaging
   rejects a different application version and reads the version directly from the
   ESP-IDF application image descriptor as well as the build description. The manifest
   records version and source.
4. `ci-required` succeeds only if every validation job succeeds. Failure, cancellation,
   or unexpected skipping of a prerequisite fails the gate.
5. Only opted-in main/develop push runs can publish. The publisher skips superseded
   branch tips. A verifyRelease plugin checks that the new semantic-release decision,
   firmware manifest, repository, branch, source SHA, and checksums match the plan
   **before a new tag is created**. The GitHub plugin publishes that tested archive.

Planning and publishing are separated in time. If another release changes the
calculated version, publication fails closed: rerun the **entire workflow** to build
new matching artifacts, not just the publishing job. Publishing across both channels
is serialized. Publication is not an atomic transaction: if GitHub fails after tag
creation, inspect the tag and release before retrying. Never rewrite a published tag.

CI runs on branch pushes, PR changes including title edits, and manual dispatch.
Tag pushes do not recursively rebuild/release. `GITHUB_TOKEN` has read-only defaults;
only the release job gets `contents: write`. No PR job receives release write access.
Git notes carry semantic-release channel history and must be fetched alongside tags.

## Commit policy

- `fix:` -> patch; `feat:` -> minor.
- `refactor!:` / `feat(scope)!:` or a `BREAKING CHANGE:` footer -> major.
- Documentation, tests, CI, and chores do not normally publish a new version.

The PR-title check enforces supported Conventional Commit syntax. Analyzer and release
notes share parser options, so breaking squash titles retain their meaning. For merge
commits, the original commit messages still matter; a valid PR title alone cannot
repair unrelated messages preserved by a merge.

## ESP-IDF version injection

`cmake/ProjectVersion.cmake` runs before the ESP-IDF project declaration. CI supplies
`PROJECT_VER`; local builds use a clean exact v-prefixed release tag or a source-SHA
snapshot. Modified tracked files receive `.dirty`. Untracked files are not considered
part of the compiled source identity. Without Git, the fallback is explicitly
`0.0.0-dev+g0000000`. Supported versions must fit the app descriptor's 31 ASCII bytes
plus terminating NUL; overlong values fail rather than silently truncate.

```sh
idf.py -DPROJECT_VER=2.0.0-dev.1 build
```

This command is an explicit local override, not a release publication. Use a separate
build directory when comparing versions because CMake cache overrides persist.
The compiled project description is checked during packaging. If a consumer enables
`CONFIG_APP_PROJECT_VER_FROM_CONFIG`, packaging will reject any version conflict.

## Assets and activation

Releases attach the root ESP32-S3 firmware archive, checksums, and manifest. Generated
sdkconfig and credentials are excluded. The archive includes flash offsets and
instructions, not an OTA deployment. Verify checksums and hardware compatibility
before flashing; physical outputs remain simulated in this template.

`ENABLE_RELEASES=true` opts a repository into both stable and development publication.
New template copies have no activation variable and build snapshots safely. The
existing upstream variable, if present, should be reviewed before merging this change:
a develop merge with relevant history will publish a pre-release automatically.
`ENABLE_PAGES=true` remains an independent main-only documentation deployment switch.

See [Repository setup](repository-setup.md) for protections and the promotion checklist.
