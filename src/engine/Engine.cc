#include "Engine.h"

#include <cstdlib>
#include <cmath>
#include <vector>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <boost/timer.hpp>

Engine::Engine(Gtp::Engine *ge, std::string ln)
{
  gtpe=ge;
  longname=ln;
  
  unsigned long seed=std::time(0);
  rand=Random(seed);
  
  params=new Parameters();
  
  params->engine=this;
  
  boardsize=9;
  params->board_size=boardsize;
  currentboard=new Go::Board(boardsize);
  komi=5.5;
  
  zobristtable=new Go::ZobristTable(params,boardsize);
  
  params->addParameter("other","rand_seed",&(params->rand_seed),seed,&Engine::updateParameterWrapper,this);
  
  std::list<std::string> *mpoptions=new std::list<std::string>();
  mpoptions->push_back("playout");
  mpoptions->push_back("1-ply");
  mpoptions->push_back("uct");
  params->addParameter("mcts","move_policy",&(params->move_policy_string),mpoptions,"uct",&Engine::updateParameterWrapper,this);
  params->move_policy=Parameters::MP_UCT;
  
  params->addParameter("mcts","book_use",&(params->book_use),BOOK_USE);
  
  params->addParameter("mcts","playouts_per_move",&(params->playouts_per_move),PLAYOUTS_PER_MOVE);
  params->addParameter("mcts","playouts_per_move_max",&(params->playouts_per_move_max),PLAYOUTS_PER_MOVE_MAX);
  params->addParameter("mcts","playouts_per_move_min",&(params->playouts_per_move_min),PLAYOUTS_PER_MOVE_MIN);
  
  params->addParameter("mcts","playout_lgrf2_enabled",&(params->playout_lgrf2_enabled),PLAYOUT_LGRF2_ENABLED);
  params->addParameter("mcts","playout_lgrf1_enabled",&(params->playout_lgrf1_enabled),PLAYOUT_LGRF1_ENABLED);
  params->addParameter("mcts","playout_atari_enabled",&(params->playout_atari_enabled),PLAYOUT_ATARI_ENABLED);
  params->addParameter("mcts","playout_lastatari_enabled",&(params->playout_lastatari_enabled),PLAYOUT_LASTATARI_ENABLED);
  params->addParameter("mcts","playout_lastcapture_enabled",&(params->playout_lastcapture_enabled),PLAYOUT_LASTCAPTURE_ENABLED);
  params->addParameter("mcts","playout_nakade_enabled",&(params->playout_nakade_enabled),PLAYOUT_NAKADE_ENABLED);
  params->addParameter("mcts","playout_fillboard_enabled",&(params->playout_fillboard_enabled),PLAYOUT_FILLBOARD_ENABLED);
  params->addParameter("mcts","playout_fillboard_n",&(params->playout_fillboard_n),PLAYOUT_FILLBOARD_N);
  params->addParameter("mcts","playout_patterns_enabled",&(params->playout_patterns_enabled),PLAYOUT_PATTERNS_ENABLED);
  params->addParameter("mcts","playout_anycapture_enabled",&(params->playout_anycapture_enabled),PLAYOUT_ANYCAPTURE_ENABLED);
  params->addParameter("mcts","playout_features_enabled",&(params->playout_features_enabled),PLAYOUT_FEATURES_ENABLED);
  params->addParameter("mcts","playout_features_incremental",&(params->playout_features_incremental),PLAYOUT_FEATURES_INCREMENTAL);
  params->addParameter("mcts","playout_random_chance",&(params->playout_random_chance),PLAYOUT_RANDOM_CHANCE);
  params->addParameter("mcts","playout_mercy_rule_enabled",&(params->playout_mercy_rule_enabled),PLAYOUT_MERCY_RULE_ENABLED);
  params->addParameter("mcts","playout_mercy_rule_factor",&(params->playout_mercy_rule_factor),PLAYOUT_MERCY_RULE_FACTOR);
  
  params->addParameter("mcts","ucb_c",&(params->ucb_c),UCB_C);
  params->addParameter("mcts","ucb_init",&(params->ucb_init),UCB_INIT);
  
  params->addParameter("mcts","rave_moves",&(params->rave_moves),RAVE_MOVES);
  params->addParameter("mcts","rave_init_wins",&(params->rave_init_wins),RAVE_INIT_WINS);
  
  params->addParameter("mcts","uct_expand_after",&(params->uct_expand_after),UCT_EXPAND_AFTER);
  params->addParameter("mcts","uct_keep_subtree",&(params->uct_keep_subtree),UCT_KEEP_SUBTREE,&Engine::updateParameterWrapper,this);
  params->addParameter("mcts","uct_symmetry_use",&(params->uct_symmetry_use),UCT_SYMMETRY_USE,&Engine::updateParameterWrapper,this);
  if (params->uct_symmetry_use)
    currentboard->turnSymmetryOn();
  else
    currentboard->turnSymmetryOff();
  params->addParameter("mcts","uct_atari_prior",&(params->uct_atari_prior),UCT_ATARI_PRIOR);
  params->addParameter("mcts","uct_pattern_prior",&(params->uct_pattern_prior),UCT_PATTERN_PRIOR);
  params->addParameter("mcts","uct_progressive_widening_enabled",&(params->uct_progressive_widening_enabled),UCT_PROGRESSIVE_WIDENING_ENABLED);
  params->addParameter("mcts","uct_progressive_widening_a",&(params->uct_progressive_widening_a),UCT_PROGRESSIVE_WIDENING_A);
  params->addParameter("mcts","uct_progressive_widening_b",&(params->uct_progressive_widening_b),UCT_PROGRESSIVE_WIDENING_B);
  params->addParameter("mcts","uct_progressive_widening_c",&(params->uct_progressive_widening_c),UCT_PROGRESSIVE_WIDENING_C);
  params->addParameter("mcts","uct_progressive_widening_count_wins",&(params->uct_progressive_widening_count_wins),UCT_PROGRESSIVE_WIDENING_COUNT_WINS);
  params->addParameter("mcts","uct_points_bonus",&(params->uct_points_bonus),UCT_POINTS_BONUS);
  params->addParameter("mcts","uct_length_bonus",&(params->uct_length_bonus),UCT_LENGTH_BONUS);
  params->addParameter("mcts","uct_progressive_bias_enabled",&(params->uct_progressive_bias_enabled),UCT_PROGRESSIVE_BIAS_ENABLED);
  params->addParameter("mcts","uct_progressive_bias_h",&(params->uct_progressive_bias_h),UCT_PROGRESSIVE_BIAS_H);
  params->addParameter("mcts","uct_progressive_bias_scaled",&(params->uct_progressive_bias_scaled),UCT_PROGRESSIVE_BIAS_SCALED);
  params->addParameter("mcts","uct_progressive_bias_relative",&(params->uct_progressive_bias_relative),UCT_PROGRESSIVE_BIAS_RELATIVE);
  
  params->addParameter("mcts","uct_slow_update_interval",&(params->uct_slow_update_interval),UCT_SLOW_UPDATE_INTERVAL);
  params->uct_slow_update_last=0;
  params->addParameter("mcts","uct_stop_early",&(params->uct_stop_early),UCT_STOP_EARLY);
  
  params->addParameter("mcts","surewin_threshold",&(params->surewin_threshold),SUREWIN_THRESHOLD);
  params->surewin_expected=false;
  
  params->addParameter("mcts","resign_ratio_threshold",&(params->resign_ratio_threshold),RESIGN_RATIO_THRESHOLD);
  params->addParameter("mcts","resign_move_factor_threshold",&(params->resign_move_factor_threshold),RESIGN_MOVE_FACTOR_THRESHOLD);
  
  params->addParameter("mcts","rules_positional_superko_enabled",&(params->rules_positional_superko_enabled),RULES_POSITIONAL_SUPERKO_ENABLED);
  params->addParameter("mcts","rules_superko_top_ply",&(params->rules_superko_top_ply),RULES_SUPERKO_TOP_PLY);
  
  params->addParameter("time","time_k",&(params->time_k),TIME_K);
  params->addParameter("time","time_buffer",&(params->time_buffer),TIME_BUFFER);
  params->addParameter("time","time_move_minimum",&(params->time_move_minimum),TIME_MOVE_MINIMUM);
  params->addParameter("time","time_ignore",&(params->time_ignore),false);
  
  params->addParameter("time","pondering_enabled",&(params->pondering_enabled),PONDERING_ENABLED,&Engine::updateParameterWrapper,this);
  params->addParameter("time","pondering_playouts_max",&(params->pondering_playouts_max),PONDERING_PLAYOUTS_MAX);
  
  params->addParameter("other","live_gfx",&(params->livegfx_on),LIVEGFX_ON);
  params->addParameter("other","live_gfx_update_playouts",&(params->livegfx_update_playouts),LIVEGFX_UPDATE_PLAYOUTS);
  params->addParameter("other","live_gfx_delay",&(params->livegfx_delay),LIVEGFX_DELAY);
  
  params->addParameter("other","outputsgf_maxchildren",&(params->outputsgf_maxchildren),OUTPUTSGF_MAXCHILDREN);
  
  params->addParameter("other","debug",&(params->debug_on),DEBUG_ON);
  
  params->addParameter("other","interrupts_enabled",&(params->interrupts_enabled),INTERRUPTS_ENABLED,&Engine::updateParameterWrapper,this);
  
  params->addParameter("other","features_only_small",&(params->features_only_small),false);
  params->addParameter("other","features_output_competitions",&(params->features_output_competitions),false);
  params->addParameter("other","features_output_competitions_mmstyle",&(params->features_output_competitions_mmstyle),false);
  params->addParameter("other","features_ordered_comparison",&(params->features_ordered_comparison),false);
  
  patterntable=new Pattern::ThreeByThreeTable();
  patterntable->loadPatternDefaults();
  
  features=new Features(params);
  features->loadGammaDefaults();
  
  circdict=new Pattern::CircularDictionary();
  
  book=new Book(params);
  
  time=new Time(params,0);
  
  playout=new Playout(params);
  
  lastexplanation="";
  
  movehistory=new std::list<Go::Move>();
  hashtree=new Go::ZobristTree();
  
  this->addGtpCommands();
  
  movetree=NULL;
  this->clearMoveTree();
  
  params->uct_last_r2=0;
}

