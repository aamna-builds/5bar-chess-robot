#include <raylib.h>
#include "ui.h"
#include "chess.h"
#include "state.h"

static const Color BG_COLOR = { 245, 245, 247, 255 };
static const Color ACCENT_COLOR = { 40, 40, 40, 255 };
static const Color SECONDARY_TEXT = { 120, 120, 130, 255 };
static const Color DIVIDER_COLOR = { 220, 220, 225, 255 };

int glyphs[] = { 0x2655, 0x2654, 0x2656, 0x2657, 0x2658, 0x2659, 0x265A, 0x265B, 0x265C, 0x265D, 0x265E, 0x265F };

Font chessFont;
Font font;

void DrawTopBar(float barHeight)
{
    DrawRectangle(0, 0, WIDTH, barHeight, WHITE);
    DrawRectangle(0, barHeight - 1, WIDTH, 1, DIVIDER_COLOR);

    const char* title = "FIBAR CHESSY";
    const char* subtitle = "5-BAR PARALLEL ROBOT";

    float titleSize = 28.0f;
    float subSize = 12.0f;
    float spacing = 1.5f;

    Vector2 titlePx = MeasureTextEx(font, title, titleSize, spacing);
    Vector2 subPx = MeasureTextEx(font, subtitle, subSize, spacing + 1.0f);

    Vector2 titlePos = { (WIDTH - titlePx.x) / 2.0f, (barHeight / 2.0f) - 15.0f };
    Vector2 subPos = { (WIDTH - subPx.x) / 2.0f, (barHeight / 2.0f) + 12.0f };

    DrawTextEx(font, title, titlePos, titleSize, spacing, ACCENT_COLOR);
    DrawTextEx(font, subtitle, subPos, subSize, spacing + 1.0f, SECONDARY_TEXT);
}

void DrawInstructions(float barHeight)
{
    const char* labels[] = { "UNDO [C]", "ENGINE [SPACE]" };
    KeyboardKey keys[] = { KEY_C, KEY_SPACE };
    
    float fontSize = 12.0f;
    float spacing = 1.0f;
    float padding = 25.0f;

    for (int i = 0; i < 2; i++)
    {
        Vector2 size = MeasureTextEx(font, labels[i], fontSize, spacing);
        float posX = (i == 0) ? padding : WIDTH - size.x - padding;
        
        bool isPressed = IsKeyDown(keys[i]);
        Color boxColor = isPressed ? ACCENT_COLOR : DIVIDER_COLOR;
        Color textColor = isPressed ? WHITE : ACCENT_COLOR;

        DrawRectangleRounded({ posX - 8, barHeight + 15, size.x + 16, size.y + 8 }, 0.4f, 8, boxColor);
        DrawTextEx(font, labels[i], { posX, barHeight + 19 }, fontSize, spacing, textColor);
    }
}

void LoadUI(void)
{
    font = LoadFontEx("assets/dejavu-sans/DejaVuSans-Bold.ttf", 64, 0, 0);
    chessFont = LoadFontEx("assets/dejavu-sans/DejaVuSans-Bold.ttf", 96, glyphs, sizeof(glyphs) / sizeof(int));
}

void UpdateUI(void)
{
}

void DrawUI(void)
{
    ClearBackground(BG_COLOR);
    DrawTopBar(80);
    DrawInstructions(80);
}

void UnloadUI(void)
{
    UnloadFont(font);
    UnloadFont(chessFont);
}