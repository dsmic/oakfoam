#include "Engine.h"

#include <cstdlib>
#include <cmath>
#include <vector>
#include <fstream>
#include <iomanip>
#include <boost/timer.hpp>
#include <config.h>

Engine::Engine(Gtp::Engine *ge)
{
  gtpe=ge;
  
  unsigned long seed=std::time(0);
  rand=Random(seed);
  
  params=new Parameters();
  
  boardsize=9;
  params->board_size=boardsize;
  currentboard=new Go::Board(boardsize);
  komi=5.5;
  
  params->addParameter("other","rand_seed",&(params->rand_seed),seed,&Engine::updateParameterWrapper,this);
  
  std::list<std::string> *mpoptions=new std::list<std::string>();
  mpoptions->push_back("playout");
  mpoptions->push_back("1-ply");
  mpoptions->push_back("uct");
  params->addParameter("mcts","move_policy",&(params->move_policy_string),mpoptions,"uct",&Engine::updateParameterWrapper,this);
  params->move_policy=Parameters::MP_UCT;
  
  params->addParameter("mcts","playouts_per_move",&(params->playouts_per_move),PLAYOUTS_PER_MOVE);
  params->addParameter("mcts","playouts_per_move_max",&(params->playouts_per_move_max),PLAYOUTS_PER_MOVE_MAX);
  params->addParameter("mcts","playouts_per_move_min",&(params->playouts_per_move_min),PLAYOUTS_PER_MOVE_MIN);
  
  params->addParameter("mcts","playout_atari_enabled",&(params->playout_atari_enabled),PLAYOUT_ATARI_ENABLED);
  params->addParameter("mcts","playout_patterns_enabled",&(params->playout_patterns_enabled),PLAYOUT_PATTERNS_ENABLED);
  params->addParameter("mcts","playout_features_enabled",&(params->playout_features_enabled),PLAYOUT_FEATURES_ENABLED);
  
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
  
  params->addParameter("mcts","surewin_threshold",&(params->surewin_threshold),SUREWIN_THRESHOLD);
  params->surewin_expected=false;
  
  params->addParameter("mcts","resign_ratio_threshold",&(params->resign_ratio_threshold),RESIGN_RATIO_THRESHOLD);
  params->addParameter("mcts","resign_move_factor_threshold",&(params->resign_move_factor_threshold),RESIGN_MOVE_FACTOR_THRESHOLD);
  
  params->addParameter("time","time_k",&(params->time_k),TIME_K);
  params->addParameter("time","time_buffer",&(params->time_buffer),TIME_BUFFER);
  params->addParameter("time","time_move_minimum",&(params->time_move_minimum),TIME_MOVE_MINIMUM);
  
  params->addParameter("other","live_gfx",&(params->livegfx_on),LIVEGFX_ON);
  params->addParameter("other","live_gfx_update_playouts",&(params->livegfx_update_playouts),LIVEGFX_UPDATE_PLAYOUTS);
  params->addParameter("other","live_gfx_delay",&(params->livegfx_delay),LIVEGFX_DELAY);
  
  params->addParameter("other","outputsgf_maxchildren",&(params->outputsgf_maxchildren),OUTPUTSGF_MAXCHILDREN);
  
  params->addParameter("other","debug",&(params->debug_on),DEBUG_ON);
  
  params->addParameter("other","features_only_small",&(params->features_only_small),false);
  params->addParameter("other","features_output_competitions",&(params->features_output_competitions),false);
  params->addParameter("other","features_output_competitions_mmstyle",&(params->features_output_competitions_mmstyle),false);
  params->addParameter("other","features_ordered_comparison",&(params->features_ordered_comparison),false);
  
  patterntable=new Pattern::ThreeByThreeTable();
  patterntable->loadPatternDefaults();
  
  features=new Features(params);
  
  time_main=0;
  time_black=0;
  time_white=0;
  
  lastexplanation="";
  
  this->addGtpCommands();
  
  movetree=NULL;
  this->clearMoveTree();
}

Engine::~Engine()
{
  delete features;
  delete patterntable;
  if (movetree!=NULL)
    delete movetree;
  delete currentboard;
  delete params;
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
}