Engine::~Engine()
{
  delete circdict;
  delete features;
  delete patterntable;
  if (movetree!=NULL)
    delete movetree;
  delete currentboard;
  delete movehistory;
  delete hashtree;
  delete params;
  delete time;
  delete book;
  delete zobristtable;
  delete playout;
}

void Engine::postCmdLineArgs(bool book_autoload)
{
  gtpe->getOutput()->printfDebug("seed: %lu\n",params->rand_seed);
  if (book_autoload)
  {
    bool loaded=false;
    if (!loaded)
    {
      std::string filename="book.dat";
      gtpe->getOutput()->printfDebug("loading opening book from '%s'... ",filename.c_str());
      loaded=book->loadFile(filename);
      if (loaded)
        gtpe->getOutput()->printfDebug("done\n");
      else
        gtpe->getOutput()->printfDebug("error\n");
    }
    #ifdef TOPSRCDIR
      if (!loaded)
      {
        std::string filename=TOPSRCDIR "/book.dat";
        gtpe->getOutput()->printfDebug("loading opening book from '%s'... ",filename.c_str());
        loaded=book->loadFile(filename);
        if (loaded)
          gtpe->getOutput()->printfDebug("done\n");
        else
          gtpe->getOutput()->printfDebug("error\n");
      }
    #endif
    
    if (!loaded)
      gtpe->getOutput()->printfDebug("no opening book loaded\n");
  }
}

void Engine::updateParameter(std::string id)
{
  if (id=="move_policy")
  {
    if (params->move_policy_string=="uct")
      params->move_policy=Parameters::MP_UCT;
    else if (params->move_policy_string=="1-ply")
      params->move_policy=Parameters::MP_ONEPLY;
    else
      params->move_policy=Parameters::MP_PLAYOUT;
  }
  else if (id=="uct_keep_subtree")
  {
    this->clearMoveTree();
  }
  else if (id=="uct_symmetry_use")
  {
    if (params->uct_symmetry_use)
      currentboard->turnSymmetryOn();
    else
    {
      currentboard->turnSymmetryOff();
      this->clearMoveTree();
    }
  }
  else if (id=="rand_seed")
  {
    if (params->rand_seed==0)
      params->rand_seed=std::time(0);
    rand=Random(params->rand_seed);
  }
  else if (id=="interrupts_enabled")
  {
    gtpe->setWorkerEnabled(params->interrupts_enabled);
  }
  else if (id=="pondering_enabled")
  {
    gtpe->setPonderEnabled(params->pondering_enabled);
  }
}

void Engine::addGtpCommands()
{
  gtpe->addFunctionCommand("boardsize",this,&Engine::gtpBoardSize);
  gtpe->addFunctionCommand("clear_board",this,&Engine::gtpClearBoard);
  gtpe->addFunctionCommand("komi",this,&Engine::gtpKomi);
  gtpe->addFunctionCommand("play",this,&Engine::gtpPlay);
  gtpe->addFunctionCommand("genmove",this,&Engine::gtpGenMove);
  gtpe->addFunctionCommand("reg_genmove",this,&Engine::gtpRegGenMove);
  gtpe->addFunctionCommand("kgs-genmove_cleanup",this,&Engine::gtpGenMove);
  gtpe->addFunctionCommand("showboard",this,&Engine::gtpShowBoard);
  gtpe->addFunctionCommand("final_score",this,&Engine::gtpFinalScore);
  gtpe->addFunctionCommand("final_status_list",this,&Engine::gtpFinalStatusList);
  
  gtpe->addFunctionCommand("param",this,&Engine::gtpParam);
  gtpe->addFunctionCommand("showliberties",this,&Engine::gtpShowLiberties);
  gtpe->addFunctionCommand("showvalidmoves",this,&Engine::gtpShowValidMoves);
  gtpe->addFunctionCommand("showgroupsize",this,&Engine::gtpShowGroupSize);
  gtpe->addFunctionCommand("showpatternmatches",this,&Engine::gtpShowPatternMatches);
  gtpe->addFunctionCommand("loadpatterns",this,&Engine::gtpLoadPatterns);
  gtpe->addFunctionCommand("clearpatterns",this,&Engine::gtpClearPatterns);
  gtpe->addFunctionCommand("doboardcopy",this,&Engine::gtpDoBoardCopy);
  gtpe->addFunctionCommand("featurematchesat",this,&Engine::gtpFeatureMatchesAt);
  gtpe->addFunctionCommand("featureprobdistribution",this,&Engine::gtpFeatureProbDistribution);
  gtpe->addFunctionCommand("listallpatterns",this,&Engine::gtpListAllPatterns);
  gtpe->addFunctionCommand("loadfeaturegammas",this,&Engine::gtpLoadFeatureGammas);
  gtpe->addFunctionCommand("listfeatureids",this,&Engine::gtpListFeatureIds);
  gtpe->addFunctionCommand("showcfgfrom",this,&Engine::gtpShowCFGFrom);
  gtpe->addFunctionCommand("showcircdistfrom",this,&Engine::gtpShowCircDistFrom);
  gtpe->addFunctionCommand("listcircpatternsat",this,&Engine::gtpListCircularPatternsAt);
  gtpe->addFunctionCommand("listallcircularpatterns",this,&Engine::gtpListAllCircularPatterns);
  gtpe->addFunctionCommand("listadjacentgroupsof",this,&Engine::gtpListAdjacentGroupsOf);
  
  gtpe->addFunctionCommand("time_settings",this,&Engine::gtpTimeSettings);
  gtpe->addFunctionCommand("time_left",this,&Engine::gtpTimeLeft);
  
  gtpe->addFunctionCommand("donplayouts",this,&Engine::gtpDoNPlayouts);
  gtpe->addFunctionCommand("outputsgf",this,&Engine::gtpOutputSGF);
  
  gtpe->addFunctionCommand("explainlastmove",this,&Engine::gtpExplainLastMove);
  gtpe->addFunctionCommand("boardstats",this,&Engine::gtpBoardStats);
  gtpe->addFunctionCommand("showsymmetrytransforms",this,&Engine::gtpShowSymmetryTransforms);
  gtpe->addFunctionCommand("shownakadecenters",this,&Engine::gtpShowNakadeCenters);
  gtpe->addFunctionCommand("showtreelivegfx",this,&Engine::gtpShowTreeLiveGfx);
  gtpe->addFunctionCommand("describeengine",this,&Engine::gtpDescribeEngine);
  
  gtpe->addFunctionCommand("bookshow",this,&Engine::gtpBookShow);
  gtpe->addFunctionCommand("bookadd",this,&Engine::gtpBookAdd);
  gtpe->addFunctionCommand("bookremove",this,&Engine::gtpBookRemove);
  gtpe->addFunctionCommand("bookclear",this,&Engine::gtpBookClear);
  gtpe->addFunctionCommand("bookload",this,&Engine::gtpBookLoad);
  gtpe->addFunctionCommand("booksave",this,&Engine::gtpBookSave);
  
  gtpe->addFunctionCommand("showcurrenthash",this,&Engine::gtpShowCurrentHash);
  gtpe->addFunctionCommand("showsafepositions",this,&Engine::gtpShowSafePositions);
  
  //gtpe->addAnalyzeCommand("final_score","Final Score","string");
  //gtpe->addAnalyzeCommand("showboard","Show Board","string");
  gtpe->addAnalyzeCommand("boardstats","Board Stats","string");
  
  gtpe->addAnalyzeCommand("showsymmetrytransforms","Show Symmetry Transforms","sboard");
  //gtpe->addAnalyzeCommand("showliberties","Show Liberties","sboard");
  //gtpe->addAnalyzeCommand("showvalidmoves","Show Valid Moves","sboard");
  //gtpe->addAnalyzeCommand("showgroupsize","Show Group Size","sboard");
  gtpe->addAnalyzeCommand("showcfgfrom %%p","Show CFG From","sboard");
  gtpe->addAnalyzeCommand("showcircdistfrom %%p","Show Circular Distance From","sboard");
  gtpe->addAnalyzeCommand("listcircpatternsat %%p","List Circular Patterns At","string");
  gtpe->addAnalyzeCommand("listadjacentgroupsof %%p","List Adjacent Groups Of","string");
  gtpe->addAnalyzeCommand("showsafepositions","Show Safe Positions","gfx");
  gtpe->addAnalyzeCommand("showpatternmatches","Show Pattern Matches","sboard");
  //gtpe->addAnalyzeCommand("shownakadecenters","Show Nakade Centers","sboard");
  gtpe->addAnalyzeCommand("featurematchesat %%p","Feature Matches At","string");
  gtpe->addAnalyzeCommand("featureprobdistribution","Feature Probability Distribution","cboard");
  gtpe->addAnalyzeCommand("loadfeaturegammas %%r","Load Feature Gammas","none");
  gtpe->addAnalyzeCommand("showtreelivegfx","Show Tree Live Gfx","gfx");
  //gtpe->addAnalyzeCommand("loadpatterns %%r","Load Patterns","none");
  //gtpe->addAnalyzeCommand("clearpatterns","Clear Patterns","none");
  gtpe->addAnalyzeCommand("doboardcopy","Do Board Copy","none");
  gtpe->addAnalyzeCommand("showcurrenthash","Show Current Hash","string");
  
  gtpe->addAnalyzeCommand("param mcts","Parameters (MCTS)","param");
  gtpe->addAnalyzeCommand("param time","Parameters (Time)","param");
  gtpe->addAnalyzeCommand("param other","Parameters (Other)","param");
  gtpe->addAnalyzeCommand("donplayouts %%s","Do N Playouts","none");
  gtpe->addAnalyzeCommand("donplayouts 1","Do 1 Playout","none");
  gtpe->addAnalyzeCommand("donplayouts 100","Do 100 Playouts","none");
  gtpe->addAnalyzeCommand("donplayouts 1000","Do 1k Playouts","none");
  gtpe->addAnalyzeCommand("donplayouts 10000","Do 10k Playouts","none");
  gtpe->addAnalyzeCommand("donplayouts 100000","Do 100k Playouts","none");
  gtpe->addAnalyzeCommand("outputsgf %%w","Output SGF","none");
  
  gtpe->addAnalyzeCommand("bookshow","Book Show","gfx");
  gtpe->addAnalyzeCommand("bookadd %%p","Book Add","none");
  gtpe->addAnalyzeCommand("bookremove %%p","Book Remove","none");
  gtpe->addAnalyzeCommand("bookclear","Book Clear","none");
  gtpe->addAnalyzeCommand("bookload %%r","Book Load","none");
  gtpe->addAnalyzeCommand("booksave %%w","Book Save","none");
  
  gtpe->setInterruptFlag(&stopthinking);
  gtpe->setPonderer(&Engine::ponderWrapper,this,&stoppondering);
  gtpe->setWorkerEnabled(params->interrupts_enabled);
  gtpe->setPonderEnabled(params->pondering_enabled);
}

