#include <windows.h>
#include <commctrl.h>
#include "types.h"
#include "window.h"
#include "resource.h"

HINSTANCE g_hInst;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
    hPrevInstance = hPrevInstance;
    lpCmdLine = lpCmdLine;
    nCmdShow = nCmdShow;

    InitCommonControls();

    g_hInst = hInstance;

    return DialogBox(hInstance,
                     MAKEINTRESOURCE(IDD_MAIN_DIALOG),
                     NULL,
                     (DLGPROC)MainWndProc);
}
