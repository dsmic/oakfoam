#ifndef DEF_OAKFOAM_ENGINE_H
#define DEF_OAKFOAM_ENGINE_H

#define PLAYOUTS_PER_MOVE 10000
#define PLAYOUTS_PER_MOVE_MAX 100000
#define PLAYOUTS_PER_MOVE_MIN 1000

#define UCB_C 0.02
#define UCB_INIT 1.1

#define RAVE_MOVES 3000
#define RAVE_INIT_WINS 5

#define UCT_EXPAND_AFTER 10
#define UCT_KEEP_SUBTREE true
#define UCT_SYMMETRY_USE true
#define UCT_VIRTUAL_LOSS true
#define UCT_LOCK_FREE false
#define UCT_ATARI_PRIOR 0
#define UCT_PATTERN_PRIOR 0
#define UCT_PASS_DETER 0
#define UCT_PROGRESSIVE_WIDENING_ENABLED true
#define UCT_PROGRESSIVE_WIDENING_A 20
#define UCT_PROGRESSIVE_WIDENING_B 1.4
#define UCT_PROGRESSIVE_WIDENING_C 0.67
#define UCT_PROGRESSIVE_WIDENING_COUNT_WINS true
#define UCT_POINTS_BONUS 0.02
#define UCT_LENGTH_BONUS -0.001
#define UCT_PROGRESSIVE_BIAS_ENABLED true
#define UCT_PROGRESSIVE_BIAS_H 10.0
#define UCT_PROGRESSIVE_BIAS_SCALED true
#define UCT_PROGRESSIVE_BIAS_RELATIVE false
#define UCT_CRITICALITY_URGENCY_FACTOR 0.00
#define UCT_CRITICALITY_URGENCY_DECAY false
#define UCT_CRITICALITY_UNPRUNE_FACTOR 0.00
#define UCT_CRITICALITY_UNPRUNE_MULTIPLY true
#define UCT_CRITICALITY_MIN_PLAYOUTS 150
#define UCT_CRITICALITY_SIBLINGS true
#define UCT_SLOW_UPDATE_INTERVAL 100
#define UCT_STOP_EARLY true
#define UCT_TERMINAL_HANDLING true

#define RULES_POSITIONAL_SUPERKO_ENABLED true
#define RULES_SUPERKO_TOP_PLY false
#define RULES_SUPERKO_PRUNE_AFTER 200
#define RULES_SUPERKO_AT_PLAYOUT true

#define PLAYOUT_MAX_MOVE_FACTOR 3
#define PLAYOUT_ATARI_ENABLED false
#define PLAYOUT_LASTCAPTURE_ENABLED true
#define PLAYOUT_PATTERNS_ENABLED true
#define PLAYOUT_FEATURES_ENABLED false
#define PLAYOUT_FEATURES_INCREMENTAL false
#define PLAYOUT_LASTATARI_ENABLED true
#define PLAYOUT_LASTATARI_LEAVEDOUBLE false
#define PLAYOUT_NAKADE_ENABLED true
#define PLAYOUT_FILLBOARD_ENABLED true
#define PLAYOUT_FILLBOARD_N 5
#define PLAYOUT_ANYCAPTURE_ENABLED true
#define PLAYOUT_LGRF1_ENABLED true
#define PLAYOUT_LGRF2_ENABLED true
#define PLAYOUT_MERCY_RULE_ENABLED true
#define PLAYOUT_MERCY_RULE_FACTOR 0.40
#define PLAYOUT_RANDOM_CHANCE 0.00
#define PLAYOUT_LAST2LIBATARI_ENABLED false
#define PLAYOUT_LAST2LIBATARI_COMPLEX false
#define PLAYOUT_AVOID_SELFATARI false
#define PLAYOUT_AVOID_SELFATARI_SIZE 6

#define PONDERING_ENABLED false
#define PONDERING_PLAYOUTS_MAX 100000

#define THREAD_COUNT 1

#define INTERRUPTS_ENABLED false

#define SUREWIN_THRESHOLD 0.90
#define SUREWIN_PASS_BONUS 1.00

#define RESIGN_RATIO_THRESHOLD 0.15
#define RESIGN_MOVE_FACTOR_THRESHOLD 0.3

#define BOOK_USE true

#define LIVEGFX_ON false
#define LIVEGFX_UPDATE_PLAYOUTS 300
#define LIVEGFX_DELAY 0.001

#define TIME_BUFFER 30.0
#define TIME_K 7
#define TIME_MOVE_MINIMUM 0.100
#define TIME_RESOLUTION 0.010

#define OUTPUTSGF_MAXCHILDREN 10

#define DEBUG_ON false

#define BOARDSIZE_MIN 2
#define BOARDSIZE_MAX 25