void Engine::gtpBoardSize(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  if (cmd->numArgs()!=1)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("size is required");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  int newsize=cmd->getIntArg(0);
  
  if (newsize<BOARDSIZE_MIN || newsize>BOARDSIZE_MAX)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("unacceptable size");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  me->setBoardSize(newsize);
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->endResponse();
}

void Engine::gtpClearBoard(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  me->clearBoard();
  
  //assume that this will be the start of a new game
  me->time->resetAll();
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->endResponse();
}

void Engine::gtpKomi(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  if (cmd->numArgs()!=1)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("komi is required");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  me->setKomi(cmd->getFloatArg(0));
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->endResponse();
}

void Engine::gtpPlay(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  if (cmd->numArgs()!=2)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("move is required");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  Gtp::Color gtpcol = cmd->getColorArg(0);
  Gtp::Vertex vert = cmd->getVertexArg(1);
  
  if (gtpcol==Gtp::INVALID || (vert.x==-3 && vert.y==-3))
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("invalid move");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  Go::Move move=Go::Move((gtpcol==Gtp::BLACK ? Go::BLACK : Go::WHITE),vert.x,vert.y,me->boardsize);
  if (!me->isMoveAllowed(move))
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("illegal move");
    gtpe->getOutput()->endResponse();
    return;
  }
  me->makeMove(move);
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->endResponse();
}

void Engine::gtpGenMove(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  if (cmd->numArgs()!=1)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("color is required");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  Gtp::Color gtpcol = cmd->getColorArg(0);
  
  if (gtpcol==Gtp::INVALID)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("invalid color");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  Go::Move *move;
  me->generateMove((gtpcol==Gtp::BLACK ? Go::BLACK : Go::WHITE),&move,true);
  Gtp::Vertex vert={move->getX(me->boardsize),move->getY(me->boardsize)};
  delete move;
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printVertex(vert);
  gtpe->getOutput()->endResponse();
}

void Engine::gtpRegGenMove(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  if (cmd->numArgs()!=1)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("color is required");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  Gtp::Color gtpcol = cmd->getColorArg(0);
  
  if (gtpcol==Gtp::INVALID)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("invalid color");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  Go::Move *move;
  me->generateMove((gtpcol==Gtp::BLACK ? Go::BLACK : Go::WHITE),&move,false);
  Gtp::Vertex vert={move->getX(me->boardsize),move->getY(me->boardsize)};
  delete move;
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printVertex(vert);
  gtpe->getOutput()->endResponse();
}

void Engine::gtpShowBoard(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printString("\n");
  gtpe->getOutput()->printString(me->currentboard->toString());
  gtpe->getOutput()->endResponse(true);
}

void Engine::gtpFinalScore(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  float score;
  
  score=me->currentboard->score()-me->komi;
  
  gtpe->getOutput()->startResponse(cmd);
  if (score==0) // jigo
    gtpe->getOutput()->printf("0");
  else
    gtpe->getOutput()->printScore(score);
  gtpe->getOutput()->endResponse();
}

void Engine::gtpFinalStatusList(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  if (cmd->numArgs()!=1)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("argument is required");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  std::string arg = cmd->getStringArg(0);
  std::transform(arg.begin(),arg.end(),arg.begin(),::tolower);
  
  if (arg=="dead")
  {
    gtpe->getOutput()->startResponse(cmd);
    gtpe->getOutput()->endResponse();
  }
  else if (arg=="alive")
  {
    gtpe->getOutput()->startResponse(cmd);
    for (int x=0;x<me->boardsize;x++)
    {
      for (int y=0;y<me->boardsize;y++)
      {
        if (me->currentboard->boardData()[Go::Position::xy2pos(x,y,me->boardsize)].color!=Go::EMPTY)
        {
          Gtp::Vertex vert={x,y};
          gtpe->getOutput()->printVertex(vert);
          gtpe->getOutput()->printf(" ");
        }
      }
    }
    gtpe->getOutput()->endResponse();
  }
  else
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("argument is not supported");
    gtpe->getOutput()->endResponse();
  }
}

void Engine::gtpShowLiberties(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printString("\n");
  for (int y=me->boardsize-1;y>=0;y--)
  {
    for (int x=0;x<me->boardsize;x++)
    {
      int pos=Go::Position::xy2pos(x,y,me->boardsize);
      if (me->currentboard->boardData()[pos].group==NULL)
        gtpe->getOutput()->printf("\"\" ");
      else
      {
        int lib=me->currentboard->boardData()[pos].group->find()->numOfPseudoLiberties();
        gtpe->getOutput()->printf("\"%d",lib);
        if (me->currentboard->boardData()[pos].group->find()->inAtari())
          gtpe->getOutput()->printf("!");
        gtpe->getOutput()->printf("\" ");
      }
    }
    gtpe->getOutput()->printf("\n");
  }

  gtpe->getOutput()->endResponse(true);
}

void Engine::gtpShowValidMoves(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printString("\n");
  for (int y=me->boardsize-1;y>=0;y--)
  {
    for (int x=0;x<me->boardsize;x++)
    {
      gtpe->getOutput()->printf("\"");
      if (me->currentboard->validMove(Go::Move(Go::BLACK,Go::Position::xy2pos(x,y,me->boardsize))))
        gtpe->getOutput()->printf("B");
      if (me->currentboard->validMove(Go::Move(Go::WHITE,Go::Position::xy2pos(x,y,me->boardsize))))
        gtpe->getOutput()->printf("W");
      gtpe->getOutput()->printf("\" ");
    }
    gtpe->getOutput()->printf("\n");
  }

  gtpe->getOutput()->endResponse(true);
}

void Engine::gtpShowGroupSize(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printString("\n");
  for (int y=me->boardsize-1;y>=0;y--)
  {
    for (int x=0;x<me->boardsize;x++)
    {
      int pos=Go::Position::xy2pos(x,y,me->boardsize);
      if (me->currentboard->boardData()[pos].group==NULL)
        gtpe->getOutput()->printf("\"\" ");
      else
      {
        int stones=me->currentboard->boardData()[pos].group->find()->numOfStones();
        gtpe->getOutput()->printf("\"%d\" ",stones);
      }
    }
    gtpe->getOutput()->printf("\n");
  }

  gtpe->getOutput()->endResponse(true);
}

void Engine::gtpShowPatternMatches(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printString("\n");
  for (int y=me->boardsize-1;y>=0;y--)
  {
    for (int x=0;x<me->boardsize;x++)
    {
      int pos=Go::Position::xy2pos(x,y,me->boardsize);
      gtpe->getOutput()->printf("\"");
      if (me->currentboard->validMove(Go::Move(Go::BLACK,pos)) && me->patterntable->isPattern(Pattern::ThreeByThree::makeHash(me->currentboard,pos)))
        gtpe->getOutput()->printf("B");
      if (me->currentboard->validMove(Go::Move(Go::WHITE,pos)) && me->patterntable->isPattern(Pattern::ThreeByThree::invert(Pattern::ThreeByThree::makeHash(me->currentboard,pos))))
        gtpe->getOutput()->printf("W");
      gtpe->getOutput()->printf("\" ");
    }
    gtpe->getOutput()->printf("\n");
  }

  gtpe->getOutput()->endResponse(true);
}

void Engine::gtpLoadPatterns(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  if (cmd->numArgs()!=1)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printf("need 1 arg");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  std::string patternfile=cmd->getStringArg(0);
  
  delete me->patterntable;
  me->patterntable=new Pattern::ThreeByThreeTable();
  bool success=me->patterntable->loadPatternFile(patternfile);
  
  if (success)
  {
    gtpe->getOutput()->startResponse(cmd);
    gtpe->getOutput()->printf("loaded pattern file: %s",patternfile.c_str());
    gtpe->getOutput()->endResponse();
  }
  else
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printf("error loading pattern file: %s",patternfile.c_str());
    gtpe->getOutput()->endResponse();
  }
}

void Engine::gtpClearPatterns(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  delete me->patterntable;
  me->patterntable=new Pattern::ThreeByThreeTable();
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->endResponse();
}

void Engine::gtpDoBoardCopy(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  Go::Board *oldboard=me->currentboard;
  Go::Board *newboard=me->currentboard->copy();
  delete oldboard;
  me->currentboard=newboard;
  if (!me->params->uct_symmetry_use)
    me->currentboard->turnSymmetryOff();
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->endResponse();
}