void Engine::addGtpCommands()
{
  gtpe->addFunctionCommand("boardsize",this,&Engine::gtpBoardSize);
  gtpe->addFunctionCommand("clear_board",this,&Engine::gtpClearBoard);
  gtpe->addFunctionCommand("komi",this,&Engine::gtpKomi);
  gtpe->addFunctionCommand("play",this,&Engine::gtpPlay);
  gtpe->addFunctionCommand("genmove",this,&Engine::gtpGenMove);
  gtpe->addFunctionCommand("reg_genmove",this,&Engine::gtpRegGenMove);
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
  
  gtpe->addFunctionCommand("time_settings",this,&Engine::gtpTimeSettings);
  gtpe->addFunctionCommand("time_left",this,&Engine::gtpTimeLeft);
  
  gtpe->addFunctionCommand("donplayouts",this,&Engine::gtpDoNPlayouts);
  gtpe->addFunctionCommand("outputsgf",this,&Engine::gtpOutputSGF);
  
  gtpe->addFunctionCommand("explainlastmove",this,&Engine::gtpExplainLastMove);
  gtpe->addFunctionCommand("boardstats",this,&Engine::gtpBoardStats);
  gtpe->addFunctionCommand("showsymmetrytransforms",this,&Engine::gtpShowSymmetryTransforms);
  gtpe->addFunctionCommand("showtreelivegfx",this,&Engine::gtpShowTreeLiveGfx);
  gtpe->addFunctionCommand("describeengine",this,&Engine::gtpDescribeEngine);
  
  gtpe->addAnalyzeCommand("final_score","Final Score","string");
  gtpe->addAnalyzeCommand("showboard","Show Board","string");
  gtpe->addAnalyzeCommand("boardstats","Board Stats","string");
  gtpe->addAnalyzeCommand("showsymmetrytransforms","Show Symmetry Transforms","sboard");
  gtpe->addAnalyzeCommand("showliberties","Show Liberties","sboard");
  gtpe->addAnalyzeCommand("showvalidmoves","Show Valid Moves","sboard");
  gtpe->addAnalyzeCommand("showgroupsize","Show Group Size","sboard");
  gtpe->addAnalyzeCommand("showpatternmatches","Show Pattern Matches","sboard");
  gtpe->addAnalyzeCommand("featurematchesat %%p","Feature Matches At","string");
  gtpe->addAnalyzeCommand("featureprobdistribution","Feature Probability Distribution","cboard");
  gtpe->addAnalyzeCommand("loadfeaturegammas %%r","Load Feature Gammas","none");
  gtpe->addAnalyzeCommand("showtreelivegfx","Show Tree Live Gfx","gfx");
  gtpe->addAnalyzeCommand("loadpatterns %%r","Load Patterns","none");
  gtpe->addAnalyzeCommand("clearpatterns","Clear Patterns","none");
  gtpe->addAnalyzeCommand("doboardcopy","Do Board Copy","none");
  gtpe->addAnalyzeCommand("param mcts","Parameters (MCTS)","param");
  gtpe->addAnalyzeCommand("param time","Parameters (Time)","param");
  gtpe->addAnalyzeCommand("param other","Parameters (Other)","param");
  gtpe->addAnalyzeCommand("donplayouts %%s","Do N Playouts","none");
  gtpe->addAnalyzeCommand("donplayouts 1","Do 1 Playout","none");
  gtpe->addAnalyzeCommand("outputsgf %%w","Output SGF","none");
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
  me->time_black=me->time_main;
  me->time_white=me->time_main;
  
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
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printf("Feature Matches for %s:\n",Go::Position::pos2string(pos,board->getSize()).c_str());
  gtpe->getOutput()->printf("PASS:            %u\n",me->features->matchFeatureClass(Features::PASS,board,move));
  gtpe->getOutput()->printf("CAPTURE:         %u\n",me->features->matchFeatureClass(Features::CAPTURE,board,move));
  gtpe->getOutput()->printf("EXTENSION:       %u\n",me->features->matchFeatureClass(Features::EXTENSION,board,move));
  gtpe->getOutput()->printf("SELFATARI:       %u\n",me->features->matchFeatureClass(Features::SELFATARI,board,move));
  gtpe->getOutput()->printf("ATARI:           %u\n",me->features->matchFeatureClass(Features::ATARI,board,move));
  gtpe->getOutput()->printf("BORDERDIST:      %u\n",me->features->matchFeatureClass(Features::BORDERDIST,board,move));
  gtpe->getOutput()->printf("LASTDIST:        %u\n",me->features->matchFeatureClass(Features::LASTDIST,board,move));
  gtpe->getOutput()->printf("SECONDLASTDIST:  %u\n",me->features->matchFeatureClass(Features::SECONDLASTDIST,board,move));
  gtpe->getOutput()->printf("PATTERN3X3:      0x%04x\n",me->features->matchFeatureClass(Features::PATTERN3X3,board,move));
  float gamma=me->features->getMoveGamma(board,move);
  float total=me->features->getBoardGamma(board,col);
  gtpe->getOutput()->printf("Gamma: %.2f/%.2f (%.2f)\n",gamma,total,gamma/total);
  gtpe->getOutput()->endResponse(true);
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
  gtpe->getOutput()->printf(PACKAGE_NAME " : " PACKAGE_VERSION " (" __DATE__ " " __TIME__ ")\n");
  gtpe->getOutput()->printf("parameters:\n");
  me->params->printParametersForDescription(gtpe);
  gtpe->getOutput()->endResponse(true);
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
  
  if ((cmd->getIntArg(1)!=0 && cmd->getIntArg(2)==0) || (cmd->getIntArg(0)==0 && cmd->getIntArg(1)==0 && cmd->getIntArg(2)==0))
  {
    me->time_main=0;
    me->time_black=0;
    me->time_white=0;
    gtpe->getOutput()->startResponse(cmd);
    gtpe->getOutput()->endResponse();
    return;
  }
  
  if (cmd->getIntArg(1)!=0)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("only absolute time supported");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  me->time_main=cmd->getIntArg(0);
  me->time_black=me->time_main;
  me->time_white=me->time_main;
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->endResponse();
}

