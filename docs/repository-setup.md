# Repository setup

Repository files describe automation; administrator settings are separate and do
not transfer automatically through Use this template. Inspect actual settings
before changing them. Never grant broad bypass or new tokens merely to make CI pass.

## Branch protection

After the new CI workflow has completed once, import
`.github/rulesets/protected-branches.json` under Settings > Rules > Rulesets,
or reproduce these rules in the UI:

| Rule | main and develop |
| --- | --- |
| Require a PR | Enabled |
| Required approving reviews | 0 while there is only one maintainer |
| Require conversation resolution | Enabled |
| Required status check | `ci-required`, expected source GitHub Actions |
| Require current base branch | Enabled |
| Block force pushes and deletion | Enabled |
| Require linear history | Disabled; release promotion preserves merge commits |
| Bypass actors | None |

The checked-in ruleset pins GitHub Actions app ID 15368, observed on this repository's
checks. It contains no organization/user IDs and can be imported into a template copy.
Verify the check's actual source and current GitHub schema when importing elsewhere.
No tag-creation restriction is added: semantic-release needs to create version tags
and channel notes with its scoped built-in token. Published tags must never be moved.

A required approving review of 1 would block a sole author from merging their own
PR. Add independent review when another maintainer is available. Signed-commit and
CODEOWNERS review requirements are deferred until contributor signing and ownership
are established. Do not add an administrator bypass as a substitute for review.

The new gate must exist on a PR before activating protection. Activating a required
check that an older main workflow cannot produce will block old-workflow hotfix PRs;
first promote the workflow through the normal release PR.

## Merge settings

Enable merge commits and squash merging. Set squash's default commit message to the
PR title (with description if desired). Feature PRs normally squash. Release PRs
from develop to main use **Create a merge commit**. Bring main changes back into
develop afterward. This is a documented operator policy; branch rules do not enforce
the source branch name or which allowed merge button is chosen.

Keep rebase merging disabled unless the team deliberately changes this policy.
Do not automatically delete develop after a release; the branch rules prevent its
deletion. Auto-merge is optional and is not needed to run CI.

## Activation and promotion checklist

1. Merge the initial versioning/CI PR into develop after `ci-required` succeeds.
   This bootstrap PR also synchronizes the existing main release history: use
   **Create a merge commit** for this PR, because squash would discard that ancestry.
2. Set the Actions repository variable `ENABLE_RELEASES=true` when ready to publish.
   If it is already true, the merge itself may publish the first dev pre-release.
3. Inspect the develop pre-release and check the manifest/application version equals
   the release version and source SHA. CI compilation is not physical validation.
4. Open a release PR from develop to main with a Conventional Commit title, for
   example `chore: promote develop to main`. Review breaking API notes and run CI.
5. Merge that PR with a merge commit, then verify the stable release artifacts.
6. Merge main back into develop. Change the repository default to main only after
   it contains the reviewed template. Mark Settings > General > Template repository
   when ready for other developers to use it.

The default branch controls what new template users receive. Keeping develop as
the default temporarily avoids directing them at the older main during promotion.
Default-branch and template settings are not silently changed by a workflow.

## Variables and permissions

`ENABLE_RELEASES=true` activates both release channels. `ENABLE_PAGES=true` enables
main-only Pages deployment; configure Settings > Pages > Source = GitHub Actions and
allow the github-pages environment to deploy from main. Neither variable needs an npm
or personal access token. The release job requests only `contents: write`; org policy
may still restrict publishing. Do not weaken protection or grant broad access on failure.

Dependabot configuration targets develop for Actions, npm release tooling, and pip
documentation dependencies. It groups weekly updates and does not auto-merge them.
Updates to ESP-IDF image digests, compiler tooling, and GoogleTest pins still need
an intentional baseline review. Dependabot reads configuration from the default
branch, so confirm activation after this configuration reaches that branch.

## Local checks

```sh
python3 tools/check_architecture.py
python3 tools/format.py --check
bash tools/test-host.sh
python3 -m unittest discover -s tests/tools -v
npm ci --ignore-scripts
npm run check:release
mkdocs build --strict
```

Node 24 and Git are required for the release-history tests. They use disposable
local repositories, never GitHub publication. For more detail see [Releases](releases.md).
