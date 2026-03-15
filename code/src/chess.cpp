#include <raylib.h>
#include <iostream>
#include <vector>
#include "chess.h"
#include "stockfish.h"
#include "ui.h"
#include "state.h"

struct Vector2i
{
    int x;
    int y;
};

struct Move
{
    Vector2i from;
    Vector2i to;
    char promotion = 0;
};

struct PieceOut
{
    Move move;
    std::pair<char, bool> piece;
};

static const char* glyphs[128] =
{
    ['k'] = "♚",
    ['q'] = "♛",
    ['r'] = "♜",
    ['b'] = "♝",
    ['n'] = "♞",
    ['p'] = "♟"
};

int squareSize = 48;
int fontSize = 35;

int boardSize = 8 * squareSize;
int offsetX = (WIDTH  - boardSize) / 2;
int offsetY = (HEIGHT - boardSize) / 2;

float stepSize = 3.0f;

std::vector<std::string> moves;
std::vector<PieceOut> pieceOut;

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

std::vector<char> GenerateDirs(Move m)
{
    std::vector<char> dirs;

    int sX = m.from.x;
    int sY = m.from.y;
    int eX = m.to.x;
    int eY = m.to.y;

    while (sX != eX || sY != eY) {
        if (sY < eY) { dirs.push_back('U'); sY++; }
        else if (sY > eY) { dirs.push_back('D'); sY--; }
        else if (sX < eX) { dirs.push_back('R'); sX++; }
        else if (sX > eX) { dirs.push_back('L'); sX--; }
    }

    return dirs;
}

