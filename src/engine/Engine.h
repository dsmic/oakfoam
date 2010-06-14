#ifndef DEF_OAKFOAM_ENGINE_H
#define DEF_OAKFOAM_ENGINE_H

#define PLAYOUTS_PER_MOVE 100
#define RESIGN_THRESHOLD 0.05

#include <cstdlib>
#include <ctime>
#include "Go.h"
#include "Util.h"
#include "../gtp/Gtp.h"

class Engine
{
  public:
    Engine(Gtp::Engine *ge);
    ~Engine();
    
    void generateMove(Go::Color col, Go::Move **move);
    bool isMoveAllowed(Go::Move move);
    void makeMove(Go::Move move);
    int getBoardSize();
    void setBoardSize(int s);
    Go::Board *getCurrentBoard();
    void clearBoard();
    float getKomi();
    void setKomi(float k);
    
    static void gtpBoardSize(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpClearBoard(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpKomi(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpPlay(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpGenMove(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpShowBoard(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
  
  private:
    Gtp::Engine *gtpe;
    Go::Board *currentboard;
    float komi;
    int boardsize;
    
    void addGtpCommands();
    
    void randomValidMove(Go::Board *board, Go::Color col, Go::Move **move);
    void randomPlayout(Go::Board *board, Go::Color col);
};

#endif
