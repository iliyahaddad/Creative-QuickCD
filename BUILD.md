# Building Creative QuickCD

This document describes how to build the QuickCD hybrid executable from source.

## Prerequisites

- [Open Watcom v2](https://open-watcom.github.io/) (wcc, wcl, wlink, wrc)
- Python 3.6+
- (Optional) DOSBox-X for testing the DOS portion
- (Optional) OTVDM / WineVDM for testing the Windows portion

## Environment Setup

Set the `WATCOM` environment variable to your Open Watcom installation root:

```
set WATCOM=C:\WATCOM
set PATH=%WATCOM%\binnt;%WATCOM%\binw;%PATH%
```

On 32-bit Windows, use `binw` instead of `binnt` if necessary.

## Building

### Option A: Using the Makefile

```
wmake
```

This builds the DOS object files, Windows object files and resource, the DLLs, links both executable portions, and produces the final `quickcd.exe` hybrid.

### Option B: Manual Commands

Build the DOS portion:

```
wcl -bt=dos -ms -zl src/dos/main.c src/dos/cdrom.c src/dos/ui.c src/dos/ports.c -fe=dos_part.exe
```

Build the Windows portion:

```
wcl -bt=windows -bw -ms -zl src/windows/main.c src/windows/dialog.c src/windows/mci.c src/windows/resource.rc -fe=win_part.exe
```

Build the DLLs:

```
wcl -bt=windows -bw -bd dlls/ctccw.c -fe=ctccw.dll
wcl -bt=windows -bw -bd dlls/ctres.c -fe=ctres.dll
```

Merge into a hybrid executable:

```
python tools/merge_hybrid.py dos_part.exe win_part.exe QCD.EXE
```

## Testing

### DOS portion

Mount the project directory in DOSBox and run:

```
mount c C:\Users\manshadi.PDF\Desktop\QCD\PLAYCD
imgmount d C:\test\test_cd.iso -t iso
c:
quickcd.exe
```

### Windows portion

Run under OTVDM or in a Windows 98 VM. The Windows portion of the hybrid will launch automatically on 32-bit Windows.

## Troubleshooting

- **Inline assembly errors**: Open Watcom 16-bit inline assembly uses `__asm` blocks. Ensure your source uses Watcom syntax.
- **Linking errors**: Verify `-ms` (small memory model) and `-zl` (suppress default library) are passed consistently.
- **MCI errors on Windows**: Ensure MSCDEX or the equivalent CD-ROM driver is loaded. On Windows 9x, this is typically automatic.
