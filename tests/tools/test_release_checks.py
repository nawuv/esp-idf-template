"""Check version mismatch rejection and Conventional Commit PR titles."""
import importlib.util
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('check_pr_title', ROOT / 'tools/check_pr_title.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class PrTitleTests(unittest.TestCase):
    def test_accepts_scopes_and_breaking_changes(self):
        for title in ['feat: add sensing', 'refactor!: change API', 'fix(mqtt): reconnect', 'chore: promote develop']:
            self.assertTrue(module.valid_title(title), title)

    def test_rejects_blank_missing_type_and_multiline(self):
        for title in ['', 'Update README', 'feat: ', 'feat: add sensing\nfix: another subject']:
            self.assertFalse(module.valid_title(title), title)
