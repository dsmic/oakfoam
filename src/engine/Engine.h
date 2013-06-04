#ifndef DEF_OAKFOAM_ENGINE_H
#define DEF_OAKFOAM_ENGINE_H

#define PLAYOUTS_PER_MOVE 10000
#define PLAYOUTS_PER_MOVE_MAX 1000000
#define PLAYOUTS_PER_MOVE_MIN 1000

#define UCB_C 0.02
#define UCB_INIT 1.1

#define BERNOULLI_A 0.0
#define BERNOULLI_B 0.0
#define WEIGHT_SCORE 0.0
#define RANDOM_F 0.0

#define RAVE_MOVES 3000
#define RAVE_INIT_WINS 5
#define UCT_PRESET_RAVE_F 0.0
#define RAVE_SKIP 0.00
#define RAVE_MOVES_USE 0.00
#define UCT_EXPAND_AFTER 10
#define UCT_KEEP_SUBTREE true
#define UCT_SYMMETRY_USE true
#define UCT_VIRTUAL_LOSS true
#define UCT_LOCK_FREE false
#define UCT_ATARI_PRIOR 0
#define UCT_PLAYOUTMOVE_PRIOR 0
#define UCT_PATTERN_PRIOR 0
#define UCT_PASS_DETER 0
#define UCT_PROGRESSIVE_WIDENING_ENABLED true
#define UCT_PROGRESSIVE_WIDENING_INIT 1
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
//value from 9x9 200 playout optimization
#define UCT_PROGRESSIVE_BIAS_MOVES 113.0
#define UCT_PROGRESSIVE_BIAS_EXPONENT 1.0
#define UCT_PROGRESSIVE_BIAS_RELATIVE false

#define UCT_CRITICALITY_URGENCY_FACTOR 0.00
#define UCT_CRITICALITY_URGENCY_DECAY 0.0
#define UCT_CRITICALITY_UNPRUNE_FACTOR 0.00
#define UCT_CRITICALITY_UNPRUNE_MULTIPLY true
#define UCT_CRITICALITY_MIN_PLAYOUTS 150
#define UCT_CRITICALITY_SIBLINGS true
#define UCT_CRITICALITY_RAVE_UNPRUNE_FACTOR 0.0
#define UCT_SLOW_UPDATE_INTERVAL 100
#define UCT_SLOW_DEBUG_INTERVAL 5000
#define UCT_STOP_EARLY true
#define UCT_TERMINAL_HANDLING true
#define UCT_PRIOR_UNPRUNE_FACTOR 0.00
#define UCT_RAVE_UNPRUNE_FACTOR 0.00
#define UCT_EARLYRAVE_UNPRUNE_FACTOR 0.00
#define UCT_RAVE_UNPRUNE_DECAY 0.00
#define UCT_OLDMOVE_UNPRUNE_FACTOR 0.00
#define UCT_OLDMOVE_UNPRUNE_FACTOR_B 0.00
#define UCT_OLDMOVE_UNPRUNE_FACTOR_C 0.00
#define UCT_AREA_OWNER_FACTOR_A 0.00
#define UCT_AREA_OWNER_FACTOR_B 0.33333
#define UCT_AREA_OWNER_FACTOR_C 1.00
#define UCT_REPRUNE_FACTOR 0.00
#define UCT_FACTOR_CIRCPATTERN 0.00
#define UCT_FACTOR_CIRCPATTERN_EXPONENT 1.00
#define UCT_CIRCPATTERN_MINSIZE 2
#define UCT_SIMPLE_PATTERN_FACTOR 1.0
#define UCT_ATARI_UNPRUNE 1.0
#define UCT_ATARI_UNPRUNE_EXP 0.0
#define UCT_DANGER_VALUE 0.0
#define UCT_RAVE_UNPRUNE_MULTIPLY false

#define UCT_DECAY_ALPHA 1
#define UCT_DECAY_K 0
#define UCT_DECAY_M 0

#define FEATURES_LADDERS false
#define FEATURES_PASS_NO_MOVE_FOR_LASTDIST false
#define MM_LEARN_DELTA 0.01
#define MM_LEARN_MIN_PLAYOUTS 100

#define RULES_POSITIONAL_SUPERKO_ENABLED true
#define RULES_SUPERKO_TOP_PLY false
#define RULES_SUPERKO_PRUNE_AFTER 200
#define RULES_SUPERKO_AT_PLAYOUT true
#define RULES_ALL_STONES_ALIVE true
#define RULES_ALL_STONES_ALIVE_PLAYOUTS 100

