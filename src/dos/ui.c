#include <stdio.h>
#include <string.h>
#include <conio.h>
#include <dos.h>
#include "types.h"
#include "cdrom.h"
#include "ui.h"

/*
 * Text-mode UI written straight to the colour text screen (B800h).
 * Attribute byte = (background << 4) | foreground.
 */

#define SCREEN_COLS 80
#define SCREEN_ROWS 25

#define UI_COLOR_BG        0x17   /* grey on blue            */
#define UI_COLOR_TITLE     0x1F   /* bright white on blue    */
#define UI_COLOR_BORDER    0x1B   /* light cyan on blue      */
#define UI_COLOR_TEXT      0x17
#define UI_COLOR_HIGHLIGHT 0x70   /* black on grey (inverse) */
#define UI_COLOR_TRACK     0x1E   /* yellow on blue          */
#define UI_COLOR_TIME      0x1B

#define TRACKS_VISIBLE 14
#define POLL_EVERY     5          /* main-loop ticks (50 ms) between CD polls */

static BYTE far *g_video;

static void PutChar(int x, int y, BYTE attr, char ch)
{
    WORD off;

    if (x < 0 || x >= SCREEN_COLS || y < 0 || y >= SCREEN_ROWS)
        return;

    off = (WORD)((y * SCREEN_COLS + x) * 2);

    g_video[off] = (BYTE)ch;
    g_video[off + 1] = attr;
}

static void PutText(int x, int y, BYTE attr, const char *s)
{
    while (*s) {
        PutChar(x, y, attr, *s);
        x++;
        s++;
    }
}

static void ClearScreen(void)
{
    int x;
    int y;

    for (y = 0; y < SCREEN_ROWS; y++)
        for (x = 0; x < SCREEN_COLS; x++)
            PutChar(x, y, UI_COLOR_BG, ' ');
}

static void DrawBox(int x1, int y1, int x2, int y2, BYTE attr)
{
    int x;
    int y;

    for (x = x1 + 1; x < x2; x++) {
        PutChar(x, y1, attr, (char)205);
        PutChar(x, y2, attr, (char)205);
    }

    for (y = y1 + 1; y < y2; y++) {
        PutChar(x1, y, attr, (char)186);
        PutChar(x2, y, attr, (char)186);
    }

    PutChar(x1, y1, attr, (char)201);
    PutChar(x2, y1, attr, (char)187);
    PutChar(x1, y2, attr, (char)200);
    PutChar(x2, y2, attr, (char)188);
}

static void DrawTitle(void)
{
    PutText(32, 0, UI_COLOR_TITLE, " QuickCD Player ");
}

static void DrawDriveInfo(char drive_letter)
{
    char buf[24];

    sprintf(buf, "Drive: %c:", drive_letter);
    PutText(2, 2, UI_COLOR_TEXT, buf);
}

static DWORD TrackLength(PLAYER_STATE far *state, BYTE track)
{
    DWORD start_lba = state->toc[track - 1].lba;
    DWORD end_lba;

    if (track < state->total_tracks)
        end_lba = state->toc[track].lba;
    else
        end_lba = state->leadout_lba;

    return (end_lba > start_lba) ? (end_lba - start_lba) : 0;
}

static void DrawTrackList(PLAYER_STATE far *state)
{
    BYTE i;
    BYTE start = 0;
    BYTE min;
    BYTE sec;
    BYTE frame;
    char buf[40];

    DrawBox(1, 4, 38, 20, UI_COLOR_BORDER);
    PutText(3, 4, UI_COLOR_TITLE, " Tracks ");

    if (state->total_tracks > TRACKS_VISIBLE &&
        state->current_track > 8) {

        start = (BYTE)(state->current_track - 8);

        if ((WORD)start + TRACKS_VISIBLE > state->total_tracks)
            start = (BYTE)(state->total_tracks - TRACKS_VISIBLE);
    }

    for (i = 0; i < TRACKS_VISIBLE; i++) {
        BYTE track = (BYTE)(start + i + 1);
        BYTE attr = UI_COLOR_TEXT;

        if (track > state->total_tracks) {
            memset(buf, ' ', 34);
            buf[34] = '\0';
        } else {
            LBAtoMSF(TrackLength(state, track), &min, &sec, &frame);

            sprintf(buf, "%c Track %2d   %2d:%02d",
                    (track == state->current_track) ? '>' : ' ',
                    (int)track, (int)min, (int)sec);

            /* pad to a fixed width so old text is always overwritten */
            while (strlen(buf) < 34)
                strcat(buf, " ");

            if (track == state->current_track)
                attr = UI_COLOR_HIGHLIGHT;
        }

        PutText(3, 6 + i, attr, buf);
    }
}

