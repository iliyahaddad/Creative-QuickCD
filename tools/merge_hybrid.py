import struct
import sys
import os


def read_u32(data, offset):
    return struct.unpack_from("<I", data, offset)[0]


def validate_mz(data, label):
    if len(data) < 64:
        raise ValueError(
            "%s is too small to contain an MZ header" % label
        )

    if data[0:2] != b"MZ":
        raise ValueError(
            "%s does not start with an MZ header" % label
        )


def get_ne_offset(data, label):
    offset = read_u32(data, 0x3C)

    if offset < 64 or offset + 2 > len(data):
        raise ValueError(
            "%s has an invalid e_lfanew: %d"
            % (label, offset)
        )

    if data[offset:offset + 2] != b"NE":
        raise ValueError(
            "%s does not contain a Windows NE header "
            "at e_lfanew (0x%X)"
            % (label, offset)
        )

    return offset


def patch_u32(data, offset, value):
    struct.pack_into("<I", data, offset, value)


def create_hybrid(dos_exe_path,
                  win_exe_path,
                  output_path):

    with open(dos_exe_path, "rb") as f:
        dos_data = bytearray(f.read())

    with open(win_exe_path, "rb") as f:
        win_data = bytes(f.read())

    validate_mz(dos_data,
                "DOS executable")

    validate_mz(win_data,
                "Windows executable")

    win_ne_offset = get_ne_offset(
        win_data,
        "Windows executable"
    )

    # Keep the complete DOS executable.
    # DOS ignores everything after its own image.
    #
    # Windows needs the top-level MZ header's e_lfanew
    # to point directly at an NE header. Therefore append
    # only the Windows NE image and not its second MZ header.
    ne_offset = len(dos_data)

    if ne_offset > 0xFFFFFFFF:
        raise ValueError(
            "DOS executable is too large"
        )

    patch_u32(dos_data,
              0x3C,
              ne_offset)

    with open(output_path, "wb") as f:
        f.write(dos_data)
        f.write(win_data[win_ne_offset:])

    print("Created hybrid EXE: " + output_path)
    print("DOS image size: " +
          str(len(dos_data)) + " bytes")
    print("NE header offset: 0x%X" % ne_offset)
    print("Windows NE image size: " +
          str(len(win_data) - win_ne_offset) +
          " bytes")


def main():
    if len(sys.argv) != 4:
        print(
            "Usage: merge_hybrid.py "
            "<dos_exe> <win_exe> <output_exe>"
        )
        return 1

    dos_exe_path = sys.argv[1]
    win_exe_path = sys.argv[2]
    output_path = sys.argv[3]

    if not os.path.isfile(dos_exe_path):
        print(
            "Error: DOS EXE not found: " +
            dos_exe_path,
            file=sys.stderr
        )
        return 1

    if not os.path.isfile(win_exe_path):
        print(
            "Error: Windows EXE not found: " +
            win_exe_path,
            file=sys.stderr
        )
        return 1

    if (os.path.exists(output_path) and
            not os.path.isfile(output_path)):

        print(
            "Error: Output path exists and is not a file: " +
            output_path,
            file=sys.stderr
        )
        return 1

    try:
        create_hybrid(
            dos_exe_path,
            win_exe_path,
            output_path
        )
    except (OSError, ValueError, struct.error) as exc:
        print(
            "Error: " + str(exc),
            file=sys.stderr
        )
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
