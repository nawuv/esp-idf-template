#!/usr/bin/env python3
"""Check owned include boundaries and CMake component dependencies."""

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
ALLOWED = {
    'domain': set(),
    'application': {'domain'},
    'drivers': {'domain', 'application'},
    'platform': {'domain', 'application'},
    'configuration': {'domain'},
    'networking': {'domain', 'application', 'configuration'},
}
PORTABLE = {'domain', 'application', 'drivers'}


def main() -> int:
    errors = []
    for layer, allowed in ALLOWED.items():
        component = ROOT / 'components' / layer
        for path in component.rglob('*'):
            if not path.is_file() or 'test' in path.parts or path.suffix not in {'.hpp', '.cpp'}:
                continue
            source = path.read_text()
            for include in re.findall(r'^\s*#include\s+"([^"]+)"', source, re.M):
                if include.split('/')[0] in ALLOWED:
                    dependency = include.split('/')[0]
                    if dependency not in allowed | {layer}:
                        errors.append(f'{path.relative_to(ROOT)}: forbidden include {include}')
                elif layer in PORTABLE:
                    errors.append(f'{path.relative_to(ROOT)}: nonportable include {include}')
            if path.suffix == '.hpp' and re.search(r'\busing\s+namespace\b', source):
                errors.append(f'{path.relative_to(ROOT)}: public using-namespace directive')
        cmake = (component / 'CMakeLists.txt').read_text()
        for match in re.finditer(r'(?:PRIV_)?REQUIRES\s+([^)]*)', cmake):
            for dependency in match[1].split():
                if dependency in ALLOWED and dependency not in allowed | {layer}:
                    errors.append(f'{layer}: forbidden component dependency {dependency}')
                elif layer in PORTABLE and dependency not in ALLOWED:
                    errors.append(f'{layer}: nonportable component dependency {dependency}')
    for error in errors:
        print(error)
    if not errors:
        print('Architecture boundaries passed.')
    return bool(errors)


if __name__ == '__main__':
    raise SystemExit(main())
