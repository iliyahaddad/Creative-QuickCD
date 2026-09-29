#ifndef TYPES_H
#define TYPES_H

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef unsigned int UINT;
typedef int BOOL;

#define TRUE 1
#define FALSE 0
#define NULL 0

#define FAR far
#define PASCAL pascal

#define CDEVENT_DOS 1

typedef struct {
    WORD e_magic;
    WORD e_cblp;
    WORD e_cp;
    WORD e_crlc;
    WORD e_cparhdr;
    WORD e_minalloc;
    WORD e_maxalloc;
    WORD e_ss;
    WORD e_sp;
    WORD e_csum;
    WORD e_ip;
    WORD e_cs;
    WORD e_lfarlc;
    WORD e_ovno;
    WORD e_res[4];
    WORD e_oemid;
    WORD e_oeminfo;
    WORD e_res2[10];
    DWORD e_lfanew;
} MSDOS_MZ_HEADER;

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
    BYTE volume;
    BOOL is_playing;
    BOOL is_paused;
    char drive_letter;
    BYTE unit_number;
    CDROM_TOC_ENTRY toc[MAX_TRACKS];
} PLAYER_STATE;

#endif
