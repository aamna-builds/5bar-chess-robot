#include <raylib.h>

struct Point
{
    float x;
    float y;

    Point() : x(0), y(0) {}
    Point(int x_, int y_) : x(x_), y(y_) {}
};

int main(void)
{
    // render config
    const int WIDTH = 600;
    const int HEIGHT = 600;

    SetConfigFlags(
        FLAG_VSYNC_HINT |
        FLAG_WINDOW_HIGHDPI |
        FLAG_WINDOW_RESIZABLE 
    );
    InitWindow(WIDTH, HEIGHT, "5-Bar Mechanism Simulation");
    SetTargetFPS(GetMonitorRefreshRate(GetCurrentMonitor()));

    // initial simulation setup
    int startX = 200;
    int startY = 300;

    int l1 = 100;
    int l2 = 100;
    int l3 = l2;
    int l4 = l1;
    int l5 = 200;

    Point P[5] = {
        {startX, startY},
        {startX, startY + l1},
        {startX + (l5 / 2), startY + (l5 / 2)},
        {startX + l5, startY + l4},
        {startX + l5, startY}
    };

    // attributes
    static bool isDragging = false;

    while (!WindowShouldClose())
    {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
            CheckCollisionPointCircle(GetMousePosition(), {P[2].x, HEIGHT - P[2].y}, 5)) isDragging = true;
        
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) isDragging = false;

        BeginDrawing();
        ClearBackground(DARKGRAY);

        for (int i = 0; i < 4; ++i)
        {
            Point p1 = P[i];
            Point p2 = P[i + 1];

            float size = !isDragging ? 5 : ((i != 1 && i != 2) ? 5 : 7.5);
            Color color = !isDragging ? WHITE : ((i != 1 && i != 2) ? WHITE : PINK);
            DrawLineEx({p1.x, HEIGHT - p1.y}, {p2.x, HEIGHT - p2.y}, size, color);
        }

        for (int i = 0; i < 5; ++i)
        {
            Point p = P[i];

            float radius = !isDragging ? 5 : (i != 2 ? 5 : 7.5);
            DrawCircle(p.x, HEIGHT - p.y, radius, BLUE);
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
