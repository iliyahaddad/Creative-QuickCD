#include <windows.h>
#include <mmsystem.h>
#include <string.h>
#include <stdlib.h>
#include "types.h"
#include "cdrom.h"
#include "window.h"
#include "resource.h"

WINDOW_PLAYER_STATE g_win_state;

static BOOL g_device_open;
static BYTE g_aux_device;
static HINSTANCE g_hInst;

static BOOL mci_ok(const char *cmd)
{
    return mciSendString(cmd,
                         NULL,
                         0,
                         NULL) == 0;
}

static BOOL mci_status(const char *item,
                       char *buf,
                       WORD size)
{
    return mciSendString(item,
                         buf,
                         size,
                         NULL) == 0;
}

static void update_play_state(void)
{
    char mode[32];

    if (!g_device_open) {
        g_win_state.is_playing = FALSE;
        g_win_state.is_paused = FALSE;
        return;
    }

    mode[0] = '\0';

    if (!mci_status("status cdplayer mode",
                    mode,
                    sizeof(mode)))
        return;

    if (lstrcmpi(mode, "playing") == 0) {
        g_win_state.is_playing = TRUE;
        g_win_state.is_paused = FALSE;
    } else if (lstrcmpi(mode, "paused") == 0) {
        g_win_state.is_playing = TRUE;
        g_win_state.is_paused = TRUE;
    } else {
        g_win_state.is_playing = FALSE;
        g_win_state.is_paused = FALSE;
    }
}

static void set_msf_time_format(void)
{
    mci_ok("set cdplayer time format milliseconds");
}

static BOOL set_aux_volume(WORD left,
                           WORD right)
{
    WORD packed;

    if (auxGetNumDevs() == 0)
        return FALSE;

    g_aux_device = 0;

    packed = (WORD)((left & 0xFF) |
                    ((right & 0xFF) << 8));

    if (auxSetVolume(g_aux_device,
                     (DWORD)packed) != 0)
        return FALSE;

    g_win_state.volume_left = left;
    g_win_state.volume_right = right;

    return TRUE;
}

BOOL WinCD_Init(HWND hwnd)
{
    g_hInst = GetWindowWord(hwnd,
                            GWW_HINSTANCE);

    memset(&g_win_state,
           0,
           sizeof(g_win_state));

    g_win_state.current_track = 1;
    g_win_state.volume_left = 200;
    g_win_state.volume_right = 200;
    g_win_state.drive_letter = 'C';

    g_device_open = FALSE;
    g_aux_device = 0;

    set_aux_volume(g_win_state.volume_left,
                   g_win_state.volume_right);

    return TRUE;
}

void WinCD_Cleanup(void)
{
    if (g_device_open) {
        mci_ok("stop cdplayer");
        mci_ok("close cdplayer");

        g_device_open = FALSE;
    }

    g_win_state.is_playing = FALSE;
    g_win_state.is_paused = FALSE;
    g_win_state.total_tracks = 0;
}

BOOL WinCD_OpenDrive(BYTE drive)
{
    char cmd[80];
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

    set_msf_time_format();

    if (!mci_status(
            "status cdplayer number of tracks",
            buf,
            sizeof(buf))) {

        WinCD_CloseDrive();
        return FALSE;
    }

    g_win_state.total_tracks =
        (BYTE)atoi(buf);

    if (g_win_state.total_tracks > MAX_TRACKS)
        g_win_state.total_tracks = MAX_TRACKS;

    if (g_win_state.total_tracks == 0) {
        WinCD_CloseDrive();
        return FALSE;
    }

    g_win_state.drive_letter = drive;
    g_win_state.current_track = 1;
    g_win_state.is_playing = FALSE;
    g_win_state.is_paused = FALSE;

    return TRUE;
}

void WinCD_CloseDrive(void)
{
    if (g_device_open) {
        mci_ok("stop cdplayer");
        mci_ok("close cdplayer");

        g_device_open = FALSE;
    }

    g_win_state.is_playing = FALSE;
    g_win_state.is_paused = FALSE;
    g_win_state.total_tracks = 0;
}