void Engine::gtpTimeLeft(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  if (cmd->numArgs()!=3)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("invalid arguments");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  if (cmd->getIntArg(2)!=0)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("only absolute time supported");
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
  
  float time=(float)cmd->getIntArg(1);
  float *time_var=(gtpcol==Gtp::BLACK ? &me->time_black : &me->time_white);
  gtpe->getOutput()->printfDebug("[time_left]: diff:%.3f\n",time-*time_var);
  *time_var=time;
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->endResponse();
}

void Engine::generateMove(Go::Color col, Go::Move **move, bool playmove)
{
  if (params->move_policy==Parameters::MP_UCT || params->move_policy==Parameters::MP_ONEPLY)
  {
    float *time_left=(col==Go::BLACK ? &time_black : &time_white);
    boost::timer timer;
    int totalplayouts=0;
    int livegfxupdate=0;
    float time_allocated;
    float playouts_per_milli;
    
    if (*time_left>0)
      time_allocated=this->getTimeAllowedThisTurn(col);
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
        break;
      }
      
      this->doPlayout(firstlist,secondlist);
      totalplayouts++;
      
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
    
    UCT::Tree *besttree=this->getBestMoves(movetree,false);
    float bestratio=0;
    if (besttree==NULL)
      *move=new Go::Move(col,Go::Move::RESIGN);
    else if (besttree->getRatio()<params->resign_ratio_threshold && currentboard->getMovesMade()>(params->resign_move_factor_threshold*boardsize*boardsize))
      *move=new Go::Move(col,Go::Move::RESIGN);
    else
    {
      *move=new Go::Move(col,besttree->getMove().getPosition());
      bestratio=besttree->getRatio();
    }
    
    if (playmove)
      this->makeMove(**move);
    
    if (params->livegfx_on)
      gtpe->getOutput()->printfDebug("gogui-gfx: CLEAR\n");
    
    float time_used=timer.elapsed();
    if (time_used>0)
      playouts_per_milli=(float)totalplayouts/(time_used*1000);
    else
      playouts_per_milli=-1;
    if (*time_left!=0)
    {
      *time_left-=time_used;
      if (*time_left<=0)
        *time_left=1;
    }
    
    std::ostringstream ss;
    ss << std::fixed;
    ss << "r:"<<std::setprecision(2)<<bestratio;
    if (*time_left>0)
      ss << " tl:"<<std::setprecision(3)<<*time_left;
    if (*time_left>0 || movetree->isTerminalResult())
      ss << " plts:"<<totalplayouts;
    ss << " ppms:"<<std::setprecision(2)<<playouts_per_milli;
    params->surewin_expected=(bestratio>=params->surewin_threshold);
    if (params->surewin_expected)
      ss << " surewin!";
    lastexplanation=ss.str();
    
    gtpe->getOutput()->printfDebug("[genmove]: %s\n",lastexplanation.c_str());
    if (params->livegfx_on)
      gtpe->getOutput()->printfDebug("gogui-gfx: TEXT [genmove]: %s\n",lastexplanation.c_str());
  }
  else
  {
    int *posarray = new int[currentboard->getPositionMax()];
    *move=new Go::Move(col,Go::Move::PASS);
    this->randomPlayoutMove(currentboard,col,**move,posarray);
    delete[] posarray;
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
  if (params->uct_keep_subtree)
    this->chooseSubTree(move);
  else
    delete movetree;
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
  delete currentboard;
  currentboard = new Go::Board(boardsize);
  if (!params->uct_symmetry_use)
    currentboard->turnSymmetryOff();
  this->clearMoveTree();
  params->surewin_expected=false;
}

