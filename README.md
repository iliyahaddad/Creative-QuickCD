# Creative QuickCD - Open Source Reconstruction

![Status](https://img.shields.io/badge/status-reconstruction-blue)
![Platform](https://img.shields.io/badge/platform-DOS%20%7C%20Windows-lightgrey)

A tribute project reconstructing the classic **Creative Labs QuickCD** CD player from the early 1990s as a hybrid DOS/Windows executable.

## Background

QuickCD was a CD audio player bundled with Creative Labs Sound Blaster CD-ROM drives in the early-to-mid 1990s. It ran on DOS and provided a native Windows 3.x dialog-based interface. The executable was a hybrid that contained both a DOS text-mode player and a Windows GUI, selected automatically at runtime.

## Original Credits

- **DOS Version**: Written by PH Beh and Andy
- **Windows Version**: Written by Nigel Tan
- **DLLs**: CTCCW.DLL (CD Control for Windows) and CTRES.DLL (Resources)
- **Copyright**: Creative Technology, Ltd.

See [CREDITS.md](CREDITS.md). This is an educational and tribute reconstruction
and is not affiliated with or endorsed by Creative Technology, Ltd.

## Features

- DOS text-mode CD player (MSCDEX, direct text-screen output)
- Windows 3.x dialog-based player (MCI)
- Hybrid single-file executable (DOS + Windows)
- MSF / LBA / millisecond position conversion

## Building

See [BUILD.md](BUILD.md) for full build instructions.

## Testing

- **DOS portion**: DOSBox / DOSBox-X with a CUE/BIN audio CD image (`test/dosbox.conf`)
- **Windows portion**: OTVDM (WineVDM) or a Windows 3.x / 9x VM

## Structure

```
src/
  common/    - Shared types and constants
  dos/       - DOS text-mode UI and MSCDEX CD-ROM control
  windows/   - Windows 3.x dialog and MCI interface
dlls/        - CTCCW.DLL and CTRES.DLL sources
tools/       - Merge script for hybrid EXE generation
test/        - DOSBox configuration
```
