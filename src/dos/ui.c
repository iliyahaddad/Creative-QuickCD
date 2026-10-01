#include "types.h"
#include "cdrom.h"
#include "conio.h"
#include "string.h"
#include "stdio.h"
#include "dos.h"

#define UI_COLOR_BG        0x17
#define UI_COLOR_TITLE     0x1F
#define UI_COLOR_BORDER    0x70
#define UI_COLOR_TEXT      0x07
#define UI_COLOR_HIGHLIGHT 0x70
#define UI_COLOR_TRACK     0x0E
#define UI_COLOR_TIME      0x0B

static void SetCursor(int x, int y)
{
    _AH = 0x02;
    _BH = 0;
    _DH = (BYTE)y;
    _DL = (BYTE)x;
    geninterrupt(0x10);
}

static void ClearScreen(void)
{
    _AH = 0x06;
    _AL = 0;
    _BH = UI_COLOR_BG;
    _CH = 0;
    _CL = 0;
    _DH = 24;
    _DL = 79;
    geninterrupt(0x10);

    SetCursor(0, 0);
}

static void DrawBox(int x1, int y1, int x2, int y2, WORD color)
{
    int x;
    int y;

    color = color;

    for (x = x1; x <= x2; x++) {
        SetCursor(x, y1);
        cprintf("%c", 205);

        SetCursor(x, y2);
        cprintf("%c", 205);
    }

    for (y = y1; y <= y2; y++) {
        SetCursor(x1, y);
        cprintf("%c", 186);

        SetCursor(x2, y);
        cprintf("%c", 186);
    }

    SetCursor(x1, y1);
    cprintf("%c", 201);

    SetCursor(x2, y1);
    cprintf("%c", 187);

    SetCursor(x1, y2);
    cprintf("%c", 200);

    SetCursor(x2, y2);
    cprintf("%c", 188);
}

static void DrawTitle(void)
{
    textcolor(UI_COLOR_TITLE);

    SetCursor(28, 0);
    cprintf(" QuickCD Player ");

    textcolor(UI_COLOR_TEXT);
}

static void DrawDriveInfo(char drive_letter)
{
    SetCursor(2, 2);

    textcolor(UI_COLOR_TEXT);
    cprintf("Drive: %c:", drive_letter);
}

static void DrawTrackList(PLAYER_STATE far *state)
{
    BYTE i;
    BYTE start = 0;
    DWORD start_lba;
    DWORD end_lba;
    DWORD length_lba;
    BYTE min;
    BYTE sec;
    BYTE frame;

    textcolor(UI_COLOR_BORDER);

    DrawBox(1, 4, 38, 20, UI_COLOR_BORDER);

    SetCursor(3, 4);
    textcolor(UI_COLOR_TITLE);
    cprintf(" Tracks ");

    textcolor(UI_COLOR_TEXT);

    if (state->total_tracks > 14) {
        if (state->current_track > 8) {
            start = state->current_track - 8;

            if (start + 14 > state->total_tracks)
                start = state->total_tracks - 14;
        }
    }

    for (i = 0; i < 14 &&
         (start + i) < state->total_tracks; i++) {

        BYTE track = start + i + 1;

        SetCursor(3, 6 + i);

        if (track == state->current_track) {
            start_lba = state->toc[track - 1].lba;

            if (track < state->total_tracks)
                end_lba = state->toc[track].lba;
            else
                end_lba = state->leadout_lba;

            length_lba =
                (end_lba > start_lba)
                ? (end_lba - start_lba)
                : 0;

            LBAtoMSF(length_lba, &min, &sec, &frame);

            textcolor(UI_COLOR_HIGHLIGHT);
            cprintf("> Track %2d %2d:%02d",
                    track, min, sec);

            textcolor(UI_COLOR_TEXT);
        } else {
            cprintf("  Track %2d --:--", track);
        }
    }
}

static void DrawTimeDisplay(PLAYER_STATE far *state)
{
    textcolor(UI_COLOR_BORDER);

    DrawBox(41, 4, 78, 12, UI_COLOR_BORDER);

    SetCursor(43, 4);
    textcolor(UI_COLOR_TITLE);
    cprintf(" Time Display ");

    textcolor(UI_COLOR_TEXT);

    SetCursor(43, 6);
    cprintf("Current: %2d:%02d.%02d",
            state->current_minutes,
            state->current_seconds,
            state->current_frames);

    SetCursor(43, 8);
    cprintf("Total:   %2d:%02d.%02d",
            state->total_minutes,
            state->total_seconds,
            0);

    SetCursor(43, 10);
    cprintf("Track:   %2d / %2d",
            state->current_track,
            state->total_tracks);
}

