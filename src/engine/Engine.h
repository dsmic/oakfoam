#ifndef DEF_OAKFOAM_ENGINE_H
#define DEF_OAKFOAM_ENGINE_H

#define PLAYOUTS_PER_MOVE 10000
#define PLAYOUTS_PER_MOVE_MAX 100000
#define PLAYOUTS_PER_MOVE_MIN 1000
#define PLAYOUT_MAX_MOVE_FACTOR 3
#define PLAYOUT_ATARI_CHANCE 0.8

#define UCB_C 0.44
#define UCB_INIT 1.2

#define RAVE_MOVES 3000

#define UCT_EXPAND_AFTER 1
#define UCT_KEEP_SUBTREE true

#define RESIGN_RATIO_THRESHOLD 0.03
#define RESIGN_MOVE_FACTOR_THRESHOLD 0.5

#define LIVEGFX_ON false
#define LIVEGFX_UPDATE_PLAYOUTS 300
#define LIVEGFX_DELAY 0.001

#define TIME_BUFFER 30000
#define TIME_PERCENTAGE_BOARD 0.75
#define TIME_MOVE_BUFFER 10
#define TIME_FACTOR 2.5
#define TIME_MOVE_MINIMUM 100

#define BOARDSIZE_MIN 2
#define BOARDSIZE_MAX 25

#include <cstdlib>
#include <cmath>
#include <string>
#include <list>
#include <vector>
#include <boost/timer.hpp>
#include "Go.h"
#include "UCT.h"
#include "Random.h"
#include "../gtp/Gtp.h"

class Engine
{
  public:
    Engine(Gtp::Engine *ge);
    ~Engine();
    
    enum MovePolicy
    {
      MP_PLAYOUT,
      MP_ONEPLY,
      MP_UCT
    };
    
    void generateMove(Go::Color col, Go::Move **move, float *ratio=NULL);
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
    static void gtpFinalStatusList(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    
    static void gtpParam(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpShowLiberties(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpShowValidMoves(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpDoBoardCopy(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    
    static void gtpTimeSettings(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpTimeLeft(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
  
  private:
    Gtp::Engine *gtpe;
    Go::Board *currentboard;
    float komi;
    int boardsize;
    int playoutspermove,playoutspermoveinit,playoutspermovemax,playoutspermovemin;
    bool livegfx;
    long timemain,timeblack,timewhite;
    float playoutspermilli;
    float resignratiothreshold,resignmovefactorthreshold;
    long timebuffer,timemoveminimum;
    int timemovebuffer;
    float timepercentageboard,timefactor;
    float ucbc,ucbinit;
    int ravemoves;
    float playoutatarichance;
    Engine::MovePolicy movepolicy;
    int uctexpandafter;
    int livegfxupdateplayouts;
    float livegfxdelay;
    bool uctkeepsubtree;
    UCT::Tree *movetree;
    Random rand;
    
    void addGtpCommands();
    
    void randomPlayoutMove(Go::Board *board, Go::Color col, Go::Move **move, int *posarray);
    void randomPlayout(Go::Board *board, std::list<Go::Move> startmoves, Go::Color colfirst, Go::BitBoard *firstlist, Go::BitBoard *secondlist);
    UCT::Tree *getPlayoutTarget(UCT::Tree *movetree);
    void expandLeaf(UCT::Tree *movetree);
    UCT::Tree *getBestMoves(UCT::Tree *movetree, bool descend);
    void clearMoveTree();
    void chooseSubTree(Go::Move move);
    
    long getTimeAllowedThisTurn(Go::Color col);
};

#endif
