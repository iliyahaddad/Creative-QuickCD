#ifndef WINDOWS_TYPES_H
#define WINDOWS_TYPES_H

#include <windows.h>

/* windows.h already defines BYTE/WORD/DWORD/BOOL/...: tell the common header. */
#ifndef QCD_WINDOWS_TYPES
#define QCD_WINDOWS_TYPES
#endif
#include "../common/types.h"

typedef struct {
    BYTE current_track;
    BYTE total_tracks;
    BYTE current_minutes;
    BYTE current_seconds;
    BYTE current_frames;
    BYTE total_minutes;
    BYTE total_seconds;
    BYTE total_frames;
    WORD volume_left;       /* 0..255 */
    WORD volume_right;      /* 0..255 */
    BOOL is_playing;
    BOOL is_paused;
    char drive_letter;
} WINDOW_PLAYER_STATE;

extern WINDOW_PLAYER_STATE g_win_state;

#endif
