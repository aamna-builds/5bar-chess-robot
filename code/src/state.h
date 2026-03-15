#pragma once
#include <raylib.h>
#include <vector>

const int HEIGHT = 700;
const int WIDTH = 800;

enum class Stage
{
    TraceReady,
    Tracing,
    TraceFinished,
    None
};

struct SimState
{
    Vector2 accPos;
    Stage stage = Stage::TraceFinished;
    std::vector<std::vector<Vector2>> moveQueue;
};

extern SimState simState;