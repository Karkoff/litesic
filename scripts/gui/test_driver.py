"""Negative tests for the production driver's target/input guards; no UI input."""
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch, Mock
import litesic_gui as driver

class DriverGuards(unittest.TestCase):
    def test_fixture_cannot_be_used_in_wrong_dialog(self):
        with self.assertRaises(ValueError): driver.fixture_text('silence','Salvar lista M3U')

    def test_unknown_fixture_rejected(self):
        with self.assertRaises(ValueError): driver.fixture_text('../arbitrary','Abrir')

    def test_save_fixture_is_confined_to_evidence_directory(self):
        result=driver.fixture_text('m3u-save','Salvar lista M3U')
        self.assertTrue(Path(result).resolve().is_relative_to(driver.OUT.resolve()))

    def test_control_modifier_translated_for_pywinauto(self):
        window=Mock(handle=42)
        window.client_rect.return_value.width.return_value=100
        window.client_rect.return_value.height.return_value=100
        gui=Mock();gui.WindowFromPoint.return_value=42;gui.GetAncestor.return_value=42
        process=Mock();process.GetWindowThreadProcessId.return_value=(1,123)
        with patch.object(driver,'modules',return_value=[Mock(),Mock(),gui,process,Mock(),Mock()]), \
             patch.object(driver,'observed'),patch.object(driver,'foreground'):
            driver.click({'pid':123},window,50,50,'left','ctrl')
            window.click_input.assert_called_once_with(coords=(50,50),button='left',pressed='control',double=False)

    def check_panel_owner(self, target):
        with tempfile.TemporaryDirectory() as tmp:
            path=Path(tmp)/'session.json'
            state={'pid':123,'exe':'expected','created':'original','sha256':'same'}
            path.write_text(json.dumps(state))
            application=Mock();app=application.return_value.connect.return_value
            main=Mock(handle=42);playlist=Mock(handle=43)
            app.windows.side_effect=lambda class_name,visible_only: [main] if class_name==driver.CLASS else [playlist]
            gui=Mock();gui.GetWindow.return_value=99
            with patch.object(driver,'STATE',path),patch.object(driver,'require_desktop'), \
                 patch.object(driver,'validate_exe'),patch.object(driver,'digest',return_value='same'), \
                 patch.object(driver,'identity',return_value={k:state[k] for k in ('pid','exe','created')}), \
                 patch.object(driver,'modules',return_value=[Mock(),Mock(),gui,Mock(),Mock(),application]):
                with self.assertRaises(RuntimeError): driver.connect(False,target)

    def test_playlist_with_unrelated_owner_rejected(self):
        self.check_panel_owner('playlist')

    def test_eq_with_unrelated_owner_rejected(self):
        self.check_panel_owner('eq')

    def test_arbitrary_executable_rejected(self):
        with self.assertRaises(ValueError): driver.validate_exe(driver.ROOT / 'build' / 'other.exe')

    def test_external_executable_rejected(self):
        with self.assertRaises(ValueError): driver.validate_exe(Path('C:/Windows/notepad.exe'))

    def test_sandbox_desktop_rejected_before_input(self):
        with patch.object(driver, 'doctor', return_value={'interactive': False}), self.assertRaises(RuntimeError):
            driver.require_desktop()

    def test_capture_filename_cannot_escape_output_directory(self):
        with self.assertRaises(ValueError): driver.capture({}, Mock(), '../outside')

    def test_pid_reuse_rejected_before_window_connection(self):
        self.check_identity_guard({'pid': 123, 'exe': 'expected', 'created': 'new'}, 'same')

    def test_wrong_process_path_rejected_before_window_connection(self):
        self.check_identity_guard({'pid': 123, 'exe': 'other', 'created': 'original'}, 'same')

    def test_rebuilt_binary_rejected_before_window_connection(self):
        self.check_identity_guard({'pid': 123, 'exe': 'expected', 'created': 'original'}, 'changed')

    def check_identity_guard(self, current, checksum):
        with tempfile.TemporaryDirectory() as tmp:
            state = Path(tmp) / 'session.json'
            state.write_text(json.dumps({'pid': 123, 'exe': 'expected', 'created': 'original', 'sha256': 'same'}))
            with patch.object(driver, 'STATE', state), patch.object(driver, 'require_desktop'), \
                 patch.object(driver, 'validate_exe'), patch.object(driver, 'identity', return_value=current), \
                 patch.object(driver, 'digest', return_value=checksum), patch.object(driver, 'modules') as modules:
                with self.assertRaises(RuntimeError): driver.connect()
                modules.assert_not_called()

    def test_input_without_observation_rejected(self): self.check_observation({})
    def test_input_with_stale_observation_rejected(self):
        self.check_observation({'hwnd': 42, 'rect': [0, 0, 100, 100], 'time': 1})
    def test_input_to_different_window_rejected(self):
        self.check_observation({'hwnd': 43, 'rect': [0, 0, 100, 100], 'time': 995})
    def test_input_after_window_moved_rejected(self):
        self.check_observation({'hwnd': 42, 'rect': [1, 0, 101, 100], 'time': 995})
    def test_future_observation_rejected(self):
        self.check_observation({'hwnd': 42, 'rect': [0, 0, 100, 100], 'time': 1001})

    def check_observation(self, observation):
        with patch.object(driver.time, 'time', return_value=1000), \
             patch.object(driver, 'window_info', return_value={'rect': [0, 0, 100, 100]}), \
             patch.object(driver, 'write_state') as write:
            with self.assertRaises(RuntimeError): driver.observed({'observation': observation}, Mock(handle=42))
            write.assert_not_called()

    def test_successful_guard_consumes_observation_before_input(self):
        state = {'observation': {'hwnd': 42, 'rect': [0, 0, 100, 100], 'time': 995}}
        with patch.object(driver.time, 'time', return_value=1000), \
             patch.object(driver, 'window_info', return_value={'rect': [0, 0, 100, 100]}), \
             patch.object(driver, 'write_state') as write:
            driver.observed(state, Mock(handle=42))
            self.assertNotIn('observation', state)
            write.assert_called_once_with(state)

    def test_outside_click_rejected_before_foreground_or_mouse(self):
        window = Mock()
        window.client_rect.return_value.width.return_value = 100
        window.client_rect.return_value.height.return_value = 100
        with patch.object(driver, 'modules', return_value=[Mock()] * 6), \
             patch.object(driver, 'observed'), patch.object(driver, 'foreground') as foreground:
            with self.assertRaises(ValueError): driver.click({}, window, 101, 50, 'left')
            foreground.assert_not_called()
            window.click_input.assert_not_called()

if __name__ == '__main__': unittest.main(verbosity=2)
