#ifndef CDROM_H
#define CDROM_H

#include "types.h"

#define MSCDEX_INT 0x2F
#define MSCDEX_AX_INSTALL_CHECK 0x1500
#define MSCDEX_AX_GET_VERSION 0x150C
#define MSCDEX_AX_GET_DISK_INFO 0x150D
#define MSCDEX_AX_GET_DRIVE_COUNT 0x150B
#define MSCDEX_AX_READ_TOC 0x1502
#define MSCDEX_AX_PLAY_AUDIO 0x1510
#define MSCDEX_AX_STOP_AUDIO 0x1511
#define MSCDEX_AX_RESUME_AUDIO 0x1512
#define MSCDEX_AX_EJECT 0x1513
#define MSCDEX_AX_CLOSE_TRAY 0x1514
#define MSCDEX_AX_GET_VOLUME 0x1515
#define MSCDEX_AX_SET_VOLUME 0x1516

#define MAX_TRACKS 100
#define MAX_DRIVES 26
#define FRAMES_PER_SECOND 75
#define SECTORS_PER_SECOND 75

BOOL CheckMSCDEX(void);
BYTE GetCDDriveCount(void);
BYTE GetCDDriveLetter(BYTE drive_index);
BYTE GetCDDriveUnit(BYTE drive_letter);
BOOL ReadCDTOC(BYTE drive_unit, CDROM_TOC_ENTRY far *toc, BYTE far *track_count);
BOOL PlayCDAudio(BYTE drive_unit, DWORD start_lba, DWORD end_lba);
BOOL StopCDAudio(BYTE drive_unit);
BOOL ResumeCDAudio(BYTE drive_unit);
BOOL EjectCD(BYTE drive_unit);
BOOL CloseTray(BYTE drive_unit);
BOOL GetVolume(BYTE drive_unit, BYTE far *left, BYTE far *right);
BOOL SetVolume(BYTE drive_unit, BYTE left, BYTE right);
void LBAtoMSF(DWORD lba, BYTE far *min, BYTE far *sec, BYTE far *frame);
DWORD MSFtoLBA(BYTE min, BYTE sec, BYTE frame);
BOOL ReadPort(WORD port);
void WritePort(WORD port, BYTE value);

#endif
