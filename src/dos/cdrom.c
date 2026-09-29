#include "types.h"
#include "cdrom.h"
#include "dos.h"
#include "string.h"
#include "bios.h"

#pragma aux CheckMSCDEX = \
    "mov ax, 0x1500" \
    "int 0x2F" \
    "mov bl, al" \
    "mov al, 0" \
    "cmp bl, 0xFF" \
    "jne L_not_found" \
    "mov al, 1" \
    "L_not_found:" \
    parm [] \
    value [al] \
    modify [ax] [bx] [cx] [dx] [si] [di]

BOOL CheckMSCDEX(void) {
}

#pragma aux GetCDDriveCount = \
    "mov ax, 0x150B" \
    "int 0x2F" \
    "mov al, bl" \
    parm [] \
    value [al] \
    modify [ax] [bx] [cx] [dx] [si] [di]

BYTE GetCDDriveCount(void) {
}

#pragma aux GetCDDriveLetter = \
    "mov ax, 0x150D" \
    "int 0x2F" \
    "mov al, dl" \
    parm [bl] \
    value [al] \
    modify [ax] [bx] [cx] [dx] [si] [di]

BYTE GetCDDriveLetter(BYTE drive_index) {
}

BYTE GetCDDriveUnit(BYTE drive_letter) {
    BYTE count = GetCDDriveCount();
    BYTE i;
    for (i = 0; i < count; i++) {
        if (GetCDDriveLetter(i) == drive_letter) {
            return i;
        }
    }
    return 0xFF;
}

#pragma aux Int2F_1502 = \
    "mov ax, 0x1502" \
    "int 0x2F" \
    parm [cx] [dx] [es bx] \
    modify [ax] [bx] [cx] [dx] [si] [di]

void Int2F_1502(WORD cx, WORD dx, void far *buffer) {
}

BOOL ReadCDTOC(BYTE drive_unit, CDROM_TOC_ENTRY far *toc, BYTE far *track_count) {
    BYTE header[8];
    BYTE first_track;
    BYTE last_track;
    BYTE i;
    BYTE far *buf_far;
    BYTE bcd_min;
    BYTE bcd_sec;
    BYTE bcd_frame;
    WORD lba_word;

    buf_far = (BYTE far *)header;
    Int2F_1502(0, drive_unit, buf_far);

    first_track = header[0];
    last_track = header[1];
    if (last_track < first_track || last_track == 0) {
        return FALSE;
    }
    if (last_track - first_track + 1 > MAX_TRACKS) {
        return FALSE;
    }

    *track_count = last_track - first_track + 1;

    for (i = 0; i < *track_count; i++) {
        buf_far = (BYTE far *)&toc[i];
        Int2F_1502(first_track + i, drive_unit, buf_far);

        toc[i].track_number = first_track + i;
        bcd_min = ((BYTE far *)buf_far)[3];
        bcd_sec = ((BYTE far *)buf_far)[4];
        bcd_frame = ((BYTE far *)buf_far)[5];

        toc[i].minutes = (bcd_min >> 4) * 10 + (bcd_min & 0x0F);
        toc[i].seconds = (bcd_sec >> 4) * 10 + (bcd_sec & 0x0F);
        toc[i].frames = (bcd_frame >> 4) * 10 + (bcd_frame & 0x0F);

        lba_word = *((WORD far *)buf_far + 3);
        toc[i].lba = lba_word;
    }

    return TRUE;
}

#pragma aux PlayCDAudio = \
    "mov ax, 0x1510" \
    "int 0x2F" \
    parm [bx] [cx dx] [si di] \
    value [ax] \
    modify [ax] [bx] [cx] [dx] [si] [di]

BOOL PlayCDAudio(BYTE drive_unit, DWORD start_lba, DWORD end_lba) {
}

#pragma aux StopCDAudio = \
    "mov ax, 0x1511" \
    "int 0x2F" \
    parm [bx] \
    modify [ax] [bx] [cx] [dx] [si] [di]

BOOL StopCDAudio(BYTE drive_unit) {
}

#pragma aux ResumeCDAudio = \
    "mov ax, 0x1512" \
    "int 0x2F" \
    parm [bx] \
    modify [ax] [bx] [cx] [dx] [si] [di]

BOOL ResumeCDAudio(BYTE drive_unit) {
}

#pragma aux EjectCD = \
    "mov ax, 0x1513" \
    "int 0x2F" \
    parm [bx] \
    modify [ax] [bx] [cx] [dx] [si] [di]

BOOL EjectCD(BYTE drive_unit) {
}

#pragma aux CloseTray = \
    "mov ax, 0x1514" \
    "int 0x2F" \
    parm [bx] \
    modify [ax] [bx] [cx] [dx] [si] [di]

BOOL CloseTray(BYTE drive_unit) {
}

BOOL GetVolume(BYTE drive_unit, BYTE far *left, BYTE far *right) {
    #pragma aux GetVolume = \
        "push bx" \
        "push dx" \
        "mov bl, al" \
        "mov ax, 0x1515" \
        "int 0x2F" \
        "mov es:[bx], ch" \
        "mov es:[dx], cl" \
        "mov al, 0" \
        "jnc L_ok" \
        "mov al, 1" \
        "L_ok:" \
        "pop dx" \
        "pop bx" \
        parm [al] [es bx] [es dx] \
        value [al] \
        modify [ax] [bx] [cx] [dx] [si] [di];
}

BOOL SetVolume(BYTE drive_unit, BYTE left, BYTE right) {
    #pragma aux SetVolume = \
        "push bx" \
        "mov bl, al" \
        "mov ax, 0x1516" \
        "mov ch, dl" \
        "mov cl, dh" \
        "int 0x2F" \
        "mov al, 0" \
        "jnc L_ok" \
        "mov al, 1" \
        "L_ok:" \
        "pop bx" \
        parm [al] [dl] [dh] \
        value [al] \
        modify [ax] [bx] [cx] [dx] [si] [di];
}

void LBAtoMSF(DWORD lba, BYTE far *min, BYTE far *sec, BYTE far *frame) {
    DWORD temp;
    *min = (BYTE)((lba / (FRAMES_PER_SECOND * 60)) % 60);
    temp = lba / FRAMES_PER_SECOND;
    *sec = (BYTE)(temp % 60);
    *frame = (BYTE)(lba % FRAMES_PER_SECOND);
}

DWORD MSFtoLBA(BYTE min, BYTE sec, BYTE frame) {
    DWORD lba;
    lba = (DWORD)min * 60 * FRAMES_PER_SECOND;
    lba += (DWORD)sec * FRAMES_PER_SECOND;
    lba += frame;
    return lba;
}
