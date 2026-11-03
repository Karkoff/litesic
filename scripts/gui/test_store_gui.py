"""Guard and privacy regression checks; never sends UI input."""
import os
from pathlib import Path
import unittest
from unittest.mock import Mock, patch

import store_gui as store


class StoreGuards(unittest.TestCase):
    def test_file_dialog_never_captures_pixels(self):
        window = Mock(handle=42)
        window.class_name.return_value = '#32770'
        original = Mock()
        state = {}
        with patch.object(store.driver, 'window_info', return_value={'hwnd': 42, 'rect': [0, 0, 100, 100]}), patch.object(store.driver, 'write_state') as write:
            result = store.capture_window(state, window, 'dialog', original)
        original.assert_not_called()
        self.assertIsNone(result['image'])
        self.assertEqual(result['method'], 'SKIPPED_PRIVATE_FILE_DIALOG')
        self.assertEqual(state['observation']['hwnd'], 42)
        write.assert_called_once_with(state)

    def test_app_window_still_captures(self):
        window = Mock()
        window.class_name.return_value = 'LiteSic.MainWnd'
        original = Mock(return_value='actual-window')
        self.assertEqual(store.capture_window({}, window, 'app', original), 'actual-window')
        original.assert_called_once()

    def test_desktop_executable_is_rejected(self):
        with self.assertRaises(ValueError):
            store.validate_exe(store.driver.ROOT / 'build/64/release/LiteSic.exe')

    def test_wrong_package_is_rejected(self):
        with self.assertRaises(ValueError):
            store.validate_exe(Path(os.environ['ProgramFiles']) / 'WindowsApps/Other.App_1.0.0.0_x64__unknown/LiteSic.exe')

    def test_wrong_publisher_is_rejected(self):
        with self.assertRaises(ValueError):
            store.validate_exe(Path(os.environ['ProgramFiles']) / 'WindowsApps/LiteSic.LocalValidation_1.0.0.0_x64__other/LiteSic.exe')

    def test_modified_executable_is_rejected(self):
        path = Path(os.environ['ProgramFiles']) / 'WindowsApps/LiteSic.LocalValidation_1.0.0.0_x64__tk4hddz7kkqza/LiteSic.exe'
        with patch.object(Path, 'is_file', return_value=True), patch.object(store.driver, 'digest', side_effect=['installed', 'rebuilt']):
            with self.assertRaises(ValueError):
                store.validate_exe(path)

    def test_wrong_process_package_identity_is_rejected(self):
        with patch.object(store, 'package_family', return_value='Other.App_unknown'):
            with self.assertRaises(ValueError):
                store.check_family(123)


if __name__ == '__main__':
    unittest.main(verbosity=2)
