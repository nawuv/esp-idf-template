#!/usr/bin/env python3
"""Format owned C++ sources with clang-format 18 and declaration boundary spacing."""

import argparse
import re
import shutil
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


def declaration_spacing(source: str) -> str:
    # clang-format preserves these boundary blank lines but does not insert them.
    source = re.sub(r'(#pragma once)\n(?:[ \t]*\n)*', r'\1\n\n', source)
    source = re.sub(r'(?m)^(namespace(?: [\w:]+)? \{)\n(?:[ \t]*\n)*', r'\1\n\n', source)
    source = re.sub(r'(?m)(\S[^\n]*)\n(} // namespace[^\n]*)$', r'\1\n\n\2', source)
    source = re.sub(r'(?m)(#include [^\n]+)\n(?=namespace\b)', r'\1\n\n', source)
    source = re.sub(r'(?m)^(};)\n(?=(?:class|struct|enum|template)\b)', r'\1\n\n', source)
    return source


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true', help='report changes without writing files')
    args = parser.parse_args()
    formatter = shutil.which('clang-format-18') or shutil.which('clang-format')
    if not formatter:
        parser.error('clang-format 18 is required')
    version = subprocess.check_output([formatter, '--version'], text=True)
    if not re.search(r'version 18\.', version):
        parser.error(f'clang-format 18 is required; found {version.strip()}')

    changed = []
    for directory in ('components', 'main', 'tests', 'examples'):
        for path in sorted((ROOT / directory).rglob('*')):
            if path.suffix not in ('.hpp', '.cpp', '.h', '.c'):
                continue
            if any(part.startswith('build') or part == 'managed_components' for part in path.relative_to(ROOT).parts):
                continue
            original = path.read_text()
            formatted = original
            # A first pass may add namespace comments; normalize those boundaries too.
            for _ in range(2):
                formatted = subprocess.check_output(
                    [formatter, '--style=file', f'--assume-filename={path}'],
                    input=declaration_spacing(formatted), text=True,
                )
            if formatted != original:
                changed.append(str(path.relative_to(ROOT)))
                if not args.check:
                    path.write_text(formatted)
    for path in changed:
        print(path)
    if args.check and changed:
        print('Run: python3 tools/format.py')
        return 1
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
