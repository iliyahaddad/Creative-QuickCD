#include <stdio.h>
#include "types.h"
#include "cdrom.h"
#include "ui.h"

/* Static: the state (with the 100-entry TOC) is too big to put on a small stack. */
static PLAYER_STATE g_state;

int main(void)
{
    BYTE drive_count;
    BYTE drive_letter;
    BYTE drive_unit;
    BYTE track_count;
    DWORD leadout_lba;
    BYTE min;
    BYTE sec;
    BYTE frame;

    if (!CheckMSCDEX()) {
        printf("MSCDEX not installed. Please load the CD-ROM driver (MSCDEX).\n");
        return 1;
    }

    drive_count = GetCDDriveCount();

    if (drive_count == 0) {
        printf("No CD-ROM drive detected.\n");
        return 1;
    }

    drive_letter = GetCDDriveLetter(0);

    if (drive_letter == 0) {
        printf("Unable to determine the CD-ROM drive letter.\n");
        return 1;
    }

    drive_unit = GetCDDriveUnit(drive_letter);

    if (drive_unit == 0xFF) {
        printf("CD-ROM drive is not supported by MSCDEX.\n");
        return 1;
    }

    if (!ReadCDTOC(drive_unit, g_state.toc,
                   &track_count, &leadout_lba)) {
        printf("Unable to read CD Table of Contents (is an audio CD inserted?).\n");
        return 1;
    }

    g_state.drive_letter = (char)drive_letter;
    g_state.unit_number = drive_unit;
    g_state.leadout_lba = leadout_lba;
    g_state.total_tracks = track_count;
    g_state.current_track = 1;
    g_state.is_playing = FALSE;
    g_state.is_paused = FALSE;
    g_state.volume = 200;

    LBAtoMSF(leadout_lba, &min, &sec, &frame);

    g_state.total_minutes = min;
    g_state.total_seconds = sec;
    g_state.current_minutes = 0;
    g_state.current_seconds = 0;
    g_state.current_frames = 0;

    SetVolume(drive_unit, g_state.volume, g_state.volume);

    RunPlayerLoop(&g_state);

    StopCDAudio(drive_unit);
    StopCDAudio(drive_unit);   /* second stop: really stop, not just pause */

    ShutdownUI();

    return 0;
}