static void DrawTimeDisplay(PLAYER_STATE far *state)
{
    char buf[40];

    DrawBox(41, 4, 78, 12, UI_COLOR_BORDER);
    PutText(43, 4, UI_COLOR_TITLE, " Time Display ");

    sprintf(buf, "Current: %2d:%02d.%02d ",
            (int)state->current_minutes,
            (int)state->current_seconds,
            (int)state->current_frames);
    PutText(43, 6, UI_COLOR_TIME, buf);

    sprintf(buf, "Total:   %2d:%02d.%02d ",
            (int)state->total_minutes,
            (int)state->total_seconds,
            0);
    PutText(43, 8, UI_COLOR_TIME, buf);

    sprintf(buf, "Track:   %2d / %2d ",
            (int)state->current_track,
            (int)state->total_tracks);
    PutText(43, 10, UI_COLOR_TIME, buf);
}

static void DrawStatusBar(PLAYER_STATE far *state)
{
    char buf[32];
    int i;

    for (i = 1; i < 79; i++)
        PutChar(i, 22, UI_COLOR_BORDER, (char)205);

    if (state->is_playing) {
        if (state->is_paused)
            PutText(2, 23, UI_COLOR_HIGHLIGHT, "[PAUSED]  ");
        else
            PutText(2, 23, UI_COLOR_TRACK, "[PLAYING] ");
    } else {
        PutText(2, 23, UI_COLOR_TIME, "[STOPPED] ");
    }

    /* volume is 0..255 internally; show it as a percentage */
    sprintf(buf, "Vol: %3d%%  ",
            (int)(((WORD)state->volume * 100U) / 255U));
    PutText(13, 23, UI_COLOR_TEXT, buf);
}

static void DrawHelp(void)
{
    PutText(2, 24, UI_COLOR_TIME,
            "P=Play S=Stop Space=Pause +/-=Vol N=Next B=Prev E=Eject Q=Quit");
}

void InitUI(void)
{
    union REGS r;

    g_video = (BYTE far *)MK_FP(0xB800, 0);

    /* 80x25 colour text mode */
    r.h.ah = 0x00;
    r.h.al = 0x03;
    int86(0x10, &r, &r);

    /* hide the hardware cursor */
    r.h.ah = 0x01;
    r.x.cx = 0x2000;
    int86(0x10, &r, &r);

    ClearScreen();
}

