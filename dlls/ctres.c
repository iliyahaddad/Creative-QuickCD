#include <windows.h>
#include <string.h>
#include "ctres.h"

static const char g_szAppName[]        = "QuickCD";
static const char g_szReady[]          = "Ready";
static const char g_szPlaying[]        = "Playing";
static const char g_szPaused[]         = "Paused";
static const char g_szStopped[]        = "Stopped";
static const char g_szNoDrive[]        = "No CD-ROM drive detected";
static const char g_szNoToc[]          = "Unable to read CD Table of Contents";
static const char g_szMscdexMissing[]  = "MSCDEX not installed. Please load the CD-ROM driver (MSCDEX).";
static const char g_szAboutText[]      = "Creative QuickCD\nOpen Source Reconstruction\n\nOriginal by Creative Technology\nDOS: PH Beh, Andy\nWindows: Nigel Tan";

static const char *find_string(int id)
{
    switch (id)
    {
        case IDS_APPNAME:        return g_szAppName;
        case IDS_READY:          return g_szReady;
        case IDS_PLAYING:        return g_szPlaying;
        case IDS_PAUSED:         return g_szPaused;
        case IDS_STOPPED:        return g_szStopped;
        case IDS_NODRIVE:        return g_szNoDrive;
        case IDS_NOTOC:          return g_szNoToc;
        case IDS_MSCDEX_MISSING: return g_szMscdexMissing;
        case IDS_ABOUT_TEXT:     return g_szAboutText;
        default:                 return NULL;
    }
}

/*
 * Exported string lookup: copies string 'id' into buf (at most len bytes,
 * always NUL terminated) and returns its length, or 0 if the id is unknown.
 */
int __far __pascal __export LoadResString(int id, LPSTR buf, int len)
{
    const char *s = find_string(id);

    if (!buf || len <= 0)
        return 0;

    if (!s) {
        buf[0] = '\0';
        return 0;
    }

    lstrcpyn(buf, s, len);
    return lstrlen(buf);
}

/* Win16 DLL entry point (called once by the C startup code). */
int __far __pascal LibMain(HINSTANCE hInst,
                           WORD wDataSeg,
                           WORD cbHeapSize,
                           LPSTR lpszCmdLine)
{
    (void)hInst;
    (void)wDataSeg;
    (void)cbHeapSize;
    (void)lpszCmdLine;

    return 1;
}
