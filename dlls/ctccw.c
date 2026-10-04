#include <windows.h>
#include <mmsystem.h>
#include <string.h>
#include <stdlib.h>
#include "../src/windows/types.h"
#include "ctccw.h"

/*
 * CTCCW.DLL - CD control for Windows (MCI based).
 * Volumes at this interface are 0..255.
 */

static WINDOW_PLAYER_STATE g_state;
static BOOL g_device_open;
static int  g_aux_id = -1;

void CTCCW_API CloseCD(void);

static BOOL mci_ok(const char *cmd)
{
    return mciSendString(cmd,
                         NULL,
                         0,
                         NULL) == 0;
}

static BOOL mci_status(const char *cmd,
                       char *buf,
                       WORD size)
{
    buf[0] = '\0';

    return mciSendString(cmd,
                         buf,
                         size,
                         NULL) == 0;
}

static BOOL find_cd_aux(void)
{
    UINT n;
    UINT i;
    AUXCAPS caps;

    if (g_aux_id >= 0)
        return TRUE;

    n = auxGetNumDevs();

    for (i = 0; i < n; i++) {
        if (auxGetDevCaps(i, &caps, sizeof(caps)) != 0)
            continue;

        if (caps.wTechnology == AUXCAPS_CDAUDIO &&
            (caps.dwSupport & AUXCAPS_VOLUME)) {
            g_aux_id = (int)i;
            return TRUE;
        }
    }

    return FALSE;
}

/* 0..255 -> 0..65535 */
static DWORD scale_up(WORD v)
{
    if (v > 255)
        v = 255;

    return (DWORD)v * 257UL;
}

static BOOL set_aux_volume(WORD left,
                           WORD right)
{
    DWORD packed;

    if (!find_cd_aux())
        return FALSE;

    packed = scale_up(left) | (scale_up(right) << 16);

    if (auxSetVolume((UINT)g_aux_id, packed) != 0)
        return FALSE;

    g_state.volume_left = left;
    g_state.volume_right = right;

    return TRUE;
}

BOOL CTCCW_API Init(HWND hwnd)
{
    (void)hwnd;

    memset(&g_state,
           0,
           sizeof(g_state));

    g_state.current_track = 1;
    g_state.volume_left = 200;
    g_state.volume_right = 200;
    g_state.drive_letter = 0;

    g_device_open = FALSE;
    g_aux_id = -1;

    set_aux_volume(g_state.volume_left,
                   g_state.volume_right);

    return TRUE;
}

void CTCCW_API Exit(void)
{
    CloseCD();
}

BOOL CTCCW_API OpenCD(BYTE drive)
{
    char cmd[80];
    char buf[32];

    CloseCD();

    wsprintf(cmd,
             "open %c: type cdaudio alias cdplayer",
             (int)drive);

    if (!mci_ok(cmd))
        return FALSE;

    g_device_open = TRUE;

    mci_ok("set cdplayer time format milliseconds");

    if (!mci_status(
            "status cdplayer number of tracks",
            buf,
            sizeof(buf))) {

        CloseCD();
        return FALSE;
    }

    g_state.total_tracks = (BYTE)atoi(buf);

    if (g_state.total_tracks > MAX_TRACKS)
        g_state.total_tracks = MAX_TRACKS;

    if (g_state.total_tracks == 0) {
        CloseCD();
        return FALSE;
    }

    g_state.current_track = 1;
    g_state.drive_letter = (char)drive;
    g_state.is_playing = FALSE;
    g_state.is_paused = FALSE;

    return TRUE;
}

void CTCCW_API CloseCD(void)
{
    if (g_device_open) {
        mci_ok("stop cdplayer");
        mci_ok("close cdplayer");

        g_device_open = FALSE;
    }

    g_state.total_tracks = 0;
    g_state.is_playing = FALSE;
    g_state.is_paused = FALSE;
}

BOOL CTCCW_API PlayTrack(BYTE track)
{
    char cmd[128];
    char buf[32];
    DWORD start;
    DWORD length;
    DWORD end;

    if (!g_device_open ||
        track == 0 ||
        track > g_state.total_tracks)
        return FALSE;

    wsprintf(cmd,
             "status cdplayer position track %u",
             (unsigned)track);

    if (!mci_status(cmd, buf, sizeof(buf)))
        return FALSE;

    start = (DWORD)atol(buf);

    wsprintf(cmd,
             "status cdplayer length track %u",
             (unsigned)track);

    if (!mci_status(cmd, buf, sizeof(buf)))
        return FALSE;

    length = (DWORD)atol(buf);
    if (length == 0)
        return FALSE;

    end = start + length;
    if (end < start)
        return FALSE;

    wsprintf(cmd,
             "play cdplayer from %lu to %lu",
             start,
             end);

    if (!mci_ok(cmd))
        return FALSE;

    g_state.current_track = track;
    g_state.is_playing = TRUE;
    g_state.is_paused = FALSE;

    return TRUE;
}