#define PLAYOUT_MAX_MOVE_FACTOR 3
#define PLAYOUT_ATARI_ENABLED false
#define PLAYOUT_LASTCAPTURE_ENABLED true
#define PLAYOUT_PATTERNS_P 1.0
#define PLAYOUT_PATTERNS_GAMMAS_P 0.0
#define PLAYOUT_FEATURES_ENABLED false
#define PLAYOUT_FEATURES_INCREMENTAL false
#define PLAYOUT_LASTATARI_P 1.0
#define PLAYOUT_LASTATARI_LEAVEDOUBLE true
#define PLAYOUT_LASTATARI_CAPTUREATTACHED 1.0
#define PLAYOUT_NAKADE_ENABLED true
#define PLAYOUT_NAKADE4_ENABLED false
#define PLAYOUT_NAKADE_BENT4_ENABLED false
#define PLAYOUT_NAKADE5_ENABLED false
#define PLAYOUT_FILLBOARD_ENABLED true
#define PLAYOUT_FILLBOARD_N 5
#define PLAYOUT_CIRCREPLACE_ENABLED false
#define PLAYOUT_FILLBOARD_BESTCIRC_ENABLED false
#define PLAYOUT_RANDOMQUICK_BESTCIRC_N 0
#define PLAYOUT_RANDOM_WEIGHT_TERRITORY_N 0
#define PLAYOUT_RANDOM_WEIGHT_TERRITORY_F 0.0
#define PLAYOUT_RANDOM_WEIGHT_TERRITORY_F0 0.0
#define PLAYOUT_RANDOM_WEIGHT_TERRITORY_F1 0.0
#define PLAYOUT_CIRCPATTERN_N 0
#define PLAYOUT_ANYCAPTURE_P 1.0
#define PLAYOUT_LGRF1_ENABLED true
#define PLAYOUT_LGRF_LOCAL false
#define PLAYOUT_LGRF1_SAFE_ENABLED false
#define PLAYOUT_AVOID_LBRF1_P 0.0
#define PLAYOUT_AVOID_LBMF_P 0.0
#define PLAYOUT_AVOID_LBRF1_P2 0.0
#define PLAYOUT_AVOID_LBMF_P2 0.0
#define PLAYOUT_LGRF1O_ENABLED true
#define PLAYOUT_LGRF2_ENABLED true
#define PLAYOUT_LGRF2_SAFE_ENABLED false
#define PLAYOUT_LGPF_ENABLED false
#define PLAYOUT_MERCY_RULE_ENABLED true
#define PLAYOUT_MERCY_RULE_FACTOR 0.40
#define PLAYOUT_RANDOM_CHANCE 0.00
#define PLAYOUT_RANDOM_APPROACH_P 0.00
#define PLAYOUT_LAST2LIBATARI_ENABLED true
#define PLAYOUT_LAST2LIBATARI_COMPLEX true
#define PLAYOUT_POOLRAVE_ENABLED false
#define PLAYOUT_POOLRAVE_CRITICALITY false
#define PLAYOUT_CRITICALITY_RANDOM_N 0
#define PLAYOUT_POOLRAVE_P 0.5
#define PLAYOUT_POOLRAVE_K 20
#define PLAYOUT_POOLRAVE_MIN_PLAYOUTS 50
#define PLAYOUT_AVOID_SELFATARI true
#define PLAYOUT_AVOID_SELFATARI_SIZE 5 // biggest killing shape is 6 stones
#define PLAYOUT_AVOID_SELFATARI_COMPLEX false // uses pseudoends to determine smaller not killing shapes
#define PLAYOUT_USELESS_MOVE false
#define PLAYOUT_ORDER 0 //numbers to test different playout orders
#define PLAYOUT_NEARBY_ENABLED false
#define PLAYOUT_FILL_WEAK_EYES false

#define PONDERING_ENABLED false
#define PONDERING_PLAYOUTS_MAX 1000000

#define TERRITORY_DECAYFACTOR 0.3
#define TERRITORY_THRESHOLD 0.6

#define THREAD_COUNT 1
#define MEMORY_USAGE_MAX (2*1024)

#define INTERRUPTS_ENABLED false