void Engine::randomPlayoutMove(Go::Board *board, Go::Color col, Go::Move &move, int *posarray)
{
  if (board->numOfValidMoves(col)==0)
  {
    move=Go::Move(col,Go::Move::PASS);
    return;
  }
  
  if (params->playout_features_enabled)
  {
    //Go::ObjectBoard<float> *gammas=new Go::ObjectBoard<float>(boardsize);
    //float totalgamma=features->getBoardGammas(board,col,gammas);
    float totalgamma=board->getFeatureTotalGamma();
    float randomgamma=totalgamma*rand.getRandomReal();
    bool foundmove=false;
    
    for (int p=0;p<board->getPositionMax();p++)
    {
      Go::Move m=Go::Move(col,p);
      if (board->validMove(m))
      {
        //float gamma=gammas->get(p);
        float gamma=board->getFeatureGamma(p);
        if (randomgamma<gamma)
        {
          move=m;
          foundmove=true;
          break;
        }
        else
          randomgamma-=gamma;
      }
    }
    
    if (!foundmove)
      move=Go::Move(col,Go::Move::PASS);
    //delete gammas;
    return;
  }
  
  if (params->playout_atari_enabled)
  {
    int *atarimoves=posarray;
    int atarimovescount=0;
    
    /*std::list<Go::Group*,Go::allocator_groupptr> *groups=board->getGroups();
    for(std::list<Go::Group*,Go::allocator_groupptr>::iterator iter=groups->begin();iter!=groups->end();++iter) 
    {
      if ((*iter)->inAtari())
      {
        int liberty=(*iter)->getAtariPosition();
        if (board->validMove(Go::Move(col,liberty)) && ((*iter)->getColor()!=col || board->touchingEmpty(liberty)>1))
        {
          atarimoves[atarimovescount]=liberty;
          atarimovescount++;
        }
      }
    }*/
    if (!board->getLastMove().isPass())
    {
      int size=board->getSize();
      foreach_adjacent(board->getLastMove().getPosition(),p,{
        if (board->inGroup(p))
        {
          Go::Group *group=board->getGroup(p);
          if (group!=NULL && group->inAtari())
          {
            int liberty=group->getAtariPosition();
            if (board->validMove(Go::Move(col,liberty)) && this->isAtariCaptureOrConnect(board,liberty,col,group))
            {
              atarimoves[atarimovescount]=liberty;
              atarimovescount++;
            }
          }
        }
      });
    }
    if (!board->getSecondLastMove().isPass())
    {
      int size=board->getSize();
      foreach_adjacent(board->getSecondLastMove().getPosition(),p,{
        if (board->inGroup(p))
        {
          Go::Group *group=board->getGroup(p);
          if (group!=NULL && group->inAtari())
          {
            int liberty=group->getAtariPosition();
            if (board->validMove(Go::Move(col,liberty)) && this->isAtariCaptureOrConnect(board,liberty,col,group))
            {
              atarimoves[atarimovescount]=liberty;
              atarimovescount++;
            }
          }
        }
      });
    }
    
    if (atarimovescount>0)
    {
      int i=rand.getRandomInt(atarimovescount);
      move=Go::Move(col,atarimoves[i]);
      return;
    }
  }
  
  if (params->playout_patterns_enabled)
  {
    int *patternmoves=posarray;
    int patternmovescount=0;
    
    if (!board->getLastMove().isPass() && !board->getLastMove().isResign())
    {
      int pos=board->getLastMove().getPosition();
      int size=board->getSize();
      
      foreach_adjdiag(pos,p,{
        if (board->validMove(Go::Move(col,p)) && !board->weakEye(col,p))
        {
          unsigned int pattern=Pattern::ThreeByThree::makeHash(board,p);
          if (col==Go::WHITE)
            pattern=Pattern::ThreeByThree::invert(pattern);
          
          if (patterntable->isPattern(pattern))
          {
            patternmoves[patternmovescount]=p;
            patternmovescount++;
          }
        }
      });
    }
    
    if (!board->getSecondLastMove().isPass() && !board->getSecondLastMove().isResign())
    {
      int pos=board->getSecondLastMove().getPosition();
      int size=board->getSize();
      
      foreach_adjdiag(pos,p,{
        if (board->validMove(Go::Move(col,p)) && !board->weakEye(col,p))
        {
          unsigned int pattern=Pattern::ThreeByThree::makeHash(board,p);
          if (col==Go::WHITE)
            pattern=Pattern::ThreeByThree::invert(pattern);
          
          if (patterntable->isPattern(pattern))
          {
            patternmoves[patternmovescount]=p;
            patternmovescount++;
          }
        }
      });
    }
    
    if (patternmovescount>0)
    {
      int i=rand.getRandomInt(patternmovescount);
      move=Go::Move(col,patternmoves[i]);
      return;
    }
  }
  
  for (int i=0;i<10;i++)
  {
    int p=rand.getRandomInt(board->getPositionMax());
    if (board->validMove(Go::Move(col,p)) && !board->weakEye(col,p))
    {
      move=Go::Move(col,p);
      return;
    }
  }
  
  Go::BitBoard *validmoves=board->getValidMoves(col);
  /*int *possiblemoves=posarray;
  int possiblemovescount=0;
  
  for (int p=0;p<board->getPositionMax();p++)
  {
    if (validmoves->get(p) && !board->weakEye(col,p))
    {
      possiblemoves[possiblemovescount]=p;
      possiblemovescount++;
    }
  }
  
  if (possiblemovescount==0)
    move=Go::Move(col,Go::Move::PASS);
  else if (possiblemovescount==1)
    move=Go::Move(col,possiblemoves[0]);
  else
  {
    int r=rand.getRandomInt(possiblemovescount);
    move=Go::Move(col,possiblemoves[r]);
  }*/
  
  int r=rand.getRandomInt(board->getPositionMax());
  int d=rand.getRandomInt(1)*2-1;
  for (int p=0;p<board->getPositionMax();p++)
  {
    int rp=(r+p*d);
    if (rp<0) rp+=board->getPositionMax();
    if (rp>=board->getPositionMax()) rp-=board->getPositionMax();
    if (validmoves->get(rp) && !board->weakEye(col,rp))
    {
      move=Go::Move(col,rp);
      return;
    }
  }
  move=Go::Move(col,Go::Move::PASS);
}

