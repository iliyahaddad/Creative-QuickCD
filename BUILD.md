# Building Creative QuickCD

This document describes how to build the QuickCD hybrid executable from source.

## Prerequisites

- [Open Watcom v2](https://open-watcom.github.io/) (`wcc`, `wcl`, `wlink`, `wrc`, `wmake`)
  with the **16-bit DOS and 16-bit Windows** targets installed
- Python 3.6+
- (Optional) DOSBox / DOSBox-X for testing the DOS portion
- (Optional) OTVDM / WineVDM for testing the Windows portion

## Environment Setup

Set the `WATCOM` environment variable and put the tools on the `PATH`:

```
set WATCOM=C:\WATCOM
set PATH=%WATCOM%\binnt;%WATCOM%\binw;%PATH%
set EDPATH=%WATCOM%\eddat
```

On 32-bit Windows, use `binnt` (or `binw`) as appropriate for your host.
The Windows sources need the Win16 headers (`%WATCOM%\h\win`); the Makefile
adds that include path itself, so `INCLUDE` does not need to be changed.

## Building

### Option A: Using the Makefile

```
wmake
```

This builds the DOS object files, the Windows object files and resource, links
both executable portions and produces the final `quickcd.exe` hybrid.
Useful targets: `wmake dos`, `wmake windows`, `wmake dlls`, `wmake test`, `wmake clean`.

### Option B: Manual Commands

Do **not** pass `-zl` (it removes the C library and the link then fails on
`printf`, `memset`, ...).

DOS portion:

```
wcl -bt=dos -ms -i=src/common src/dos/main.c src/dos/cdrom.c src/dos/ui.c src/dos/ports.c -fe=dos_part.exe
```

Windows portion (Windows 3.x / Win16):

```
wrc -r -bt=windows -i=src/windows -i=%WATCOM%\h\win -fo=win_part.res src/windows/resource.rc
wcl -l=windows -bt=windows -bw -ms -k8192 -i=src/windows -i=%WATCOM%\h\win src/windows/main.c src/windows/dialog.c src/windows/mci.c win_part.res -fe=win_part.exe -"library mmsystem"
```

DLLs:

```
wcl -l=windows_dll -bt=windows -bd -mc -zu -i=src/windows -i=%WATCOM%\h\win dlls/ctccw.c -fe=ctccw.dll -"library mmsystem"
wcl -l=windows_dll -bt=windows -bd -mc -zu -i=src/windows -i=%WATCOM%\h\win dlls/ctres.c -fe=ctres.dll
```

Merge into a hybrid executable:

```
python tools/merge_hybrid.py dos_part.exe win_part.exe quickcd.exe
```

## What the merge tool does

`tools/merge_hybrid.py` places the DOS program first (its MZ header is grown to
at least 0x40 bytes so the `e_lfanew` field is legal) and appends the complete
Windows executable on an aligned boundary. NE segment/resource locations and
the non-resident-name file offset are rebased to the new file position; NE
table pointers that are relative to the NE header are left unchanged. DOS
runs the first image; Windows follows `e_lfanew` to the NE header.

## Testing

### Build-tool self-test

The repository includes a platform-independent test for the MZ/NE hybrid
merger. It uses synthetic executable headers, so it can be run without
Open Watcom, DOSBox, Windows, or a CD-ROM:

```
wmake test
```

It verifies that segment/resource/non-resident offsets are rebased correctly
without corrupting the NE resource-count or target-OS fields.

### DOS portion

`test/dosbox.conf` mounts the project folder and an image of an audio CD.
Edit the two paths to match your machine. **Use a CUE/BIN image** - an `.iso`
cannot contain audio tracks, so the player would report "Unable to read CD
Table of Contents". Then start DOSBox with:

```
dosbox -conf test\dosbox.conf
```

Keys: `P` play, `S` stop, `Space` pause/resume, `+`/`-` (or Up/Down) volume,
`N`/`B` (or Right/Left) next/previous track, `E` eject, `Q`/`Esc` quit.

### Windows portion

Run under OTVDM or in a Windows 3.x / 9x VM.

## Troubleshooting

- **Inline assembly errors**: the DOS code uses Open Watcom `#pragma aux`
  byte-code definitions at file scope (`ports.c`, `cdrom.c`); the functions are
  only declared, never given a C body.
- **Linking errors**: use `-ms` consistently and do not pass `-zl`.
- **`windows.h` not found**: make sure the Win16 target is installed and
  `WATCOM` is set (the Makefile uses `%WATCOM%\h\win`).
- **MCI errors on Windows**: MSCDEX (or the equivalent CD-ROM driver) must be
  loaded and an audio CD inserted. The Windows player retries every 500 ms, so
  a disc inserted later is picked up automatically.