void Engine::gtpOutputSGF(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  if (cmd->numArgs()!=1)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printf("need 1 arg");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  std::string sgffile=cmd->getStringArg(0);
  bool success=me->writeSGF(sgffile,me->currentboard,me->movetree);
  
  if (success)
  {
    gtpe->getOutput()->startResponse(cmd);
    gtpe->getOutput()->printf("wrote sgf file: %s",sgffile.c_str());
    gtpe->getOutput()->endResponse();
  }
  else
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printf("error writing sgf file: %s",sgffile.c_str());
    gtpe->getOutput()->endResponse();
  }
}

void Engine::gtpDoNPlayouts(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  if (cmd->numArgs()!=1)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printf("need 1 arg");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  int n=cmd->getIntArg(0);
  me->doNPlayouts(n);
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->endResponse();
}

void Engine::gtpExplainLastMove(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printString(me->lastexplanation);
  gtpe->getOutput()->endResponse();
}

void Engine::gtpBoardStats(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printf("board stats:\n");
  gtpe->getOutput()->printf("komi: %.1f\n",me->komi);
  gtpe->getOutput()->printf("moves: %d\n",me->currentboard->getMovesMade());
  gtpe->getOutput()->printf("next to move: %c\n",(me->currentboard->nextToMove()==Go::BLACK?'B':'W'));
  gtpe->getOutput()->printf("passes: %d\n",me->currentboard->getPassesPlayed());
  gtpe->getOutput()->printf("simple ko: ");
  int simpleko=me->currentboard->getSimpleKo();
  if (simpleko==-1)
    gtpe->getOutput()->printf("NONE");
  else
  {
    Gtp::Vertex vert={Go::Position::pos2x(simpleko,me->boardsize),Go::Position::pos2y(simpleko,me->boardsize)};
    gtpe->getOutput()->printVertex(vert);
  }
  gtpe->getOutput()->printf("\n");
  #if SYMMETRY_ONLYDEGRAGE
    gtpe->getOutput()->printf("stored symmetry: %s (degraded)\n",me->currentboard->getSymmetryString(me->currentboard->getSymmetry()).c_str());
  #else
    gtpe->getOutput()->printf("stored symmetry: %s\n",me->currentboard->getSymmetryString(me->currentboard->getSymmetry()).c_str());
  #endif
  gtpe->getOutput()->printf("computed symmetry: %s\n",me->currentboard->getSymmetryString(me->currentboard->computeSymmetry()).c_str());
  gtpe->getOutput()->endResponse(true);
}

void Engine::gtpFeatureMatchesAt(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  if (cmd->numArgs()!=1)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("vertex is required");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  Gtp::Vertex vert = cmd->getVertexArg(0);
  
  if (vert.x==-3 && vert.y==-3)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("invalid vertex");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  Go::Board *board=me->currentboard;
  Go::Color col=board->nextToMove();
  int pos=Go::Position::xy2pos(vert.x,vert.y,me->boardsize);
  Go::Move move=Go::Move(col,pos);
  
  me->features->setupCFGDist(board);
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printf("Feature Matches for %s:\n",move.toString(board->getSize()).c_str());
  gtpe->getOutput()->printf("PASS:              %u\n",me->features->matchFeatureClass(Features::PASS,board,move));
  gtpe->getOutput()->printf("CAPTURE:           %u\n",me->features->matchFeatureClass(Features::CAPTURE,board,move));
  gtpe->getOutput()->printf("EXTENSION:         %u\n",me->features->matchFeatureClass(Features::EXTENSION,board,move));
  gtpe->getOutput()->printf("SELFATARI:         %u\n",me->features->matchFeatureClass(Features::SELFATARI,board,move));
  gtpe->getOutput()->printf("ATARI:             %u\n",me->features->matchFeatureClass(Features::ATARI,board,move));
  gtpe->getOutput()->printf("BORDERDIST:        %u\n",me->features->matchFeatureClass(Features::BORDERDIST,board,move));
  gtpe->getOutput()->printf("LASTDIST:          %u\n",me->features->matchFeatureClass(Features::LASTDIST,board,move));
  gtpe->getOutput()->printf("SECONDLASTDIST:    %u\n",me->features->matchFeatureClass(Features::SECONDLASTDIST,board,move));
  gtpe->getOutput()->printf("CFGLASTDIST:       %u\n",me->features->matchFeatureClass(Features::CFGLASTDIST,board,move));
  gtpe->getOutput()->printf("CFGSECONDLASTDIST: %u\n",me->features->matchFeatureClass(Features::CFGSECONDLASTDIST,board,move));
  gtpe->getOutput()->printf("PATTERN3X3:        0x%04x\n",me->features->matchFeatureClass(Features::PATTERN3X3,board,move));
  float gamma=me->features->getMoveGamma(board,move);
  float total=me->features->getBoardGamma(board,col);
  gtpe->getOutput()->printf("Gamma: %.2f/%.2f (%.2f)\n",gamma,total,gamma/total);
  gtpe->getOutput()->endResponse(true);
  
  me->features->clearCFGDist();
}

void Engine::gtpFeatureProbDistribution(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  Go::Board *board=me->currentboard;
  Go::Color col=board->nextToMove();
  float totalgamma=me->features->getBoardGamma(board,col);
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printString("\n");
  for (int y=me->boardsize-1;y>=0;y--)
  {
    for (int x=0;x<me->boardsize;x++)
    {
      Go::Move move=Go::Move(col,Go::Position::xy2pos(x,y,me->boardsize)); 
      float gamma=me->features->getMoveGamma(board,move);
      if (gamma<=0)
        gtpe->getOutput()->printf("\"\" ");
      else
      {
        float prob=gamma/totalgamma;
        float point1=0.15;
        float point2=0.65;
        float r,g,b;
        // scale from blue-green-red-reddest?
        if (prob>=point2)
        {
          b=0;
          r=1;
          g=0;
        }
        else if (prob>=point1)
        {
          b=0;
          r=(prob-point1)/(point2-point1);
          g=1-r;
        }
        else
        {
          r=0;
          g=prob/point1;
          b=1-g;
        }
        if (r<0)
          r=0;
        if (g<0)
          g=0;
        if (b<0)
          b=0;
        gtpe->getOutput()->printf("#%02x%02x%02x ",(int)(r*255),(int)(g*255),(int)(b*255));
        //gtpe->getOutput()->printf("#%06x ",(int)(prob*(1<<24)));
      }
    }
    gtpe->getOutput()->printf("\n");
  }

  gtpe->getOutput()->endResponse(true);
}

void Engine::gtpListAllPatterns(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  Go::Board *board=me->currentboard;
  Go::Color col=board->nextToMove();
  
  gtpe->getOutput()->startResponse(cmd);
  for (int p=0;p<board->getPositionMax();p++)
  {
    if (me->currentboard->validMove(Go::Move(col,p)))
    {
      unsigned int hash=Pattern::ThreeByThree::makeHash(me->currentboard,p);
      if (col==Go::WHITE)
        hash=Pattern::ThreeByThree::invert(hash);
      hash=Pattern::ThreeByThree::smallestEquivalent(hash);
      gtpe->getOutput()->printf("0x%04x ",hash);
    }
  }

  gtpe->getOutput()->endResponse();
}

void Engine::gtpLoadFeatureGammas(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  if (cmd->numArgs()!=1)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printf("need 1 arg");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  std::string filename=cmd->getStringArg(0);
  
  delete me->features;
  me->features=new Features(me->params);
  bool success=me->features->loadGammaFile(filename);
  
  if (success)
  {
    gtpe->getOutput()->startResponse(cmd);
    gtpe->getOutput()->printf("loaded features gamma file: %s",filename.c_str());
    gtpe->getOutput()->endResponse();
  }
  else
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printf("error loading features gamma file: %s",filename.c_str());
    gtpe->getOutput()->endResponse();
  }
}

void Engine::gtpListFeatureIds(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printf("\n%s",me->features->getFeatureIdList().c_str());
  gtpe->getOutput()->endResponse(true);
}

void Engine::gtpShowCFGFrom(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  if (cmd->numArgs()!=1)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("vertex is required");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  Gtp::Vertex vert = cmd->getVertexArg(0);
  
  if (vert.x==-3 && vert.y==-3)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("invalid vertex");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  int pos=Go::Position::xy2pos(vert.x,vert.y,me->boardsize);
  Go::ObjectBoard<int> *cfgdist=me->currentboard->getCFGFrom(pos);
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printString("\n");
  for (int y=me->boardsize-1;y>=0;y--)
  {
    for (int x=0;x<me->boardsize;x++)
    {
      int p=Go::Position::xy2pos(x,y,me->boardsize);
      int dist=cfgdist->get(p);
      if (dist!=-1)
        gtpe->getOutput()->printf("\"%d\" ",dist);
      else
        gtpe->getOutput()->printf("\"\" ");
    }
    gtpe->getOutput()->printf("\n");
  }

  gtpe->getOutput()->endResponse(true);
  
  delete cfgdist;
}

void Engine::gtpShowCircDistFrom(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  if (cmd->numArgs()!=1)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("vertex is required");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  Gtp::Vertex vert = cmd->getVertexArg(0);
  
  if (vert.x==-3 && vert.y==-3)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("invalid vertex");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printString("\n");
  for (int y=me->boardsize-1;y>=0;y--)
  {
    for (int x=0;x<me->boardsize;x++)
    {
      int dist=Go::circDist(vert.x,vert.y,x,y);
      gtpe->getOutput()->printf("\"%d\" ",dist);
    }
    gtpe->getOutput()->printf("\n");
  }

  gtpe->getOutput()->endResponse(true);
}

