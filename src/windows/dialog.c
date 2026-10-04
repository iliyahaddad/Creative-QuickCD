#include <windows.h>
#include <string.h>
#include "types.h"
#include "cdrom.h"
#include "window.h"
#include "resource.h"

/*
 * Win16 conventions used here:
 *  - WM_COMMAND: wParam = control id, LOWORD(lParam) = control handle,
 *    HIWORD(lParam) = notification code.
 *  - WM_HSCROLL from a scroll-bar control: LOWORD(wParam) = scroll code,
 *    HIWORD(wParam) = thumb position, lParam = control handle.
 *  - Pointers passed through LPARAM must be explicit far pointers.
 */

#define TIMER_ID        1
#define VOLUME_STEP     8
#define VOLUME_PAGE     32

static HWND g_hwndTrackList = NULL;
static HWND g_hwndTimeCurrent = NULL;
static HWND g_hwndTimeTotal = NULL;
static HWND g_hwndTimeTrack = NULL;
static HWND g_hwndStatusText = NULL;
static HWND g_hwndVolumeSlider = NULL;
static HWND g_hwndVolumeLabel = NULL;
static BOOL g_timer_running = FALSE;

extern HINSTANCE g_hInst;

void UpdateTrackList(HWND hwnd)
{
    int count;
    int sel;
    int want;

    (void)hwnd;

    if (!g_hwndTrackList)
        return;

    count = (int)SendMessage(g_hwndTrackList, LB_GETCOUNT, 0, 0L);

    /* Rebuild only when the number of tracks changed: rebuilding on every
     * timer tick flickers and throws away the user's scroll position. */
    if (count != (int)g_win_state.total_tracks) {
        char buf[32];
        BYTE i;

        SendMessage(g_hwndTrackList, LB_RESETCONTENT, 0, 0L);

        for (i = 1; i <= g_win_state.total_tracks; i++) {
            wsprintf(buf, "Track %2d", (int)i);

            SendMessage(g_hwndTrackList,
                        LB_ADDSTRING,
                        0,
                        (LPARAM)(LPSTR)buf);
        }
    }

    if (g_win_state.current_track > 0 &&
        g_win_state.current_track <= g_win_state.total_tracks)
        want = (int)g_win_state.current_track - 1;
    else
        want = -1;

    sel = (int)SendMessage(g_hwndTrackList, LB_GETCURSEL, 0, 0L);

    if (sel != want)
        SendMessage(g_hwndTrackList, LB_SETCURSEL, (WPARAM)want, 0L);
}

void UpdateTimeDisplay(HWND hwnd)
{
    char buf[32];

    (void)hwnd;

    if (g_hwndTimeCurrent) {
        wsprintf(buf,
                 "%02d:%02d.%02d",
                 (int)g_win_state.current_minutes,
                 (int)g_win_state.current_seconds,
                 (int)g_win_state.current_frames);

        SetWindowText(g_hwndTimeCurrent, buf);
    }

    if (g_hwndTimeTotal) {
        wsprintf(buf,
                 "%02d:%02d.%02d",
                 (int)g_win_state.total_minutes,
                 (int)g_win_state.total_seconds,
                 (int)g_win_state.total_frames);

        SetWindowText(g_hwndTimeTotal, buf);
    }

    if (g_hwndTimeTrack) {
        wsprintf(buf,
                 "%d / %d",
                 (int)g_win_state.current_track,
                 (int)g_win_state.total_tracks);

        SetWindowText(g_hwndTimeTrack, buf);
    }
}

void UpdateStatusText(HWND hwnd, UINT string_id)
{
    char buf[128];

    (void)hwnd;

    if (!g_hwndStatusText)
        return;

    if (LoadString(g_hInst,
                   string_id,
                   buf,
                   sizeof(buf)) == 0)
        buf[0] = '\0';

    SetWindowText(g_hwndStatusText, buf);
}

/* vol: 0..255 */
static void ShowVolume(int vol)
{
    char buf[32];

    if (vol < 0)
        vol = 0;
    if (vol > 255)
        vol = 255;

    if (g_hwndVolumeSlider)
        SetScrollPos(g_hwndVolumeSlider, SB_CTL, vol, TRUE);

    if (g_hwndVolumeLabel) {
        wsprintf(buf,
                 "Volume: %d%%",
                 (int)(((long)vol * 100L) / 255L));

        SetWindowText(g_hwndVolumeLabel, buf);
    }
}

void UpdateVolumeSlider(HWND hwnd)
{
    WORD left;
    WORD right;

    (void)hwnd;

    if (!g_hwndVolumeSlider ||
        !g_hwndVolumeLabel)
        return;

    if (WinCD_GetVolume(&left, &right))
        ShowVolume((int)(((long)left + (long)right) / 2L));
    else
        ShowVolume((int)g_win_state.volume_left);
}

/* Enumerating drives means one MCI open/close per drive letter, so the
 * result is cached and a new enumeration is only attempted every
 * ENUM_RETRY_TICKS timer ticks while no CD-ROM drive is known. */
#define ENUM_RETRY_TICKS 10

static BYTE g_drive = 0;
static int  g_enum_wait = 0;

static BOOL TryOpenDrive(HWND hwnd)
{
    if (g_drive == 0) {
        if (g_enum_wait > 0) {
            g_enum_wait--;
            return FALSE;
        }

        g_enum_wait = ENUM_RETRY_TICKS;

        if (WinCD_GetDriveCount() > 0)
            g_drive = WinCD_GetDriveLetter(0);

        if (g_drive == 0) {
            UpdateStatusText(hwnd, IDS_NODRIVE);
            return FALSE;
        }
    }

    if (!WinCD_OpenDrive(g_drive)) {
        UpdateStatusText(hwnd, IDS_NOTOC);
        return FALSE;
    }

    WinCD_UpdateDisplay(hwnd);
    UpdateStatusText(hwnd, IDS_READY);

    return TRUE;
}

