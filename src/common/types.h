#ifndef TYPES_H
#define TYPES_H

/*
 * Basic types shared by the DOS and Windows parts.
 *
 * <windows.h> already provides BYTE, WORD, DWORD, UINT, BOOL, TRUE, FALSE,
 * NULL, FAR and PASCAL. Any file that includes <windows.h> must define
 * QCD_WINDOWS_TYPES before including this header so that those names are
 * not declared a second time (conflicting typedefs / macro redefinitions).
 */
#ifndef QCD_WINDOWS_TYPES

typedef unsigned char  BYTE;
typedef unsigned short WORD;
typedef unsigned long  DWORD;
typedef unsigned int   UINT;
typedef int            BOOL;

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef NULL
#define NULL 0
#endif

#endif /* QCD_WINDOWS_TYPES */

#define MAX_TRACKS 100

typedef struct {
    BYTE track_number;
    BYTE minutes;
    BYTE seconds;
    BYTE frames;
    DWORD lba;
} CDROM_TOC_ENTRY;

typedef struct {
    char drive_letter;
    BYTE unit_number;
    BOOL is_audio;
} CDROM_DRIVE_INFO;

typedef struct {
    BYTE current_track;
    BYTE current_minutes;
    BYTE current_seconds;
    BYTE current_frames;
    BYTE total_tracks;
    BYTE total_minutes;
    BYTE total_seconds;
    BYTE volume;            /* 0..255 */
    BOOL is_playing;
    BOOL is_paused;
    char drive_letter;
    BYTE unit_number;       /* MSCDEX drive number, 0 = A: */
    DWORD leadout_lba;
    CDROM_TOC_ENTRY toc[MAX_TRACKS];
} PLAYER_STATE;

#endif
