#ifndef CTCCW_H
#define CTCCW_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CTCCW_API __far __pascal __export

typedef BOOL (CTCCW_API *LPFN_INIT)(HWND hwnd);
typedef void (CTCCW_API *LPFN_EXIT)(void);
typedef BOOL (CTCCW_API *LPFN_OPENCD)(BYTE drive);
typedef void (CTCCW_API *LPFN_CLOSECD)(void);
typedef BOOL (CTCCW_API *LPFN_PLAYTRACK)(BYTE track);
typedef void (CTCCW_API *LPFN_STOP)(void);
typedef void (CTCCW_API *LPFN_PAUSE)(void);
typedef void (CTCCW_API *LPFN_RESUME)(void);
typedef BOOL (CTCCW_API *LPFN_EJECT)(void);
typedef BOOL (CTCCW_API *LPFN_CLOSETRAY)(void);
typedef BOOL (CTCCW_API *LPFN_GETSTATUS)(BYTE far *track, DWORD far *pos, DWORD far *length);
typedef BOOL (CTCCW_API *LPFN_SETVOLUME)(WORD left, WORD right);
typedef BOOL (CTCCW_API *LPFN_GETVOLUME)(WORD far *left, WORD far *right);
typedef BYTE (CTCCW_API *LPFN_GETDRIVECOUNT)(void);
typedef BYTE (CTCCW_API *LPFN_GETDRIVELETTER)(BYTE index);

#ifdef __cplusplus
}
#endif

#endif