static void OnHScroll(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    int pos;

    if ((HWND)HIWORD(lParam) != g_hwndVolumeSlider)
        return;

    pos = GetScrollPos(g_hwndVolumeSlider, SB_CTL);

    switch (wParam) {
        case SB_LINEUP:        pos -= VOLUME_STEP; break;
        case SB_LINEDOWN:      pos += VOLUME_STEP; break;
        case SB_PAGEUP:        pos -= VOLUME_PAGE; break;
        case SB_PAGEDOWN:      pos += VOLUME_PAGE; break;
        case SB_TOP:           pos = 0;            break;
        case SB_BOTTOM:        pos = 255;          break;
        case SB_THUMBTRACK:
        case SB_THUMBPOSITION: pos = (int)HIWORD(wParam); break;
        default:
            return;            /* SB_ENDSCROLL etc. */
    }

    if (pos < 0)
        pos = 0;
    if (pos > 255)
        pos = 255;

    WinCD_SetVolume((WORD)pos, (WORD)pos);
    ShowVolume(pos);

    (void)hwnd;
}

static void OnCommand(HWND hwnd,
                      WPARAM wParam,
                      LPARAM lParam)
{
    WORD id = (WORD)wParam;
    WORD code = HIWORD(lParam);

    switch (id) {
        case IDC_BTN_PLAY:
            if (g_win_state.is_paused)
                WinCD_Resume();
            else if (g_win_state.current_track > 0)
                WinCD_PlayTrack(g_win_state.current_track);

            WinCD_UpdateDisplay(hwnd);
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
                if (g_win_state.is_playing) {
                    WinCD_PlayTrack(
                        (BYTE)(g_win_state.current_track - 1));
                } else {
                    g_win_state.current_track--;
                }

                WinCD_UpdateDisplay(hwnd);
            }
            break;

        case IDC_BTN_NEXT:
            if (g_win_state.current_track <
                g_win_state.total_tracks) {

                if (g_win_state.is_playing) {
                    WinCD_PlayTrack(
                        (BYTE)(g_win_state.current_track + 1));
                } else {
                    g_win_state.current_track++;
                }

                WinCD_UpdateDisplay(hwnd);
            }
            break;

        case IDC_BTN_EJECT:
            if (WinCD_Eject()) {
                /* the old table of contents is no longer valid */
                WinCD_CloseDrive();
            }

            WinCD_UpdateDisplay(hwnd);
            break;

        case IDM_ABOUT:
        {
            char buf[512];

            if (LoadString(g_hInst,
                           IDS_ABOUT_TEXT,
                           buf,
                           sizeof(buf)) == 0)
                lstrcpy(buf, "Creative QuickCD");

            MessageBox(hwnd,
                       buf,
                       "About QuickCD",
                       MB_OK | MB_ICONINFORMATION);
            break;
        }

        case IDC_BTN_CLOSE:
        case IDM_EXIT:
        case IDCANCEL:
            PostMessage(hwnd,
                        WM_CLOSE,
                        0,
                        0L);
            break;

        case IDC_TRACK_LIST:
            if (code == LBN_SELCHANGE || code == LBN_DBLCLK) {
                int sel;

                sel = (int)SendMessage(g_hwndTrackList,
                                       LB_GETCURSEL,
                                       0,
                                       0L);

                if (sel != LB_ERR) {
                    g_win_state.current_track =
                        (BYTE)(sel + 1);

                    if (g_win_state.is_playing ||
                        code == LBN_DBLCLK) {
                        WinCD_PlayTrack(
                            g_win_state.current_track);
                    }

                    WinCD_UpdateDisplay(hwnd);
                }
            }
            break;
    }
}

static void OnTimer(HWND hwnd)
{
    if (!WinCD_IsOpen()) {
        /* no disc yet (or it was ejected): keep trying */
        TryOpenDrive(hwnd);
        return;
    }

    if (g_win_state.is_playing ||
        g_win_state.is_paused) {

        WinCD_UpdateDisplay(hwnd);
    }
}

static void OnInitDialog(HWND hwnd)
{
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

    WinCD_Init(hwnd);

    SetScrollRange(g_hwndVolumeSlider, SB_CTL, 0, 255, FALSE);
    UpdateVolumeSlider(hwnd);

    TryOpenDrive(hwnd);

    /* Always start the timer so a disc inserted later is picked up. */
    g_timer_running = (SetTimer(hwnd, TIMER_ID, 500, NULL) != 0);
}

BOOL CALLBACK __export MainWndProc(HWND hwnd,
                                   UINT msg,
                                   WPARAM wParam,
                                   LPARAM lParam)
{
    switch (msg) {
        case WM_INITDIALOG:
            OnInitDialog(hwnd);
            return TRUE;

        case WM_COMMAND:
            OnCommand(hwnd,
                      wParam,
                      lParam);
            return TRUE;

        case WM_HSCROLL:
            OnHScroll(hwnd,
                      wParam,
                      lParam);
            return TRUE;

        case WM_TIMER:
            OnTimer(hwnd);
            return TRUE;

        case WM_CLOSE:
            if (g_timer_running) {
                KillTimer(hwnd, TIMER_ID);
                g_timer_running = FALSE;
            }

            WinCD_Cleanup();
            EndDialog(hwnd, 0);
            return TRUE;
    }

    return FALSE;
}
