#!/usr/bin/env python3
"""Self-test for the QuickCD MZ+NE hybrid merger.

This test uses synthetic headers; it does not require Open Watcom, DOSBox,
Windows, or a real CD-ROM.
"""
import os
import struct
import tempfile
import unittest

from tools import merge_hybrid


def p16(b, o, v):
    struct.pack_into("<H", b, o, v)


def p32(b, o, v):
    struct.pack_into("<I", b, o, v)


class MergeHybridTest(unittest.TestCase):
    def test_rebases_file_offsets_without_corrupting_ne_header(self):
        with tempfile.TemporaryDirectory() as td:
            dos = bytearray(48)
            dos[0:2] = b"MZ"
            p16(dos, 2, 48)
            p16(dos, 4, 1)
            p16(dos, 6, 0)
            p16(dos, 8, 2)
            p16(dos, 0x18, 0x1C)
            dos_path = os.path.join(td, "dos.exe")
            with open(dos_path, "wb") as f:
                f.write(dos)

            # A minimal MZ+NE image with one segment and one resource.
            win = bytearray(0x300)
            win[0:2] = b"MZ"
            p16(win, 2, 0x300 % 512)
            p16(win, 4, (0x300 + 511) // 512)
            p16(win, 8, 8)
            p16(win, 0x18, 0x40)
            p32(win, 0x3C, 0x100)

            ne = 0x100
            win[ne:ne + 2] = b"NE"
            p16(win, ne + 0x1C, 1)       # segments
            p16(win, ne + 0x20, 4)       # non-resident name length
            p16(win, ne + 0x22, 0x40)    # segment table
            p16(win, ne + 0x24, 0x48)    # resource table
            p16(win, ne + 0x26, 0x80)    # resident names
            p32(win, ne + 0x2C, 0x220)   # absolute non-resident offset
            p16(win, ne + 0x32, 9)       # 512-byte sectors
            p16(win, ne + 0x34, 0x1234)  # must not change
            win[ne + 0x36] = 2          # Windows target; must not change
            p16(win, ne + 0x38, 0x2222)  # relative fast-load field
            p16(win, ne + 0x3A, 0x3333)

            p16(win, ne + 0x40, 1)       # segment at sector 1

            p16(win, ne + 0x48, 9)       # resource alignment
            p16(win, ne + 0x4A, 0x8001) # integer resource type
            p16(win, ne + 0x4C, 1)
            p16(win, ne + 0x52, 2)       # resource at unit 2
            p16(win, ne + 0x54, 1)
            p16(win, ne + 0x56, 0x20)
            p16(win, ne + 0x58, 1)
            p32(win, ne + 0x5A, 0)
            p16(win, ne + 0x5E, 0)

            win[0x220:0x224] = b"TEST"
            win_path = os.path.join(td, "win.exe")
            with open(win_path, "wb") as f:
                f.write(win)

            out_path = os.path.join(td, "hybrid.exe")
            merge_hybrid.create_hybrid(dos_path, win_path, out_path)
            with open(out_path, "rb") as f:
                out = f.read()

            ne_out = 0x200 + ne
            self.assertEqual(out[ne_out:ne_out + 2], b"NE")
            self.assertEqual(struct.unpack_from("<H", out, ne_out + 0x34)[0], 0x1234)
            self.assertEqual(out[ne_out + 0x36], 2)
            self.assertEqual(struct.unpack_from("<H", out, ne_out + 0x40)[0], 2)
            self.assertEqual(struct.unpack_from("<H", out, ne_out + 0x52)[0], 3)
            self.assertEqual(struct.unpack_from("<I", out, ne_out + 0x2C)[0], 0x420)
            self.assertEqual(struct.unpack_from("<H", out, ne_out + 0x38)[0], 0x2222)
            self.assertEqual(struct.unpack_from("<H", out, ne_out + 0x3A)[0], 0x3333)
            self.assertEqual(struct.unpack_from("<I", out, ne_out + 0x08)[0], 0)


if __name__ == "__main__":
    unittest.main()