#define SUREWIN_THRESHOLD 0.90
#define SUREWIN_PASS_BONUS 1.00
#define SUREWIN_TOUCHDEAD_BONUS 0.50
#define SUREWIN_OPPOAREA_PENALTY 0.50

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
#define TIME_MOVE_MAX 3600.00

#define OUTPUTSGF_MAXCHILDREN 10

#define DEBUG_ON false

#define BOARDSIZE_MIN 2
#define BOARDSIZE_MAX 25

#define ZOBRIST_HASH_SEED 0x713df891

#define MPI_STRING_MAX 255
#define MPI_HASHTABLE_SIZE 65536
#define MPI_UPDATE_PERIOD 0.1
#define MPI_UPDATE_DEPTH 3
#define MPI_UPDATE_THRESHOLD 0.05

#include "config.h"
#include <string>
#include <list>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>
#ifdef HAVE_MPI
  #include <mpi.h>
#endif
#include "Go.h"
#include "Tree.h"
#include "Random.h"
//from "Pattern.h":
namespace Pattern
{
  class ThreeByThreeGammas;
  class ThreeByThreeTable;
  class Circular;
  class CircularDictionary;
};
#include "Parameters.h"
#include "Features.h"
#include "Time.h"
#include "Book.h"
#include "Playout.h"
#include "Benson.h"
#include "Worker.h"
//from "DecisionTree.h":
class DecisionTree;
#include "../gtp/Gtp.h"
#ifdef HAVE_WEB
  //from "../web/Web.h":
  class Web;
#endif


  

/** Core Engine. */
class Engine
{
  public:
    //contains the order of the statistics printed after the move
    //with usefull names
    enum StatNames
    {
      LASTATARI,    //must start with 0, should be standard conform
      LASTCAPTURE,
      LAST2LIBATARI,
      NAKED,
      PATTERN,
      ANYCAPTURE,
      CIRCPATTERN_QUICK,
      FILL_BOARD,
      RANDOM_QUICK,
      RANDOM,
      FILL_WEAK_EYE,
      PASS,
      REPLACE_WITH_CIRC,
      RANDOM_QUICK_CIRC,
      RANDOM_QUICK_TERRITORY,
      LGRF2,
      STATISTICS_NUM     //is set to the number of entries !!
    };

    /** Create an engine.
     * @param ge GTP engine to use.
     * @param ln The long name of the engine.
     */
    Engine(Gtp::Engine *ge, std::string ln);
    ~Engine();
    
    /** Run the engine.
     * @param web_inf If set, rather use the web interface.
     * @param web_addr The web address to bind to, if applicable.
     * @param web_port The web port to bind to, if applicable.
     */
    void run(bool web_inf, std::string web_addr, int web_port);
    /** Finish initialising the engine.
     * Should be called after any command line arguments are resolved.
     */
    void postCmdLineArgs(bool book_autoload);
    /** Generate a move using the current parameters. */
    void generateMove(Go::Color col, Go::Move **move, bool playmove);
    /** Determine if a certain move is legal. */
    bool isMoveAllowed(Go::Move move);
    /** Make a move on the current board. */
    void makeMove(Go::Move move);
    /** Get the current board size. */
    int getBoardSize() const { return currentboard->getSize(); };
    /** Set the current board size.
     * This will also clear the board.
     */
    void setBoardSize(int s);
    /** Get a pointer to the current board. */
    Go::Board *getCurrentBoard() const { return currentboard; };
    /** Clear the current board.
     * The current MCTS tree is also cleared.
     */
    void clearBoard();
    /** Get the current komi. */
    float getKomi() const { return komi; };
    float getScoreKomi() const;
    /** Set the current komi. */
    void setKomi(float k);
    void setHandicapKomi(float k) {komi_handicap=k;};
    /** Undo the last move made.
     * Return true if successful.
     */
    bool undo();
    /** Get the 3x3 pattern table in use. */
    Pattern::ThreeByThreeTable *getPatternTable() const { return patterntable; };
    /** Get the features in use. */
    Features *getFeatures() const { return features; };
    /** Get the decision trees in use. */
    std::list<DecisionTree*> *getDecisionTrees() { return &decisiontrees; };
    /** Get the Zobrist table is use. */
    Go::ZobristTable *getZobristTable() const { return zobristtable; };
    /** Get the hash tree of Zobrist hashes that have occured in this game. */
    Go::ZobristTree *getZobristHashTree() const { return hashtree; };
    /** Get the GTP engine. */
    Gtp::Engine *getGtpEngine() const { return gtpe; };
    /** Get the engine parameters. */
    Parameters *getParams() const { return params; };
    /** Stop any thinking, if applicable. */
    void stopThinking() { stopthinking=true; };
    /** Do work for a thread. */
    void doThreadWork(Worker::Settings *settings);
    /** Output the current engine state to a SGF file. */
    bool writeSGF(std::string filename, Go::Board *board=NULL, Tree *tree=NULL);
    /** Output a playout from the current position to a SGF file. */
    bool writeSGF(std::string filename, Go::Board *board, std::list<Go::Move> playoutmoves, std::list<std::string> *movereasons=NULL);
    /** Output the current game to a SGF file. */
    bool writeGameSGF(std::string filename);