void Engine::randomPlayout(Go::Board *board, std::list<Go::Move> startmoves, Go::Color colfirst, Go::BitBoard *firstlist, Go::BitBoard *secondlist)
{
  if (board->getPassesPlayed()>=2)
    return;
  
  board->turnSymmetryOff();
  
  if (params->debug_on)
    gtpe->getOutput()->printfDebug("[playout]:");
  for(std::list<Go::Move>::iterator iter=startmoves.begin();iter!=startmoves.end();++iter)
  {
    board->makeMove((*iter));
    if (params->debug_on)
      gtpe->getOutput()->printfDebug(" %s",(*iter).toString(boardsize).c_str());
    if (((*iter).getColor()==colfirst?firstlist:secondlist)!=NULL && !(*iter).isPass() && !(*iter).isResign())
      ((*iter).getColor()==colfirst?firstlist:secondlist)->set((*iter).getPosition());
    if (board->getPassesPlayed()>=2 || (*iter).isResign())
    {
      if (params->debug_on)
        gtpe->getOutput()->printfDebug("\n");
      return;
    }
  }
  if (params->debug_on)
    gtpe->getOutput()->printfDebug("\n");
  
  Go::Color coltomove=board->nextToMove();
  Go::Move move=Go::Move(coltomove,Go::Move::PASS);
  int movesalready=board->getMovesMade();
  int *posarray = new int[board->getPositionMax()];
  while (board->getPassesPlayed()<2)
  {
    bool resign;
    this->randomPlayoutMove(board,coltomove,move,posarray);
    board->makeMove(move);
    if ((coltomove==colfirst?firstlist:secondlist)!=NULL && !move.isPass() && !move.isResign())
      (coltomove==colfirst?firstlist:secondlist)->set(move.getPosition());
    resign=move.isResign();
    coltomove=Go::otherColor(coltomove);
    if (resign)
      break;
    if (board->getMovesMade()>(boardsize*boardsize*PLAYOUT_MAX_MOVE_FACTOR+movesalready))
      break;
  }
  delete[] posarray;
}

