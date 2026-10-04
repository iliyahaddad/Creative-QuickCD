#!/usr/bin/env python
"""
Merge a DOS MZ executable and a Windows 3.x NE executable into one hybrid EXE.

Layout of the result:

    [ DOS MZ image (header grown to >= 0x40 bytes) ]  <- DOS runs this
    [ padding to an alignment boundary              ]
    [ complete Windows EXE (own stub + NE image)    ]  <- Windows runs this
                                                         (e_lfanew points at its NE header)

Why this is more than "append the NE part and patch e_lfanew":

  * The NE header stores file-relative offsets for segment/resource data
    and an absolute file offset for the non-resident name table.  Those
    file offsets must be rebased when the Windows image is moved.  Segment
    and resource offsets are stored in alignment units, therefore the
    Windows image has to start on a compatible boundary.
  * Windows only looks at e_lfanew (offset 0x3C) if the MZ header's
    relocation-table offset (0x18) is >= 0x40. A DOS program from Open Watcom
    normally has a 0x1E-byte header, so offset 0x3C would be inside the
    relocation table / program image. The DOS header is therefore rebuilt
    with room for the extra fields.

Usage: merge_hybrid.py <dos_exe> <win_exe> <output_exe>
"""
import os
import struct
import sys

MZ_MIN_HEADER = 0x40


def u16(data, off):
    return struct.unpack_from("<H", data, off)[0]


def u32(data, off):
    return struct.unpack_from("<I", data, off)[0]


def put16(data, off, value):
    if not 0 <= value <= 0xFFFF:
        raise ValueError("value 0x%X does not fit in 16 bits at 0x%X" % (value, off))
    struct.pack_into("<H", data, off, value)


def put32(data, off, value):
    struct.pack_into("<I", data, off, value)


def validate_mz(data, label):
    if len(data) < 0x1C:
        raise ValueError("%s is too small to contain an MZ header" % label)
    if data[0:2] not in (b"MZ", b"ZM"):
        raise ValueError("%s does not start with an MZ header" % label)


def get_ne_offset(data, label):
    if len(data) < 0x40:
        raise ValueError("%s has no extended header" % label)
    offset = u32(data, 0x3C)
    if offset < 0x40 or offset + 0x40 > len(data):
        raise ValueError("%s has an invalid e_lfanew: %d" % (label, offset))
    if data[offset:offset + 2] != b"NE":
        raise ValueError(
            "%s does not contain a Windows NE header at e_lfanew (0x%X)"
            % (label, offset))
    return offset


def dos_image_size(data):
    """Size of the MZ image as declared in its header (ignores overlays)."""
    last_page = u16(data, 2)
    pages = u16(data, 4)
    if pages == 0:
        raise ValueError("DOS executable declares zero pages")
    size = (pages - 1) * 512 + (last_page if last_page else 512)
    if size < 0x1C or size > len(data):
        raise ValueError(
            "DOS executable declares an image size outside the file: %d" % size)
    return size


