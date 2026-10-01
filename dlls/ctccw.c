#include <windows.h>
#include <mmsystem.h>
#include <string.h>
#include <stdlib.h>
#include "../src/windows/types.h"
#include "../src/common/player.h"
#include "ctccw.h"

static WINDOW_PLAYER_STATE g_state;
static BYTE g_track_count;
static BOOL g_device_open;
static BYTE g_aux_device;
static char g_device[8];
static HINSTANCE g_hInst;

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
    return mciSendString(cmd,
                         buf,
                         size,
                         NULL) == 0;
}

static BOOL set_aux_volume(WORD left,
                           WORD right)
{
    DWORD packed;

    if (auxGetNumDevs() == 0)
        return FALSE;

    g_aux_device = 0;

    packed = (DWORD)(
        (left & 0xFFFF) |
        ((DWORD)(right & 0xFFFF) << 16)
    );

    if (auxSetVolume(g_aux_device,
                     packed) != 0)
        return FALSE;

    g_state.volume_left = left;
    g_state.volume_right = right;

    return TRUE;
}

BOOL CTCCW_API Init(HWND hwnd)
{
    g_hInst = GetWindowWord(hwnd,
                            GWW_HINSTANCE);

    memset(&g_state,
           0,
           sizeof(g_state));

    g_state.current_track = 1;
    g_state.volume_left = 200;
    g_state.volume_right = 200;
    g_state.drive_letter = 'C';

    g_track_count = 0;
    g_device_open = FALSE;
    g_aux_device = 0;
    g_device[0] = '\0';

    set_aux_volume(g_state.volume_left,
                   g_state.volume_right);

    return TRUE;
}

void CTCCW_API Exit(void)
{
    if (g_device_open) {
        mci_ok("stop cdplayer");
        mci_ok("close cdplayer");

        g_device_open = FALSE;
    }

    g_track_count = 0;
    g_state.is_playing = FALSE;
    g_state.is_paused = FALSE;
}

BOOL CTCCW_API OpenCD(BYTE drive)
{
    char cmd[64];
    char buf[32];

    if (g_device_open) {
        mci_ok("stop cdplayer");
        mci_ok("close cdplayer");

        g_device_open = FALSE;
    }

    wsprintf(cmd,
             "open %c:\\ type cdaudio alias cdplayer",
             drive);

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

    g_track_count = (BYTE)atoi(buf);

    if (g_track_count > MAX_TRACKS)
        g_track_count = MAX_TRACKS;

    if (g_track_count == 0) {
        CloseCD();
        return FALSE;
    }

    g_state.current_track = 1;
    g_state.total_tracks = g_track_count;
    g_state.drive_letter = drive;
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

    g_track_count = 0;
    g_state.total_tracks = 0;
    g_state.is_playing = FALSE;
    g_state.is_paused = FALSE;
}

BOOL CTCCW_API PlayTrack(BYTE track)
{
    char cmd[64];

    if (!g_device_open ||
        track == 0 ||
        track > g_track_count)
        return FALSE;

    wsprintf(cmd,
             "play cdplayer from %u to %u",
             track,
             (unsigned)(track + 1));

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

    {
        char cmd[64];

        wsprintf(cmd,
                 "status cdplayer position %u",
                 current_track);

        if (mci_status(cmd,
                       buf,
                       sizeof(buf))) {

            track_start = (DWORD)atol(buf);
        }
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

    if (!left ||
        !right ||
        auxGetNumDevs() == 0)
        return FALSE;

    g_aux_device = 0;

    if (auxGetVolume(g_aux_device,
                     &value) != 0)
        return FALSE;

    *left = (WORD)(value & 0xFFFF);
    *right = (WORD)((value >> 16) & 0xFFFF);

    return TRUE;
}

BYTE CTCCW_API GetDriveCount(void)
{
    BYTE count = 0;
    char root[4];
    char d;

    for (d = 'C'; d <= 'Z'; ++d) {
        wsprintf(root,
                 "%c:\\",
                 d);

        if (GetDriveType(root) == DRIVE_CDROM)
            ++count;
    }

    return count;
}

BYTE CTCCW_API GetDriveLetter(BYTE index)
{
    BYTE count = 0;
    char root[4];
    char d;

    for (d = 'C'; d <= 'Z'; ++d) {
        wsprintf(root,
                 "%c:\\",
                 d);

        if (GetDriveType(root) == DRIVE_CDROM) {
            if (count == index)
                return (BYTE)d;

            ++count;
        }
    }

    return 0;
}

int __far __pascal LibMain(HINSTANCE hInst,
                           WORD wDataSeg,
                           WORD cbHeapSize)
{
    g_hInst = hInst;

    wDataSeg = wDataSeg;
    cbHeapSize = cbHeapSize;

    return 1;
}

int __far __pascal _WEP(int bSystemExit)
{
    bSystemExit = bSystemExit;

    Exit();

    return 1;
}
