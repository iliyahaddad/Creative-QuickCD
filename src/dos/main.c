#include "types.h"
#include "cdrom.h"
#include "ui.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

int main(void) {
    PLAYER_STATE state;
    BYTE drive_count;
    BYTE drive_letter;
    BYTE drive_unit;
    BYTE track_count;
    BYTE i;
    
    if (!CheckMSCDEX()) {
        printf("MSCDEX not installed. Please insert the Sound Blaster driver CD.\n");
        return 1;
    }
    
    drive_count = GetCDDriveCount();
    if (drive_count == 0) {
        printf("No CD-ROM drive detected.\n");
        return 1;
    }
    
    drive_letter = GetCDDriveLetter(0);
    drive_unit = GetCDDriveUnit(drive_letter);
    
    if (!ReadCDTOC(drive_unit, state.toc, &track_count)) {
        printf("Unable to read CD Table of Contents.\n");
        return 1;
    }
    
    state.drive_letter = drive_letter;
    state.unit_number = drive_unit;
    state.total_tracks = track_count;
    state.current_track = 1;
    state.is_playing = FALSE;
    state.is_paused = FALSE;
    state.volume = 200;
    
    BYTE min, sec, frame;
    LBAtoMSF(state.toc[track_count - 1].lba + 1, &min, &sec, &frame);
    state.total_minutes = min;
    state.total_seconds = sec;
    state.current_minutes = 0;
    state.current_seconds = 0;
    state.current_frames = 0;
    
    SetVolume(drive_unit, state.volume, state.volume);
    
    RunPlayerLoop(&state);
    
    StopCDAudio(drive_unit);
    return 0;
}