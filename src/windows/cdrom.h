#ifndef WINDOWS_CDROM_H
#define WINDOWS_CDROM_H

#include "types.h"

/* Volumes are 0..255 at this API; they are scaled to the 16-bit range that
 * the Windows aux API uses internally. */

BOOL WinCD_Init(HWND hwnd);
void WinCD_Cleanup(void);
BOOL WinCD_OpenDrive(BYTE drive);
void WinCD_CloseDrive(void);
BOOL WinCD_IsOpen(void);
BOOL WinCD_PlayTrack(BYTE track);
void WinCD_Stop(void);
void WinCD_Pause(void);
void WinCD_Resume(void);
BOOL WinCD_Eject(void);
BOOL WinCD_CloseTray(void);
BOOL WinCD_GetStatus(BYTE far *track, DWORD far *pos, DWORD far *length);
BOOL WinCD_SetVolume(WORD left, WORD right);
BOOL WinCD_GetVolume(WORD far *left, WORD far *right);
BYTE WinCD_GetDriveCount(void);
BYTE WinCD_GetDriveLetter(BYTE index);
void WinCD_UpdateDisplay(HWND hwnd);

#endif