BOOL WinCD_PlayTrack(BYTE track)
{
    char cmd[80];

    if (!g_device_open ||
        track == 0 ||
        track > g_win_state.total_tracks)
        return FALSE;

    wsprintf(cmd,
             "play cdplayer from %u to %u",
             track,
             (unsigned)(track + 1));

    if (!mci_ok(cmd))
        return FALSE;

    g_win_state.current_track = track;
    g_win_state.is_playing = TRUE;
    g_win_state.is_paused = FALSE;

    return TRUE;
}

void WinCD_Stop(void)
{
    if (g_device_open)
        mci_ok("stop cdplayer");

    g_win_state.is_playing = FALSE;
    g_win_state.is_paused = FALSE;
}

void WinCD_Pause(void)
{
    if (g_device_open &&
        g_win_state.is_playing &&
        !g_win_state.is_paused) {

        if (mci_ok("pause cdplayer"))
            g_win_state.is_paused = TRUE;
    }
}

void WinCD_Resume(void)
{
    if (g_device_open &&
        g_win_state.is_paused) {

        if (mci_ok("resume cdplayer"))
            g_win_state.is_paused = FALSE;
    }
}

BOOL WinCD_Eject(void)
{
    if (!g_device_open)
        return FALSE;

    if (!mci_ok("set cdplayer door open"))
        return FALSE;

    g_win_state.is_playing = FALSE;
    g_win_state.is_paused = FALSE;

    return TRUE;
}

BOOL WinCD_CloseTray(void)
{
    if (!g_device_open)
        return FALSE;

    return mci_ok("set cdplayer door closed");
}

BOOL WinCD_GetStatus(BYTE far *track,
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

    update_play_state();

    return TRUE;
}

BOOL WinCD_SetVolume(WORD left,
                     WORD right)
{
    return set_aux_volume(left, right);
}

BOOL WinCD_GetVolume(WORD far *left,
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

    g_win_state.volume_left = *left;
    g_win_state.volume_right = *right;

    return TRUE;
}

BYTE WinCD_GetDriveCount(void)
{
    BYTE count = 0;
    char root[4];
    char d;

    for (d = 'C'; d <= 'Z'; d++) {
        wsprintf(root,
                 "%c:\\",
                 d);

        if (GetDriveType(root) == DRIVE_CDROM)
            ++count;
    }

    return count;
}

BYTE WinCD_GetDriveLetter(BYTE index)
{
    BYTE count = 0;
    char root[4];
    char d;

    for (d = 'C'; d <= 'Z'; d++) {
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

void WinCD_UpdateDisplay(HWND hwnd)
{
    BYTE track;
    DWORD pos;
    DWORD length;

    if (!g_device_open)
        return;

    if (WinCD_GetStatus(&track,
                        &pos,
                        &length)) {

        g_win_state.current_track = track;

        g_win_state.current_minutes =
            (BYTE)(pos / 60000UL);

        g_win_state.current_seconds =
            (BYTE)((pos % 60000UL) / 1000UL);

        g_win_state.current_frames =
            (BYTE)(((pos % 1000UL) * 75UL) /
                   1000UL);

        g_win_state.total_minutes =
            (BYTE)(length / 60000UL);

        g_win_state.total_seconds =
            (BYTE)((length % 60000UL) /
                   1000UL);

        g_win_state.total_frames =
            (BYTE)(((length % 1000UL) * 75UL) /
                   1000UL);
    }

    UpdateTrackList(hwnd);
    UpdateTimeDisplay(hwnd);

    if (g_win_state.is_paused)
        UpdateStatusText(hwnd, IDS_PAUSED);
    else if (g_win_state.is_playing)
        UpdateStatusText(hwnd, IDS_PLAYING);
    else
        UpdateStatusText(hwnd, IDS_STOPPED);

    UpdateVolumeSlider(hwnd);
}
