#include <dos.h>
#include <string.h>
#include "types.h"
#include "cdrom.h"

/*
 * MSCDEX access through INT 2Fh.
 *
 *   AX=1500h  installation check, BX = number of CD-ROM drives
 *   AX=150Bh  drive check (CX = drive, 0=A:); BX = ADADh and AX <> 0 if CD-ROM
 *   AX=150Dh  get drive letters (ES:BX -> 1 byte per drive, 0=A:)
 *   AX=1510h  send device driver request (CX = drive, ES:BX -> request header)
 *
 * Note: in this file "drive_unit" is the MSCDEX drive number (0 = A:), which
 * is what INT 2Fh/1510h expects in CX.
 */

#define MSCDEX_REQ_HEADER_SIZE 13
#define MSCDEX_CMD_IOCTL_INPUT   3
#define MSCDEX_CMD_IOCTL_OUTPUT 12
#define MSCDEX_CMD_PLAY_AUDIO  132
#define MSCDEX_CMD_STOP_AUDIO  133
#define MSCDEX_CMD_RESUME_AUDIO 136

/* IOCTL INPUT control codes */
#define MSCDEX_IOCTL_AUDIO_CHANNEL_INFO 4
#define MSCDEX_IOCTL_AUDIO_DISK_INFO   10
#define MSCDEX_IOCTL_AUDIO_TRACK_INFO  11
#define MSCDEX_IOCTL_AUDIO_Q_INFO      12
#define MSCDEX_IOCTL_AUDIO_STATUS      15

/* IOCTL OUTPUT control codes */
#define MSCDEX_IOCTL_EJECT                0
#define MSCDEX_IOCTL_AUDIO_CHANNEL_CONTROL 3
#define MSCDEX_IOCTL_CLOSE_TRAY           5

#define MSCDEX_STATUS_ERROR 0x8000
#define MSCDEX_STATUS_BUSY  0x0200

static BYTE g_drive_letters[MAX_DRIVES];
static BYTE g_request[64];
static BYTE g_control[32];
static DWORD g_disc_end_lba;

WORD MscdexGetDriveCount(void);
#pragma aux MscdexGetDriveCount = \
    "xor bx, bx" \
    "mov ax, 1500h" \
    "int 2Fh" \
    "mov ax, bx" \
    value [ax] \
    modify [bx cx dx si di es];

void MscdexGetDriveLetters(BYTE far *buffer);
#pragma aux MscdexGetDriveLetters = \
    "mov ax, 150Dh" \
    "int 2Fh" \
    parm [es bx] \
    modify [ax bx cx dx si di es];

/* Returns BX in the high word and AX in the low word. */
DWORD MscdexDriveCheckRaw(WORD drive_number);
#pragma aux MscdexDriveCheckRaw = \
    "mov ax, 150Bh" \
    "int 2Fh" \
    "mov dx, bx" \
    parm [cx] \
    value [dx ax] \
    modify [bx cx si di es];

void MscdexSendRequest(WORD drive_number, void far *request);
#pragma aux MscdexSendRequest = \
    "mov ax, 1510h" \
    "int 2Fh" \
    parm [cx] [es bx] \
    modify [ax bx cx dx si di es];

static void SetFarPointer(BYTE far *dst, void far *src)
{
    WORD off = FP_OFF(src);
    WORD seg = FP_SEG(src);

    dst[0] = (BYTE)(off & 0xFF);
    dst[1] = (BYTE)(off >> 8);
    dst[2] = (BYTE)(seg & 0xFF);
    dst[3] = (BYTE)(seg >> 8);
}

static WORD RequestStatus(void)
{
    return (WORD)g_request[3] | ((WORD)g_request[4] << 8);
}

static BOOL RequestSucceeded(void)
{
    return (RequestStatus() & MSCDEX_STATUS_ERROR) == 0;
}

static BOOL IoctlRequest(WORD drive_number, BYTE command,
                         BYTE *control, WORD length)
{
    memset(g_request, 0, sizeof(g_request));

    g_request[0] = (BYTE)(MSCDEX_REQ_HEADER_SIZE + 13);
    g_request[2] = command;
    SetFarPointer((BYTE far *)&g_request[14], (void far *)control);
    g_request[18] = (BYTE)(length & 0xFF);
    g_request[19] = (BYTE)(length >> 8);

    MscdexSendRequest(drive_number, (void far *)g_request);
    return RequestSucceeded();
}

static BOOL IoctlInput(WORD drive_number, BYTE *control, WORD length)
{
    return IoctlRequest(drive_number, MSCDEX_CMD_IOCTL_INPUT,
                        control, length);
}

static BOOL IoctlOutput(WORD drive_number, BYTE *control, WORD length)
{
    return IoctlRequest(drive_number, MSCDEX_CMD_IOCTL_OUTPUT,
                        control, length);
}

