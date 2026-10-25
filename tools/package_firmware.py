#!/usr/bin/env python3
"""Package exact build inputs without generated sdkconfig or credentials."""

import argparse
import gzip
import hashlib
import io
import json
import re
import tarfile
from pathlib import Path


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def image_application_version(image: bytes) -> str:
    # ESP-IDF 5.4 app descriptor: image header (24), segment header (8),
    # descriptor prefix (16), then the NUL-terminated version field (32).
    if len(image) < 80 or image[0] != 0xE9 or int.from_bytes(image[32:36], 'little') != 0xABCD5432:
        raise ValueError('missing ESP-IDF application image descriptor')
    field = image[48:80]
    if b'\0' not in field or field[0] == 0:
        raise ValueError('invalid application image version field')
    return field.split(b'\0', 1)[0].decode('ascii')


def package(build: Path, output: Path, source_sha: str, sdk_version: str, expected_version: str | None = None) -> dict:
    if not re.fullmatch(r'[0-9a-f]{40}', source_sha):
        raise ValueError('source SHA must be a full Git commit SHA')
    build = build.resolve()
    description = json.loads((build / 'project_description.json').read_text())
    if expected_version is not None and description['project_version'] != expected_version:
        raise ValueError('firmware version does not match the planned version')
    flash = json.loads((build / 'flasher_args.json').read_text())
    files = {}
    names = {'flash_args', 'flasher_args.json', *flash['flash_files'].values()}
    if not flash['flash_files']:
        raise ValueError('no firmware binaries were found')
    for name in sorted(names):
        path = Path(name)
        resolved = (build / path).resolve()
        if path.is_absolute() or '..' in path.parts or not resolved.is_relative_to(build):
            raise ValueError(f'unsafe build artifact path: {name}')
        if name not in ('flash_args', 'flasher_args.json') and path.suffix != '.bin':
            raise ValueError(f'expected a firmware binary: {name}')
        files[path.as_posix()] = resolved.read_bytes()
    application = files.get(description['app_bin'])
    if application is None or image_application_version(application) != description['project_version']:
        raise ValueError('application image version differs from build description')
    target = description['target']
    if target != flash['extra_esptool_args']['chip']:
        raise ValueError('build target and flashing target do not match')
    manifest = {
        'schema_version': 1,
        'source_sha': source_sha,
        'project_name': description['project_name'],
        'application_version': description['project_version'],
        'target': target,
        'sdk_version': sdk_version,
        'files': {name: sha256(data) for name, data in sorted(files.items())},
    }
    manifest_bytes = (json.dumps(manifest, indent=2, sort_keys=True) + '\n').encode()
    files['manifest.json'] = manifest_bytes
    files['FLASHING.md'] = f'''# Flash this firmware

Source commit: `{source_sha}`
Target: `{target}`
SDK: `{sdk_version}`

Verify the archive with `sha256sum -c SHA256SUMS`, then extract it into a new directory.
Use a compatible board and make connected outputs safe before flashing.
From the extracted directory, with esptool installed in the ESP-IDF environment:

```sh
python -m esptool --chip {target} -p PORT write_flash @flash_args
```

Replace PORT with the actual serial port. Flash arguments preserve the build's
bootloader, partition table, and application offsets. This is a full serial flash
bundle, not an OTA payload or a promise of compatibility with every board.
'''.encode()
    output.mkdir(parents=True, exist_ok=True)
    archive = output / 'firmware.tar.gz'
    with archive.open('wb') as raw:
        with gzip.GzipFile(filename='', mode='wb', fileobj=raw, mtime=0) as compressed:
            with tarfile.open(fileobj=compressed, mode='w') as tar:
                for name, data in sorted(files.items()):
                    info = tarfile.TarInfo(name)
                    info.size = len(data)
                    info.mode = 0o644
                    tar.addfile(info, io.BytesIO(data))
    (output / 'manifest.json').write_bytes(manifest_bytes)
    (output / 'SHA256SUMS').write_text(
        f'{sha256(archive.read_bytes())}  firmware.tar.gz\n'
        f'{sha256(manifest_bytes)}  manifest.json\n'
    )
    return manifest


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--source-sha', required=True)
    parser.add_argument('--sdk-version', required=True)
    parser.add_argument('--expected-version')
    args = parser.parse_args()
    manifest = package(args.build_dir, args.output, args.source_sha, args.sdk_version, args.expected_version)
    print(f"Packaged {manifest['project_name']} ({manifest['target']}) at {args.output}")


if __name__ == '__main__':
    main()