float Engine::getTimeAllowedThisTurn(Go::Color col)
{
  float time_left=(col==Go::BLACK ? time_black : time_white);
  time_left-=params->time_buffer;
  float time_per_move=time_left/params->time_k; //allow much more time in beginning
  if (time_per_move<params->time_move_minimum)
    time_per_move=params->time_move_minimum;
  
  gtpe->getOutput()->printfDebug("[time_allowed]: %.3f\n",time_per_move);
  return time_per_move;
}

UCT::Tree *Engine::getPlayoutTarget(UCT::Tree *movetree)
{
  float besturgency=0;
  UCT::Tree *besttree=NULL;
  
  for(std::list<UCT::Tree*>::iterator iter=movetree->getChildren()->begin();iter!=movetree->getChildren()->end();++iter) 
  {
    if ((*iter)->isPrimary())
    {
      float urgency;
      
      if ((*iter)->getPlayouts()==0 && (*iter)->getRAVEPlayouts()==0 && (*iter)->getPriorPlayouts()==0)
        urgency=(*iter)->getUrgency()+(rand.getRandomReal()/1000);
      else
      {
        urgency=(*iter)->getUrgency();
        if (params->debug_on)
          gtpe->getOutput()->printfDebug("[urg]:%s %.3f %.2f(%d) %.2f(%d) %.2f(%d)\n",(*iter)->getMove().toString(boardsize).c_str(),urgency,(*iter)->getRatio(),(*iter)->getPlayouts(),(*iter)->getRAVERatio(),(*iter)->getRAVEPlayouts(),(*iter)->getPriorRatio(),(*iter)->getPriorPlayouts());
      }
          
      if (urgency>besturgency || besttree==NULL)
      {
        besttree=(*iter);
        besturgency=urgency;
      }
    }
  }
  
  if (params->debug_on)
  {
    if (besttree!=NULL)
      gtpe->getOutput()->printfDebug("[best]:%s\n",besttree->getMove().toString(boardsize).c_str());
    else
      gtpe->getOutput()->printfDebug("WARNING! No best move found\n");
  }
  
  if (besttree==NULL)
    return NULL;
  
  if (params->move_policy==Parameters::MP_UCT && besttree->isLeaf() && !besttree->isTerminal())
  {
    if (besttree->getPlayouts()>params->uct_expand_after)
      this->expandLeaf(besttree);
  }
  
  if (besttree->isLeaf())
    return besttree;
  else
    return this->getPlayoutTarget(besttree);
}

