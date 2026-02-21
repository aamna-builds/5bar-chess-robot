#include <raylib.h>
#include <iostream>
#include <vector>
#include "chess.h"
#include "config.h"

int squareSize = 32;
int fontSize = 20;

int boardSize = 8 * squareSize;
int offsetX = (WIDTH  - boardSize) / 2;
int offsetY = (HEIGHT - boardSize) / 2;

std::vector<Vector2> points;

std::pair<char, bool> mat[8][8] =
{
    {{'r', 0}, {'n', 0}, {'b', 0}, {'q', 0}, {'k', 0}, {'b', 0}, {'n', 0}, {'r', 0}},
    {{'p', 0}, {'p', 0}, {'p', 0}, {'p', 0}, {'p', 0}, {'p', 0}, {'p', 0}, {'p', 0}},
    {{' ', 0}, {' ', 0}, {' ', 0}, {' ', 0}, {' ', 0}, {' ', 0}, {' ', 0}, {' ', 0}},
    {{' ', 0}, {' ', 0}, {' ', 0}, {' ', 0}, {' ', 0}, {' ', 0}, {' ', 0}, {' ', 0}},
    {{' ', 0}, {' ', 0}, {' ', 0}, {' ', 0}, {' ', 0}, {' ', 0}, {' ', 0}, {' ', 0}},
    {{' ', 0}, {' ', 0}, {' ', 0}, {' ', 0}, {' ', 0}, {' ', 0}, {' ', 0}, {' ', 0}},
    {{'p', 1}, {'p', 1}, {'p', 1}, {'p', 1}, {'p', 1}, {'p', 1}, {'p', 1}, {'p', 1}},
    {{'r', 1}, {'n', 1}, {'b', 1}, {'q', 1}, {'k', 1}, {'b', 1}, {'n', 1}, {'r', 1}}
};

float EaseInOut(float t)
{
    return (t < 0.5f)
        ? 2.0f * t * t
        : 1.0f - pow(-2.0f * t + 2.0f, 2.0f) / 2.0f;
}

std::vector<char> GenerateMove(std::string start, std::string end)
{
    std::vector<char> dirs;

    int sX = start[0] - '0';
    int sY = start[1];
    int eX = end[0] - '0';
    int eY = end[1];

    while (sX != eX || sY != eY) {
        if (sY < eY) { dirs.push_back('U'); sY++; }
        else if (sY > eY) { dirs.push_back('D'); sY--; }
        else if (sX < eX) { dirs.push_back('R'); sX++; }
        else if (sX > eX) { dirs.push_back('L'); sX--; }
    }

    return dirs;
}

std::vector<Vector2> GetEdgePath(const std::vector<char>& dirs, std::string cPos, std::string endPos)
{
    std::vector<Vector2> vec;

    int startCol = cPos[0] - 'a';
    int startRow = 7 - (cPos[1] - '1');

    int endCol = endPos[0] - 'a';
    int endRow = 7 - (endPos[1] - '1');

    float startCX = offsetX + startCol * squareSize + squareSize / 2.0f;
    float startCY = offsetY + startRow * squareSize + squareSize / 2.0f;

    float endCX = offsetX + endCol * squareSize + squareSize / 2.0f;
    float endCY = offsetY + endRow * squareSize + squareSize / 2.0f;

    float midX = (startCX + endCX) / 2.0f;
    float midY = (startCY + endCY) / 2.0f;

    vec.push_back({startCX, startCY});

    int col = startCol;
    int row = startRow;

    for (size_t i = 0; i < dirs.size(); i++)
    {
        float cx = offsetX + col * squareSize + squareSize / 2.0f;
        float cy = offsetY + row * squareSize + squareSize / 2.0f;

        float dx = (midX >= cx) ? squareSize / 2.0f : -squareSize / 2.0f;
        float dy = (midY >= cy) ? squareSize / 2.0f : -squareSize / 2.0f;

        vec.push_back({cx + dx, cy + dy});

        char d = dirs[i];

        if (d == 'U' && row > 0) row--;
        else if (d == 'D' && row < 7) row++;
        else if (d == 'L' && col > 0) col--;
        else if (d == 'R' && col < 7) col++;
    }

    float cx = offsetX + col * squareSize + squareSize / 2.0f;
    float cy = offsetY + row * squareSize + squareSize / 2.0f;

    float dx = (midX >= cx) ? squareSize / 2.0f : -squareSize / 2.0f;
    float dy = (midY >= cy) ? squareSize / 2.0f : -squareSize / 2.0f;

    vec.push_back({cx + dx, cy + dy});
    vec.push_back({endCX, endCY});

    vec.erase(
    std::unique(vec.begin(), vec.end(),
        [](const Vector2& a, const Vector2& b)
        {
            const float eps = 0.0001f;
            return fabs(a.x - b.x) < eps && fabs(a.y - b.y) < eps;
        }),
    vec.end());

    return vec;
}

std::vector<Vector2> BuildEasedCycle(const std::vector<Vector2>& base, int stepsPerSegment)
{
    std::vector<Vector2> result;

    if (base.size() < 2) return base;

    for (size_t i = 0; i < base.size() - 1; i++)
    {
        Vector2 a = base[i];
        Vector2 b = base[i + 1];

        for (int s = 0; s < stepsPerSegment; s++)
        {
            float t = (float)s / (float)stepsPerSegment;
            t = EaseInOut(t);

            Vector2 p;
            p.x = a.x + (b.x - a.x) * t;
            p.y = a.y + (b.y - a.y) * t;

            result.push_back(p);
        }
    }

    result.push_back(base.back());

    return result;
}

void UpdateChess(void)
{
    if (IsKeyPressed(KEY_SPACE)) points = BuildEasedCycle(GetEdgePath(GenerateMove("b2", "f7"), "b2", "f7"), 4);
}

void DrawChess(void)
{
    for (int row = 0; row < 8; ++row)
    {
        for (int col = 0; col < 8; ++col)
        {
            bool isLight = (row + col) % 2 == 0;
            Color tileColor = isLight ? Color{240, 217, 181, 255} : Color{181, 136, 99, 255};

            DrawRectangle(
                offsetX + col * squareSize,
                offsetY + row * squareSize,
                squareSize, squareSize, tileColor
            );
        }
    }

    for (int row = 0; row < 8; ++row)
    {
        for (int col = 0; col < 8; ++col)
        {
            char piece = mat[row][col].first;

            if (piece == ' ') continue;

            bool isWhite = mat[row][col].second;
            Color pieceColor = isWhite ? WHITE : BLACK;

            DrawText(
                TextFormat("%c", piece),
                offsetX + col * squareSize + squareSize / 3,
                offsetY + row * squareSize + squareSize / 4,
                fontSize,
                pieceColor
            );
        }
    }

    for (const auto& p : points) DrawCircleV(p, 2.5f, MAROON);
}