std::vector<Vector2> GetEdgePath(const std::vector<char>& dirs, Move m)
{
    std::vector<Vector2> vec;

    int startCol = m.from.x;
    int startRow = 7 - m.from.y;
    int endCol = m.to.x;
    int endRow = 7 - m.to.y;

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

float SegmentLength(Vector2 a, Vector2 b)
{
    float dx = b.x - a.x;
    float dy = b.y - a.y;
    return sqrtf(dx * dx + dy * dy);
}

float PathLength(const std::vector<Vector2>& path)
{
    float length = 0;

    for (size_t i = 0; i < path.size() - 1; i++)
    {
        length += SegmentLength(path[i + 1], path[i]);
    }

    return length;
}

std::vector<Vector2> BuildEasedPath(const std::vector<Vector2>& base, int totalSteps)
{
    std::vector<Vector2> result;

    if (base.size() < 2) return base;

    std::vector<float> lengths;
    lengths.push_back(0);

    float totalLength = 0;

    for (size_t i = 0; i < base.size() - 1; i++)
    {
        totalLength += SegmentLength(base[i], base[i+1]);
        lengths.push_back(totalLength);
    }

    for (int step = 0; step <= totalSteps; step++)
    {
        float t = (float)step / (float)totalSteps;

        float targetDist = t * totalLength;

        for (size_t i = 0; i < lengths.size() - 1; i++)
        {
            if (targetDist >= lengths[i] && targetDist <= lengths[i+1])
            {
                float segT = (targetDist - lengths[i]) / (lengths[i+1] - lengths[i]);

                Vector2 a = base[i];
                Vector2 b = base[i+1];

                Vector2 p;
                p.x = a.x + (b.x - a.x) * segT;
                p.y = a.y + (b.y - a.y) * segT;

                result.push_back(p);
                break;
            }
        }
    }

    return result;
}

Move ParseMove(const std::string& uci)
{
    Move m;

    m.from.x = uci[0] - 'a';
    m.from.y = 8 - (uci[1] - '0');

    m.to.x = uci[2] - 'a';
    m.to.y = 8 - (uci[3] - '0');

    if (uci.length() == 5) m.promotion = uci[4];

    return m;
}

void ApplyMoveToBoard(Move m, bool undo)
{
    auto piece = mat[m.from.y][m.from.x];
    auto pieceOld = mat[m.to.y][m.to.x];

    if (!undo) pieceOut.push_back({m, pieceOld});

    mat[m.to.y][m.to.x] = piece;
    mat[m.from.y][m.from.x] = {' ', 0};

    if (m.promotion)
    {
        mat[m.to.y][m.to.x].first = m.promotion;
    }
}

std::string GetEngineMove(const std::vector<std::string>& moves)
{
    std::string cmd = "position startpos moves ";

    for (const std::string& m : moves) cmd += m + " ";

    sf.send(cmd);
    sf.send("go movetime 150");

    std::string line;

    while (true)
    {
        line = sf.readLine();
        if (line.find("bestmove") != std::string::npos) return line.substr(9, 4);
    }
}

void MakeMove(Move m, bool undo)
{
    std::vector<Vector2> edgePath = GetEdgePath(GenerateDirs(m), m);

    simState.latchQueue.push_back(true);
    simState.moveQueue.push_back(BuildEasedPath(edgePath, ceil(PathLength(edgePath) / stepSize)));

    ApplyMoveToBoard(m, undo);
}

void Undo()
{
    if (moves.empty()) return;
    if (pieceOut.empty()) return;
    
    std::string move = moves.back();

    Move m = ParseMove(move);
    PieceOut po = pieceOut.back();
    
    std::swap(move[0], move[2]);
    std::swap(move[1], move[3]);
    
    m = ParseMove(move);
    MakeMove(m, true);
    
    mat[m.from.y][m.from.x] = po.piece;

    pieceOut.pop_back();
    moves.pop_back();
}

void EngineTurn()
{
    std::string uci = GetEngineMove(moves);

    Move m = ParseMove(uci);
    MakeMove(m, false);

    moves.push_back(uci);
}

void PlayerTurn(const std::string& playerMove)
{
    
}

void DrawMoveList()
{
    const int fontSize = 14;
    const int padding = 25;
    const int lineHeight = fontSize + 8;
    const Color MOVE_TEXT = { 100, 100, 110, 255 };

    int y = HEIGHT - padding - lineHeight;
    std::string line;

    for (int i = 0; i < moves.size(); i++)
    {
        line += moves[i] + "  ";

        if ((i + 1) % 12 == 0 || i == (int)moves.size() - 1)
        {
            DrawTextEx(font, line.c_str(), { (float)padding, (float)y }, fontSize, 1.0f, MOVE_TEXT);
            y -= lineHeight;
            line.clear();
        }
    }
}

void UpdateChess(void)
{
    if (simState.stage == Stage::TraceFinished)
    {
        if (IsKeyPressed(KEY_SPACE)) EngineTurn();
        else if (IsKeyPressed(KEY_C)) Undo();
    }
}

 void DrawChess(void)
{
    static const Color TILE_LIGHT = { 235, 235, 208, 255 };
    static const Color TILE_DARK = { 119, 148, 85, 255 };
    static const Color BORDER_COLOR = { 80, 80, 70, 255 };
    static const Color PIECE_WHITE = { 255, 255, 255, 255 };
    static const Color PIECE_BLACK = { 20, 20, 20, 255 };
    static const Color PIECE_OUTLINE = { 40, 40, 40, 100 };
    static const Color TRAJECTORY_COLOR = { 0, 150, 255, 255 }; 

    for (int i = 0; i < 8; i++)
    {
        const char* rank = TextFormat("%d", 8 - i);
        const char* file = TextFormat("%c", 'A' + i);
        DrawTextEx(font, rank, { (float)offsetX - 20, (float)offsetY + i * squareSize + squareSize / 3.0f }, 16, 1, BORDER_COLOR);
        DrawTextEx(font, file, { (float)offsetX + i * squareSize + squareSize / 2.5f, (float)offsetY + 8 * squareSize + 10 }, 16, 1, BORDER_COLOR);
    }

    for (int row = 0; row < 8; ++row)
    {
        for (int col = 0; col < 8; ++col)
        {
            Color tileColor = ((row + col) % 2 == 0) ? TILE_LIGHT : TILE_DARK;
            DrawRectangle(offsetX + col * squareSize, offsetY + row * squareSize, squareSize, squareSize, tileColor);
        }
    }

    DrawRectangleLinesEx({ (float)offsetX - 4, (float)offsetY - 4, squareSize * 8.0f + 8, squareSize * 8.0f + 8 }, 4.0f, BORDER_COLOR);

    if (!simState.moveQueue.empty())
    {
        for (const Vector2& p : simState.moveQueue.front()) 
        {
            Vector2 screenPos = { p.x, HEIGHT - p.y };
            DrawCircleV(screenPos, 2.0f, WHITE);
            DrawCircleV(screenPos, 1.5f, TRAJECTORY_COLOR);
        }
    }

    for (int row = 0; row < 8; ++row)
    {
        for (int col = 0; col < 8; ++col)
        {
            char piece = mat[row][col].first;
            if (piece == ' ') continue;

            bool isWhite = mat[row][col].second;
            Color pieceColor = isWhite ? PIECE_WHITE : PIECE_BLACK;
            Vector2 pos = { offsetX + col * squareSize + squareSize / 4.0f, offsetY + row * squareSize + squareSize / 6.0f };

            if (isWhite)
            {
                DrawTextEx(chessFont, glyphs[piece], { pos.x - 1, pos.y }, fontSize, 0.0f, PIECE_OUTLINE);
                DrawTextEx(chessFont, glyphs[piece], { pos.x + 1, pos.y }, fontSize, 0.0f, PIECE_OUTLINE);
                DrawTextEx(chessFont, glyphs[piece], { pos.x, pos.y - 1 }, fontSize, 0.0f, PIECE_OUTLINE);
                DrawTextEx(chessFont, glyphs[piece], { pos.x, pos.y + 1 }, fontSize, 0.0f, PIECE_OUTLINE);
            }
            else 
            {
                DrawTextEx(chessFont, glyphs[piece], { pos.x + 2, pos.y + 2 }, fontSize, 0.0f, { 0, 0, 0, 50 });
            }
            
            DrawTextEx(chessFont, glyphs[piece], pos, fontSize, 0.0f, pieceColor);
        }
    }
    
    DrawMoveList();
}