void Engine::expandLeaf(UCT::Tree *movetree)
{
  if (!movetree->isLeaf())
    return;
  else if (movetree->isTerminal())
    fprintf(stderr,"WARNING! Trying to expand a terminal node!\n");
  
  std::list<Go::Move> startmoves=movetree->getMovesFromRoot();
  Go::Board *startboard=currentboard->copy();
  
  for(std::list<Go::Move>::iterator iter=startmoves.begin();iter!=startmoves.end();++iter)
  {
    startboard->makeMove((*iter));
    if (startboard->getPassesPlayed()>=2 || (*iter).isResign())
    {
      delete startboard;
      fprintf(stderr,"WARNING! Trying to expand a terminal node?\n");
      return;
    }
  }
  
  Go::Color col=startboard->nextToMove();
  
  bool winnow=Go::Board::isWinForColor(col,startboard->score()-komi);
  UCT::Tree *nmt=new UCT::Tree(params,Go::Move(col,Go::Move::PASS));
  if (winnow)
    nmt->addWin();
  if (startboard->getPassesPlayed()==0 && !(params->surewin_expected && col==currentboard->nextToMove()))
    nmt->addPriorLoses(UCT_PASS_DETER);
  nmt->addRAVEWins(params->rave_init_wins);
  movetree->addChild(nmt);
  
  if (startboard->numOfValidMoves(col)>0)
  {
    Go::BitBoard *validmovesbitboard=startboard->getValidMoves(col);
    for (int p=0;p<startboard->getPositionMax();p++)
    {
      if (validmovesbitboard->get(p))
      {
        UCT::Tree *nmt=new UCT::Tree(params,Go::Move(col,p));
        if (params->uct_pattern_prior>0 && !startboard->weakEye(col,p))
        {
          unsigned int pattern=Pattern::ThreeByThree::makeHash(startboard,p);
          if (col==Go::WHITE)
            pattern=Pattern::ThreeByThree::invert(pattern);
          if (patterntable->isPattern(pattern))
            nmt->addPriorWins(params->uct_pattern_prior);
        }
        nmt->addRAVEWins(params->rave_init_wins);
        movetree->addChild(nmt);
      }
    }
    
    if (params->uct_atari_prior>0)
    {
      std::list<Go::Group*,Go::allocator_groupptr> *groups=startboard->getGroups();
      for(std::list<Go::Group*,Go::allocator_groupptr>::iterator iter=groups->begin();iter!=groups->end();++iter) 
      {
        if ((*iter)->inAtari())
        {
          int liberty=(*iter)->getAtariPosition();
          if (startboard->validMove(Go::Move(col,liberty)) && this->isAtariCaptureOrConnect(startboard,liberty,col,(*iter)))
          {
            UCT::Tree *mt=movetree->getChild(Go::Move(col,liberty));
            if (mt!=NULL)
              mt->addPriorWins(params->uct_atari_prior);
          }
        }
      }
    }
  }
  
  if (params->uct_symmetry_use)
  {
    Go::Board::Symmetry sym=startboard->getSymmetry();
    if (sym!=Go::Board::NONE)
    {
      for(std::list<UCT::Tree*>::iterator iter=movetree->getChildren()->begin();iter!=movetree->getChildren()->end();++iter) 
      {
        int pos=(*iter)->getMove().getPosition();
        Go::Board::SymmetryTransform trans=startboard->getSymmetryTransformToPrimary(sym,pos);
        int primarypos=startboard->doSymmetryTransform(trans,pos);
        if (pos!=primarypos)
        {
          UCT::Tree *pmt=movetree->getChild(Go::Move(col,primarypos));
          if (pmt!=NULL)
          {
            if (!pmt->isPrimary())
              gtpe->getOutput()->printfDebug("WARNING! bad primary\n");
            (*iter)->setPrimary(pmt);
          }
          else
            gtpe->getOutput()->printfDebug("WARNING! missing primary\n");
        }
      }
    }
  }
  
  delete startboard;
}

UCT::Tree *Engine::getBestMoves(UCT::Tree *movetree, bool descend)
{
  float bestsims=0;
  UCT::Tree *besttree=NULL;
  
  for(std::list<UCT::Tree*>::iterator iter=movetree->getChildren()->begin();iter!=movetree->getChildren()->end();++iter) 
  {
    if ((*iter)->getPlayouts()>bestsims || (*iter)->isTerminalWin() || besttree==NULL)
    {
      besttree=(*iter);
      bestsims=(*iter)->getPlayouts();
      if (besttree->isTerminalWin())
       break;
    }
  }
  
  if (besttree==NULL)
    return NULL;
  
  if (besttree->isLeaf() || !descend)
    return besttree;
  else
    return this->getBestMoves(besttree,descend);
}

void Engine::clearMoveTree()
{
  if (movetree!=NULL)
    delete movetree;
  
  movetree=new UCT::Tree(params);
}

void Engine::chooseSubTree(Go::Move move)
{
  UCT::Tree *subtree=movetree->getChild(move);
  
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
}