BOOL CheckMSCDEX(void)
{
    return MscdexGetDriveCount() != 0;
}

BYTE GetCDDriveCount(void)
{
    WORD count = MscdexGetDriveCount();

    if (count > MAX_DRIVES)
        count = MAX_DRIVES;

    return (BYTE)count;
}

BYTE GetCDDriveLetter(BYTE drive_index)
{
    BYTE count = GetCDDriveCount();

    if (drive_index >= count)
        return 0;

    memset(g_drive_letters, 0, sizeof(g_drive_letters));
    MscdexGetDriveLetters((BYTE far *)g_drive_letters);

    return (BYTE)('A' + g_drive_letters[drive_index]);
}

BYTE GetCDDriveUnit(BYTE drive_letter)
{
    WORD letter;
    DWORD raw;

    if (drive_letter >= 'a' && drive_letter <= 'z')
        drive_letter = (BYTE)(drive_letter - 'a' + 'A');

    if (drive_letter < 'A' || drive_letter > 'Z')
        return 0xFF;

    letter = (WORD)(drive_letter - 'A');

    raw = MscdexDriveCheckRaw(letter);

    if ((WORD)(raw >> 16) != 0xADAD || (WORD)(raw & 0xFFFF) == 0)
        return 0xFF;

    return (BYTE)letter;
}

/* Red Book (frame, second, minute in the low three bytes) -> HSG/LBA. */
static DWORD RedBookToLBA(DWORD address)
{
    BYTE frame = (BYTE)(address & 0xFF);
    BYTE sec = (BYTE)((address >> 8) & 0xFF);
    BYTE min = (BYTE)((address >> 16) & 0xFF);
    DWORD abs_frames = MSFtoLBA(min, sec, frame);

    if (abs_frames < 150UL)
        return 0;

    return abs_frames - 150UL;
}

BOOL ReadCDTOC(BYTE drive_unit, CDROM_TOC_ENTRY far *toc,
               BYTE far *track_count, DWORD far *leadout_lba)
{
    BYTE first_track;
    BYTE last_track;
    BYTE i;
    BYTE count;
    DWORD redbook;
    DWORD lba;

    if (!toc || !track_count)
        return FALSE;

    if (drive_unit == 0xFF)
        return FALSE;

    memset(g_control, 0, sizeof(g_control));
    g_control[0] = MSCDEX_IOCTL_AUDIO_DISK_INFO;

    if (!IoctlInput((WORD)drive_unit, g_control, 7))
        return FALSE;

    first_track = g_control[1];
    last_track = g_control[2];

    if (first_track == 0 || last_track < first_track)
        return FALSE;

    if ((WORD)last_track - first_track + 1 > MAX_TRACKS)
        return FALSE;

    redbook = (DWORD)g_control[3]
            | ((DWORD)g_control[4] << 8)
            | ((DWORD)g_control[5] << 16)
            | ((DWORD)g_control[6] << 24);

    lba = RedBookToLBA(redbook);
    g_disc_end_lba = lba;

    if (leadout_lba)
        *leadout_lba = lba;

    count = (BYTE)(last_track - first_track + 1);

    for (i = 0; i < count; ++i)
    {
        BYTE track = (BYTE)(first_track + i);

        memset(g_control, 0, sizeof(g_control));
        g_control[0] = MSCDEX_IOCTL_AUDIO_TRACK_INFO;
        g_control[1] = track;

        if (!IoctlInput((WORD)drive_unit, g_control, 7))
            return FALSE;

        redbook = (DWORD)g_control[2]
                | ((DWORD)g_control[3] << 8)
                | ((DWORD)g_control[4] << 16)
                | ((DWORD)g_control[5] << 24);

        lba = RedBookToLBA(redbook);

        toc[i].track_number = track;
        toc[i].lba = lba;

        LBAtoMSF(lba, &toc[i].minutes,
                 &toc[i].seconds,
                 &toc[i].frames);
    }

    /* Only publish the count once the whole TOC has been read. */
    *track_count = count;

    return TRUE;
}

static BOOL SendSimpleRequest(WORD drive_number, BYTE command)
{
    memset(g_request, 0, sizeof(g_request));

    g_request[0] = MSCDEX_REQ_HEADER_SIZE;
    g_request[2] = command;

    MscdexSendRequest(drive_number, (void far *)g_request);

    return RequestSucceeded();
}

