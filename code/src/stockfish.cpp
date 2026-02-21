#include <iostream>
#include "stockfish.h"

Stockfish sf;

bool Stockfish::start(const std::string& path)
{
    engine = popen(path.c_str(), "r+");
    if (!engine) return false;

    send("uci");
    readUntil("uciok");

    send("isready");
    readUntil("readyok");

    return true;
}

void Stockfish::stop()
{
    if (engine)
    {
        pclose(engine);
        engine = nullptr;
    }
}

void Stockfish::send(const std::string& cmd)
{
    if (!engine) return;

    fprintf(engine, "%s\n", cmd.c_str());
    fflush(engine);
}

std::string Stockfish::readLine()
{
    if (!engine) return "";

    char buffer[512];
    if (fgets(buffer, sizeof(buffer), engine)) return std::string(buffer);

    return "";
}

std::string Stockfish::readUntil(const std::string& token)
{
    std::string line;

    while (true)
    {
        line = readLine();
        if (line.find(token) != std::string::npos) return line;
    }
}