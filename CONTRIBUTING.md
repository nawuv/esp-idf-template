# Contributing

Use English for code comments, documentation, commit messages, and pull requests.
Keep portable application code independent of ESP-IDF. Add meaningful tests for
behavior changes and run `bash tools/test-host.sh` plus an ESP32-S3 firmware build.
State whether hardware validation was performed. Use clang-format 18 with the
project's LLVM-derived `.clang-format` settings. Run:

```sh
python3 tools/format.py
python3 tools/format.py --check
```

The wrapper also enforces blank lines around header/namespace boundaries and
between adjacent type definitions. It excludes generated build/dependency trees.
CI checks the same command. Use 4-space indentation, a 100-column limit, multiline
function bodies/enums, and separated definition blocks. Access labels are indented
4 spaces, matching member declarations. Namespace contents are not indented.
Never commit secrets, generated build outputs, or personal SDK configuration.
Keep new abstractions tied to a concrete use case and document public contracts.

## Documentation, examples, and releases

Run `mkdocs build --strict` after installing `requirements-docs.txt`. Build examples
with `idf.py -C examples/<name> build` after setting the ESP32-S3 target. Test bundle
packaging with `python3 -m unittest discover -s tests/tools -v`.

Use Node 24.22.1 and `npm ci --ignore-scripts && npm run check:release` for automation
changes. Node is development tooling, not a firmware dependency. Work from develop
and use Conventional Commit messages; see `docs/releases.md` and `docs/repository-setup.md`.