void CTCCW_API Stop(void)
{
    if (g_device_open)
        mci_ok("stop cdplayer");

    g_state.is_playing = FALSE;
    g_state.is_paused = FALSE;
}

void CTCCW_API Pause(void)
{
    if (g_device_open &&
        g_state.is_playing &&
        !g_state.is_paused) {

        if (mci_ok("pause cdplayer"))
            g_state.is_paused = TRUE;
    }
}

void CTCCW_API Resume(void)
{
    if (g_device_open &&
        g_state.is_paused) {

        if (mci_ok("resume cdplayer"))
            g_state.is_paused = FALSE;
    }
}

BOOL CTCCW_API Eject(void)
{
    if (!g_device_open)
        return FALSE;

    if (!mci_ok("set cdplayer door open"))
        return FALSE;

    g_state.is_playing = FALSE;
    g_state.is_paused = FALSE;

    return TRUE;
}

BOOL CTCCW_API CloseTray(void)
{
    if (!g_device_open)
        return FALSE;

    return mci_ok("set cdplayer door closed");
}

BOOL CTCCW_API GetStatus(BYTE far *track,
                         DWORD far *pos,
                         DWORD far *length)
{
    char buf[32];
    char cmd[64];
    BYTE current_track;
    DWORD current_position;
    DWORD track_start;
    DWORD total_length;

    if (!g_device_open)
        return FALSE;

    if (!mci_status(
            "status cdplayer current track",
            buf,
            sizeof(buf)))
        return FALSE;

    current_track = (BYTE)atoi(buf);

    if (!mci_status(
            "status cdplayer position",
            buf,
            sizeof(buf)))
        return FALSE;

    current_position = (DWORD)atol(buf);

    if (mci_status(
            "status cdplayer length",
            buf,
            sizeof(buf))) {

        total_length = (DWORD)atol(buf);
    } else {
        total_length = 0;
    }

    track_start = 0;

    wsprintf(cmd,
             "status cdplayer position track %u",
             (unsigned)current_track);

    if (mci_status(cmd,
                   buf,
                   sizeof(buf))) {

        track_start = (DWORD)atol(buf);
    }

    if (current_position >= track_start)
        current_position -= track_start;
    else
        current_position = 0;

    if (track)
        *track = current_track;

    if (pos)
        *pos = current_position;

    if (length)
        *length = total_length;

    return TRUE;
}

BOOL CTCCW_API SetVolume(WORD left,
                         WORD right)
{
    return set_aux_volume(left, right);
}

BOOL CTCCW_API GetVolume(WORD far *left,
                         WORD far *right)
{
    DWORD value;

    if (!left || !right || !find_cd_aux())
        return FALSE;

    if (auxGetVolume((UINT)g_aux_id,
                     &value) != 0)
        return FALSE;

    *left = (WORD)((value & 0xFFFFUL) >> 8);
    *right = (WORD)(((value >> 16) & 0xFFFFUL) >> 8);

    return TRUE;
}

/*
 * Windows 3.0/3.1 GetDriveType() cannot distinguish CD-ROM drives.
 * CTCCW is already MCI-based, so enumerate through the same cdaudio
 * interface instead of issuing DOS INT 2Fh calls from Win16 protected mode.
 */
static BOOL probe_cd_drive(BYTE drive_letter)
{
    char cmd[96];

    if (drive_letter < 'A' || drive_letter > 'Z')
        return FALSE;

    wsprintf(cmd,
             "open %c: type cdaudio alias ctccwprobe wait",
             (int)drive_letter);

    if (!mci_ok(cmd))
        return FALSE;

    mci_ok("close ctccwprobe wait");
    return TRUE;
}

BYTE CTCCW_API GetDriveCount(void)
{
    BYTE count = 0;
    int d;

    for (d = 0; d < 26; ++d) {
        if (probe_cd_drive((BYTE)('A' + d)))
            ++count;
    }

    return count;
}

BYTE CTCCW_API GetDriveLetter(BYTE index)
{
    BYTE count = 0;
    int d;

    for (d = 0; d < 26; ++d) {
        if (probe_cd_drive((BYTE)('A' + d))) {
            if (count == index)
                return (BYTE)('A' + d);

            ++count;
        }
    }

    return 0;
}

/*
 * Win16 DLL entry point. Watcom's C startup calls LibMain with FOUR
 * arguments (a __pascal callee pops its own arguments, so declaring only
 * three corrupts the stack). The default WEP supplied by the runtime is
 * used for unload; applications call Exit() themselves.
 */
int __far __pascal LibMain(HINSTANCE hInst,
                           WORD wDataSeg,
                           WORD cbHeapSize,
                           LPSTR lpszCmdLine)
{
    (void)hInst;
    (void)wDataSeg;
    (void)cbHeapSize;
    (void)lpszCmdLine;

    return 1;
}
