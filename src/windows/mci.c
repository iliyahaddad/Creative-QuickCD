#include <windows.h>
#include <mmsystem.h>
#include <string.h>
#include <stdlib.h>
#include "windows/types.h"
#include "windows/cdrom.h"

static WINDOW_PLAYER_STATE g_win_state;
static HANDLE g_hDevice = NULL;
static char g_device[32];
static HINSTANCE g_hInst;

static void mci_command(const char *cmd)
{
    char buf[256];
    mciSendString(cmd, buf, sizeof(buf), NULL);
}

static BOOL ensure_device(BYTE drive)
{
    if (g_hDevice)
        return TRUE;

    wsprintf(g_device, "%c:\\", drive);
    char opencmd[64];
    wsprintf(opencmd, "open %s type cdaudio alias cdplayer", g_device);
    if (mciSendString(opencmd, NULL, 0, NULL) != 0)
        return FALSE;

    g_hDevice = (HANDLE)1;
    return TRUE;
}

BOOL WinCD_Init(HWND hwnd)
{
    g_hInst = GetWindowWord(hwnd, GWW_HINSTANCE);
    g_win_state.current_track = 1;
    g_win_state.total_tracks = 0;
    g_win_state.current_minutes = 0;
    g_win_state.current_seconds = 0;
    g_win_state.total_minutes = 0;
    g_win_state.total_seconds = 0;
    g_win_state.volume_left = 255;
    g_win_state.volume_right = 255;
    g_win_state.is_playing = FALSE;
    g_win_state.is_paused = FALSE;
    g_win_state.drive_letter = 'C';
    g_hDevice = NULL;
    g_device[0] = '\0';
    return TRUE;
}

void WinCD_Cleanup(void)
{
    if (g_hDevice)
    {
        mci_command("close cdplayer");
        g_hDevice = NULL;
    }
    g_win_state.is_playing = FALSE;
    g_win_state.is_paused = FALSE;
}

BOOL WinCD_OpenDrive(BYTE drive)
{
    if (!ensure_device(drive))
        return FALSE;

    char cmd[64];
    wsprintf(cmd, "status cdplayer number of tracks");
    char buf[32];
    if (mciSendString(cmd, buf, sizeof(buf), NULL) != 0)
    {
        g_win_state.total_tracks = 0;
        return FALSE;
    }

    g_win_state.total_tracks = (BYTE)atoi(buf);
    if (g_win_state.total_tracks > MAX_TRACKS)
        g_win_state.total_tracks = MAX_TRACKS;

    g_win_state.drive_letter = drive;
    g_win_state.current_track = 1;
    g_win_state.is_playing = FALSE;
    g_win_state.is_paused = FALSE;
    return TRUE;
}

void WinCD_CloseDrive(void)
{
    if (g_hDevice)
    {
        mci_command("stop cdplayer");
        mci_command("close cdplayer");
        g_hDevice = NULL;
    }
    g_win_state.is_playing = FALSE;
    g_win_state.is_paused = FALSE;
    g_win_state.total_tracks = 0;
}

BOOL WinCD_PlayTrack(BYTE track)
{
    if (!g_hDevice || track == 0 || track > g_win_state.total_tracks)
        return FALSE;

    char cmd[64];
    wsprintf(cmd, "play cdplayer from %u to %u", track, track + 1);
    if (mciSendString(cmd, NULL, 0, NULL) != 0)
        return FALSE;

    g_win_state.current_track = track;
    g_win_state.is_playing = TRUE;
    g_win_state.is_paused = FALSE;
    return TRUE;
}

void WinCD_Stop(void)
{
    if (g_hDevice)
    {
        mci_command("stop cdplayer");
        mci_command("seek cdplayer to start");
    }
    g_win_state.is_playing = FALSE;
    g_win_state.is_paused = FALSE;
}

void WinCD_Pause(void)
{
    if (g_hDevice && g_win_state.is_playing)
    {
        mci_command("pause cdplayer");
        g_win_state.is_paused = TRUE;
        g_win_state.is_playing = FALSE;
    }
}

void WinCD_Resume(void)
{
    if (g_hDevice && g_win_state.is_paused)
    {
        mci_command("resume cdplayer");
        g_win_state.is_playing = TRUE;
        g_win_state.is_paused = FALSE;
    }
}