void Engine::gtpListCircularPatternsAt(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  if (cmd->numArgs()!=1)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("vertex is required");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  Gtp::Vertex vert = cmd->getVertexArg(0);
  
  if (vert.x==-3 && vert.y==-3)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("invalid vertex");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  int pos=Go::Position::xy2pos(vert.x,vert.y,me->boardsize);
  Pattern::Circular pattcirc=Pattern::Circular(me->circdict,me->currentboard,pos,PATTERN_CIRC_MAXSIZE);
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printf("Circular Patterns at %s:\n",Go::Position::pos2string(pos,me->boardsize).c_str());
  for (int s=2;s<=PATTERN_CIRC_MAXSIZE;s++)
  {
    gtpe->getOutput()->printf(" %s\n",pattcirc.getSubPattern(me->circdict,s).toString(me->circdict).c_str());
  }
  /*pattcirc.rotateRight(me->circdict);
  gtpe->getOutput()->printf(" %s\n",pattcirc.toString(me->circdict).c_str());
  pattcirc.rotateRight(me->circdict);
  gtpe->getOutput()->printf(" %s\n",pattcirc.toString(me->circdict).c_str());
  pattcirc.rotateRight(me->circdict);
  gtpe->getOutput()->printf(" %s\n",pattcirc.toString(me->circdict).c_str());
  pattcirc.rotateRight(me->circdict);*/
  //pattcirc.rotateRight(me->circdict);
  //pattcirc.flipHorizontal(me->circdict);
  gtpe->getOutput()->printf("Smallest Equivalent:\n");
  pattcirc.convertToSmallestEquivalent(me->circdict);
  gtpe->getOutput()->printf(" %s\n",pattcirc.toString(me->circdict).c_str());
  gtpe->getOutput()->endResponse(true);
}

void Engine::gtpListAllCircularPatterns(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  int size=0;
  if (cmd->numArgs()>=1)
  {
    size=cmd->getIntArg(0);
  }
  
  Go::Board *board=me->currentboard;
  Go::Color col=board->nextToMove();
  
  gtpe->getOutput()->startResponse(cmd);
  for (int p=0;p<board->getPositionMax();p++)
  {
    if (me->currentboard->validMove(Go::Move(col,p)))
    {
      Pattern::Circular pattcirc=Pattern::Circular(me->circdict,board,p,PATTERN_CIRC_MAXSIZE);
      if (col==Go::WHITE)
        pattcirc.invert();
      pattcirc.convertToSmallestEquivalent(me->circdict);
      if (size==0)
      {
        for (int s=4;s<=PATTERN_CIRC_MAXSIZE;s++)
        {
          gtpe->getOutput()->printf(" %s",pattcirc.getSubPattern(me->circdict,s).toString(me->circdict).c_str());
        }
      }
      else
        gtpe->getOutput()->printf(" %s",pattcirc.getSubPattern(me->circdict,size).toString(me->circdict).c_str());
    }
  }

  gtpe->getOutput()->endResponse();
}

void Engine::gtpListAdjacentGroupsOf(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  if (cmd->numArgs()!=1)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("vertex is required");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  Gtp::Vertex vert = cmd->getVertexArg(0);
  
  if (vert.x==-3 && vert.y==-3)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("invalid vertex");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  int pos=Go::Position::xy2pos(vert.x,vert.y,me->boardsize);
  Go::Group *group=NULL;
  if (me->currentboard->inGroup(pos))
    group=me->currentboard->getGroup(pos);
  
  if (group!=NULL)
  {
    std::list<int,Go::allocator_int> *adjacentgroups=group->getAdjacentGroups();
    
    gtpe->getOutput()->startResponse(cmd);
    gtpe->getOutput()->printf("list of size %d:\n",adjacentgroups->size());
    for(std::list<int,Go::allocator_int>::iterator iter=adjacentgroups->begin();iter!=adjacentgroups->end();++iter)
    {
      if (me->currentboard->inGroup((*iter)))
        gtpe->getOutput()->printf("%s\n",Go::Position::pos2string((*iter),me->boardsize).c_str());
    }
    gtpe->getOutput()->endResponse(true);
  }
  else
  {
    gtpe->getOutput()->startResponse(cmd);
    gtpe->getOutput()->endResponse();
  }
}

void Engine::gtpShowSymmetryTransforms(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  Go::Board::Symmetry sym=me->currentboard->getSymmetry();
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printString("\n");
  for (int y=me->boardsize-1;y>=0;y--)
  {
    for (int x=0;x<me->boardsize;x++)
    {
      int pos=Go::Position::xy2pos(x,y,me->boardsize);
      int transpos=me->currentboard->doSymmetryTransformToPrimary(sym,pos);
      if (transpos==pos)
        gtpe->getOutput()->printf("\"P\" ");
      else
      {
        Gtp::Vertex vert={Go::Position::pos2x(transpos,me->boardsize),Go::Position::pos2y(transpos,me->boardsize)};
        gtpe->getOutput()->printf("\"");
        gtpe->getOutput()->printVertex(vert);
        gtpe->getOutput()->printf("\" ");
      }
    }
    gtpe->getOutput()->printf("\n");
  }

  gtpe->getOutput()->endResponse(true);
}

void Engine::gtpShowNakadeCenters(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printString("\n");
  for (int y=me->boardsize-1;y>=0;y--)
  {
    for (int x=0;x<me->boardsize;x++)
    {
      int pos=Go::Position::xy2pos(x,y,me->boardsize);
      int centerpos=me->currentboard->getThreeEmptyGroupCenterFrom(pos);
      if (centerpos==-1)
        gtpe->getOutput()->printf("\"\" ");
      else
      {
        Gtp::Vertex vert={Go::Position::pos2x(centerpos,me->boardsize),Go::Position::pos2y(centerpos,me->boardsize)};
        gtpe->getOutput()->printf("\"");
        gtpe->getOutput()->printVertex(vert);
        gtpe->getOutput()->printf("\" ");
      }
    }
    gtpe->getOutput()->printf("\n");
  }

  gtpe->getOutput()->endResponse(true);
}

void Engine::gtpShowTreeLiveGfx(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  gtpe->getOutput()->startResponse(cmd);
  if (me->params->move_policy==Parameters::MP_UCT || me->params->move_policy==Parameters::MP_ONEPLY)
    me->displayPlayoutLiveGfx(-1,false);
  gtpe->getOutput()->endResponse();
}

void Engine::gtpDescribeEngine(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printf("%s\n",me->longname.c_str());
  gtpe->getOutput()->printf("parameters:\n");
  me->params->printParametersForDescription(gtpe);
  gtpe->getOutput()->endResponse(true);
}

void Engine::gtpShowCurrentHash(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printf("Current hash: 0x%016llx",me->currentboard->getZobristHash(me->zobristtable));
  gtpe->getOutput()->endResponse();
}

void Engine::gtpBookShow(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printString(me->book->show(me->boardsize,me->movehistory));
  gtpe->getOutput()->endResponse(true);
}

void Engine::gtpBookAdd(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  if (cmd->numArgs()!=1)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("vertex is required");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  Gtp::Vertex vert = cmd->getVertexArg(0);
  
  if (vert.x==-3 && vert.y==-3)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("invalid vertex");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  int pos=Go::Position::xy2pos(vert.x,vert.y,me->boardsize);
  Go::Move move=Go::Move(me->currentboard->nextToMove(),pos);
  
  me->book->add(me->boardsize,me->movehistory,move);
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->endResponse();
}

void Engine::gtpBookRemove(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  if (cmd->numArgs()!=1)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("vertex is required");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  Gtp::Vertex vert = cmd->getVertexArg(0);
  
  if (vert.x==-3 && vert.y==-3)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("invalid vertex");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  int pos=Go::Position::xy2pos(vert.x,vert.y,me->boardsize);
  Go::Move move=Go::Move(me->currentboard->nextToMove(),pos);
  
  me->book->remove(me->boardsize,me->movehistory,move);
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->endResponse();
}

void Engine::gtpBookClear(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  me->book->clear(me->boardsize);
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->endResponse();
}

void Engine::gtpBookLoad(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  if (cmd->numArgs()!=1)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printf("need 1 arg");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  std::string filename=cmd->getStringArg(0);
  
  delete me->book;
  me->book=new Book(me->params);
  bool success=me->book->loadFile(filename);
  
  if (success)
  {
    gtpe->getOutput()->startResponse(cmd);
    gtpe->getOutput()->printf("loaded book: %s",filename.c_str());
    gtpe->getOutput()->endResponse();
  }
  else
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printf("error loading book: %s",filename.c_str());
    gtpe->getOutput()->endResponse();
  }
}

void Engine::gtpBookSave(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  if (cmd->numArgs()!=1)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printf("need 1 arg");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  std::string filename=cmd->getStringArg(0);
  bool success=me->book->saveFile(filename);
  
  if (success)
  {
    gtpe->getOutput()->startResponse(cmd);
    gtpe->getOutput()->printf("saved book: %s",filename.c_str());
    gtpe->getOutput()->endResponse();
  }
  else
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printf("error saving book: %s",filename.c_str());
    gtpe->getOutput()->endResponse();
  }
}

void Engine::gtpShowSafePositions(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  Go::Board *board=me->currentboard;
  Benson *benson=new Benson(board);
  benson->solve();
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printf("BLACK");
  for (int p=0;p<board->getPositionMax();p++)
  {
    if (benson->getSafePositions()->get(p)==Go::BLACK)
      gtpe->getOutput()->printf(" %s",Go::Position::pos2string(p,me->boardsize).c_str());
  }
  gtpe->getOutput()->printf("\nWHITE");
  for (int p=0;p<board->getPositionMax();p++)
  {
    if (benson->getSafePositions()->get(p)==Go::WHITE)
      gtpe->getOutput()->printf(" %s",Go::Position::pos2string(p,me->boardsize).c_str());
  }
  gtpe->getOutput()->endResponse();
  
  delete benson;
}

