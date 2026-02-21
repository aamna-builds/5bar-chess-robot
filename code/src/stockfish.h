#pragma once
#include <cstdio>
#include <string>

class Stockfish
{
public:
    bool start(const std::string& path);
    void stop();

    void send(const std::string& cmd);
    std::string readLine();
    std::string readUntil(const std::string& token);

private:
    FILE* engine = nullptr;
};

extern Stockfish sf;  