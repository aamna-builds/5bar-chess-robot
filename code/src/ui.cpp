#include <raylib.h>
#include "ui.h"
#include "chess.h"
#include "state.h"

int glyphs[] = {0x2655, 0x2654, 0x2656, 0x2657, 0x2658, 0x2659, 0x265A, 0x265B, 0x265C, 0x265D, 0x265E, 0x265F};

Font chessFont;

void DrawTopBar(float barHeight)
{
    DrawRectangle(0, 0, WIDTH, barHeight, BLACK);

    Font font = GetFontDefault();

    const char* subtitle = "(A 5-Bar Parallal Chess Robot)";
    const char* title = "FiBar Chessy";

    int titleSize = 28;
    int subSize = 16;
    float spacing = 5.5;

    Vector2 titleSizePx = MeasureTextEx(font, title, titleSize, spacing);
    Vector2 subSizePx = MeasureTextEx(font, subtitle, subSize, spacing);

    float centerX = WIDTH / 2.0f;

    DrawTextEx(
        font,
        subtitle,
        {centerX - subSizePx.x / 2.0f,
        barHeight / 2.0f + 4},
        subSize,
        spacing,
        neonGreen
    );

    DrawTextEx(
        font,
        title,
        {centerX - titleSizePx.x / 2.0f,
        barHeight / 2.0f - titleSize},
        titleSize,
        spacing,
        neonGreen
    );
}

void DrawInstructions(float barHeight)
{
    Font font = GetFontDefault();

    const char* lines[] =
    {
        // "Press X -> New Game",
        "Press C -> Undo Move",
        "Press SPACE -> Engine Move"
    };

    const int fontSize = 18;
    const float spacing = 2.0f;
    const float lineHeight = fontSize + 6;

    const float paddingLeft = 20.0f;

    float startY = barHeight + 10;

    for (int i = 0; i < 2; i++)
    {
        DrawTextEx(
            font,
            lines[i],
            {paddingLeft,
            startY + i * lineHeight},
            fontSize,
            spacing,
            neonGreen
        );
    }
}

void LoadUI(void)
{
    chessFont = LoadFontEx("assets/dejavu-sans/DejaVuSans-Bold.ttf", 96, glyphs, sizeof(glyphs)/sizeof(int));
}

void UpdateUI(void)
{
    
}

void DrawUI(void)
{
    DrawTopBar(70);
    DrawInstructions(70);
}

void UnloadUI(void)
{
    UnloadFont(chessFont);
}