void Engine::gtpParam(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  if (cmd->numArgs()==0)
  {
    gtpe->getOutput()->startResponse(cmd);
    me->params->printParametersForGTP(gtpe);
    gtpe->getOutput()->endResponse(true);
  }
  else if (cmd->numArgs()==1)
  {
    std::string category=cmd->getStringArg(0);
    
    gtpe->getOutput()->startResponse(cmd);
    me->params->printParametersForGTP(gtpe,category);
    gtpe->getOutput()->endResponse(true);
  }
  else if (cmd->numArgs()==2)
  {
    std::string param=cmd->getStringArg(0);
    std::transform(param.begin(),param.end(),param.begin(),::tolower);
    
    if (me->params->setParameter(param,cmd->getStringArg(1)))
    {
      gtpe->getOutput()->startResponse(cmd);
      gtpe->getOutput()->endResponse();
    }
    else
    {
      gtpe->getOutput()->startResponse(cmd,false);
      gtpe->getOutput()->printf("error setting parameter: %s",param.c_str());
      gtpe->getOutput()->endResponse();
    }
  }
  else if (cmd->numArgs()==3)
  {
    std::string category=cmd->getStringArg(0); //check this parameter in category?
    std::string param=cmd->getStringArg(1);
    std::transform(param.begin(),param.end(),param.begin(),::tolower);
    
    if (me->params->setParameter(param,cmd->getStringArg(2)))
    {
      gtpe->getOutput()->startResponse(cmd);
      gtpe->getOutput()->endResponse();
    }
    else
    {
      gtpe->getOutput()->startResponse(cmd,false);
      gtpe->getOutput()->printf("error setting parameter: %s",param.c_str());
      gtpe->getOutput()->endResponse();
    }
  }
  else
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printf("need 0 to 3 args");
    gtpe->getOutput()->endResponse();
  }
}

void Engine::gtpTimeSettings(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  if (cmd->numArgs()!=3)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("invalid arguments");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  delete me->time;
  me->time=new Time(me->params,cmd->getIntArg(0),cmd->getIntArg(1),cmd->getIntArg(2));
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->endResponse();
}

void Engine::gtpTimeLeft(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  if (me->time->isNoTiming())
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("no time settings defined");
    gtpe->getOutput()->endResponse();
    return;
  }
  else if (cmd->numArgs()!=3)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("invalid arguments");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  Gtp::Color gtpcol = cmd->getColorArg(0);
  if (gtpcol==Gtp::INVALID)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("invalid color");
    gtpe->getOutput()->endResponse();
    return;
  }
  Go::Color col=(gtpcol==Gtp::BLACK ? Go::BLACK : Go::WHITE);
  
  float oldtime=me->time->timeLeft(col);
  //int oldstones=me->time->stonesLeft(col);
  float newtime=(float)cmd->getIntArg(1);
  int newstones=cmd->getIntArg(2);
  
  if (newstones==0)
    gtpe->getOutput()->printfDebug("[time_left]: diff:%.3f\n",newtime-oldtime);
  else
    gtpe->getOutput()->printfDebug("[time_left]: diff:%.3f s:%d\n",newtime-oldtime,newstones);
  
  me->time->updateTimeLeft(col,newtime,newstones);
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->endResponse();
}

void Engine::generateMove(Go::Color col, Go::Move **move, bool playmove)
{
  if (params->book_use)
  {
    std::list<Go::Move> bookmoves=book->getMoves(boardsize,movehistory);
    
    if (bookmoves.size()>0)
    {
      int r=rand.getRandomInt(bookmoves.size());
      int i=0;
      for (std::list<Go::Move>::iterator iter=bookmoves.begin();iter!=bookmoves.end();++iter)
      {
        if (i==r)
        {
          *move=new Go::Move(*iter);
          
          if (playmove)
            this->makeMove(**move);
            
          lastexplanation="selected move from book";
          
          gtpe->getOutput()->printfDebug("[genmove]: %s\n",lastexplanation.c_str());
          if (params->livegfx_on)
            gtpe->getOutput()->printfDebug("gogui-gfx: TEXT [genmove]: %s\n",lastexplanation.c_str());
          
          return;
        }
        i++;
      }
    }
  }
  if (params->move_policy==Parameters::MP_UCT || params->move_policy==Parameters::MP_ONEPLY)
  {
    boost::timer timer;
    int totalplayouts=0;
    int livegfxupdate=0;
    float time_allocated;
    float playouts_per_milli;
    bool early_stop=false;
    
    stopthinking=false;
    
    if (!time->isNoTiming())
    {
      if (params->time_ignore)
      {
        time_allocated=0;
        gtpe->getOutput()->printfDebug("[time_allowed]: ignoring time settings!\n");
      }
      else
      {
        time_allocated=time->getAllocatedTimeForNextTurn(col);
        gtpe->getOutput()->printfDebug("[time_allowed]: %.3f\n",time_allocated);
      }
    }
    else
      time_allocated=0;
    
    if (params->livegfx_on)
      gtpe->getOutput()->printfDebug("gogui-gfx: TEXT [genmove]: starting...\n");
    
    Go::Color expectedcol=currentboard->nextToMove();
    currentboard->setNextToMove(col);
    
    if (expectedcol!=col)
    {
      gtpe->getOutput()->printfDebug("WARNING! Unexpected color. Discarding tree.\n");
      this->clearMoveTree();
    }
    
    movetree->prunePossibleSuperkoViolations();
    this->allowContinuedPlay();
    params->uct_slow_update_last=0;
    params->uct_last_r2=-1;
    
    Go::BitBoard *firstlist=new Go::BitBoard(boardsize);
    Go::BitBoard *secondlist=new Go::BitBoard(boardsize);
    
    for (int i=0;i<params->playouts_per_move_max;i++)
    {
      if (i>=params->playouts_per_move && time_allocated==0)
        break;
      else if (i>=params->playouts_per_move_min && time_allocated>0 && timer.elapsed()>time_allocated)
        break;
      else if (movetree->isTerminalResult())
      {
        gtpe->getOutput()->printfDebug("SOLVED: found 100%% sure result after %d plts!\n",totalplayouts);
        early_stop=true;
        break;
      }
      else if (stopthinking)
      {
        early_stop=true;
        break;
      }
      
      this->doPlayout(firstlist,secondlist);
      totalplayouts++;
      
      if (params->uct_stop_early && params->uct_slow_update_last==0)
      {
        Tree *besttree=movetree->getRobustChild();
        if (besttree!=NULL)
        {
          float currentpart=(besttree->getPlayouts()-besttree->secondBestPlayouts())/totalplayouts;
          float overallratio;
          if (time_allocated>0) // timed search
            overallratio=(float)(time_allocated+TIME_RESOLUTION)/timer.elapsed();
          else
            overallratio=(float)params->playouts_per_move/totalplayouts;
          
          if ((overallratio-1)<currentpart)
          {
            gtpe->getOutput()->printfDebug("best move cannot change! (%.3f %.3f)\n",currentpart,overallratio);
            early_stop=true;
            break;
          }
        }
      }
      
      if (params->livegfx_on)
      {
        if (livegfxupdate>=(params->livegfx_update_playouts-1))
        {
          livegfxupdate=0;
          
          this->displayPlayoutLiveGfx(totalplayouts);
          
          boost::timer delay;
          while (delay.elapsed()<params->livegfx_delay) {}
        }
        else
          livegfxupdate++;
      }
    }
    
    delete firstlist;
    delete secondlist;
    
    Tree *besttree=movetree->getRobustChild();
    float bestratio=0;
    float ratiodelta=-1;
    bool bestsame=false;
    if (besttree==NULL)
    {
      fprintf(stderr,"WARNING! No move found!\n");
      *move=new Go::Move(col,Go::Move::RESIGN);
    }
    else if (!besttree->isTerminalWin() && besttree->getRatio()<params->resign_ratio_threshold && currentboard->getMovesMade()>(params->resign_move_factor_threshold*boardsize*boardsize))
      *move=new Go::Move(col,Go::Move::RESIGN);
    else
    {
      *move=new Go::Move(col,besttree->getMove().getPosition());
      bestratio=besttree->getRatio();
      
      ratiodelta=besttree->bestChildRatioDiff();
      bestsame=(besttree==(movetree->getBestRatioChild(10)));
    }
    
    if (params->uct_slow_update_last!=0)
      this->doSlowUpdate();
    
    if (playmove)
      this->makeMove(**move);
    
    if (params->livegfx_on)
      gtpe->getOutput()->printfDebug("gogui-gfx: CLEAR\n");
    
    float time_used=timer.elapsed();
    if (time_used>0)
      playouts_per_milli=(float)totalplayouts/(time_used*1000);
    else
      playouts_per_milli=-1;
    if (!time->isNoTiming())
      time->useTime(col,time_used);
    
    std::ostringstream ss;
    ss << std::fixed;
    ss << "r:"<<std::setprecision(2)<<bestratio;
    if (!time->isNoTiming())
    {
      ss << " tl:"<<std::setprecision(3)<<time->timeLeft(col);
      if (time->stonesLeft(col)>0)
        ss << " s:"<<time->stonesLeft(col);
    }
    if (!time->isNoTiming() || early_stop)
      ss << " plts:"<<totalplayouts;
    ss << " ppms:"<<std::setprecision(2)<<playouts_per_milli;
    ss << " rd:"<<std::setprecision(2)<<ratiodelta;
    ss << " r2:"<<std::setprecision(2)<<params->uct_last_r2;
    ss << " bs:"<<bestsame;
    Tree *pvtree=movetree->getRobustChild(true);
    if (pvtree!=NULL)
    {
      std::list<Go::Move> pvmoves=pvtree->getMovesFromRoot();
      ss<<" pv:(";
      for(std::list<Go::Move>::iterator iter=pvmoves.begin();iter!=pvmoves.end();++iter) 
      {
        ss<<(iter!=pvmoves.begin()?",":"")<<Go::Position::pos2string((*iter).getPosition(),boardsize);
      }
      ss<<")";
    }
    if (params->surewin_expected)
      ss << " surewin!";
    lastexplanation=ss.str();
    
    gtpe->getOutput()->printfDebug("[genmove]: %s\n",lastexplanation.c_str());
    if (params->livegfx_on)
      gtpe->getOutput()->printfDebug("gogui-gfx: TEXT [genmove]: %s\n",lastexplanation.c_str());
  }
  else
  {
    *move=new Go::Move(col,Go::Move::PASS);
    Go::Board *playoutboard=currentboard->copy();
    playoutboard->turnSymmetryOff();
    if (params->playout_features_enabled)
      playoutboard->setFeatures(features,params->playout_features_incremental);
    playout->getPlayoutMove(playoutboard,col,**move);
    delete playoutboard;
    this->makeMove(**move);
  }
}

