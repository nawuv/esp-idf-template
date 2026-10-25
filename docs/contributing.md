# Contributing

Use English for documentation, comments, and commit messages. Work on a feature
branch based on `develop`, then open a pull request targeting `develop`.

```sh
python3 tools/format.py
python3 tools/format.py --check
python3 tools/check_architecture.py
bash tools/test-host.sh
mkdocs build --strict
```

Use clang-format 18. Build affected examples and the ESP32-S3 firmware with ESP-IDF
5.4.2. Report hardware checks separately from compilation. Keep application workers
independent of SDK headers and inject dependencies through composition.

Use Conventional Commit titles such as `feat: add a sensor adapter`, `fix: reject
stale readings`, or `docs: explain wiring`. Stable releases analyze commits merged
to `main`. Preserve feature/fix titles in the release branch history; do not squash
an entire release into an uninformative `chore` title.

Read [releases](releases.md) and [repository setup](repository-setup.md). Never submit
credentials, private keys, generated configuration, or build directories.
