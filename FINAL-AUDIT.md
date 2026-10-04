# QuickCD Reconstruction — Final Audit

## Fixed in this revision

1. **Hybrid NE rebasing corruption**
   - The merger previously treated NE offsets `0x34/0x36` as a gang-load
     offset/length. In the NE header these fields are the resource-entry
     count and target-OS byte.
   - The merger now rebases only actual file-offset fields: segment locations,
     resource locations, and the absolute non-resident-name-table offset.
   - The relative fast-load/gangload fields at `0x38/0x3A` are preserved.
   - A stale NE file-load CRC is cleared after the image is relocated.

2. **DOS MZ validation**
   - The merger no longer silently truncates an EXE when its declared MZ
     image is larger than the input file.
   - Invalid MZ header sizes and relocation-table ranges are rejected.

3. **Win16 protected-mode drive enumeration**
   - The Windows GUI and CTCCW DLL no longer issue DOS `INT 2Fh` through
     `int86()` for drive enumeration.
   - They probe the existing MCI `cdaudio` interface instead, which matches
     the Windows-side architecture of the reconstruction.

4. **Windows track playback**
   - `PlayTrack()` now obtains the selected track's length and sends an
     explicit `from ... to ...` play range, so selecting a track does not
     continue into all following tracks.

5. **Windows track timing**
   - The Windows display now prefers `status ... length track N` for the
     selected/current track, with whole-disc length as a fallback.

6. **Regression coverage**
   - Added a platform-independent merger self-test under `test/python/`.
   - Added `wmake test`.

## Verification performed

- Python syntax compilation: passed.
- Hybrid merger synthetic regression test: passed.
- Test specifically verifies that NE resource-count/target-OS fields are
  not corrupted and that segment/resource/non-resident offsets are rebased.

## Not claimed as locally verified

This environment does not contain an Open Watcom 16-bit toolchain, DOSBox,
OTVDM/WineVDM, or a physical/virtual MSCDEX CD-audio device. Therefore the
final C sources have not been compiled and exercised against a real Win16/DOS
runtime here. The build instructions remain targeted at Open Watcom v2.


## v4 line-by-line compatibility fixes

7. **Open Watcom Win16 DLL memory model**
   - Win16 DLLs now build with `-mc -zu` and the explicit `windows_dll` linker target.
   - This matches Open Watcom's Win16 DLL guidance: DLL code must use a big-data model and `-zu` because SS belongs to the calling application.

8. **Win16 WM_HSCROLL thumb position**
   - `dialog.c` now reads the thumb position from `HIWORD(wParam)`.
   - `lParam` is reserved for the scrollbar HWND; the previous code incorrectly read `LOWORD(lParam)`.

9. **CTCCW.DLL track playback boundary**
   - `PlayTrack()` now queries the selected track length and issues an explicit `from ... to ...` MCI range, preventing playback from spilling into subsequent tracks.

10. **Win16 linker target clarity**
   - The EXE explicitly uses `-l=windows`; DLLs explicitly use `-l=windows_dll`.
   - Removed the unnecessary `-zw` from the Win16 EXE flags.

## v4 verification

- Python syntax compilation: PASS.
- Hybrid merger regression test: PASS.
- Static source audit for Win16/DOS build flags and the corrected WM_HSCROLL/CTCCW patterns: PASS.
- Real Open Watcom compilation and real DOSBox/Win16 runtime execution are still environment-dependent and are not falsely claimed as locally executed.
