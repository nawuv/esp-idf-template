"""Verify flash bundle integrity and rejection of escaping build paths."""

import importlib.util
import json
import tarfile
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('package_firmware', ROOT / 'tools/package_firmware.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class FirmwarePackageTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.build = self.root / 'build'
        (self.build / 'bootloader').mkdir(parents=True)
        (self.build / 'bootloader/bootloader.bin').write_bytes(b'bootloader')
        (self.build / 'device.bin').write_bytes(b'application')
        (self.build / 'flash_args').write_text('0x0 bootloader/bootloader.bin\n0x10000 device.bin\n')
        (self.build / 'sdkconfig').write_text('SECRET=do-not-package')
        (self.build / 'project_description.json').write_text(json.dumps({
            'project_name': 'device', 'project_version': '0.1.0', 'target': 'esp32s3'}))
        self.flash = {'flash_files': {'0x0': 'bootloader/bootloader.bin', '0x10000': 'device.bin'},
                      'extra_esptool_args': {'chip': 'esp32s3'}}
        self.write_flash()

    def write_flash(self):
        (self.build / 'flasher_args.json').write_text(json.dumps(self.flash))

    def package(self, name='out'):
        return module.package(self.build, self.root / name, 'a' * 40, 'ESP-IDF v5.4.2')

    def test_bundle_preserves_flash_files_checksums_and_is_reproducible(self):
        manifest = self.package()
        self.package('second')
        archive = self.root / 'out/firmware.tar.gz'
        self.assertEqual(archive.read_bytes(), (self.root / 'second/firmware.tar.gz').read_bytes())
        with tarfile.open(archive) as tar:
            self.assertNotIn('sdkconfig', tar.getnames())
            for name, checksum in manifest['files'].items():
                self.assertEqual(module.sha256(tar.extractfile(name).read()), checksum)
            self.assertIn('FLASHING.md', tar.getnames())
        for line in (self.root / 'out/SHA256SUMS').read_text().splitlines():
            checksum, name = line.split()
            self.assertEqual(module.sha256((self.root / 'out' / name).read_bytes()), checksum)

    def test_rejects_path_traversal(self):
        self.flash['flash_files']['0x10000'] = '../outside.bin'
        self.write_flash()
        with self.assertRaises(ValueError):
            self.package()

    def test_rejects_target_mismatch(self):
        self.flash['extra_esptool_args']['chip'] = 'esp32c3'
        self.write_flash()
        with self.assertRaises(ValueError):
            self.package()


if __name__ == '__main__':
    unittest.main()
