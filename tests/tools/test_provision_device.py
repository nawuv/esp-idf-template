"""Validate private configuration generation without flashing any hardware."""

import csv
import importlib.util
import json
import os
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('provision_device', ROOT / 'tools/provision_device.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class ProvisionTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.config = json.loads((ROOT / 'config/device.example.json').read_text())
        self.path = self.root / 'device.json'

    def load(self, allow_plaintext=False):
        self.path.write_text(json.dumps(self.config))
        return module.load_config(self.path, allow_plaintext)

    def test_csv_quotes_values_and_is_private(self):
        self.config['wifi_ssid'] = 'lab,"sensor"'
        config = self.load()
        output = self.root / 'device.csv'
        module.write_csv(config, output)
        with output.open() as stream:
            rows = list(csv.reader(stream))
        self.assertIn(['wifi_ssid', 'data', 'string', 'lab,"sensor"'], rows)
        self.assertIn(['schema', 'data', 'u8', '1'], rows)
        self.assertEqual([row[0] for row in rows],
                         ['key', 'espkit', 'schema', *module.LIMITS])
        self.assertEqual(module.SCHEMA_VERSION, 1)
        self.assertEqual(os.stat(output).st_mode & 0o777, 0o600)

    def test_rejects_topic_injection(self):
        self.config['site_id'] = 'farm/commands'
        with self.assertRaises(ValueError):
            self.load()

    def test_plaintext_needs_explicit_development_option(self):
        self.config['mqtt_uri'] = 'mqtt://localhost:1883'
        with self.assertRaises(ValueError):
            self.load()
        self.assertTrue(self.load(True))

    def test_rejects_credentials_in_uri_and_unknown_fields(self):
        self.config['mqtt_uri'] = 'mqtts://user:password@localhost:8883'
        with self.assertRaises(ValueError):
            self.load()
        self.config = json.loads((ROOT / 'config/device.example.json').read_text())
        self.config['unexpected'] = 'value'
        with self.assertRaises(ValueError):
            self.load()
