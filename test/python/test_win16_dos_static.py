import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]

class Win16DosStaticAudit(unittest.TestCase):
    def read(self, rel):
        return (ROOT / rel).read_text()

    def test_makefile_uses_correct_win16_models(self):
        s = self.read("Makefile")
        self.assertIn("WIN_CFLAGS = -bt=windows -bw -ms", s)
        self.assertIn("DLL_CFLAGS = -bt=windows -bd -mc -zu", s)
        self.assertIn("-l=windows -bt=windows", s)
        self.assertIn("-l=windows_dll", s)
        self.assertNotIn("-zw", s)

    def test_hscroll_uses_wparam_thumb_position(self):
        s = self.read("src/windows/dialog.c")
        self.assertIn("pos = (int)HIWORD(wParam); break;", s)
        self.assertNotIn("pos = (int)LOWORD(lParam); break;", s)

    def test_ctccw_track_play_has_explicit_end(self):
        s = self.read("dlls/ctccw.c")
        self.assertIn('status cdplayer length track %u', s)
        self.assertIn('play cdplayer from %lu to %lu', s)

if __name__ == "__main__":
    unittest.main()
