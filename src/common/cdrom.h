#ifndef CDROM_H
#define CDROM_H

#include "types.h"

#define MAX_DRIVES 26
#define FRAMES_PER_SECOND 75

/* DOS only: MSCDEX / port helpers (implemented in src/dos). */

BOOL CheckMSCDEX(void);
BYTE GetCDDriveCount(void);
BYTE GetCDDriveLetter(BYTE drive_index);
BYTE GetCDDriveUnit(BYTE drive_letter);
BOOL ReadCDTOC(BYTE drive_unit, CDROM_TOC_ENTRY far *toc,
               BYTE far *track_count, DWORD far *leadout_lba);
BOOL PlayCDAudio(BYTE drive_unit, DWORD start_lba, DWORD end_lba);
BOOL StopCDAudio(BYTE drive_unit);
BOOL ResumeCDAudio(BYTE drive_unit);
BOOL EjectCD(BYTE drive_unit);
BOOL CloseTray(BYTE drive_unit);
BOOL GetVolume(BYTE drive_unit, BYTE far *left, BYTE far *right);
BOOL SetVolume(BYTE drive_unit, BYTE left, BYTE right);
BOOL IsCDPlaying(BYTE drive_unit);
BOOL GetCDAudioPosition(BYTE drive_unit, BYTE far *track,
                        BYTE far *min, BYTE far *sec, BYTE far *frame);
void LBAtoMSF(DWORD lba, BYTE far *min, BYTE far *sec, BYTE far *frame);
DWORD MSFtoLBA(BYTE min, BYTE sec, BYTE frame);

BYTE ReadPort(WORD port);
void WritePort(WORD port, BYTE value);
WORD ReadPort16(WORD port);
void WritePort16(WORD port, WORD value);

#endif
