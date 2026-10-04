#ifndef CTCCW_H
#define CTCCW_H

#include <windows.h>

#ifndef QCD_WINDOWS_TYPES
#define QCD_WINDOWS_TYPES
#endif
#include "../src/common/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Calling convention of the exported functions (used by the typedefs) ... */
#define CTCCW_CALL __far __pascal
/* ... and the same plus __export for the definitions. */
#define CTCCW_API  CTCCW_CALL __export

/* Volumes are 0..255. */
typedef BOOL (CTCCW_CALL *LPFN_INIT)(HWND hwnd);
typedef void (CTCCW_CALL *LPFN_EXIT)(void);
typedef BOOL (CTCCW_CALL *LPFN_OPENCD)(BYTE drive);
typedef void (CTCCW_CALL *LPFN_CLOSECD)(void);
typedef BOOL (CTCCW_CALL *LPFN_PLAYTRACK)(BYTE track);
typedef void (CTCCW_CALL *LPFN_STOP)(void);
typedef void (CTCCW_CALL *LPFN_PAUSE)(void);
typedef void (CTCCW_CALL *LPFN_RESUME)(void);
typedef BOOL (CTCCW_CALL *LPFN_EJECT)(void);
typedef BOOL (CTCCW_CALL *LPFN_CLOSETRAY)(void);
typedef BOOL (CTCCW_CALL *LPFN_GETSTATUS)(
    BYTE far *track,
    DWORD far *pos,
    DWORD far *length
);
typedef BOOL (CTCCW_CALL *LPFN_SETVOLUME)(
    WORD left,
    WORD right
);
typedef BOOL (CTCCW_CALL *LPFN_GETVOLUME)(
    WORD far *left,
    WORD far *right
);
typedef BYTE (CTCCW_CALL *LPFN_GETDRIVECOUNT)(void);
typedef BYTE (CTCCW_CALL *LPFN_GETDRIVELETTER)(BYTE index);

#ifdef __cplusplus
}
#endif

#endif
