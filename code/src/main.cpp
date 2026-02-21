#include <raylib.h>
#include "bar.h"
#include "chess.h"
#include "stockfish.h"
#include "config.h"

int main(void)
{
    SetConfigFlags(
        FLAG_VSYNC_HINT |
        FLAG_WINDOW_HIGHDPI |
        FLAG_WINDOW_RESIZABLE 
    );
    InitWindow(WIDTH, HEIGHT, "5-Bar Mechanism Simulation");
    SetTargetFPS(GetMonitorRefreshRate(GetCurrentMonitor()));

    sf.start("../bin/stockfish-macos");

    while (!WindowShouldClose())
    {
        UpdateChess();
        UpdateBar();

        BeginDrawing();
        ClearBackground(LIGHTGRAY);

        DrawChess();
        DrawBar();

        EndDrawing();
    }

    sf.stop();
    CloseWindow();
    return 0;
}