    float valueCircPattern(std::string circpattern) {return features->valueCircPattern(circpattern);}

    bool isCircPattern(std::string circpattern) {return features->isCircPattern(circpattern);}

    Pattern::CircularDictionary *getCircDict() {return features->getCircDict();}
    int getCircSize() {return features->getCircSize();}
    void statisticsPlus(StatNames i) {statistics[i]++;}
    void clearStatistics() {int i; for (i=0;i<STATISTICS_NUM;i++) statistics[i]=0;}
    long statisticsSum() {int i; long sum=0; for (i=0;i<STATISTICS_NUM;i++) sum+=statistics[i]; return sum;}
    long getStatistics(int i) {return statistics[i]*1000/(statisticsSum()+1);} //+1 avoid crash
    Go::TerritoryMap *getTerritoryMap() const {return territorymap;}
    float getOldMoveValue(Go::Move m);
    void getOnePlayoutMove(Go::Board *board, Go::Color col, Go::Move *move);

    void addpresetplayout(float p) {presetplayouts+=p; presetnum++;}
  private:
    Gtp::Engine *gtpe;
    std::string longname;
    Go::Board *currentboard;
    float komi;
    float komi_handicap;
    int boardsize;
    Time *time;
    Tree *movetree;
    Pattern::ThreeByThreeTable *patterntable;
    Parameters *const params;
    Features *features;
    Book *book;
    std::list<Go::Move> *movehistory;
    std::list<std::string> *moveexplanations;
    Go::ZobristTable *zobristtable;
    Go::ZobristTree *hashtree;
    Playout *playout;
    volatile bool stopthinking;
    volatile bool stoppondering;
    Worker::Pool *threadpool;
    Go::TerritoryMap *territorymap;
    long statistics[STATISTICS_NUM];

    bool isgamefinished;
    std::list<DecisionTree*> decisiontrees;

    //This holds the values of moves, calculated earlier
    //If a move is done the not used moves are here
    float *blackOldMoves;
    float *whiteOldMoves;
    int blackOldMovesNum;
    int whiteOldMovesNum;
    float blackOldMean,whiteOldMean;

    float presetplayouts;
    int presetnum;

    std::string learn_filename_features,learn_filename_circ_patterns;
    
    enum MovePolicy
    {
      MP_PLAYOUT,
      MP_ONEPLY,
      MP_UCT
    };
    
    #ifdef HAVE_WEB
      Web *web;
    #endif
    
    #ifdef HAVE_MPI
      int mpiworldsize,mpirank;
      bool mpisynced;
      
      enum MPICommand
      {
        MPICMD_QUIT,
        MPICMD_MAKEMOVE,
        MPICMD_SETBOARDSIZE,
        MPICMD_SETKOMI,
        MPICMD_CLEARBOARD,
        MPICMD_SETPARAM,
        MPICMD_TIMESETTINGS,
        MPICMD_TIMELEFT,
        MPICMD_LOADPATTERNS,
        MPICMD_CLEARPATTERNS,
        MPICMD_LOADFEATUREGAMMAS,
        MPICMD_BOOKADD,
        MPICMD_BOOKREMOVE,
        MPICMD_BOOKCLEAR,
        MPICMD_BOOKLOAD,
        MPICMD_CLEARTREE,
        MPICMD_GENMOVE
      };
      
      class MpiHashTable
      {
        public:
          void clear();
          std::list<Tree*> *lookup(Go::ZobristHash hash);
          void add(Go::ZobristHash hash, Tree *node);
        
        private:
          class TableEntry
          {
            public:
              Go::ZobristHash hash;
              std::list<Tree*> nodes;
          };
          