BOOL WinCD_Eject(void)
{
    if (!g_hDevice)
        return FALSE;

    if (mciSendString("set cdplayer door open", NULL, 0, NULL) != 0)
        return FALSE;

    g_win_state.is_playing = FALSE;
    g_win_state.is_paused = FALSE;
    return TRUE;
}

BOOL WinCD_CloseTray(void)
{
    if (!g_hDevice)
        return FALSE;

    if (mciSendString("set cdplayer door closed", NULL, 0, NULL) != 0)
        return FALSE;

    return TRUE;
}

BOOL WinCD_GetStatus(BYTE far *track, DWORD far *pos, DWORD far *length)
{
    if (!g_hDevice)
        return FALSE;

    char buf[32];
    DWORD msf = 0;

    if (pos)
    {
        *pos = 0;
        if (mciSendString("status cdplayer position", buf, sizeof(buf), NULL) == 0)
        {
            DWORD total = atol(buf);
            *pos = total * 1000 / 75;
        }
    }

    if (length)
    {
        *length = 0;
        if (mciSendString("status cdplayer length", buf, sizeof(buf), NULL) == 0)
        {
            DWORD total = atol(buf);
            *length = total * 1000 / 75;
        }
    }

    if (track)
    {
        *track = 1;
        if (mciSendString("status cdplayer current track", buf, sizeof(buf), NULL) == 0)
        {
            *track = (BYTE)atoi(buf);
        }
    }

    return TRUE;
}

BOOL WinCD_SetVolume(WORD left, WORD right)
{
    if (!g_hDevice)
        return FALSE;

    WORD vol = (WORD)((left & 0xFF) | ((right & 0xFF) << 8));
    char cmd[32];
    wsprintf(cmd, "set cdplayer audio volume to %u", vol);
    return mciSendString(cmd, NULL, 0, NULL) == 0;
}

BOOL WinCD_GetVolume(WORD far *left, WORD far *right)
{
    if (!g_hDevice || !left || !right)
        return FALSE;

    char buf[32];
    if (mciSendString("status cdplayer volume", buf, sizeof(buf), NULL) != 0)
    {
        *left = 255;
        *right = 255;
        return FALSE;
    }

    WORD vol = (WORD)atoi(buf);
    *left = (BYTE)(vol & 0xFF);
    *right = (BYTE)((vol >> 8) & 0xFF);
    return TRUE;
}

BYTE WinCD_GetDriveCount(void)
{
    BYTE count = 0;
    for (char d = 'C'; d <= 'Z'; d++)
    {
        char root[4];
        wsprintf(root, "%c:\\", d);
        if (GetDriveType(root) == DRIVE_CDROM)
            count++;
    }
    return count;
}

BYTE WinCD_GetDriveLetter(BYTE index)
{
    BYTE count = 0;
    for (char d = 'C'; d <= 'Z'; d++)
    {
        char root[4];
        wsprintf(root, "%c:\\", d);
        if (GetDriveType(root) == DRIVE_CDROM)
        {
            if (count == index)
                return (BYTE)d;
            count++;
        }
    }
    return 'C';
}

void WinCD_UpdateDisplay(HWND hwnd)
{
    char buf[64];
    BYTE track;
    DWORD pos, length;
    
    if (WinCD_GetStatus(&track, &pos, &length))
    {
        g_win_state.current_track = track;
        
        BYTE min = (BYTE)(pos / 60000);
        BYTE sec = (BYTE)((pos % 60000) / 1000);
        BYTE frame = (BYTE)((pos % 1000) * 75 / 1000);
        g_win_state.current_minutes = min;
        g_win_state.current_seconds = sec;
        
        min = (BYTE)(length / 60000);
        sec = (BYTE)((length % 60000) / 1000);
        g_win_state.total_minutes = min;
        g_win_state.total_seconds = sec;
    }
    
    UpdateTrackList(hwnd);
    UpdateTimeDisplay(hwnd);
    
    if (g_win_state.is_playing) {
        if (g_win_state.is_paused) UpdateStatusText(hwnd, IDS_PAUSED);
        else UpdateStatusText(hwnd, IDS_PLAYING);
    } else {
        UpdateStatusText(hwnd, IDS_STOPPED);
    }
    
    UpdateVolumeSlider(hwnd);
}