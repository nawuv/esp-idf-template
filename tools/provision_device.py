#!/usr/bin/env python3
"""Generate a private NVS image; flashing requires explicit replacement acknowledgement."""

import argparse
import csv
import json
import os
import re
import subprocess
from pathlib import Path
from urllib.parse import urlsplit

# On-disk settings schema 1 in the espkit namespace. Firmware migrates a missing
# schema key in memory and does not rewrite NVS. Keep these keys and this version
# so already-provisioned devices, and firmware that requires schema == 1, still load.
SCHEMA_VERSION = 1
LIMITS = {'wifi_ssid': 32, 'wifi_pass': 63, 'mqtt_uri': 191, 'mqtt_user': 63,
          'mqtt_pass': 127, 'device_id': 63, 'site_id': 31}


def load_config(path: Path, allow_plaintext: bool = False) -> dict:
    config = json.loads(path.read_text())
    if not isinstance(config, dict) or set(config) - set(LIMITS):
        raise ValueError('configuration contains unknown keys')
    for name, limit in LIMITS.items():
        value = config.get(name, '' if name in ('mqtt_user', 'mqtt_pass') else None)
        if not isinstance(value, str) or '\0' in value or len(value.encode('utf-8')) > limit:
            raise ValueError(f'invalid or oversized field: {name}')
        config[name] = value
    if not config['wifi_ssid'] or len(config['wifi_pass'].encode('utf-8')) < 8:
        raise ValueError('SSID and a WPA2/WPA3 password of 8-63 bytes are required')
    for name in ('site_id', 'device_id'):
        if not re.fullmatch('[A-Za-z0-9_-]+', config[name]):
            raise ValueError(f'invalid identifier: {name}')
    uri = urlsplit(config['mqtt_uri'])
    allowed = ('mqtts', 'mqtt') if allow_plaintext else ('mqtts',)
    if uri.scheme not in allowed or not uri.hostname or uri.username or uri.password or uri.path or uri.query or uri.fragment:
        raise ValueError('broker URI must use an allowed scheme and a host, without credentials/query/fragment')
    if any(c.isspace() for c in config['mqtt_uri']) or (uri.port is not None and not 1 <= uri.port <= 65535):
        raise ValueError('invalid broker URI')
    return config


def write_csv(config: dict, destination: Path) -> None:
    destination.parent.mkdir(parents=True, exist_ok=True)
    # Refuse symlink destinations; never follow one while writing credentials.
    fd = os.open(destination, os.O_WRONLY | os.O_CREAT | os.O_TRUNC | os.O_NOFOLLOW, 0o600)
    os.fchmod(fd, 0o600)
    with os.fdopen(fd, 'w', newline='') as stream:
        writer = csv.writer(stream)
        writer.writerow(['key', 'type', 'encoding', 'value'])
        writer.writerow(['espkit', 'namespace', '', ''])
        writer.writerow(['schema', 'data', 'u8', SCHEMA_VERSION])
        for name in LIMITS:
            writer.writerow([name, 'data', 'string', config[name]])


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--config', type=Path, required=True)
    parser.add_argument('--output-dir', type=Path, default=Path('local/provisioning'))
    parser.add_argument('--allow-plaintext', action='store_true', help='isolated development brokers only')
    parser.add_argument('--flash', action='store_true')
    parser.add_argument('--replace-nvs', action='store_true', help='acknowledge replacement of all existing NVS settings')
    parser.add_argument('--port')
    args = parser.parse_args()
    if args.flash and (not args.replace_nvs or not args.port):
        parser.error('--flash requires --replace-nvs and --port')
    try:
        config = load_config(args.config, args.allow_plaintext)
    except (ValueError, OSError):
        parser.error('invalid configuration; check keys, lengths, identifiers, and URI (values are not logged)')
    sdk = os.environ.get('IDF_PATH')
    if not sdk:
        parser.error('activate ESP-IDF 5.4.2 first (IDF_PATH is required)')
    generator = Path(sdk) / 'components/nvs_flash/nvs_partition_generator/nvs_partition_gen.py'
    csv_path = args.output_dir / 'device.csv'
    image_path = args.output_dir / 'device.nvs.bin'
    os.umask(0o077)
    write_csv(config, csv_path)
    import sys
    subprocess.run([sys.executable, str(generator), 'generate', str(csv_path), str(image_path), '0x6000'], check=True)
    image_path.chmod(0o600)
    print('Generated private NVS image. Do not commit or attach this image to releases.')
    if args.flash:
        # Matches the root firmware's partitions.csv only; this replaces its whole NVS partition.
        subprocess.run([sys.executable, '-m', 'esptool', '--chip', 'esp32s3', '-p', args.port,
                        'write_flash', '0x9000', str(image_path)], check=True)


if __name__ == '__main__':
    main()