def rebuild_dos_exe(data):
    """Return a copy of the DOS EXE whose header is at least 0x40 bytes with
    e_lfarlc >= 0x40, so that the e_lfanew field at 0x3C is legal."""
    data = bytes(data[:dos_image_size(data)])

    hdr_size = u16(data, 8) * 16
    reloc_off = u16(data, 0x18)
    reloc_cnt = u16(data, 6)

    if hdr_size < 0x1C or hdr_size > len(data):
        raise ValueError("DOS executable has an invalid header size")
    if reloc_off + reloc_cnt * 4 > hdr_size:
        raise ValueError("DOS executable has a corrupt relocation table")

    if hdr_size >= MZ_MIN_HEADER and reloc_off >= MZ_MIN_HEADER:
        return bytearray(data)          # already roomy enough

    relocs = data[reloc_off:reloc_off + reloc_cnt * 4]
    image = data[hdr_size:]

    new_reloc_off = MZ_MIN_HEADER
    new_hdr_size = (new_reloc_off + len(relocs) + 15) & ~15

    header = bytearray(new_hdr_size)
    header[0:0x1C] = data[0:0x1C]        # standard MZ fields
    header[new_reloc_off:new_reloc_off + len(relocs)] = relocs

    put16(header, 0x18, new_reloc_off)   # e_lfarlc
    put16(header, 8, new_hdr_size // 16) # e_cparhdr

    total = new_hdr_size + len(image)
    put16(header, 2, total % 512)        # e_cblp
    put16(header, 4, (total + 511) // 512)  # e_cp

    return bytearray(header) + bytearray(image)


def rebase_ne(win, ne_off, delta):
    """Rebase file-offset fields in a standalone NE image.

    NE table pointers inside the NE header are relative to the NE header and
    therefore do not change when the whole NE image is moved.  The segment
    table and resource entries, however, contain offsets into the file, and
    the non-resident name table offset is an absolute file offset.
    """
    if delta < 0:
        raise ValueError("delta must be non-negative")

    seg_count = u16(win, ne_off + 0x1C)
    seg_table = ne_off + u16(win, ne_off + 0x22)

    # Segment table entries contain file-sector offsets.
    shift = u16(win, ne_off + 0x32) or 9
    unit = 1 << shift

    if delta % unit:
        raise ValueError(
            "internal error: image is not aligned to 1<<%d" % shift)

    for i in range(seg_count):
        ent = seg_table + i * 8
        if ent + 8 > len(win):
            raise ValueError("NE segment table extends beyond the image")

        sector = u16(win, ent)
        if sector:
            rebased = sector + (delta >> shift)
            if rebased > 0xFFFF:
                raise ValueError("NE segment offset exceeds 16-bit range")
            put16(win, ent, rebased)

    # Resource entries also contain file offsets, expressed in the resource
    # table's own alignment units.  The resource table ends at the resident
    # name table, but decoding until the terminating TYPEINFO entry is safer
    # than trusting the resource-entry count field.
    res_table = ne_off + u16(win, ne_off + 0x24)
    res_end = ne_off + u16(win, ne_off + 0x26)

    if res_table + 2 > len(win) or res_end > len(win) or res_end < res_table:
        raise ValueError("invalid NE resource table bounds")

    if res_end > res_table + 2:
        rshift = u16(win, res_table)
        runit = 1 << rshift
        if delta % runit:
            raise ValueError(
                "internal error: image is not aligned to resource unit 1<<%d"
                % rshift)

        pos = res_table + 2
        while pos + 2 <= res_end:
            type_id = u16(win, pos)
            if type_id == 0:
                break

            if pos + 8 > res_end:
                raise ValueError("truncated NE resource type entry")

            count = u16(win, pos + 2)
            pos += 8

            needed = count * 12
            if pos + needed > res_end:
                raise ValueError("truncated NE resource entry list")

            for _ in range(count):
                off_units = u16(win, pos)
                if off_units:
                    rebased = off_units + (delta >> rshift)
                    if rebased > 0xFFFF:
                        raise ValueError("NE resource offset exceeds 16-bit range")
                    put16(win, pos, rebased)
                pos += 12

    # This is an absolute file offset, unlike the other NE table pointers.
    nonres_len = u16(win, ne_off + 0x20)
    nonres_off = u32(win, ne_off + 0x2C)
    if nonres_len and nonres_off:
        rebased = nonres_off + delta
        put32(win, ne_off + 0x2C, rebased)

    # 0x38/0x3A are offsets/lengths relative to the NE image for the
    # return-thunk/gangload area.  They must NOT be adjusted when the whole
    # image is moved.  Older versions of this script incorrectly modified
    # 0x34/0x36, corrupting the resource-count and target-OS fields.

    # A non-zero NE file-load CRC is no longer valid after rebasing/merging.
    # Zero is the conventional "no checksum" value and is accepted by NE
    # loaders; this avoids leaving a stale checksum in the hybrid image.
    put32(win, ne_off + 0x08, 0)

    return shift


def alignment_unit(win, ne_off):
    shift = u16(win, ne_off + 0x32) or 9
    res_table = ne_off + u16(win, ne_off + 0x24)
    res_end = ne_off + u16(win, ne_off + 0x26)
    rshift = u16(win, res_table) if res_end > res_table + 2 else 0
    return 1 << max(shift, rshift, 9)


def create_hybrid(dos_exe_path, win_exe_path, output_path):
    with open(dos_exe_path, "rb") as f:
        dos_raw = f.read()
    with open(win_exe_path, "rb") as f:
        win_raw = f.read()

    validate_mz(dos_raw, "DOS executable")
    validate_mz(win_raw, "Windows executable")

    win_ne_offset = get_ne_offset(win_raw, "Windows executable")

    dos = rebuild_dos_exe(dos_raw)
    win = bytearray(win_raw)

    unit = alignment_unit(win, win_ne_offset)
    win_start = (len(dos) + unit - 1) // unit * unit
    pad = win_start - len(dos)

    rebase_ne(win, win_ne_offset, win_start)

    ne_offset = win_start + win_ne_offset
    put32(dos, 0x3C, ne_offset)

    with open(output_path, "wb") as f:
        f.write(dos)
        f.write(b"\0" * pad)
        f.write(win)

    print("Created hybrid EXE: " + output_path)
    print("DOS image size: %d bytes" % len(dos))
    print("Windows image start: 0x%X (alignment 0x%X)" % (win_start, unit))
    print("NE header offset: 0x%X" % ne_offset)
    print("Total size: %d bytes" % (win_start + len(win)))


def main():
    if len(sys.argv) != 4:
        print("Usage: merge_hybrid.py <dos_exe> <win_exe> <output_exe>")
        return 1

    dos_exe_path, win_exe_path, output_path = sys.argv[1:4]

    for label, path in (("DOS EXE", dos_exe_path), ("Windows EXE", win_exe_path)):
        if not os.path.isfile(path):
            print("Error: %s not found: %s" % (label, path), file=sys.stderr)
            return 1

    if os.path.exists(output_path) and not os.path.isfile(output_path):
        print("Error: Output path exists and is not a file: " + output_path,
              file=sys.stderr)
        return 1

    try:
        create_hybrid(dos_exe_path, win_exe_path, output_path)
    except (OSError, ValueError, struct.error) as exc:
        print("Error: " + str(exc), file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
