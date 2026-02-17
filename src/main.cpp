#include <raylib.h>
#include <string>
#include <math.h>
#include <cfloat>
#include <algorithm>

enum class Mode
{
    Inward,
    Outward
};

auto Dist = [](Vector2 a, Vector2 b)
{
    float dx = a.x - b.x;
    float dy = a.y - b.y;

    return sqrt(dx * dx + dy * dy);
};

void ModulateVector2(Vector2& p, Vector2 a, Vector2 b, float l1, float l2, float& pTheta)
{
    auto SelectStableAngle = [](float theta, float plus, float minus)
    {
    return (fabs(plus - theta) < fabs(minus - theta)) ? plus : minus;
    };

    Vector2 r = {a.x - b.x, a.y - b.y};
    float d = sqrt(pow(r.x, 2) + pow(r.y, 2));

    if (d < 1e-6f) return;

    float alpha = acos(std::clamp((float)(pow(l1, 2) + pow(d, 2) - pow(l2, 2)) / (2.0f * l1 * d), -1.0f, 1.0f));
    float theta = atan2(r.y, r.x);

    float thetaPlus = theta + alpha;
    float thetaMinus = theta - alpha;

    float fTheta = SelectStableAngle(pTheta, thetaPlus, thetaMinus);

    pTheta = fTheta;

    p.x = b.x + l1 * cos(fTheta);
    p.y = b.y + l1 * sin(fTheta);
}

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
    float startX = 250;
    float startY = 300;
    
    float l1 = 50;
    float l2 = 125;
    float l3 = l2;
    float l4 = l1;
    float l5 = 140;

    float pThetal1 = 0;
    float pThetal4 = 0;

    Vector2 P[5] = {
        {startX, startY},
        {startX - l1, startY},
        {startX + l5 / 2.0f, startY},
        {startX + l5 + l4, startY},
        {startX + l5, startY}
    };
    
    static bool isDragging = false;

    while (!WindowShouldClose())
    {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
            CheckCollisionPointCircle(GetMousePosition(), {P[2].x, HEIGHT - P[2].y}, 10)) isDragging = true;
        
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) isDragging = false;

        if (isDragging)
        {
            Vector2 mPos = GetMousePosition();
            mPos.y = HEIGHT - mPos.y;

            Vector2 A = P[0];
            Vector2 C = P[2];
            Vector2 E = P[4];

            float d1x = mPos.x - A.x;
            float d1y = mPos.y - A.y;
            float d2x = mPos.x - E.x;
            float d2y = mPos.y - E.y;

            float dCA = sqrt(d1x * d1x + d1y * d1y);
            float dCE = sqrt(d2x * d2x + d2y * d2y);
            
            if (dCA < l1 + l2 && dCE < l3 + l4)
            {
                C.x = mPos.x;
                C.y = mPos.y;
            }
            else
            {
                float R1 = l1 + l2;
                float R2 = l3 + l4;

                auto ProjectToCircle = [](Vector2 p, Vector2 c, float R)
                {
                    Vector2 v = {p.x - c.x, p.y - c.y};
                    float d = sqrt(v.x * v.x + v.y * v.y);

                    if (d < 1e-6f) return Vector2{c.x, c.y};

                    float scale = R / d;

                    return Vector2{
                        c.x + v.x * scale,
                        c.y + v.y * scale
                    };
                };

                Vector2 c1 = ProjectToCircle(mPos, A, R1);
                Vector2 c2 = ProjectToCircle(mPos, E, R2);

                auto Valid = [&](Vector2 p)
                {
                    float dxA = p.x - A.x;
                    float dyA = p.y - A.y;

                    float dxE = p.x - E.x;
                    float dyE = p.y - E.y;

                    float dA = sqrt(dxA * dxA + dyA * dyA);
                    float dE = sqrt(dxE * dxE + dyE * dyE);

                    return (dA <= R1 && dE <= R2);
                };

                // fallback
                Vector2 best = {C.x, C.y};
                float bestDist = FLT_MAX;

                if (Valid(c1))
                {
                    float dx = c1.x - mPos.x;
                    float dy = c1.y - mPos.y;

                    float dist = dx * dx + dy * dy;

                    if (dist < bestDist)
                    {
                        best = c1;
                        bestDist = dist;
                    }
                }

                if (Valid(c2))
                {
                    float dx = c2.x - mPos.x;
                    float dy = c2.y - mPos.y;

                    float dist = dx * dx + dy * dy;

                    if (dist < bestDist)
                    {
                        best = c2;
                        bestDist = dist;
                    }
                }

                C.x = best.x;
                C.y = best.y;
            }

            ModulateVector2(P[1], C, P[0], l1, l2, pThetal1);
            ModulateVector2(P[3], C, P[4], l4, l3, pThetal4);

            P[2] = C;
        }

        BeginDrawing();
        ClearBackground(DARKGRAY);

        DrawText(TextFormat("l1: %.2f", Dist(P[0], P[1])), 20, 20, 20, BLACK);
        DrawText(TextFormat("l2: %.2f", Dist(P[1], P[2])), 20, 45, 20, BLACK);
        DrawText(TextFormat("l3: %.2f", Dist(P[2], P[3])), 20, 70, 20, BLACK);
        DrawText(TextFormat("l4: %.2f", Dist(P[3], P[4])), 20, 95, 20, BLACK);
        DrawText(TextFormat("l5: %.2f", Dist(P[4], P[0])), 20, 120, 20, BLACK);

        for (int i = 0; i < 4; ++i)
        {
            Vector2 p1 = P[i];
            Vector2 p2 = P[i + 1];

            float size = !isDragging ? 5 : ((i != 1 && i != 2) ? 5 : 7.5);
            Color color = !isDragging ? WHITE : ((i != 1 && i != 2) ? WHITE : PINK);
            DrawLineEx({p1.x, HEIGHT - p1.y}, {p2.x, HEIGHT - p2.y}, size, color);
        }

        for (int i = 0; i < 5; ++i)
        {
            Vector2 p = P[i];

            float radius = !isDragging ? 5 : (i != 2 ? 5 : 7.5);
            DrawCircle(p.x, HEIGHT - p.y, radius, BLUE);
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