void ShutdownUI(void)
{
    union REGS r;

    /* reset the mode: restores the cursor and clears the screen */
    r.h.ah = 0x00;
    r.h.al = 0x03;
    int86(0x10, &r, &r);
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

/*
 * Returns an ASCII command character, or 0 if no key is waiting.
 * Cursor keys are translated to the equivalent letter command; other
 * extended keys are ignored (their scan codes must NOT be treated as ASCII,
 * otherwise e.g. Down-arrow (80 = 'P') would start playback).
 */
BYTE HandleKeyboard(PLAYER_STATE far *state)
{
    BYTE key;

    (void)state;

    if (!kbhit())
        return 0;

    key = (BYTE)getch();

    if (key == 0 || key == 0xE0) {
        key = (BYTE)getch();

        switch (key) {
            case 72: return '+';   /* Up    */
            case 80: return '-';   /* Down  */
            case 77: return 'n';   /* Right */
            case 75: return 'b';   /* Left  */
        }

        return 0;
    }

    if (key == 27)                 /* Esc */
        return 'q';

    return key;
}

/* End-of-disc detection state: some drivers/emulators do not set the busy
 * bit while playing, so "not busy" alone is not trusted - the position must
 * also have stopped advancing for two consecutive polls. */
static BYTE g_last_min = 0xFF;
static BYTE g_last_sec;
static BYTE g_last_frame;
static BYTE g_still;

static void ResetPoll(void)
{
    g_last_min = 0xFF;
    g_still = 0;
}

static void ResetTime(PLAYER_STATE far *state)
{
    state->current_minutes = 0;
    state->current_seconds = 0;
    state->current_frames = 0;
}

static void StartTrack(PLAYER_STATE far *state)
{
    StopCDAudio(state->unit_number);

    if (PlayCDAudio(state->unit_number,
                    state->toc[state->current_track - 1].lba,
                    0xFFFFFFFFUL)) {
        state->is_playing = TRUE;
    } else {
        state->is_playing = FALSE;
    }

    state->is_paused = FALSE;
    ResetTime(state);
    ResetPoll();
}

void RunPlayerLoop(PLAYER_STATE far *state)
{
    BYTE key;
    BYTE cmd;
    BYTE poll = 0;

    InitUI();
    DrawMainScreen(state);

    while (1) {
        if (state->is_playing && !state->is_paused && poll == 0) {
            BYTE track;
            BYTE min;
            BYTE sec;
            BYTE frame;

            BOOL busy = IsCDPlaying(state->unit_number);
            BOOL got = GetCDAudioPosition(state->unit_number,
                                          &track, &min, &sec, &frame);
            BOOL changed = FALSE;

            if (got) {
                changed = (min != g_last_min ||
                           sec != g_last_sec ||
                           frame != g_last_frame);

                g_last_min = min;
                g_last_sec = sec;
                g_last_frame = frame;

                if (track >= 1 && track <= state->total_tracks)
                    state->current_track = track;

                state->current_minutes = min;
                state->current_seconds = sec;
                state->current_frames = frame;
            }

            if (busy || changed)
                g_still = 0;
            else if (++g_still >= 2) {
                /* disc finished (or the drive stopped) */
                state->is_playing = FALSE;
                state->is_paused = FALSE;
                ResetTime(state);
                ResetPoll();
            }

            UpdateTrackDisplay(state);
        }

        poll++;
        if (poll >= POLL_EVERY)
            poll = 0;

        key = HandleKeyboard(state);

        if (key) {
            cmd = key;
            if (cmd >= 'A' && cmd <= 'Z')
                cmd = (BYTE)(cmd + ('a' - 'A'));

            switch (cmd) {
                case 'q':
                    StopCDAudio(state->unit_number);
                    StopCDAudio(state->unit_number);
                    return;

                case 'p':
                    if (!state->is_playing)
                        StartTrack(state);
                    break;

                case 's':
                    /* 1st stop pauses, 2nd stop really stops */
                    StopCDAudio(state->unit_number);
                    StopCDAudio(state->unit_number);

                    state->is_playing = FALSE;
                    state->is_paused = FALSE;
                    ResetTime(state);
                    break;

                case ' ':
                    if (state->is_playing && !state->is_paused) {
                        if (StopCDAudio(state->unit_number))
                            state->is_paused = TRUE;
                    } else if (state->is_playing && state->is_paused) {
                        if (ResumeCDAudio(state->unit_number)) {
                            state->is_paused = FALSE;
                            ResetPoll();
                        }
                    }
                    break;

                case '+':
                case '=':
                    if (state->volume < 255) {
                        if (state->volume > 239)
                            state->volume = 255;
                        else
                            state->volume = (BYTE)(state->volume + 16);

                        SetVolume(state->unit_number,
                                  state->volume,
                                  state->volume);
                    }
                    break;

                case '-':
                case '_':
                    if (state->volume > 0) {
                        if (state->volume >= 16)
                            state->volume = (BYTE)(state->volume - 16);
                        else
                            state->volume = 0;

                        SetVolume(state->unit_number,
                                  state->volume,
                                  state->volume);
                    }
                    break;

                case 'n':
                    if (state->current_track < state->total_tracks) {
                        state->current_track++;
                        ResetTime(state);

                        if (state->is_playing)
                            StartTrack(state);
                    }
                    break;

                case 'b':
                    if (state->current_track > 1) {
                        state->current_track--;
                        ResetTime(state);

                        if (state->is_playing)
                            StartTrack(state);
                    }
                    break;

                case 'e':
                    StopCDAudio(state->unit_number);
                    StopCDAudio(state->unit_number);
                    EjectCD(state->unit_number);
                    state->is_playing = FALSE;
                    state->is_paused = FALSE;
                    ResetTime(state);
                    break;
            }

            UpdateTrackDisplay(state);
        }

        delay(50);
    }
}
