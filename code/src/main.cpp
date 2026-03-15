#include "bar.h"
#include "chess.h"
#include "stockfish.h"
#include "ui.h"
#include "state.h"
#include <raylib.h>

SimState simState;

int main(void)
{
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI | FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_MAXIMIZED);
    InitWindow(WIDTH, HEIGHT, "5-Bar Mechanism Simulation");
    SetTargetFPS(GetMonitorRefreshRate(GetCurrentMonitor()));
    
    sf.start("../bin/stockfish-macos");
    LoadUI();

    while (!WindowShouldClose())
    {
        UpdateUI();
        UpdateChess();
        UpdateBar();

        BeginDrawing();
        ClearBackground(DARKGRAY);

        DrawUI();
        DrawChess();
        DrawBar();

        EndDrawing();
    }

    sf.stop();
    UnloadUI();
    CloseWindow();
    return 0;
}
