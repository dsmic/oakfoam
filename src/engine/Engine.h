#ifndef DEF_OAKFOAM_ENGINE_H
#define DEF_OAKFOAM_ENGINE_H

#define PLAYOUTS_PER_MOVE 100
#define RESIGN_THRESHOLD 0.05
#define BOARDSIZE_MIN 2
#define BOARDSIZE_MAX 25


#include <cstdlib>
#include <ctime>
#include <list>
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
    int getBoardSize() { return currentboard->getSize(); };
    void setBoardSize(int s);
    Go::Board *getCurrentBoard() { return currentboard; };
    void clearBoard();
    float getKomi() { return komi; };
    void setKomi(float k) { komi=k; };
    
    static void gtpBoardSize(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpClearBoard(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpKomi(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpPlay(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpGenMove(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpShowBoard(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpFinalScore(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    
    static void gtpShowGroups(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpShowLiberties(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
  
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