#include <config.h>
#include <string>
#include <list>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "Go.h"
#include "Tree.h"
#include "Random.h"
#include "Pattern.h"
#include "Parameters.h"
#include "Features.h"
#include "Time.h"
#include "Book.h"
#include "Playout.h"
#include "Benson.h"
#include "Worker.h"
#include "../gtp/Gtp.h"

class Engine
{
  public:
    Engine(Gtp::Engine *ge, std::string ln);
    ~Engine();
    
    enum MovePolicy
    {
      MP_PLAYOUT,
      MP_ONEPLY,
      MP_UCT
    };
    
    void run();
    void postCmdLineArgs(bool book_autoload);
    void generateMove(Go::Color col, Go::Move **move, bool playmove);
    bool isMoveAllowed(Go::Move move);
    void makeMove(Go::Move move);
    int getBoardSize() const { return currentboard->getSize(); };
    void setBoardSize(int s);
    Go::Board *getCurrentBoard() const { return currentboard; };
    void clearBoard();
    float getKomi() const { return komi; };
    void setKomi(float k);
    Pattern::ThreeByThreeTable *getPatternTable() const { return patterntable; };
    Features *getFeatures() const { return features; };
    Go::ZobristTable *getZobristTable() const { return zobristtable; };
    Go::ZobristTree *getZobristHashTree() const { return hashtree; };
    Gtp::Engine *getGtpEngine() const { return gtpe; };
    void stopThinking() { stopthinking=true; };
    static void ponderWrapper(void *instance) { ((Engine*)instance)->ponder(); };
    void ponder();
    void generateThread(Worker::Settings *settings);
    void ponderThread(Worker::Settings *settings);
    void doNPlayoutsThread(Worker::Settings *settings);
    void doThreadWork(Worker::Settings *settings);
    bool writeSGF(std::string filename, Go::Board *board=NULL, Tree *tree=NULL);
    
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
    static void gtpListAllPatterns(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpLoadFeatureGammas(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpListFeatureIds(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpShowCFGFrom(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpShowCircDistFrom(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpListCircularPatternsAt(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpListAllCircularPatterns(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpListAdjacentGroupsOf(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    
    static void gtpTimeSettings(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpTimeLeft(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    
    static void gtpDoNPlayouts(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpOutputSGF(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    
    static void gtpExplainLastMove(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpBoardStats(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpShowSymmetryTransforms(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpShowNakadeCenters(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpShowTreeLiveGfx(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpDescribeEngine(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    
    static void gtpBookShow(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpBookAdd(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpBookRemove(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpBookClear(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpBookLoad(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpBookSave(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    
    static void gtpShowCurrentHash(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpShowSafePositions(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpDoBenchmark(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpShowCriticality(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpShowTerritory(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
  
  private:
    Gtp::Engine *gtpe;
    std::string longname;
    Go::Board *currentboard;
    float komi;
    int boardsize;
    Time *time;
    Tree *movetree;
    Pattern::ThreeByThreeTable *patterntable;
    std::string lastexplanation;
    Parameters *const params;
    Features *features;
    Pattern::CircularDictionary *circdict;
    Book *book;
    std::list<Go::Move> *movehistory;
    Go::ZobristTable *zobristtable;
    Go::ZobristTree *hashtree;
    Playout *playout;
    volatile bool stopthinking;
    volatile bool stoppondering;
    Worker::Pool *threadpool;
    
    #ifdef HAVE_MPI
      int mpiworldsize,mpirank;
      
      enum MPICommand
      {
        MPICMD_QUIT,
        MPICMD_MAKEMOVE,
        MPICMD_SETBOARDSIZE,
        MPICMD_SETKOMI,
        MPICMD_CLEARBOARD
      };
      
      void mpiCommandHandler();
      void mpiBroadcastCommand(Engine::MPICommand cmd, unsigned int *arg1=NULL, unsigned int *arg2=NULL, unsigned int *arg3=NULL);
      void mpiRecvBroadcastedArgs(unsigned int *arg1=NULL, unsigned int *arg2=NULL, unsigned int *arg3=NULL);
    #endif
    
    void addGtpCommands();
    
    void clearMoveTree();
    void chooseSubTree(Go::Move move);
    
    void doNPlayouts(int n);
    void doPlayout(Worker::Settings *settings, Go::BitBoard *firstlist, Go::BitBoard *secondlist);
    void displayPlayoutLiveGfx(int totalplayouts=-1, bool livegfx=true);
    void doSlowUpdate();
    
    void allowContinuedPlay();
    
    boost::posix_time::ptime timeNow() { return boost::posix_time::microsec_clock::local_time(); };
    float timeSince(boost::posix_time::ptime past) { return (float)(boost::posix_time::microsec_clock::local_time()-past).total_milliseconds()/1000; };
};

#endif
