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
        component = ROOT / 'components' / f'espkit_{layer}'
        for path in component.rglob('*'):
            if not path.is_file() or 'test' in path.parts or path.suffix not in {'.hpp', '.cpp'}:
                continue
            source = path.read_text()
            for include in re.findall(r'^\s*#include\s+"([^"]+)"', source, re.M):
                if include.startswith('espkit/'):
                    dependency = include.split('/')[1]
                    if dependency not in allowed | {layer}:
                        errors.append(f'{path.relative_to(ROOT)}: forbidden include {include}')
                elif layer in PORTABLE:
                    errors.append(f'{path.relative_to(ROOT)}: nonportable include {include}')
            if path.suffix == '.hpp' and re.search(r'\busing\s+namespace\b', source):
                errors.append(f'{path.relative_to(ROOT)}: public using-namespace directive')
        cmake = (component / 'CMakeLists.txt').read_text()
        for dependency in re.findall(r'\bespkit_(\w+)\b', cmake):
            if dependency not in allowed | {layer}:
                errors.append(f'espkit_{layer}: forbidden component dependency espkit_{dependency}')
        if layer in PORTABLE:
            match = re.search(r'(?:PRIV_)?REQUIRES\s+([^)]*)', cmake)
            if match:
                for dependency in match[1].split():
                    if not dependency.startswith('espkit_'):
                        errors.append(f'espkit_{layer}: nonportable component dependency {dependency}')
    for error in errors:
        print(error)
    if not errors:
        print('Architecture boundaries passed.')
    return bool(errors)


if __name__ == '__main__':
    raise SystemExit(main())
