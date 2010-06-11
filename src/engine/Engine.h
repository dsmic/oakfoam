#ifndef DEF_OAKFOAM_ENGINE_H
#define DEF_OAKFOAM_ENGINE_H

#include <cstdlib>
#include <ctime>
#include "Go.h"
#include "Util.h"

class Engine
{
  public:
    void init();
    void generateMove(Go::Color col, Go::Move **move);
    bool isMoveAllowed(Go::Move move);
    void makeMove(Go::Move move);
    int getBoardSize();
    void setBoardSize(int s);
    Go::Board *getCurrentBoard();
    void clearBoard();
    float getKomi();
    void setKomi(float k);
  private:
    void randomValidMove(Go::Board *board, Go::Color col, Go::Move **move);
    void randomPlayout(Go::Board *board, Go::Color col);
};

#endif