bool Engine::isMoveAllowed(Go::Move move)
{
  return currentboard->validMove(move);
}

void Engine::makeMove(Go::Move move)
{
  if (params->features_output_competitions)
  {
    bool isawinner=true;
    features->setupCFGDist(currentboard);
    
    if (params->features_output_competitions_mmstyle)
    {
      int p=move.getPosition();
      std::string featurestring=features->getMatchingFeaturesString(currentboard,move,!params->features_output_competitions_mmstyle);
      if (featurestring.length()>0)
      {
        gtpe->getOutput()->printfDebug("[features]:# competition (%d,%s)\n",(currentboard->getMovesMade()+1),Go::Position::pos2string(move.getPosition(),boardsize).c_str());
        gtpe->getOutput()->printfDebug("[features]:%s*",Go::Position::pos2string(p,boardsize).c_str());
        gtpe->getOutput()->printfDebug("%s",featurestring.c_str());
        gtpe->getOutput()->printfDebug("\n");
      }
      else
        isawinner=false; 
    }
    else
      gtpe->getOutput()->printfDebug("[features]:# competition (%d,%s)\n",(currentboard->getMovesMade()+1),Go::Position::pos2string(move.getPosition(),boardsize).c_str());
    
    if (isawinner)
    {
      Go::Color col=move.getColor();
      for (int p=0;p<currentboard->getPositionMax();p++)
      {
        Go::Move m=Go::Move(col,p);
        if (currentboard->validMove(m) || m==move)
        {
          std::string featurestring=features->getMatchingFeaturesString(currentboard,m,!params->features_output_competitions_mmstyle);
          if (featurestring.length()>0)
          {
            gtpe->getOutput()->printfDebug("[features]:%s",Go::Position::pos2string(p,boardsize).c_str());
            if (m==move)
              gtpe->getOutput()->printfDebug("*");
            else
              gtpe->getOutput()->printfDebug(":");
            gtpe->getOutput()->printfDebug("%s",featurestring.c_str());
            gtpe->getOutput()->printfDebug("\n");
          }
        }
      }
      {
        Go::Move m=Go::Move(col,Go::Move::PASS);
        if (currentboard->validMove(m) || m==move)
        {
          gtpe->getOutput()->printfDebug("[features]:PASS");
          if (m==move)
            gtpe->getOutput()->printfDebug("*");
          else
            gtpe->getOutput()->printfDebug(":");
          gtpe->getOutput()->printfDebug("%s",features->getMatchingFeaturesString(currentboard,m,!params->features_output_competitions_mmstyle).c_str());
          gtpe->getOutput()->printfDebug("\n");
        }
      }
    }
    
    features->clearCFGDist();
  }
  
  if (params->features_ordered_comparison)
  {
    bool usedpos[currentboard->getPositionMax()+1];
    for (int i=0;i<=currentboard->getPositionMax();i++)
      usedpos[i]=false;
    int posused=0;
    float bestgamma=-1;
    int bestpos=0;
    Go::Color col=move.getColor();
    int matchedat=0;
  
    gtpe->getOutput()->printfDebug("[feature_comparison]:# comparison (%d,%s)\n",(currentboard->getMovesMade()+1),Go::Position::pos2string(move.getPosition(),boardsize).c_str());
    
    gtpe->getOutput()->printfDebug("[feature_comparison]:");
    while (true)
    {
      for (int p=0;p<currentboard->getPositionMax();p++)
      {
        if (!usedpos[p])
        {
          Go::Move m=Go::Move(col,p);
          if (currentboard->validMove(m) || m==move)
          {
            float gamma=features->getMoveGamma(currentboard,m);;
            if (gamma>bestgamma)
            {
              bestgamma=gamma;
              bestpos=p;
            }
          }
        }
      }
      
      {
        int p=currentboard->getPositionMax();
        if (!usedpos[p])
        {
          Go::Move m=Go::Move(col,Go::Move::PASS);
          if (currentboard->validMove(m) || m==move)
          {
            float gamma=features->getMoveGamma(currentboard,m);;
            if (gamma>bestgamma)
            {
              bestgamma=gamma;
              bestpos=p;
            }
          }
        }
      }
      
      if (bestgamma!=-1)
      {
        Go::Move m;
        if (bestpos==currentboard->getPositionMax())
          m=Go::Move(col,Go::Move::PASS);
        else
          m=Go::Move(col,bestpos);
        posused++;
        usedpos[bestpos]=true;
        gtpe->getOutput()->printfDebug(" %s",Go::Position::pos2string(m.getPosition(),boardsize).c_str());
        if (m==move)
        {
          gtpe->getOutput()->printfDebug("*");
          matchedat=posused;
        }
      }
      else
        break;
      
      bestgamma=-1;
    }
    gtpe->getOutput()->printfDebug("\n");
    gtpe->getOutput()->printfDebug("[feature_comparison]:matched at: %d\n",matchedat);
  }
  
  currentboard->makeMove(move);
  movehistory->push_back(move);
  hashtree->addHash(currentboard->getZobristHash(zobristtable));
  params->uct_slow_update_last=0;
  if (params->uct_keep_subtree)
    this->chooseSubTree(move);
  else
    this->clearMoveTree();
}

void Engine::setBoardSize(int s)
{
  if (s<BOARDSIZE_MIN || s>BOARDSIZE_MAX)
    return;
  
  boardsize=s;
  params->board_size=boardsize;
  this->clearBoard();
}

void Engine::clearBoard()
{
  bool newsize=(zobristtable->getSize()!=boardsize);
  delete currentboard;
  delete movehistory;
  delete hashtree;
  if (newsize)
    delete zobristtable;
  currentboard = new Go::Board(boardsize);
  movehistory = new std::list<Go::Move>();
  hashtree=new Go::ZobristTree();
  if (newsize)
    zobristtable=new Go::ZobristTable(params,boardsize);
  if (!params->uct_symmetry_use)
    currentboard->turnSymmetryOff();
  this->clearMoveTree();
  params->surewin_expected=false;
  playout->resetLGRF();
}

void Engine::clearMoveTree()
{
  if (movetree!=NULL)
    delete movetree;
  
  if (currentboard->getMovesMade()>0)
    movetree=new Tree(params,currentboard->getZobristHash(zobristtable),currentboard->getLastMove());
  else
    movetree=new Tree(params,0);
  
  params->uct_slow_update_last=0;
}

void Engine::chooseSubTree(Go::Move move)
{
  Tree *subtree=movetree->getChild(move);
  
  if (subtree==NULL)
  {
    //gtpe->getOutput()->printfDebug("no such subtree...\n");
    this->clearMoveTree();
    return;
  }
  
  if (!subtree->isPrimary())
  {
    //gtpe->getOutput()->printfDebug("doing transformation...\n");
    subtree->performSymmetryTransformParentPrimary();
    subtree=movetree->getChild(move);
    if (subtree==NULL || !subtree->isPrimary())
      gtpe->getOutput()->printfDebug("WARNING! symmetry transformation failed! (null:%d)\n",(subtree==NULL));
  }
  
  if (subtree==NULL) // only true if a symmetry transform failed
  {
    gtpe->getOutput()->printfDebug("WARNING! clearing tree...\n");
    this->clearMoveTree();
    return;
  }
  
  movetree->divorceChild(subtree);
  delete movetree;
  movetree=subtree;
  movetree->prunePossibleSuperkoViolations();
}

bool Engine::writeSGF(std::string filename, Go::Board *board, Tree *tree)
{
  std::ofstream sgffile;
  sgffile.open(filename.c_str());
  sgffile<<"(;\nFF[4]SZ["<<boardsize<<"]KM["<<komi<<"]\n";
  sgffile<<board->toSGFString()<<"\n";
  if (tree!=NULL)
    sgffile<<tree->toSGFString()<<"\n)";
  sgffile.close();
  
  return true;
}

void Engine::doNPlayouts(int n)
{
  if (params->move_policy==Parameters::MP_UCT || params->move_policy==Parameters::MP_ONEPLY)
  {
    int livegfxupdate=0;
    
    stopthinking=false;
    
    this->allowContinuedPlay();
    
    Go::BitBoard *firstlist=new Go::BitBoard(boardsize);
    Go::BitBoard *secondlist=new Go::BitBoard(boardsize);
    
    for (int i=0;i<n;i++)
    {
      if (movetree->isTerminalResult())
      {
        gtpe->getOutput()->printfDebug("SOLVED! found 100%% sure result after %d plts!\n",i);
        break;
      }
      else if (stopthinking)
        break;
      
      this->doPlayout(firstlist,secondlist);
      
      if (params->livegfx_on)
      {
        if (livegfxupdate>=(params->livegfx_update_playouts-1))
        {
          livegfxupdate=0;
          
          this->displayPlayoutLiveGfx(i+1);
          
          boost::timer delay;
          while (delay.elapsed()<params->livegfx_delay) {}
        }
        else
          livegfxupdate++;
      }
    }
    
    delete firstlist;
    delete secondlist;
    
    if (params->livegfx_on)
      gtpe->getOutput()->printfDebug("gogui-gfx: CLEAR\n");
  }
}