static void DrawStatusBar(PLAYER_STATE far *state)
{
    int i;

    SetCursor(1, 22);

    textcolor(UI_COLOR_BORDER);

    for (i = 0; i < 78; i++)
        cprintf("%c", 205);

    SetCursor(2, 23);

    if (state->is_playing) {
        if (state->is_paused) {
            textcolor(UI_COLOR_HIGHLIGHT);
            cprintf("[PAUSED] ");
            textcolor(UI_COLOR_TEXT);
        } else {
            textcolor(UI_COLOR_TRACK);
            cprintf("[PLAYING] ");
            textcolor(UI_COLOR_TEXT);
        }
    } else {
        textcolor(UI_COLOR_TIME);
        cprintf("[STOPPED] ");
        textcolor(UI_COLOR_TEXT);
    }

    cprintf("Vol: %3d%%  ", state->volume);
}

static void DrawHelp(void)
{
    SetCursor(2, 24);

    textcolor(UI_COLOR_TIME);

    cprintf("P=Play  S=Stop  Space=Pause  +/-=Vol  N=Next B=Previous  E=Eject  Q=Quit");

    textcolor(UI_COLOR_TEXT);
}

void InitUI(void)
{
    _AH = 0x00;
    _AL = 0x03;

    geninterrupt(0x10);

    ClearScreen();

    textbackground(UI_COLOR_BG >> 4);
    textcolor(UI_COLOR_TEXT);
}

void DrawMainScreen(PLAYER_STATE far *state)
{
    ClearScreen();

    DrawTitle();
    DrawDriveInfo(state->drive_letter);
    DrawTrackList(state);
    DrawTimeDisplay(state);
    DrawStatusBar(state);
    DrawHelp();
}

void UpdateTrackDisplay(PLAYER_STATE far *state)
{
    DrawTrackList(state);
    DrawTimeDisplay(state);
    DrawStatusBar(state);
}

BYTE HandleKeyboard(PLAYER_STATE far *state)
{
    BYTE key;

    state = state;

    if (!kbhit())
        return 0;

    key = getch();

    if (key == 0) {
        key = getch();

        switch (key) {
            case 72:
            case 80:
                break;
        }

        return key;
    }

    return key;
}

void RunPlayerLoop(PLAYER_STATE far *state)
{
    BYTE key;

    InitUI();
    DrawMainScreen(state);

    while (1) {
        if (state->is_playing) {
            BYTE track;
            BYTE min;
            BYTE sec;
            BYTE frame;

            if (GetCDAudioPosition(state->unit_number,
                                    &track,
                                    &min,
                                    &sec,
                                    &frame)) {

                state->current_track = track;
                state->current_minutes = min;
                state->current_seconds = sec;
                state->current_frames = frame;
            }

            UpdateTrackDisplay(state);
        }

        key = HandleKeyboard(state);

        if (key) {
            switch (key | 0x20) {
                case 'q':
                    StopCDAudio(state->unit_number);
                    return;

                case 'p':
                    if (!state->is_playing) {
                        if (PlayCDAudio(
                                state->unit_number,
                                state->toc[state->current_track - 1].lba,
                                0xFFFFFFFFUL)) {

                            state->is_playing = TRUE;
                            state->is_paused = FALSE;
                        }
                    }
                    break;

                case 's':
                    StopCDAudio(state->unit_number);

                    state->is_playing = FALSE;
                    state->is_paused = FALSE;
                    break;

                case ' ':
                    if (state->is_playing && !state->is_paused) {
                        if (StopCDAudio(state->unit_number))
                            state->is_paused = TRUE;
                    } else if (state->is_playing &&
                               state->is_paused) {

                        if (ResumeCDAudio(state->unit_number))
                            state->is_paused = FALSE;
                    }
                    break;

                case '+':
                case '=':
                    if (state->volume < 255) {
                        if (state->volume > 239)
                            state->volume = 255;
                        else
                            state->volume += 16;

                        SetVolume(state->unit_number,
                                  state->volume,
                                  state->volume);
                    }
                    break;

                case '-':
                    if (state->volume >= 16) {
                        state->volume -= 16;

                        SetVolume(state->unit_number,
                                  state->volume,
                                  state->volume);
                    }
                    break;

                case 'n':
                    if (state->current_track < state->total_tracks) {
                        state->current_track++;

                        if (state->is_playing) {
                            StopCDAudio(state->unit_number);

                            if (!PlayCDAudio(
                                    state->unit_number,
                                    state->toc[
                                        state->current_track - 1
                                    ].lba,
                                    0xFFFFFFFFUL)) {

                                state->is_playing = FALSE;
                            }

                            state->is_paused = FALSE;
                        }
                    }
                    break;

                case 'b':
                    if (state->current_track > 1) {
                        state->current_track--;

                        if (state->is_playing) {
                            StopCDAudio(state->unit_number);

                            if (!PlayCDAudio(
                                    state->unit_number,
                                    state->toc[
                                        state->current_track - 1
                                    ].lba,
                                    0xFFFFFFFFUL)) {

                                state->is_playing = FALSE;
                            }

                            state->is_paused = FALSE;
                        }
                    }
                    break;

                case 'e':
                    EjectCD(state->unit_number);
                    state->is_playing = FALSE;
                    state->is_paused = FALSE;
                    break;
            }

            UpdateTrackDisplay(state);
        }

        delay(50);
    }
}
