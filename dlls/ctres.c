#include <windows.h>
#include <string.h>
#include "ctres.h"

static HINSTANCE g_hInst;

static const char g_szAppName[]        = "QuickCD";
static const char g_szReady[]          = "Ready";
static const char g_szPlaying[]        = "Playing";
static const char g_szPaused[]         = "Paused";
static const char g_szStopped[]        = "Stopped";
static const char g_szNoDrive[]        = "No CD-ROM drive detected";
static const char g_szNoToc[]          = "Unable to read CD Table of Contents";
static const char g_szMscdexMissing[]  = "MSCDEX not installed. Please insert the Sound Blaster driver CD.";
static const char g_szAboutText[]      = "Creative QuickCD\nOpen Source Reconstruction\n\nOriginal by Creative Technology\nDOS: PH Beh, Andy\nWindows: Nigel Tan";

static int load_string(int id, char *buf, int len)
{
    switch (id)
    {
        case IDS_APPNAME:        lstrcpyn(buf, g_szAppName,        len); return lstrlen(buf);
        case IDS_READY:          lstrcpyn(buf, g_szReady,          len); return lstrlen(buf);
        case IDS_PLAYING:        lstrcpyn(buf, g_szPlaying,        len); return lstrlen(buf);
        case IDS_PAUSED:         lstrcpyn(buf, g_szPaused,         len); return lstrlen(buf);
        case IDS_STOPPED:        lstrcpyn(buf, g_szStopped,        len); return lstrlen(buf);
        case IDS_NODRIVE:        lstrcpyn(buf, g_szNoDrive,        len); return lstrlen(buf);
        case IDS_NOTOC:          lstrcpyn(buf, g_szNoToc,          len); return lstrlen(buf);
        case IDS_MSCDEX_MISSING: lstrcpyn(buf, g_szMscdexMissing,  len); return lstrlen(buf);
        case IDS_ABOUT_TEXT:     lstrcpyn(buf, g_szAboutText,      len); return lstrlen(buf);
        default:                 buf[0] = '\0'; return 0;
    }
}

int __far __pascal LibMain(HINSTANCE hInst, WORD wDataSeg, WORD cbHeapSize)
{
    g_hInst = hInst;
    return 1;
}

int __far __pascal _WEP(int bSystemExit)
{
    return 1;
}
