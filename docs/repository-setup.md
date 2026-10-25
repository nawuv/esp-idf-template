# Repository setup

These settings require administrator access and are not enabled by copying files.

## Template and branches

1. Mark the repository as a template under Settings > General.
2. Keep `main` as the default branch and push `develop` for contributor integration.
3. Protect both branches with pull requests and required successful CI checks.
4. For main, require the host-tests, documentation, and all firmware matrix checks.
   Consider review requirements and prohibit force pushes.

Creating a local branch does not configure GitHub rulesets. Select actual status
check names after the first CI run. Keep release commit messages meaningful.

## Documentation hosting

1. Set Settings > Pages > Source to **GitHub Actions**.
2. Create Actions repository variable `ENABLE_PAGES` with value `true`.
3. Allow the `github-pages` environment to deploy from `main`.
4. Push a documentation change to main through the normal reviewed workflow.

Pages availability depends on repository visibility and GitHub plan. Without the
variable, CI still builds and uploads the documentation as a preview artifact.
The repository URL is derived from `github.repository` during CI for template copies.

## GitHub Releases

Set Actions repository variable `ENABLE_RELEASES` to `true`. The release job receives
scoped `contents: write` permission and uses the built-in `GITHUB_TOKEN`. It does not
need an npm token. Repository or organization policies may still restrict tag creation.

Release jobs run only for main pushes after validation. Do not enable deployment
variables until the repository settings and documentation have been reviewed.

## Local documentation preview

```sh
python3 -m venv .venv-docs
. .venv-docs/bin/activate
python -m pip install -r requirements-docs.txt
mkdocs serve
```

For a renamed repository, CI supplies its source URL automatically. For local
customization, set `REPOSITORY_URL` or change `mkdocs.yml`.
