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
static int  g_aux_id = -1;      /* aux device id of the CD audio line, -1 = unknown */

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
    buf[0] = '\0';

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

/* All positions/lengths are handled in milliseconds. */
static void set_ms_time_format(void)
{
    mci_ok("set cdplayer time format milliseconds");
}

/* Locate the aux device that controls the CD audio line. */
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

    g_win_state.volume_left = left;
    g_win_state.volume_right = right;

    return TRUE;
}

BOOL WinCD_Init(HWND hwnd)
{
    (void)hwnd;

    memset(&g_win_state,
           0,
           sizeof(g_win_state));

    g_win_state.current_track = 1;
    g_win_state.volume_left = 200;
    g_win_state.volume_right = 200;
    g_win_state.drive_letter = 0;

    g_device_open = FALSE;
    g_aux_id = -1;

    set_aux_volume(g_win_state.volume_left,
                   g_win_state.volume_right);

    return TRUE;
}

void WinCD_Cleanup(void)
{
    WinCD_CloseDrive();
}

BOOL WinCD_IsOpen(void)
{
    return g_device_open;
}

BOOL WinCD_OpenDrive(BYTE drive)
{
    char cmd[80];
    char buf[32];

    WinCD_CloseDrive();

    wsprintf(cmd,
             "open %c: type cdaudio alias cdplayer",
             (int)drive);

    if (!mci_ok(cmd))
        return FALSE;

    g_device_open = TRUE;

    set_ms_time_format();

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

    g_win_state.drive_letter = (char)drive;
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
    char cmd[128];
    char buf[32];
    DWORD start;
    DWORD length;
    DWORD end;

    if (!g_device_open ||
        track == 0 ||
        track > g_win_state.total_tracks)
        return FALSE;

    /* MCI position/length queries use the current time format (milliseconds). */
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
    if (end < start)              /* DWORD overflow guard */
        return FALSE;

    /*
     * Explicitly supply an end position.  Without "to", MCI continues
     * through all following tracks instead of stopping at the selected
     * track's boundary.
     */
    wsprintf(cmd,
             "play cdplayer from %lu to %lu",
             start,
             end);

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

    /*
     * Display the length of the current track, not the entire disc.  The
     * cdaudio MCI status command explicitly supports "length track N".
     */
    wsprintf(cmd,
             "status cdplayer length track %u",
             (unsigned)current_track);

    if (mci_status(cmd, buf, sizeof(buf))) {
        total_length = (DWORD)atol(buf);
    } else if (mci_status("status cdplayer length",
                          buf,
                          sizeof(buf))) {
        total_length = (DWORD)atol(buf);
    } else {
        total_length = 0;
    }

    /* start of the current track: "position track N" (not just "position N") */
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

    update_play_state();

    return TRUE;
}

/* left/right: 0..255 */
BOOL WinCD_SetVolume(WORD left,
                     WORD right)
{
    return set_aux_volume(left, right);
}

/* left/right: 0..255 */
BOOL WinCD_GetVolume(WORD far *left,
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

    g_win_state.volume_left = *left;
    g_win_state.volume_right = *right;

    return TRUE;
}

/*
 * Windows 3.0/3.1 GetDriveType() cannot distinguish a CD-ROM from other
 * removable drives.  Do not call the DOS INT 2Fh interface from a Win16
 * protected-mode application: probe the cdaudio MCI driver instead.
 *
 * A successful "open X: type cdaudio" identifies X: as a CD-audio-capable
 * drive.  This also works when the tray is empty; the caller can subsequently
 * open the drive and query its table of contents.
 */
static BOOL probe_cd_drive(BYTE drive_letter)
{
    char cmd[96];

    if (drive_letter < 'A' || drive_letter > 'Z')
        return FALSE;

    wsprintf(cmd,
             "open %c: type cdaudio alias qcdprobe wait",
             (int)drive_letter);

    if (!mci_ok(cmd))
        return FALSE;

    mci_ok("close qcdprobe wait");
    return TRUE;
}

BYTE WinCD_GetDriveCount(void)
{
    BYTE count = 0;
    int d;

    for (d = 0; d < 26; ++d) {
        if (probe_cd_drive((BYTE)('A' + d)))
            ++count;
    }

    return count;
}

BYTE WinCD_GetDriveLetter(BYTE index)
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

void WinCD_UpdateDisplay(HWND hwnd)
{
    BYTE track;
    DWORD pos;
    DWORD length;

    if (g_device_open &&
        WinCD_GetStatus(&track,
                        &pos,
                        &length)) {

        if (track >= 1 && track <= g_win_state.total_tracks)
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
}