bool Engine::writeSGF(std::string filename, Go::Board *board, UCT::Tree *tree)
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
    
    Go::BitBoard *firstlist=new Go::BitBoard(boardsize);
    Go::BitBoard *secondlist=new Go::BitBoard(boardsize);
    
    for (int i=0;i<n;i++)
    {
      if (movetree->isTerminalResult())
      {
        gtpe->getOutput()->printfDebug("SOLVED! found 100%% sure result after %d plts!\n",i);
        break;
      }
      
      this->doPlayout(firstlist,secondlist);
      
      if (params->livegfx_on)
      {
        if (livegfxupdate>=(params->livegfx_update_playouts-1))
        {
          livegfxupdate=0;
          
          this->displayPlayoutLiveGfx(i);
          
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
  bool givenfirstlist,givensecondlist;
  Go::Color col=currentboard->nextToMove();
  
  if (movetree->isLeaf())
    this->expandLeaf(movetree);
  
  givenfirstlist=(firstlist==NULL);
  givensecondlist=(secondlist==NULL);
  
  UCT::Tree *playouttree = this->getPlayoutTarget(movetree);
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
  
  if (!givenfirstlist)
    firstlist=new Go::BitBoard(boardsize);
  if (!givensecondlist)
    secondlist=new Go::BitBoard(boardsize);
  
  Go::Board *playoutboard=currentboard->copy();
  playoutboard->turnSymmetryOff();
  if (params->playout_features_enabled)
    playoutboard->setFeatures(features);
  if (params->rave_moves>0)
  {
    firstlist->clear();
    secondlist->clear();
  }
  this->randomPlayout(playoutboard,playoutmoves,col,(params->rave_moves>0?firstlist:NULL),(params->rave_moves>0?secondlist:NULL));
  
  bool playoutwin=Go::Board::isWinForColor(playoutmoves.back().getColor(),playoutboard->score()-komi);
  if (playoutwin)
    playouttree->addWin();
  else
    playouttree->addLose();
  
  if (params->debug_on)
  {
    if (playoutwin && playoutmoves.back().getColor()==col)
      gtpe->getOutput()->printfDebug("[result]:win\n");
    else
      gtpe->getOutput()->printfDebug("[result]:lose\n");
  }
  
  if (params->rave_moves>0)
  {
    bool ravewin=Go::Board::isWinForColor(col,playoutboard->score()-komi);
    
    for (int p=0;p<playoutboard->getPositionMax();p++)
    {
      if (firstlist->get(p))
      {
        UCT::Tree *subtree=movetree->getChild(Go::Move(col,p));
        if (subtree!=NULL)
        {
          if (ravewin)
            subtree->addRAVEWin();
          else
            subtree->addRAVELose();
        }
      }
    }
    
    UCT::Tree *secondtree=movetree->getChild(playoutmoves.front());
    if (!secondtree->isLeaf())
    {
      for (int p=0;p<playoutboard->getPositionMax();p++)
      {
        if (secondlist->get(p))
        {
          UCT::Tree *subtree=secondtree->getChild(Go::Move(Go::otherColor(col),p));
          if (subtree!=NULL)
          {
            if (ravewin)
              subtree->addRAVELose();
            else
              subtree->addRAVEWin();
          }
        }
      }
    }
  }
  
  delete playoutboard;
  
  if (!givenfirstlist)
    delete firstlist;
  if (!givensecondlist)
    delete secondlist;
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
  for(std::list<UCT::Tree*>::iterator iter=movetree->getChildren()->begin();iter!=movetree->getChildren()->end();++iter) 
  {
    if ((*iter)->getPlayouts()>maxplayouts)
      maxplayouts=(*iter)->getPlayouts();
  }
  float colorfactor=(col==Go::BLACK?1:-1);
  for(std::list<UCT::Tree*>::iterator iter=movetree->getChildren()->begin();iter!=movetree->getChildren()->end();++iter) 
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
    UCT::Tree *besttree=this->getBestMoves(movetree,true);
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
    for(std::list<UCT::Tree*>::iterator iter=movetree->getChildren()->begin();iter!=movetree->getChildren()->end();++iter) 
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

bool Engine::isAtariCaptureOrConnect(Go::Board *board, int pos, Go::Color col, Go::Group *touchinggroup)
{
  int size=board->getSize();
  if (params->debug_on)
  {
    int postchgrp=touchinggroup->getPosition();
    gtpe->getOutput()->printfDebug("[ataricheck]: start for %s %c %s\n",Go::Position::pos2string(pos,board->getSize()).c_str(),(col==Go::BLACK?'B':'W'),Go::Position::pos2string(postchgrp,board->getSize()).c_str());
  }
  if (board->validMove(Go::Move(col,pos)))
  {
    if (touchinggroup->getColor()!=col || board->touchingEmpty(pos)>1)
      return true; //capture or definitely add a liberty
    else if (touchinggroup->getColor()==col)
    {
      foreach_adjacent(pos,p,{
        if (board->inGroup(p))
        {
          Go::Group *group=board->getGroup(p);
          if (params->debug_on && group!=NULL)
          {
            int posgrp=group->getPosition();
            gtpe->getOutput()->printfDebug("[ataricheck]: touching group %c %s %d\n",(group->getColor()==Go::BLACK?'B':'W'),Go::Position::pos2string(posgrp,board->getSize()).c_str(),group->inAtari());
          }
          if (group!=NULL && group->getColor()==col && group!=touchinggroup && !group->inAtari())
            return true; //connect to a group with another liberty at least
          else if (group!=NULL && group->getColor()!=col && group->inAtari())
            return true; //capture a group
        }
      });
    }
  }
  if (params->debug_on)
    gtpe->getOutput()->printfDebug("[ataricheck]: failed\n");
  return false;
}

