#include "bar.h"
#include "chess.h"
#include "stockfish.h"
#include "ui.h"
#include "state.h"
#include "serialib.h"
#include <raylib.h>

serialib arduino;
SimState simState;

int main(void)
{
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI | FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_MAXIMIZED);
    InitWindow(WIDTH, HEIGHT, "5-Bar Mechanism Simulation");
    SetTargetFPS(GetMonitorRefreshRate(GetCurrentMonitor()));
    
    if (arduino.openDevice("/dev/cu.usbserial-1120", 115200) != 1) { 
        std::cout << "Error: Could not open /dev/cu.usbserial-1120" << std::endl;
        std::cout << "Check if Arduino IDE Serial Monitor is open and close it." << std::endl;
    } else {
        std::cout << "Successfully connected to Arduino!" << std::endl;
        #ifdef _WIN32
            Sleep(2000);
        #else
            usleep(2000000); 
        #endif
    }
    
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

    arduino.closeDevice();
    sf.stop();
    UnloadUI();
    CloseWindow();
    return 0;
}