void Engine::doPlayout(Go::BitBoard *firstlist,Go::BitBoard *secondlist)
{
  //bool givenfirstlist,givensecondlist;
  Go::Color col=currentboard->nextToMove();
  
  if (movetree->isLeaf())
  {
    this->allowContinuedPlay();
    movetree->expandLeaf();
    movetree->prunePossibleSuperkoViolations();
  }
  
  //givenfirstlist=(firstlist==NULL);
  //givensecondlist=(secondlist==NULL);
  
  Tree *playouttree = movetree->getUrgentChild();
  if (playouttree==NULL)
  {
    if (params->debug_on)
      gtpe->getOutput()->printfDebug("WARNING! No playout target found.\n");
    return;
  }
  std::list<Go::Move> playoutmoves=playouttree->getMovesFromRoot();
  if (playoutmoves.size()==0)
  {
    if (params->debug_on)
      gtpe->getOutput()->printfDebug("WARNING! Bad playout target found.\n");
    return;
  }
  
  //if (!givenfirstlist)
  //  firstlist=new Go::BitBoard(boardsize);
  //if (!givensecondlist)
  //  secondlist=new Go::BitBoard(boardsize);
  
  Go::Board *playoutboard=currentboard->copy();
  playoutboard->turnSymmetryOff();
  if (params->playout_features_enabled)
    playoutboard->setFeatures(features,params->playout_features_incremental);
  if (params->rave_moves>0)
  {
    firstlist->clear();
    secondlist->clear();
  }
  Go::Color playoutcol=playoutmoves.back().getColor();
  
  float finalscore;
  playout->doPlayout(playoutboard,finalscore,playoutmoves,col,(params->rave_moves>0?firstlist:NULL),(params->rave_moves>0?secondlist:NULL));
  
  bool playoutwin=Go::Board::isWinForColor(playoutcol,finalscore);
  bool playoutjigo=(finalscore==0);
  if (playoutjigo)
    playouttree->addPartialResult(0.5,1,false);
  else if (playoutwin)
    playouttree->addWin();
  else
    playouttree->addLose();
  
  if (!playouttree->isTerminalResult())
  {
    if (params->uct_points_bonus!=0)
    {
      float scorediff=(playoutcol==Go::BLACK?1:-1)*finalscore;
      //float bonus=params->uct_points_bonus*scorediff;
      float bonus;
      if (scorediff>0)
        bonus=params->uct_points_bonus*log(scorediff+1);
      else
        bonus=-params->uct_points_bonus*log(-scorediff+1);
      playouttree->addPartialResult(bonus,0);
      //fprintf(stderr,"[points_bonus]: %+6.1f %+f\n",scorediff,bonus);
    }
    if (params->uct_length_bonus!=0)
    {
      int moves=playoutboard->getMovesMade();
      float bonus=(playoutwin?1:-1)*params->uct_length_bonus*log(moves);
      playouttree->addPartialResult(bonus,0);
      //fprintf(stderr,"[length_bonus]: %6d %+f\n",moves,bonus);
    }
  }
  
  if (params->debug_on)
  {
    if (finalscore==0)
      gtpe->getOutput()->printfDebug("[result]:jigo\n");
    else if (playoutwin && playoutcol==col)
      gtpe->getOutput()->printfDebug("[result]:win (fs:%+.1f)\n",finalscore);
    else
      gtpe->getOutput()->printfDebug("[result]:lose (fs:%+.1f)\n",finalscore);
  }
  
  if (params->rave_moves>0)
  {
    if (!playoutjigo) // ignore jigos for RAVE
    {
      bool blackwin=Go::Board::isWinForColor(Go::BLACK,finalscore);
      Go::Color wincol=(blackwin?Go::BLACK:Go::WHITE);
      
      if (col==Go::BLACK)
        playouttree->updateRAVE(wincol,firstlist,secondlist);
      else
        playouttree->updateRAVE(wincol,secondlist,firstlist);
    }
  }
  
  params->uct_slow_update_last++;
  if (params->uct_slow_update_last>=params->uct_slow_update_interval)
  {
    params->uct_slow_update_last=0;
    
    this->doSlowUpdate();
  }
  
  delete playoutboard;
  
  //if (!givenfirstlist)
  //  delete firstlist;
  //if (!givensecondlist)
  //  delete secondlist;
}

void Engine::displayPlayoutLiveGfx(int totalplayouts, bool livegfx)
{
  Go::Color col=currentboard->nextToMove();
  
  if (livegfx)
    gtpe->getOutput()->printfDebug("gogui-gfx:\n");
  if (totalplayouts!=-1)
    gtpe->getOutput()->printfDebug("TEXT [genmove]: thinking... playouts:%d\n",totalplayouts);
  
  if (livegfx)
    gtpe->getOutput()->printfDebug("INFLUENCE");
  else
    gtpe->getOutput()->printf("INFLUENCE");
  int maxplayouts=1; //prevent div by zero
  for(std::list<Tree*>::iterator iter=movetree->getChildren()->begin();iter!=movetree->getChildren()->end();++iter) 
  {
    if ((*iter)->getPlayouts()>maxplayouts)
      maxplayouts=(int)(*iter)->getPlayouts();
  }
  float colorfactor=(col==Go::BLACK?1:-1);
  for(std::list<Tree*>::iterator iter=movetree->getChildren()->begin();iter!=movetree->getChildren()->end();++iter) 
  {
    if (!(*iter)->getMove().isPass() && !(*iter)->getMove().isResign())
    {
      Gtp::Vertex vert={(*iter)->getMove().getX(boardsize),(*iter)->getMove().getY(boardsize)};
      float playoutpercentage=(float)(*iter)->getPlayouts()/maxplayouts;
      if (playoutpercentage>1)
        playoutpercentage=1;
      
      if (livegfx)
      {
        gtpe->getOutput()->printfDebug(" ");
        gtpe->getOutput()->printDebugVertex(vert);
        gtpe->getOutput()->printfDebug(" %.2f",playoutpercentage*colorfactor);
      }
      else
      {
        gtpe->getOutput()->printf(" ");
        gtpe->getOutput()->printVertex(vert);
        gtpe->getOutput()->printf(" %.2f",playoutpercentage*colorfactor);
      }
    }
  }
  if (livegfx)
    gtpe->getOutput()->printfDebug("\n");
  else
    gtpe->getOutput()->printf("\n");
  if (params->move_policy==Parameters::MP_UCT)
  {
    if (livegfx)
      gtpe->getOutput()->printfDebug("VAR");
    else
      gtpe->getOutput()->printf("VAR");
    Tree *besttree=movetree->getRobustChild(true);
    if (besttree!=NULL)
    {
      std::list<Go::Move> bestmoves=besttree->getMovesFromRoot();
      for(std::list<Go::Move>::iterator iter=bestmoves.begin();iter!=bestmoves.end();++iter) 
      {
        if (!(*iter).isPass() && !(*iter).isResign())
        {
          Gtp::Vertex vert={(*iter).getX(boardsize),(*iter).getY(boardsize)};
          if (livegfx)
          {
            gtpe->getOutput()->printfDebug(" %c ",((*iter).getColor()==Go::BLACK?'B':'W'));
            gtpe->getOutput()->printDebugVertex(vert);
          }
          else
          {
            gtpe->getOutput()->printf(" %c ",((*iter).getColor()==Go::BLACK?'B':'W'));
            gtpe->getOutput()->printVertex(vert);
          }
        }
        else if ((*iter).isPass())
        {
          if (livegfx)
            gtpe->getOutput()->printfDebug(" %c PASS",((*iter).getColor()==Go::BLACK?'B':'W'));
          else
            gtpe->getOutput()->printf(" %c PASS",((*iter).getColor()==Go::BLACK?'B':'W'));
        }
      }
    }
  }
  else
  {
    if (livegfx)
      gtpe->getOutput()->printfDebug("SQUARE");
    else
      gtpe->getOutput()->printf("SQUARE");
    for(std::list<Tree*>::iterator iter=movetree->getChildren()->begin();iter!=movetree->getChildren()->end();++iter) 
    {
      if (!(*iter)->getMove().isPass() && !(*iter)->getMove().isResign())
      {
        if ((*iter)->getPlayouts()==maxplayouts)
        {
          Gtp::Vertex vert={(*iter)->getMove().getX(boardsize),(*iter)->getMove().getY(boardsize)};
          if (livegfx)
          {
            gtpe->getOutput()->printfDebug(" ");
            gtpe->getOutput()->printDebugVertex(vert);
          }
          else
          {
            gtpe->getOutput()->printf(" ");
            gtpe->getOutput()->printVertex(vert);
          }
        }
      }
    }
  }
  if (livegfx)
    gtpe->getOutput()->printfDebug("\n\n");
}

void Engine::allowContinuedPlay()
{
  if (currentboard->getPassesPlayed()>=2)
  {
    currentboard->resetPassesPlayed();
    movetree->allowContinuedPlay();
    gtpe->getOutput()->printfDebug("WARNING! continuing play from a terminal position\n");
  }
}

void Engine::ponder()
{
  if (!(params->pondering_enabled) || (currentboard->getMovesMade()<=0) || (currentboard->getPassesPlayed()>=2) || (currentboard->getLastMove().isResign()) || (book->getMoves(boardsize,movehistory).size()>0))
    return;
  
  if (params->move_policy==Parameters::MP_UCT || params->move_policy==Parameters::MP_ONEPLY)
  {
    //fprintf(stderr,"pondering starting!\n");
    this->allowContinuedPlay();
    params->uct_slow_update_last=0;
    
    Go::BitBoard *firstlist=new Go::BitBoard(boardsize);
    Go::BitBoard *secondlist=new Go::BitBoard(boardsize);
    
    int playouts=0;
    while (!stoppondering && movetree->getPlayouts()<(params->pondering_playouts_max))
    {
      if (movetree->isTerminalResult())
      {
        gtpe->getOutput()->printfDebug("SOLVED! found 100%% sure result after %d plts!\n",playouts);
        break;
      }
      
      this->doPlayout(firstlist,secondlist);
      playouts++;
    }
    
    delete firstlist;
    delete secondlist;
    //fprintf(stderr,"pondering done! %d %.0f\n",playouts,movetree->getPlayouts());
  }
}

void Engine::doSlowUpdate()
{
  Tree *besttree=movetree->getRobustChild();
  if (besttree!=NULL)
  {
    params->surewin_expected=(besttree->getRatio()>=params->surewin_threshold);
    
    params->uct_last_r2=besttree->secondBestPlayoutRatio();
  }
}


