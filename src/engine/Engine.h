#ifndef DEF_OAKFOAM_ENGINE_H
#define DEF_OAKFOAM_ENGINE_H

#define PLAYOUTS_PER_MOVE 10000
#define PLAYOUTS_PER_MOVE_MAX 100000
#define PLAYOUTS_PER_MOVE_MIN 1000

#define UCB_C 0.02
#define UCB_INIT 1.1

#define RAVE_MOVES 3000
#define RAVE_INIT_WINS 0

#define UCT_EXPAND_AFTER 1
#define UCT_KEEP_SUBTREE true
#define UCT_SYMMETRY_USE true
#define UCT_ATARI_PRIOR 8
#define UCT_PATTERN_PRIOR 4
#define UCT_PASS_DETER 1

#define PLAYOUT_MAX_MOVE_FACTOR 3
#define PLAYOUT_ATARI_ENABLED true
#define PLAYOUT_PATTERNS_ENABLED true

#define SUREWIN_THRESHOLD 0.85

#define RESIGN_RATIO_THRESHOLD 0.05
#define RESIGN_MOVE_FACTOR_THRESHOLD 0.4

#define LIVEGFX_ON false
#define LIVEGFX_UPDATE_PLAYOUTS 300
#define LIVEGFX_DELAY 0.001

#define TIME_BUFFER 30.0
#define TIME_K 7
#define TIME_MOVE_MINIMUM 0.100

#define OUTPUTSGF_MAXCHILDREN 10

#define DEBUG_ON false

#define BOARDSIZE_MIN 2
#define BOARDSIZE_MAX 25

#include <cstdlib>
#include <cmath>
#include <string>
#include <list>
#include <vector>
#include <fstream>
#include <iomanip>
#include <boost/timer.hpp>
#include <config.h>
#include "Go.h"
#include "UCT.h"
#include "Random.h"
#include "Pattern.h"
#include "Parameters.h"
#include "Features.h"
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
    
    void generateMove(Go::Color col, Go::Move **move, bool playmove);
    bool isMoveAllowed(Go::Move move);
    void makeMove(Go::Move move);
    int getBoardSize() { return currentboard->getSize(); };
    void setBoardSize(int s);
    Go::Board *getCurrentBoard() { return currentboard; };
    void clearBoard();
    float getKomi() { return komi; };
    void setKomi(float k) { komi=k; };
    
    static void updateParameterWrapper(void *instance, std::string id)
    {
      Engine *me=(Engine*)instance;
      me->updateParameter(id);
    };
    void updateParameter(std::string id);
    
    static void gtpBoardSize(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpClearBoard(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpKomi(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpPlay(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpGenMove(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpRegGenMove(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpShowBoard(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpFinalScore(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpFinalStatusList(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    
    static void gtpParam(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpShowLiberties(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpShowValidMoves(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpShowGroupSize(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpShowPatternMatches(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpLoadPatterns(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpClearPatterns(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpDoBoardCopy(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpFeatureMatchesAt(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpFeatureProbDistribution(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    
    static void gtpTimeSettings(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpTimeLeft(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    
    static void gtpDoNPlayouts(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpOutputSGF(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    
    static void gtpExplainLastMove(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpBoardStats(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpShowSymmetryTransforms(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpShowTreeLiveGfx(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpDescribeEngine(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
  
  private:
    Gtp::Engine *gtpe;
    Go::Board *currentboard;
    float komi;
    int boardsize;
    float time_main,time_black,time_white;
    UCT::Tree *movetree;
    Random rand;
    Pattern::ThreeByThreeTable *patterntable;
    std::string lastexplanation;
    Parameters *params;
    Features *features;
    
    void addGtpCommands();
    
    void randomPlayoutMove(Go::Board *board, Go::Color col, Go::Move &move, int *posarray);
    void randomPlayout(Go::Board *board, std::list<Go::Move> startmoves, Go::Color colfirst, Go::BitBoard *firstlist, Go::BitBoard *secondlist);
    UCT::Tree *getPlayoutTarget(UCT::Tree *movetree);
    void expandLeaf(UCT::Tree *movetree);
    UCT::Tree *getBestMoves(UCT::Tree *movetree, bool descend);
    void clearMoveTree();
    void chooseSubTree(Go::Move move);
    bool isAtariCaptureOrConnect(Go::Board *board, int pos, Go::Color col, Go::Group *touchinggroup);
    
    float getTimeAllowedThisTurn(Go::Color col);
    
    void doNPlayouts(int n);
    bool writeSGF(std::string filename, Go::Board *board, UCT::Tree *tree);
    void doPlayout(Go::BitBoard *firstlist=NULL,Go::BitBoard *secondlist=NULL);
    void displayPlayoutLiveGfx(int totalplayouts=-1, bool livegfx=true);
};

#endif
