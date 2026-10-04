#ifndef WINDOW_H
#define WINDOW_H

#include "types.h"
#include "cdrom.h"

/* Win16 dialog procedure: must be exported (and wrapped with
 * MakeProcInstance by the caller). */
BOOL CALLBACK __export MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

void UpdateTrackList(HWND hwnd);
void UpdateTimeDisplay(HWND hwnd);
void UpdateStatusText(HWND hwnd, UINT string_id);
void UpdateVolumeSlider(HWND hwnd);

#endif