          std::list<Engine::MpiHashTable::TableEntry> table[MPI_HASHTABLE_SIZE];
          
          Engine::MpiHashTable::TableEntry *lookupEntry(Go::ZobristHash hash);
      };
      
      Engine::MpiHashTable mpihashtable;
      
      typedef struct
      {
        Go::ZobristHash hash;
        Go::ZobristHash parenthash;
        float playouts;
        float wins;
      } mpistruct_updatemsg;
      MPI::Datatype mpitype_updatemsg;
      
      void mpiCommandHandler();
      void mpiBroadcastCommand(Engine::MPICommand cmd, unsigned int *arg1=NULL, unsigned int *arg2=NULL, unsigned int *arg3=NULL);
      void mpiBroadcastString(std::string input);
      void mpiRecvBroadcastedArgs(unsigned int *arg1=NULL, unsigned int *arg2=NULL, unsigned int *arg3=NULL);
      std::string mpiRecvBroadcastedString();
      void mpiSendString(int destrank, std::string input);
      std::string mpiRecvString(int srcrank);
      void mpiGenMove(Go::Color col);
      bool mpiSyncUpdate(bool stop=false);
      void mpiBuildDerivedTypes();
      void mpiFreeDerivedTypes();
      void mpiFillList(std::list<mpistruct_updatemsg> &list, float threshold, int depthleft, Tree *tree);
    #endif
    
    void addGtpCommands();
    
    void clearMoveTree();
    void chooseSubTree(Go::Move move);
    
    void doNPlayouts(int n);
    void doPlayout(Worker::Settings *settings, Go::BitBoard *firstlist, Go::BitBoard *secondlist, Go::BitBoard *earlyfirstlist, Go::BitBoard *earlysecondlist);
    void displayPlayoutLiveGfx(int totalplayouts=-1, bool livegfx=true);
    void doSlowUpdate();
    
    void allowContinuedPlay();
    void updateTerritoryScoringInTree();
    
    boost::posix_time::ptime timeNow() { return boost::posix_time::microsec_clock::local_time(); };
    float timeSince(boost::posix_time::ptime past) { return (float)(boost::posix_time::microsec_clock::local_time()-past).total_milliseconds()/1000; };

    unsigned long getTreeMemoryUsage() { if (params->tree_instances>0) {return (unsigned long)(params->tree_instances)*sizeof(Tree);} else {return 0;}};

    void generateThread(Worker::Settings *settings);
    void ponderThread(Worker::Settings *settings);
    void doNPlayoutsThread(Worker::Settings *settings);

    std::string chat(bool pm,std::string name,std::string msg);

    void gameFinished();
    void learnFromTree(Go::Board *tmpboard, Tree *learntree, std::ostringstream *ssun, int move_num);
    
    static void ponderWrapper(void *instance) { ((Engine*)instance)->ponder(); };
    void ponder();

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
    static void gtpGenMoveCleanup(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpRegGenMove(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpShowBoard(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpFinalScore(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpFinalStatusList(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpUndo(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpChat(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpGameOver(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpEcho(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpPlaceFreeHandicap(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpSetFreeHandicap(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    
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
    static void gtpSaveFeatureGammas(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpSaveFeatureGammasInline(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpLoadCircPatterns(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpLoadCircPatternsNot(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpSaveCircPatternValues(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpLoadCircPatternValues(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpListFeatureIds(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpShowCFGFrom(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpShowCircDistFrom(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpListCircularPatternsAt(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpListCircularPatternsAtSize(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpListCircularPatternsAtSizeNot(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpListAllCircularPatterns(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpListAdjacentGroupsOf(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    
    static void gtpTimeSettings(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpTimeLeft(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    
    static void gtpDoNPlayouts(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);

    static void gtpOutputSGF(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpPlayoutSGF(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpPlayoutSGF_pos(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpGameSGF(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);

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

    static void gtpDTLoad(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpDTClear(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpDTPrint(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpDTAt(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpDTUpdate(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpDTSave(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpDTSet(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpDTDistribution(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpDTStats(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpDTPath(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    
    static void gtpShowCurrentHash(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpShowSafePositions(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpDoBenchmark(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpShowCriticality(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpShowTerritory(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpShowRatios(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpShowUnPrune(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpShowUnPruneColor(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpShowRAVERatios(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpShowRAVERatiosColor(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
    static void gtpShowRAVERatiosOther(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
};

#endif
