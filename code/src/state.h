#pragma once
#include <raylib.h>
#include <vector>
#include "chess.h"

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
    Vector2i accPos;
    Stage stage = Stage::TraceFinished;
    std::vector<bool> latchQueue;
    std::vector<std::vector<Vector2>> moveQueue;
};

extern SimState simState;

enum class DemoStage
{
    Idle,
    Running,
    Done
};

struct DemoState
{
    DemoStage stage = DemoStage::Idle;
    float timer = 0.0f;
    int step = 0;
};

extern DemoState demoState;

struct ManualControl
{
    bool active = false;
    Vector2 pos;
};

extern ManualControl manualControl;