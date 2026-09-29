#ifndef PLAYER_H
#define PLAYER_H

#include "types.h"

#define STATE_STOPPED 0
#define STATE_PLAYING 1
#define STATE_PAUSED  2

#define MAX_TRACKS 100

extern PLAYER_STATE g_player_state;

BOOL PlayerInit(BYTE drive_letter);
void PlayerCleanup(void);
BOOL PlayerPlayTrack(BYTE track);
void PlayerStop(void);
void PlayerPause(void);
void PlayerResume(void);
BOOL PlayerEject(void);
BOOL PlayerCloseTray(void);
BOOL PlayerGetStatus(BYTE far *track, DWORD far *pos, DWORD far *length);
BOOL PlayerSetVolume(WORD left, WORD right);
BOOL PlayerGetVolume(WORD far *left, WORD far *right);

#endif