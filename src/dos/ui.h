#ifndef UI_H
#define UI_H

#include "types.h"
#include "player.h"

void InitUI(void);
void DrawMainScreen(PLAYER_STATE far *state);
void UpdateTrackDisplay(PLAYER_STATE far *state);
BYTE HandleKeyboard(PLAYER_STATE far *state);
void RunPlayerLoop(PLAYER_STATE far *state);

#endif