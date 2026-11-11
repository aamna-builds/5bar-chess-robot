#pragma once
#include "raylib.h"
#include <vector>

struct Vector2i
{
    int x;
    int y;
};

extern Font chessFont;

void UpdateChess(void);
void DrawChess(void);