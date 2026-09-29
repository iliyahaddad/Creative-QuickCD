import struct
import sys
import os


def create_hybrid(dos_exe_path, win_exe_path, output_path):
    dos_data = None
    win_data = None

    try:
        with open(dos_exe_path, 'rb') as f:
            dos_data = f.read()
    except OSError as e:
        print("Error reading DOS EXE: " + str(e), file=sys.stderr)
        sys.exit(1)

    try:
        with open(win_exe_path, 'rb') as f:
            win_data = f.read()
    except OSError as e:
        print("Error reading Windows EXE: " + str(e), file=sys.stderr)
        sys.exit(1)

    if len(dos_data) < 64:
        print("Error: DOS EXE too small to contain valid DOS header", file=sys.stderr)
        sys.exit(1)

    e_lfanew_offset = 0x3C
    if e_lfanew_offset + 4 > len(dos_data):
        print("Error: DOS EXE header truncated", file=sys.stderr)
        sys.exit(1)

    e_lfanew = struct.unpack('<I', dos_data[e_lfanew_offset:e_lfanew_offset + 4])[0]

    if e_lfanew == 0 or e_lfanew >= len(dos_data):
        print("Error: Invalid e_lfanew value in DOS EXE: " + str(e_lfanew), file=sys.stderr)
        sys.exit(1)

    try:
        with open(output_path, 'wb') as f:
            f.write(dos_data[:e_lfanew])
            f.write(win_data)
    except OSError as e:
        print("Error writing output EXE: " + str(e), file=sys.stderr)
        sys.exit(1)

    print("Created hybrid EXE: " + output_path)
    print("DOS part size: " + str(e_lfanew) + " bytes")
    print("NE header at offset: 0x%X" % e_lfanew)


if __name__ == "__main__":
    if len(sys.argv) != 4:
        print("Usage: merge_hybrid.py <dos_exe> <win_exe> <output_exe>")
        sys.exit(1)

    dos_exe_path = sys.argv[1]
    win_exe_path = sys.argv[2]
    output_path = sys.argv[3]

    if not os.path.isfile(dos_exe_path):
        print("Error: DOS EXE not found: " + dos_exe_path, file=sys.stderr)
        sys.exit(1)

    if not os.path.isfile(win_exe_path):
        print("Error: Windows EXE not found: " + win_exe_path, file=sys.stderr)
        sys.exit(1)

    if os.path.exists(output_path) and not os.path.isfile(output_path):
        print("Error: Output path exists and is not a file: " + output_path, file=sys.stderr)
        sys.exit(1)

    create_hybrid(dos_exe_path, win_exe_path, output_path)
