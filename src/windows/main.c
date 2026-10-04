#include <windows.h>
#include "types.h"
#include "window.h"
#include "resource.h"

HINSTANCE g_hInst;

int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
    FARPROC lpProc;
    int result;

    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;

    g_hInst = hInstance;

    /* Win16: a dialog procedure needs a procedure-instance thunk. */
    lpProc = MakeProcInstance((FARPROC)MainWndProc, hInstance);

    result = DialogBox(hInstance,
                       MAKEINTRESOURCE(IDD_MAIN_DIALOG),
                       NULL,
                       (DLGPROC)lpProc);

    FreeProcInstance(lpProc);

    return result;
}
