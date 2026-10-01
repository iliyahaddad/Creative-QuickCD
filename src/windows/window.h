#ifndef WINDOW_H
#define WINDOW_H

#include "types.h"
#include "cdrom.h"

BOOL CreateMainWindow(HINSTANCE hInstance, int nCmdShow);
BOOL CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void UpdateTrackList(HWND hwnd);
void UpdateTimeDisplay(HWND hwnd);
void UpdateStatusText(HWND hwnd, UINT string_id);
void UpdateVolumeSlider(HWND hwnd);

#endif
