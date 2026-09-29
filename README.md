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

## License

This project is released as an educational and tribute reconstruction. See [LICENSE](LICENSE) for details.

## Features

- DOS text-mode CD player
- Windows 3.x dialog-based player
- Hybrid single-file executable (DOS + Windows)
- MCI-based CD control
- MSF-to-millisecond position conversion

## Building

See [BUILD.md](BUILD.md) for full build instructions.

## Testing

- **DOS portion**: Test in DOSBox or DOSBox-X
- **Windows portion**: Test in OTVDM (WineVDM) or a Windows 98 VM

## Structure

```
PLAYCD/
  src/
    common/    - Shared types and player state
    dos/       - DOS text-mode UI and CD-ROM control
    windows/   - Windows 3.x dialog and MCI interface
  dlls/        - CTCCW.DLL and CTRES.DLL sources
  tools/       - Merge script for hybrid EXE generation
  /        - Documentation
  test/        - DOSBox configuration
```
