#include <windows.h>
#include <mmsystem.h>
#include <string.h>
#include "ctccw.h"
#include "types.h"

static PLAYER_STATE g_state;
static BYTE g_tracks[MAX_TRACKS];
static BYTE g_track_count;
static HANDLE g_hDevice;
static HINSTANCE g_hInst;
static char g_device[32];

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

BOOL CTCCW_API Init(HWND hwnd)
{
    g_hInst = GetWindowWord(hwnd, GWW_HINSTANCE);
    g_state = STATE_STOPPED;
    g_track_count = 0;
    g_hDevice = NULL;
    g_device[0] = '\0';
    return TRUE;
}

void CTCCW_API Exit(void)
{
    if (g_hDevice)
    {
        mci_command("close cdplayer");
        g_hDevice = NULL;
    }
    g_state = STATE_STOPPED;
}

BOOL CTCCW_API OpenCD(BYTE drive)
{
    if (!ensure_device(drive))
        return FALSE;

    char cmd[64];
    wsprintf(cmd, "status cdplayer number of tracks");
    char buf[32];
    if (mciSendString(cmd, buf, sizeof(buf), NULL) != 0)
    {
        g_track_count = 0;
        return FALSE;
    }

    g_track_count = (BYTE)atoi(buf);
    if (g_track_count > MAX_TRACKS)
        g_track_count = MAX_TRACKS;

    for (BYTE i = 0; i < g_track_count; i++)
    {
        g_tracks[i] = i + 1;
    }

    g_state = STATE_STOPPED;
    return TRUE;
}

void CTCCW_API CloseCD(void)
{
    if (g_hDevice)
    {
        mci_command("stop cdplayer");
        mci_command("close cdplayer");
        g_hDevice = NULL;
    }
    g_state = STATE_STOPPED;
    g_track_count = 0;
}

BOOL CTCCW_API PlayTrack(BYTE track)
{
    if (!g_hDevice || track == 0 || track > g_track_count)
        return FALSE;

    char cmd[64];
    wsprintf(cmd, "play cdplayer from %u to %u", track, track + 1);
    if (mciSendString(cmd, NULL, 0, NULL) != 0)
        return FALSE;

    g_state = STATE_PLAYING;
    return TRUE;
}

void CTCCW_API Stop(void)
{
    if (g_hDevice)
    {
        mci_command("stop cdplayer");
        mci_command("seek cdplayer to start");
    }
    g_state = STATE_STOPPED;
}

void CTCCW_API Pause(void)
{
    if (g_hDevice && g_state == STATE_PLAYING)
    {
        mci_command("pause cdplayer");
        g_state = STATE_PAUSED;
    }
}

void CTCCW_API Resume(void)
{
    if (g_hDevice && g_state == STATE_PAUSED)
    {
        mci_command("resume cdplayer");
        g_state = STATE_PLAYING;
    }
}

BOOL CTCCW_API Eject(void)
{
    if (!g_hDevice)
        return FALSE;

    if (mciSendString("set cdplayer door open", NULL, 0, NULL) != 0)
        return FALSE;

    g_state = STATE_STOPPED;
    return TRUE;
}

BOOL CTCCW_API CloseTray(void)
{
    if (!g_hDevice)
        return FALSE;

    if (mciSendString("set cdplayer door closed", NULL, 0, NULL) != 0)
        return FALSE;

    return TRUE;
}

BOOL CTCCW_API GetStatus(BYTE far *track, DWORD far *pos, DWORD far *length)
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

BOOL CTCCW_API SetVolume(WORD left, WORD right)
{
    if (!g_hDevice)
        return FALSE;

    WORD vol = (WORD)((left & 0xFF) | ((right & 0xFF) << 8));
    char cmd[32];
    wsprintf(cmd, "set cdplayer audio volume to %u", vol);
    return mciSendString(cmd, NULL, 0, NULL) == 0;
}

BOOL CTCCW_API GetVolume(WORD far *left, WORD far *right)
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

BYTE CTCCW_API GetDriveCount(void)
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

BYTE CTCCW_API GetDriveLetter(BYTE index)
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

int __far __pascal LibMain(HINSTANCE hInst, WORD wDataSeg, WORD cbHeapSize)
{
    g_hInst = hInst;
    return 1;
}

int __far __pascal _WEP(int bSystemExit)
{
    Exit();
    return 1;
}
