#include <windows.h>
#include <commctrl.h>
#include <string.h>
#include "types.h"
#include "cdrom.h"
#include "window.h"
#include "resource.h"

static HWND g_hwndMain = NULL;
static HWND g_hwndTrackList = NULL;
static HWND g_hwndTimeCurrent = NULL;
static HWND g_hwndTimeTotal = NULL;
static HWND g_hwndTimeTrack = NULL;
static HWND g_hwndStatusText = NULL;
static HWND g_hwndVolumeSlider = NULL;
static HWND g_hwndVolumeLabel = NULL;

extern HINSTANCE g_hInst;

void UpdateTrackList(HWND hwnd)
{
    if (!g_hwndTrackList)
        return;

    SendMessage(g_hwndTrackList,
                LB_RESETCONTENT,
                0,
                0);

    {
        char buf[64];
        BYTE i;

        for (i = 1; i <= g_win_state.total_tracks; i++) {
            wsprintf(buf, "Track %2d", i);

            SendMessage(g_hwndTrackList,
                        LB_ADDSTRING,
                        0,
                        (LPARAM)buf);
        }
    }

    if (g_win_state.current_track > 0 &&
        g_win_state.current_track <= g_win_state.total_tracks) {

        SendMessage(g_hwndTrackList,
                    LB_SETCURSEL,
                    g_win_state.current_track - 1,
                    0);
    }
}

void UpdateTimeDisplay(HWND hwnd)
{
    char buf[32];

    hwnd = hwnd;

    if (g_hwndTimeCurrent) {
        wsprintf(buf,
                 "%02d:%02d.%02d",
                 g_win_state.current_minutes,
                 g_win_state.current_seconds,
                 g_win_state.current_frames);

        SetWindowText(g_hwndTimeCurrent, buf);
    }

    if (g_hwndTimeTotal) {
        wsprintf(buf,
                 "%02d:%02d.%02d",
                 g_win_state.total_minutes,
                 g_win_state.total_seconds,
                 g_win_state.total_frames);

        SetWindowText(g_hwndTimeTotal, buf);
    }

    if (g_hwndTimeTrack) {
        wsprintf(buf,
                 "%d / %d",
                 g_win_state.current_track,
                 g_win_state.total_tracks);

        SetWindowText(g_hwndTimeTrack, buf);
    }
}

void UpdateStatusText(HWND hwnd, UINT string_id)
{
    char buf[128];

    hwnd = hwnd;

    if (!g_hwndStatusText)
        return;

    LoadString(g_hInst,
               string_id,
               buf,
               sizeof(buf));

    SetWindowText(g_hwndStatusText, buf);
}

void UpdateVolumeSlider(HWND hwnd)
{
    WORD left;
    WORD right;
    int vol;
    char buf[32];

    hwnd = hwnd;

    if (!g_hwndVolumeSlider ||
        !g_hwndVolumeLabel)
        return;

    if (WinCD_GetVolume(&left, &right)) {
        vol = (left + right) / 2;

        SendMessage(g_hwndVolumeSlider,
                    TBM_SETPOS,
                    TRUE,
                    vol);

        wsprintf(buf,
                 "Volume: %d%%",
                 vol * 100 / 255);

        SetWindowText(g_hwndVolumeLabel, buf);
    }
}

