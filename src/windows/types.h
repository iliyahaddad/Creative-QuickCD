#ifndef WINDOWS_TYPES_H
#define WINDOWS_TYPES_H

#include <windows.h>
#include "../common/types.h"

#define WM_USER_PLAY    (WM_USER + 100)
#define WM_USER_STOP    (WM_USER + 101)
#define WM_USER_PAUSE   (WM_USER + 102)
#define WM_USER_RESUME  (WM_USER + 103)
#define WM_USER_EJECT   (WM_USER + 104)
#define WM_USER_TRACK   (WM_USER + 105)
#define WM_USER_VOLUME  (WM_USER + 106)
#define WM_USER_TIMER   (WM_USER + 107)

typedef struct {
    BYTE current_track;
    BYTE total_tracks;
    BYTE current_minutes;
    BYTE current_seconds;
    BYTE current_frames;
    BYTE total_minutes;
    BYTE total_seconds;
    BYTE total_frames;
    WORD volume_left;
    WORD volume_right;
    BOOL is_playing;
    BOOL is_paused;
    char drive_letter;
} WINDOW_PLAYER_STATE;

extern WINDOW_PLAYER_STATE g_win_state;

#endif