BOOL PlayCDAudio(BYTE drive_unit, DWORD start_lba, DWORD end_lba)
{
    DWORD count;

    if (start_lba >= g_disc_end_lba)
        return FALSE;

    if (end_lba == 0xFFFFFFFFUL || end_lba > g_disc_end_lba)
        end_lba = g_disc_end_lba;

    if (end_lba <= start_lba)
        return FALSE;

    count = end_lba - start_lba;

    memset(g_request, 0, sizeof(g_request));

    g_request[0] = 22;
    g_request[2] = MSCDEX_CMD_PLAY_AUDIO;

    /* Addressing mode 0 = HSG/LBA. */
    g_request[13] = 0;

    g_request[14] = (BYTE)(start_lba & 0xFF);
    g_request[15] = (BYTE)((start_lba >> 8) & 0xFF);
    g_request[16] = (BYTE)((start_lba >> 16) & 0xFF);
    g_request[17] = (BYTE)((start_lba >> 24) & 0xFF);

    g_request[18] = (BYTE)(count & 0xFF);
    g_request[19] = (BYTE)((count >> 8) & 0xFF);
    g_request[20] = (BYTE)((count >> 16) & 0xFF);
    g_request[21] = (BYTE)((count >> 24) & 0xFF);

    MscdexSendRequest((WORD)drive_unit, (void far *)g_request);

    return RequestSucceeded();
}

/*
 * MSCDEX "stop audio": while playing it PAUSES; issued again while paused it
 * really stops. ResumeCDAudio continues after a pause.
 */
BOOL StopCDAudio(BYTE drive_unit)
{
    return SendSimpleRequest((WORD)drive_unit,
                             MSCDEX_CMD_STOP_AUDIO);
}

BOOL ResumeCDAudio(BYTE drive_unit)
{
    return SendSimpleRequest((WORD)drive_unit,
                             MSCDEX_CMD_RESUME_AUDIO);
}

BOOL EjectCD(BYTE drive_unit)
{
    memset(g_control, 0, sizeof(g_control));
    g_control[0] = MSCDEX_IOCTL_EJECT;
    return IoctlOutput((WORD)drive_unit, g_control, 1);
}

BOOL CloseTray(BYTE drive_unit)
{
    memset(g_control, 0, sizeof(g_control));
    g_control[0] = MSCDEX_IOCTL_CLOSE_TRAY;
    return IoctlOutput((WORD)drive_unit, g_control, 1);
}

BOOL GetVolume(BYTE drive_unit, BYTE far *left, BYTE far *right)
{
    if (!left || !right)
        return FALSE;

    memset(g_control, 0, sizeof(g_control));
    g_control[0] = MSCDEX_IOCTL_AUDIO_CHANNEL_INFO;

    if (!IoctlInput((WORD)drive_unit, g_control, 9))
        return FALSE;

    *left = g_control[2];
    *right = g_control[4];

    return TRUE;
}

BOOL SetVolume(BYTE drive_unit, BYTE left, BYTE right)
{
    memset(g_control, 0, sizeof(g_control));

    g_control[0] = MSCDEX_IOCTL_AUDIO_CHANNEL_CONTROL;

    g_control[1] = 0;
    g_control[2] = left;

    g_control[3] = 1;
    g_control[4] = right;

    g_control[5] = 2;
    g_control[6] = 0;

    g_control[7] = 3;
    g_control[8] = 0;

    return IoctlOutput((WORD)drive_unit, g_control, 9);
}

/* TRUE while the drive reports that an audio play operation is in progress
 * (busy bit of the request status word). */
BOOL IsCDPlaying(BYTE drive_unit)
{
    memset(g_control, 0, sizeof(g_control));
    g_control[0] = MSCDEX_IOCTL_AUDIO_STATUS;

    if (!IoctlInput((WORD)drive_unit, g_control, 11))
        return FALSE;

    return (RequestStatus() & MSCDEX_STATUS_BUSY) != 0;
}

BOOL GetCDAudioPosition(BYTE drive_unit, BYTE far *track,
                        BYTE far *min, BYTE far *sec,
                        BYTE far *frame)
{
    if (!track || !min || !sec || !frame)
        return FALSE;

    memset(g_control, 0, sizeof(g_control));
    g_control[0] = MSCDEX_IOCTL_AUDIO_Q_INFO;

    if (!IoctlInput((WORD)drive_unit, g_control, 11))
        return FALSE;

    if ((g_control[1] & 0x0F) != 1)
        return FALSE;

    *track = g_control[2];
    *min = g_control[4];
    *sec = g_control[5];
    *frame = g_control[6];

    return TRUE;
}

void LBAtoMSF(DWORD lba, BYTE far *min,
              BYTE far *sec, BYTE far *frame)
{
    DWORD temp;

    if (!min || !sec || !frame)
        return;

    *min = (BYTE)(lba / (FRAMES_PER_SECOND * 60UL));

    temp = lba / FRAMES_PER_SECOND;
    *sec = (BYTE)(temp % 60UL);

    *frame = (BYTE)(lba % FRAMES_PER_SECOND);
}

DWORD MSFtoLBA(BYTE min, BYTE sec, BYTE frame)
{
    return (DWORD)min * 60UL * FRAMES_PER_SECOND
         + (DWORD)sec * FRAMES_PER_SECOND
         + frame;
}