static void OnCommand(HWND hwnd,
                      WPARAM wParam,
                      LPARAM lParam)
{
    WORD id = LOWORD(wParam);
    WORD code = HIWORD(wParam);

    switch (id) {
        case IDC_BTN_PLAY:
            if (g_win_state.current_track > 0) {
                WinCD_PlayTrack(g_win_state.current_track);
                WinCD_UpdateDisplay(hwnd);
            }
            break;

        case IDC_BTN_STOP:
            WinCD_Stop();
            WinCD_UpdateDisplay(hwnd);
            break;

        case IDC_BTN_PAUSE:
            if (g_win_state.is_paused) {
                WinCD_Resume();
            } else if (g_win_state.is_playing) {
                WinCD_Pause();
            }

            WinCD_UpdateDisplay(hwnd);
            break;

        case IDC_BTN_PREV:
            if (g_win_state.current_track > 1) {
                WinCD_PlayTrack(
                    g_win_state.current_track - 1);

                WinCD_UpdateDisplay(hwnd);
            }
            break;

        case IDC_BTN_NEXT:
            if (g_win_state.current_track <
                g_win_state.total_tracks) {

                WinCD_PlayTrack(
                    g_win_state.current_track + 1);

                WinCD_UpdateDisplay(hwnd);
            }
            break;

        case IDC_BTN_EJECT:
            WinCD_Eject();
            WinCD_UpdateDisplay(hwnd);
            break;

        case IDM_ABOUT:
        {
            char buf[512];

            LoadString(g_hInst,
                       IDS_ABOUT_TEXT,
                       buf,
                       sizeof(buf));

            MessageBox(hwnd,
                       buf,
                       "About QuickCD",
                       MB_OK | MB_ICONINFORMATION);
            break;
        }

        case IDM_EXIT:
            PostMessage(hwnd,
                        WM_CLOSE,
                        0,
                        0);
            break;
    }

    if (id == IDC_TRACK_LIST &&
        code == LBN_SELCHANGE) {

        int sel;

        sel = SendMessage(g_hwndTrackList,
                          LB_GETCURSEL,
                          0,
                          0);

        if (sel != LB_ERR) {
            g_win_state.current_track =
                (BYTE)(sel + 1);

            if (g_win_state.is_playing) {
                WinCD_PlayTrack(
                    g_win_state.current_track);

                WinCD_UpdateDisplay(hwnd);
            }
        }
    }

    if (id == IDC_VOLUME_SLIDER &&
        code == TB_THUMBTRACK) {

        int pos;

        pos = SendMessage(g_hwndVolumeSlider,
                          TBM_GETPOS,
                          0,
                          0);

        WinCD_SetVolume((WORD)pos,
                        (WORD)pos);

        UpdateVolumeSlider(hwnd);
    }
}

static void OnTimer(HWND hwnd)
{
    if (g_win_state.is_playing ||
        g_win_state.is_paused) {

        WinCD_UpdateDisplay(hwnd);
    }
}

static void OnInitDialog(HWND hwnd)
{
    BYTE drive_count;
    BYTE drive;

    g_hwndMain = hwnd;

    WinCD_Init(hwnd);

    g_hwndTrackList =
        GetDlgItem(hwnd, IDC_TRACK_LIST);

    g_hwndTimeCurrent =
        GetDlgItem(hwnd, IDC_TIME_CURRENT);

    g_hwndTimeTotal =
        GetDlgItem(hwnd, IDC_TIME_TOTAL);

    g_hwndTimeTrack =
        GetDlgItem(hwnd, IDC_TIME_TRACK);

    g_hwndStatusText =
        GetDlgItem(hwnd, IDC_STATUS_TEXT);

    g_hwndVolumeSlider =
        GetDlgItem(hwnd, IDC_VOLUME_SLIDER);

    g_hwndVolumeLabel =
        GetDlgItem(hwnd, IDC_VOLUME_LABEL);

    SendMessage(g_hwndVolumeSlider,
                TBM_SETRANGE,
                TRUE,
                MAKELONG(0, 255));

    SendMessage(g_hwndVolumeSlider,
                TBM_SETPOS,
                TRUE,
                200);

    drive_count = WinCD_GetDriveCount();

    if (drive_count == 0) {
        UpdateStatusText(hwnd, IDS_NODRIVE);
        return;
    }

    drive = WinCD_GetDriveLetter(0);

    if (drive == 0) {
        UpdateStatusText(hwnd, IDS_NODRIVE);
        return;
    }

    if (WinCD_OpenDrive(drive)) {
        UpdateStatusText(hwnd, IDS_READY);
        WinCD_UpdateDisplay(hwnd);
    } else {
        UpdateStatusText(hwnd, IDS_NOTOC);
    }

    SetTimer(hwnd, 1, 500, NULL);
}

BOOL CALLBACK MainWndProc(HWND hwnd,
                          UINT msg,
                          WPARAM wParam,
                          LPARAM lParam)
{
    lParam = lParam;

    switch (msg) {
        case WM_INITDIALOG:
            OnInitDialog(hwnd);
            return TRUE;

        case WM_COMMAND:
            OnCommand(hwnd,
                      wParam,
                      lParam);
            return TRUE;

        case WM_TIMER:
            OnTimer(hwnd);
            return TRUE;

        case WM_CLOSE:
            KillTimer(hwnd, 1);
            WinCD_Cleanup();
            EndDialog(hwnd, 0);
            return TRUE;
    }

    return FALSE;
}
