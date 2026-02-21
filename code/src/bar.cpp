#include <raylib.h>
#include <iostream>
#include <vector>
#include "bar.h"
#include "chess.h"
#include "config.h"

const float EP = 1e-6f;

float L1 = 160;
float L2 = 160;
float L3 = L2;
float L4 = L1;
float L5 = 90;

float R1 = L1 + L2;
float R2 = L3 + L4;

Vector2 A = {(WIDTH - L5) / 2.0f, HEIGHT / 3.5f};
Vector2 B = {A.x, A.y + L1};
Vector2 C = {A.x + L5 / 2.0f, A.y + L4 + sqrt(L2 * L2 - L5 * L5 / 4.0f)};
Vector2 D = {A.x + L5, A.y + L4};
Vector2 E = {A.x + L5, A.y};

std::vector<Vector2*> point = {&A, &B, &C, &D, &E};

bool dragging = false;

auto Dist = [](Vector2 a, Vector2 b)
{
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return sqrt(dx * dx + dy * dy);
};

void ModelIK(Vector2 mPos)
{    
    float rx, ry, d, a, b, t, cd;
 
    // point B
    rx = mPos.x - A.x;
    ry = mPos.y - A.y;

    d = sqrt(rx * rx + ry * ry);
    if (d < EP) return;
    
    a = atan2(ry, rx);

    cd = std::clamp(d, fabs(L1 - L2), L1 + L2);

    C.x = A.x + cd * cos(a);
    C.y = A.y + cd * sin(a);
    
    b = (L1 * L1 + cd * cd - L2 * L2) / (2.0f * L1 * cd);
    b = acos(std::clamp(b, -1.0f, 1.0f));

    t = a + b;

    B.x = A.x + L1 * cos(t);
    B.y = A.y + L1 * sin(t);
    
    // point D
    rx = C.x - E.x;
    ry = C.y - E.y;  

    d = sqrt(rx * rx + ry * ry);
    if (d < EP) return;
    
    a = atan2(ry, rx);

    cd = std::clamp(d, fabs(L3 - L4), L3 + L4);

    C.x = E.x + cd * cos(a);
    C.y = E.y + cd * sin(a);
    
    b = (L4 * L4 + d * d - L3 * L3) / (2.0f * L4 * d);
    b = acos(std::clamp(b, -1.0f, 1.0f));
    
    t = a - b;

    D.x = E.x + L4 * cos(t);
    D.y = E.y + L4 * sin(t);
}

void ModelK()
{
    static int step = 0;

    if (step < points.size())
    {
        Vector2 p = points[step];
        ModelIK({p.x, p.y});
        step++;
    }
    else step = 0;
}

void UpdateBar(void)
{
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
        CheckCollisionPointCircle(GetMousePosition(), {C.x, HEIGHT - C.y}, 10)) dragging = true;
    
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) dragging = false;

    static float cTick = 0.0f;
    static float tick = 0.1f;

    cTick += GetFrameTime();
    if (cTick >= tick)
    {
        cTick = 0.0f;
        ModelK();
    }

    // if (dragging)
    // {
    //     Vector2 mPos = GetMousePosition();
    //     mPos.y = HEIGHT - mPos.y;

    //     ModelIK(mPos);
    // }
}

void DrawRange(void)
{
    // testing
    float l1d = Dist(*point[0], *point[1]);
    float l2d = Dist(*point[1], *point[2]);
    float l3d = Dist(*point[2], *point[3]);
    float l4d = Dist(*point[3], *point[4]);
    float l5d = Dist(*point[4], *point[0]);

    DrawText(TextFormat("l1: %.2f", l1d), 20, 20, 20, BLACK);
    DrawText(TextFormat("l2: %.2f", l2d), 20, 45, 20, BLACK);
    DrawText(TextFormat("l3: %.2f", l3d), 20, 70, 20, BLACK);
    DrawText(TextFormat("l4: %.2f", l4d), 20, 95, 20, BLACK);
    DrawText(TextFormat("l5: %.2f", l5d), 20, 120, 20, BLACK);

    auto Valid = [&](Vector2 p)
    {
        float dxA = p.x - A.x;
        float dyA = p.y - A.y;

        float dxE = p.x - E.x;
        float dyE = p.y - E.y;

        float dA = dxA * dxA + dyA * dyA;
        float dE = dxE * dxE + dyE * dyE;

        return (dA <= R1 * R1 && dE <= R2 * R2);
    };

    const int step = 7;
    for (int x = 0; x < WIDTH; x += step)
    {
        for (int y = A.y; y < HEIGHT; y += step)
        {
            Vector2 p = {(float)x, (float)y};
            if (Valid(p)) DrawPixel(x, HEIGHT - y, GRAY);
        }
    }
}

void DrawBar(void)
{
    for (int i = 0; i < 4; ++i)
    {
        Vector2 p1 = *point[i];
        Vector2 p2 = *point[i + 1];
        DrawLineEx({p1.x, HEIGHT - p1.y}, {p2.x, HEIGHT - p2.y}, 2.5f, DARKGRAY);
    }

    for (int i = 0; i < 5; ++i)
    {
        Vector2 p = *point[i];
        DrawCircle(p.x, HEIGHT - p.y, 2.5f, (i != 2 ? BLACK : DARKGREEN));
    }
}