#include "Engine.h"

Engine::Engine(Gtp::Engine *ge)
{
  gtpe=ge;
  
  rand=Random(std::time(0));
  
  boardsize=9;
  currentboard=new Go::Board(boardsize);
  komi=5.5;
  
  movepolicy=Engine::MP_UCT;
  
  livegfx=LIVEGFX_ON;
  livegfxupdateplayouts=LIVEGFX_UPDATE_PLAYOUTS;
  livegfxdelay=LIVEGFX_DELAY;
  
  playoutspermilli=0;
  playoutspermove=PLAYOUTS_PER_MOVE;
  playoutspermoveinit=PLAYOUTS_PER_MOVE;
  playoutspermovemax=PLAYOUTS_PER_MOVE_MAX;
  playoutspermovemin=PLAYOUTS_PER_MOVE_MIN;
  playoutatarichance=PLAYOUT_ATARI_CHANCE;
  
  ucbc=UCB_C;
  ucbinit=UCB_INIT;
  
  ravemoves=RAVE_MOVES;
  
  uctexpandafter=UCT_EXPAND_AFTER;
  uctkeepsubtree=UCT_KEEP_SUBTREE;
  uctatarigamma=UCT_ATARI_GAMMA;
  uctpatterngamma=UCT_PATTERN_GAMMA;
  
  playoutpatternsenabled=PLAYOUT_PATTERNS_ENABLED;
  patterntable=new Pattern::ThreeByThreeTable();
  patterntable->loadPatternDefaults();
  
  resignratiothreshold=RESIGN_RATIO_THRESHOLD;
  resignmovefactorthreshold=RESIGN_MOVE_FACTOR_THRESHOLD;
  
  outputsgfmaxchildren=OUTPUTSGF_MAXCHILDREN;
  
  timebuffer=TIME_BUFFER;
  timek=TIME_K;
  timemoveminimum=TIME_MOVE_MINIMUM;
  
  timemain=0;
  timeblack=0;
  timewhite=0;
  
  lastexplanation="";
  
  debugon=DEBUG_ON;
  
  this->addGtpCommands();
  
  movetree=NULL;
  this->clearMoveTree();
}

Engine::~Engine()
{  
  delete patterntable;
  if (movetree!=NULL)
    delete movetree;
  delete currentboard;
}

void Engine::addGtpCommands()
{
  gtpe->addFunctionCommand("boardsize",this,&Engine::gtpBoardSize);
  gtpe->addFunctionCommand("clear_board",this,&Engine::gtpClearBoard);
  gtpe->addFunctionCommand("komi",this,&Engine::gtpKomi);
  gtpe->addFunctionCommand("play",this,&Engine::gtpPlay);
  gtpe->addFunctionCommand("genmove",this,&Engine::gtpGenMove);
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
  
  gtpe->addFunctionCommand("time_settings",this,&Engine::gtpTimeSettings);
  gtpe->addFunctionCommand("time_left",this,&Engine::gtpTimeLeft);
  
  gtpe->addFunctionCommand("donplayouts",this,&Engine::gtpDoNPlayouts);
  gtpe->addFunctionCommand("outputsgf",this,&Engine::gtpOutputSGF);
  
  gtpe->addFunctionCommand("explainlastmove",this,&Engine::gtpExplainLastMove);
  
  gtpe->addAnalyzeCommand("final_score","Final Score","string");
  gtpe->addAnalyzeCommand("showboard","Show Board","string");
  gtpe->addAnalyzeCommand("showliberties","Show Liberties","sboard");
  gtpe->addAnalyzeCommand("showvalidmoves","Show Valid Moves","sboard");
  gtpe->addAnalyzeCommand("showgroupsize","Show Group Size","sboard");
  gtpe->addAnalyzeCommand("showpatternmatches","Show Pattern Matches","sboard");
  gtpe->addAnalyzeCommand("loadpatterns %%r","Load Patterns","none");
  gtpe->addAnalyzeCommand("clearpatterns","Clear Patterns","none");
  gtpe->addAnalyzeCommand("doboardcopy","Do Board Copy","none");
  gtpe->addAnalyzeCommand("param","Parameters","param");
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
  me->playoutspermilli=0;
  me->playoutspermove=me->playoutspermoveinit;
  me->timeblack=me->timemain;
  me->timewhite=me->timemain;
  
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
  me->generateMove((gtpcol==Gtp::BLACK ? Go::BLACK : Go::WHITE),&move);
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

void Engine::gtpParam(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  if (cmd->numArgs()==0)
  {
    gtpe->getOutput()->startResponse(cmd);
    gtpe->getOutput()->printf("[list/playout/1-ply/uct] move_policy %s\n",(me->movepolicy==Engine::MP_UCT?"uct":(me->movepolicy==Engine::MP_ONEPLY?"1-ply":"playout")));
    gtpe->getOutput()->printf("[string] playouts_per_move %d\n",me->playoutspermove);
    gtpe->getOutput()->printf("[string] playouts_per_move_max %d\n",me->playoutspermovemax);
    gtpe->getOutput()->printf("[string] playouts_per_move_min %d\n",me->playoutspermovemin);
    gtpe->getOutput()->printf("[string] playout_atari_chance %.2f\n",me->playoutatarichance);
    gtpe->getOutput()->printf("[string] ucb_c %.2f\n",me->ucbc);
    gtpe->getOutput()->printf("[string] ucb_init %.2f\n",me->ucbinit);
    gtpe->getOutput()->printf("[string] rave_moves %d\n",me->ravemoves);
    gtpe->getOutput()->printf("[string] uct_expand_after %d\n",me->uctexpandafter);
    gtpe->getOutput()->printf("[bool] uct_keep_subtree %d\n",me->uctkeepsubtree);
    gtpe->getOutput()->printf("[string] uct_atari_gamma %d\n",me->uctatarigamma);
    gtpe->getOutput()->printf("[string] uct_pattern_gamma %d\n",me->uctpatterngamma);
    gtpe->getOutput()->printf("[bool] playout_patterns_enabled %d\n",me->playoutpatternsenabled);
    gtpe->getOutput()->printf("[string] resign_ratio_threshold %.3f\n",me->resignratiothreshold);
    gtpe->getOutput()->printf("[string] resign_move_factor_threshold %.2f\n",me->resignmovefactorthreshold);
    gtpe->getOutput()->printf("[string] time_k %.2f\n",me->timek);
    gtpe->getOutput()->printf("[string] time_buffer %ld\n",me->timebuffer);
    gtpe->getOutput()->printf("[string] time_move_minimum %ld\n",me->timemoveminimum);
    gtpe->getOutput()->printf("[bool] live_gfx %d\n",me->livegfx);
    gtpe->getOutput()->printf("[string] live_gfx_update_playouts %d\n",me->livegfxupdateplayouts);
    gtpe->getOutput()->printf("[string] live_gfx_delay %.3f\n",me->livegfxdelay);
    gtpe->getOutput()->printf("[string] output_sgf_max_children %d\n",me->outputsgfmaxchildren);
    gtpe->getOutput()->printf("[bool] debug %d\n",me->debugon);
    gtpe->getOutput()->endResponse(true);
  }
  else if (cmd->numArgs()==2)
  {
    std::string param=cmd->getStringArg(0);
    std::transform(param.begin(),param.end(),param.begin(),::tolower);
    if (param=="playouts_per_move")
    {
      me->playoutspermove=cmd->getIntArg(1);
      me->playoutspermoveinit=me->playoutspermove;
    }
    else if (param=="playouts_per_move_max")
      me->playoutspermovemax=cmd->getIntArg(1);
    else if (param=="playouts_per_move_min")
      me->playoutspermovemin=cmd->getIntArg(1);
    else if (param=="playout_atari_chance")
      me->playoutatarichance=cmd->getFloatArg(1);
    else if (param=="ucb_c")
      me->ucbc=cmd->getFloatArg(1);
    else if (param=="ucb_init")
      me->ucbinit=cmd->getFloatArg(1);
    else if (param=="rave_moves")
      me->ravemoves=cmd->getIntArg(1);
    else if (param=="uct_expand_after")
      me->uctexpandafter=cmd->getIntArg(1);
    else if (param=="uct_keep_subtree")
    {
      me->uctkeepsubtree=(cmd->getIntArg(1)==1);
      me->clearMoveTree();
    }
    else if (param=="uct_atari_gamma")
      me->uctatarigamma=cmd->getIntArg(1);
    else if (param=="uct_pattern_gamma")
      me->uctpatterngamma=cmd->getIntArg(1);
    else if (param=="playout_patterns_enabled")
      me->playoutpatternsenabled=(cmd->getIntArg(1)==1);
    else if (param=="live_gfx")
      me->livegfx=(cmd->getIntArg(1)==1);
    else if (param=="live_gfx_update_playouts")
      me->livegfxupdateplayouts=cmd->getIntArg(1);
    else if (param=="live_gfx_delay")
      me->livegfxdelay=cmd->getFloatArg(1);
    else if (param=="resign_ratio_threshold")
      me->resignratiothreshold=cmd->getFloatArg(1);
    else if (param=="resign_move_factor_threshold")
      me->resignmovefactorthreshold=cmd->getFloatArg(1);
    else if (param=="time_k")
      me->timek=cmd->getFloatArg(1);
    else if (param=="time_buffer")
      me->timebuffer=cmd->getIntArg(1);
    else if (param=="time_move_minimum")
      me->timemoveminimum=cmd->getIntArg(1);
    else if (param=="output_sgf_max_children")
      me->outputsgfmaxchildren=cmd->getIntArg(1);
    else if (param=="debug")
      me->debugon=(cmd->getIntArg(1)==1);
    else if (param=="move_policy")
    {
      std::string val=cmd->getStringArg(1);
      std::transform(val.begin(),val.end(),val.begin(),::tolower);
      if (val=="uct")
        me->movepolicy=Engine::MP_UCT;
      else if (val=="1-ply")
        me->movepolicy=Engine::MP_ONEPLY;
      else
        me->movepolicy=Engine::MP_PLAYOUT;
    }
    else
    {
      gtpe->getOutput()->startResponse(cmd,false);
      gtpe->getOutput()->printf("unknown parameter: %s",param.c_str());
      gtpe->getOutput()->endResponse();
      return;
    }
    gtpe->getOutput()->startResponse(cmd);
    gtpe->getOutput()->endResponse();
  }
  else
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printf("need 0 or 2 args");
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
    me->timemain=0;
    me->timeblack=0;
    me->timewhite=0;
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
  
  me->timemain=cmd->getIntArg(0)*1000;
  me->timeblack=me->timemain;
  me->timewhite=me->timemain;
  
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
  
  int time=cmd->getIntArg(1)*1000;
  long *timevar=(gtpcol==Gtp::BLACK ? &me->timeblack : &me->timewhite);
  gtpe->getOutput()->printfDebug("[time_left]: diff:%ld\n",time-*timevar);
  *timevar=time;
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->endResponse();
}

void Engine::generateMove(Go::Color col, Go::Move **move)
{
  if (movepolicy==Engine::MP_UCT || movepolicy==Engine::MP_ONEPLY)
  {
    long *timeleft=(col==Go::BLACK ? &timeblack : &timewhite);
    boost::timer timer;
    int totalplayouts=0;
    int livegfxupdate=0;
    long timeforthismove;
    
    if (*timeleft>0)
      timeforthismove=this->getTimeAllowedThisTurn(col);
    else
      timeforthismove=0;
    
    if (livegfx)
      gtpe->getOutput()->printfDebug("gogui-gfx: TEXT [genmove]: starting...\n");
    
    currentboard->setNextToMove(col);
    
    if (!uctkeepsubtree)
      this->clearMoveTree();
    
    if (movetree->isLeaf())
      this->expandLeaf(movetree);
    
    Go::BitBoard *firstlist=new Go::BitBoard(boardsize);
    Go::BitBoard *secondlist=new Go::BitBoard(boardsize);
    
    for (int i=0;i<playoutspermovemax;i++)
    {
      if (i>=playoutspermove && timeforthismove==0)
        break;
      else if (i>=playoutspermovemin && timeforthismove>0 && (timer.elapsed()*1000)>timeforthismove)
        break;
      
      UCT::Tree *playouttree = this->getPlayoutTarget(movetree);
      if (playouttree==NULL)
        break;
      std::list<Go::Move> playoutmoves=playouttree->getMovesFromRoot();
      if (playoutmoves.size()==0)
        break;
      
      Go::Board *playoutboard=currentboard->copy();
      if (ravemoves>0)
      {
        firstlist->clear();
        secondlist->clear();
      }
      this->randomPlayout(playoutboard,playoutmoves,col,(ravemoves>0?firstlist:NULL),(ravemoves>0?secondlist:NULL));
      totalplayouts++;
      
      bool playoutwin=Go::Board::isWinForColor(playoutmoves.back().getColor(),playoutboard->score()-komi);
      if (playoutwin)
        playouttree->addWin();
      else
        playouttree->addLose();
      
      if (ravemoves>0)
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
      
      if (livegfx)
      {
        if (livegfxupdate>=(livegfxupdateplayouts-1))
        {
          livegfxupdate=0;
          
          gtpe->getOutput()->printfDebug("gogui-gfx:\n");
          gtpe->getOutput()->printfDebug("TEXT [genmove]: thinking... playouts:%d\n",totalplayouts);
          
          gtpe->getOutput()->printfDebug("INFLUENCE");
          int maxplayouts=0;
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
              gtpe->getOutput()->printfDebug(" ");
              gtpe->getOutput()->printDebugVertex(vert);
              float playoutpercentage=(float)(*iter)->getPlayouts()/maxplayouts;
              if (playoutpercentage>1)
                playoutpercentage=1;
              gtpe->getOutput()->printfDebug(" %.2f",playoutpercentage*colorfactor);
            }
          }
          gtpe->getOutput()->printfDebug("\n");
          if (movepolicy==Engine::MP_UCT)
          {
            gtpe->getOutput()->printfDebug("VAR");
            std::list<Go::Move> bestmoves=this->getBestMoves(movetree,true)->getMovesFromRoot();
            for(std::list<Go::Move>::iterator iter=bestmoves.begin();iter!=bestmoves.end();++iter) 
            {
              if (!(*iter).isPass() && !(*iter).isResign())
              {
                Gtp::Vertex vert={(*iter).getX(boardsize),(*iter).getY(boardsize)};
                gtpe->getOutput()->printfDebug(" %c ",((*iter).getColor()==Go::BLACK?'B':'W'));
                gtpe->getOutput()->printDebugVertex(vert);
              }
            }
          }
          else
          {
            gtpe->getOutput()->printfDebug("SQUARE");
            for(std::list<UCT::Tree*>::iterator iter=movetree->getChildren()->begin();iter!=movetree->getChildren()->end();++iter) 
            {
              if (!(*iter)->getMove().isPass() && !(*iter)->getMove().isResign())
              {
                if ((*iter)->getPlayouts()==maxplayouts)
                {
                  Gtp::Vertex vert={(*iter)->getMove().getX(boardsize),(*iter)->getMove().getY(boardsize)};
                  gtpe->getOutput()->printfDebug(" ");
                  gtpe->getOutput()->printDebugVertex(vert);
                }
              }
            }
          }
          gtpe->getOutput()->printfDebug("\n\n");
          
          boost::timer delay;
          while (delay.elapsed()<livegfxdelay) {}
        }
        else
          livegfxupdate++;
      }
      
      delete playoutboard;
    }
    
    delete firstlist;
    delete secondlist;
    
    UCT::Tree *besttree=this->getBestMoves(movetree,false);
    float bestratio=0;
    if (besttree==NULL)
      *move=new Go::Move(col,Go::Move::RESIGN);
    else if (besttree->getRatio()<resignratiothreshold && currentboard->getMovesMade()>(resignmovefactorthreshold*boardsize*boardsize))
      *move=new Go::Move(col,Go::Move::RESIGN);
    else
    {
      *move=new Go::Move(col,besttree->getMove().getPosition());
      bestratio=besttree->getRatio();
    }
    
    this->makeMove(**move);
    
    if (livegfx)
      gtpe->getOutput()->printfDebug("gogui-gfx: CLEAR\n");
    
    long timeused=timer.elapsed()*1000;
    if (timeused>0)
      playoutspermilli=(float)totalplayouts/timeused;
    if (*timeleft!=0)
    {
      *timeleft-=timeused;
      if (*timeleft<0)
        *timeleft=1;
    }
    
    std::ostringstream ss;
    ss << "r:"<<bestratio<< " tl:"<<*timeleft<< " plts:"<<totalplayouts<< " ppms:"<<playoutspermilli;
    lastexplanation=ss.str();
    gtpe->getOutput()->printfDebug("[genmove]: r:%.2f tl:%ld plts:%d ppms:%.3f\n",bestratio,*timeleft,totalplayouts,playoutspermilli);
    if (livegfx)
      gtpe->getOutput()->printfDebug("gogui-gfx: TEXT [genmove]: r:%.2f tl:%ld plts:%d ppms:%.3f\n",bestratio,*timeleft,totalplayouts,playoutspermilli);
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
  currentboard->makeMove(move);
  if (uctkeepsubtree)
    this->chooseSubTree(move);
}

void Engine::setBoardSize(int s)
{
  if (s<BOARDSIZE_MIN || s>BOARDSIZE_MAX)
    return;
  
  boardsize=s;
  this->clearBoard();
}

void Engine::clearBoard()
{
  delete currentboard;
  currentboard = new Go::Board(boardsize);
  this->clearMoveTree();
}

void Engine::randomPlayoutMove(Go::Board *board, Go::Color col, Go::Move &move, int *posarray)
{
  if (board->numOfValidMoves(col)==0)
  {
    move=Go::Move(col,Go::Move::PASS);
    return;
  }
  
  if (playoutatarichance>0)
  {
    std::list<Go::Group*,Go::allocator_groupptr> *groups=board->getGroups();
    int *atarimoves=posarray;
    int atarimovescount=0;
    
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
    }
    
    if (atarimovescount>0 && playoutatarichance>rand.getRandomReal())
    {
      int i=rand.getRandomInt(atarimovescount);
      move=Go::Move(col,atarimoves[i]);
      return;
    }
  }
  
  if (playoutpatternsenabled)
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
  
  //fprintf(stderr,"[playout]:");
  for(std::list<Go::Move>::iterator iter=startmoves.begin();iter!=startmoves.end();++iter)
  {
    board->makeMove((*iter));
    //fprintf(stderr," %s",(*iter).toString(boardsize).c_str());
    if (((*iter).getColor()==colfirst?firstlist:secondlist)!=NULL && !(*iter).isPass() && !(*iter).isResign())
      ((*iter).getColor()==colfirst?firstlist:secondlist)->set((*iter).getPosition());
    if (board->getPassesPlayed()>=2 || (*iter).isResign())
      return;
  }
  //fprintf(stderr,"\n");
  
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

long Engine::getTimeAllowedThisTurn(Go::Color col)
{
  long timeleft=(col==Go::BLACK ? timeblack : timewhite);
  timeleft-=timebuffer;
  long timepermove=timeleft/timek; //allow much more time in beginning
  if (timepermove<timemoveminimum)
    timepermove=timemoveminimum;
  return timepermove;
}

UCT::Tree *Engine::getPlayoutTarget(UCT::Tree *movetree)
{
  float besturgency=0;
  UCT::Tree *besttree=NULL;
  
  for(std::list<UCT::Tree*>::iterator iter=movetree->getChildren()->begin();iter!=movetree->getChildren()->end();++iter) 
  {
    float urgency;
    
    if ((*iter)->getPlayouts()==0 && (*iter)->getRAVEPlayouts()==0)
      urgency=(*iter)->getUrgency()+(rand.getRandomReal()/1000);
    else
    {
      urgency=(*iter)->getUrgency();
      if (debugon)
        fprintf(stderr,"[urg]:%s %.3f %.2f(%d) %.2f(%d)\n",(*iter)->getMove().toString(boardsize).c_str(),urgency,(*iter)->getRatio(),(*iter)->getPlayouts(),(*iter)->getRAVERatio(),(*iter)->getRAVEPlayouts());
    }
    
    if (urgency>besturgency)
    {
      besttree=(*iter);
      besturgency=urgency;
    }
  }
  
  if (debugon)
    fprintf(stderr,"[best]:%s\n",besttree->getMove().toString(boardsize).c_str());
  
  if (besttree==NULL)
    return NULL;
  
  if (movepolicy==Engine::MP_UCT && besttree->isLeaf() && !besttree->isTerminal())
  {
    if (besttree->getPlayouts()>uctexpandafter)
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
  
  std::list<Go::Move> startmoves=movetree->getMovesFromRoot();
  Go::Board *startboard=currentboard->copy();
  
  for(std::list<Go::Move>::iterator iter=startmoves.begin();iter!=startmoves.end();++iter)
  {
    startboard->makeMove((*iter));
    if (startboard->getPassesPlayed()>=2 || (*iter).isResign())
    {
      delete startboard;
      return;
    }
  }
  
  Go::Color col=startboard->nextToMove();
  
  if (startboard->numOfValidMoves(col)==0 || Go::Board::isWinForColor(col,startboard->score()-komi))
  {
    UCT::Tree *nmt=new UCT::Tree(ucbc,ucbinit,ravemoves,Go::Move(col,Go::Move::PASS));
    nmt->addWin();
    movetree->addChild(nmt);
  }
  
  if (startboard->numOfValidMoves(col)>0)
  {
    Go::BitBoard *validmovesbitboard=startboard->getValidMoves(col);
    for (int p=0;p<startboard->getPositionMax();p++)
    {
      if (validmovesbitboard->get(p))
      {
        UCT::Tree *nmt=new UCT::Tree(ucbc,ucbinit,ravemoves,Go::Move(col,p));
        if (uctpatterngamma>0 && !startboard->weakEye(col,p))
        {
          unsigned int pattern=Pattern::ThreeByThree::makeHash(startboard,p);
          if (col==Go::WHITE)
            pattern=Pattern::ThreeByThree::invert(pattern);
          if (patterntable->isPattern(pattern))
            nmt->addRAVEWins(uctpatterngamma);
            //nmt->addNodeWins(uctpatterngamma);
        }
        movetree->addChild(nmt);
      }
    }
    
    if (uctatarigamma>0)
    {
      std::list<Go::Group*,Go::allocator_groupptr> *groups=startboard->getGroups();
      for(std::list<Go::Group*,Go::allocator_groupptr>::iterator iter=groups->begin();iter!=groups->end();++iter) 
      {
        if ((*iter)->inAtari())
        {
          int liberty=(*iter)->getAtariPosition();
          if (startboard->validMove(Go::Move(col,liberty)) && ((*iter)->getColor()!=col || startboard->touchingEmpty(liberty)>1))
          {
            UCT::Tree *mt=movetree->getChild(Go::Move(col,liberty));
            if (mt!=NULL)
              mt->addRAVEWins(uctatarigamma);
              //mt->addNodeWins(uctatarigamma);
          }
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
    if ((*iter)->getPlayouts()>bestsims)
    {
      besttree=(*iter);
      bestsims=(*iter)->getPlayouts();
    }
  }
  
  if (besttree==NULL)
    return NULL;
  
  if (besttree->isLeaf() || !descend)
    return besttree;
  else
    return this->getPlayoutTarget(besttree);
}

void Engine::clearMoveTree()
{
  if (movetree!=NULL)
    delete movetree;
  
  movetree=new UCT::Tree(ucbc,ucbinit,ravemoves);
}

void Engine::chooseSubTree(Go::Move move)
{
  UCT::Tree *subtree=movetree->getChild(move);
  
  if (subtree==NULL)
    this->clearMoveTree();
  else
  {
    movetree->divorceChild(subtree);
    delete movetree;
    movetree=subtree;
  }
}

bool Engine::writeSGF(std::string filename, Go::Board *board, UCT::Tree *tree)
{
  std::ofstream sgffile;
  sgffile.open(filename.c_str());
  sgffile<<"(;\nFF[4]SZ["<<boardsize<<"]KM["<<komi<<"]\n";
  sgffile<<board->toSGFString()<<"\n";
  if (tree!=NULL)
    sgffile<<tree->toSGFString(boardsize,outputsgfmaxchildren)<<"\n)";
  sgffile.close();
  
  return true;
}

void Engine::doNPlayouts(int n)
{
  if (movepolicy==Engine::MP_UCT || movepolicy==Engine::MP_ONEPLY)
  {
    int totalplayouts=0;
    int livegfxupdate=0;
    Go::Color col=currentboard->nextToMove();
    
    if (!uctkeepsubtree)
      this->clearMoveTree();
    
    if (movetree->isLeaf())
      this->expandLeaf(movetree);
    
    Go::BitBoard *firstlist=new Go::BitBoard(boardsize);
    Go::BitBoard *secondlist=new Go::BitBoard(boardsize);
    
    for (int i=0;i<n;i++)
    { 
      UCT::Tree *playouttree = this->getPlayoutTarget(movetree);
      if (playouttree==NULL)
        break;
      std::list<Go::Move> playoutmoves=playouttree->getMovesFromRoot();
      if (playoutmoves.size()==0)
        break;
      
      Go::Board *playoutboard=currentboard->copy();
      if (ravemoves>0)
      {
        firstlist->clear();
        secondlist->clear();
      }
      this->randomPlayout(playoutboard,playoutmoves,col,(ravemoves>0?firstlist:NULL),(ravemoves>0?secondlist:NULL));
      totalplayouts++;
      
      bool playoutwin=Go::Board::isWinForColor(playoutmoves.back().getColor(),playoutboard->score()-komi);
      if (playoutwin)
        playouttree->addWin();
      else
        playouttree->addLose();
      
      if (debugon)
      {
        if (playoutwin && playoutmoves.back().getColor()==col)
          fprintf(stderr,"[result]:win\n");
        else
          fprintf(stderr,"[result]:lose\n");
      }
      
      if (ravemoves>0)
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
      
      if (livegfx)
      {
        if (livegfxupdate>=(livegfxupdateplayouts-1))
        {
          livegfxupdate=0;
          
          gtpe->getOutput()->printfDebug("gogui-gfx:\n");
          gtpe->getOutput()->printfDebug("TEXT [genmove]: thinking... playouts:%d\n",totalplayouts);
          
          gtpe->getOutput()->printfDebug("INFLUENCE");
          int maxplayouts=0;
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
              gtpe->getOutput()->printfDebug(" ");
              gtpe->getOutput()->printDebugVertex(vert);
              float playoutpercentage=(float)(*iter)->getPlayouts()/maxplayouts;
              if (playoutpercentage>1)
                playoutpercentage=1;
              gtpe->getOutput()->printfDebug(" %.2f",playoutpercentage*colorfactor);
            }
          }
          gtpe->getOutput()->printfDebug("\n");
          if (movepolicy==Engine::MP_UCT)
          {
            gtpe->getOutput()->printfDebug("VAR");
            std::list<Go::Move> bestmoves=this->getBestMoves(movetree,true)->getMovesFromRoot();
            for(std::list<Go::Move>::iterator iter=bestmoves.begin();iter!=bestmoves.end();++iter) 
            {
              if (!(*iter).isPass() && !(*iter).isResign())
              {
                Gtp::Vertex vert={(*iter).getX(boardsize),(*iter).getY(boardsize)};
                gtpe->getOutput()->printfDebug(" %c ",((*iter).getColor()==Go::BLACK?'B':'W'));
                gtpe->getOutput()->printDebugVertex(vert);
              }
            }
          }
          else
          {
            gtpe->getOutput()->printfDebug("SQUARE");
            for(std::list<UCT::Tree*>::iterator iter=movetree->getChildren()->begin();iter!=movetree->getChildren()->end();++iter) 
            {
              if (!(*iter)->getMove().isPass() && !(*iter)->getMove().isResign())
              {
                if ((*iter)->getPlayouts()==maxplayouts)
                {
                  Gtp::Vertex vert={(*iter)->getMove().getX(boardsize),(*iter)->getMove().getY(boardsize)};
                  gtpe->getOutput()->printfDebug(" ");
                  gtpe->getOutput()->printDebugVertex(vert);
                }
              }
            }
          }
          gtpe->getOutput()->printfDebug("\n\n");
          
          boost::timer delay;
          while (delay.elapsed()<livegfxdelay) {}
        }
        else
          livegfxupdate++;
      }
      
      delete playoutboard;
    }
    
    delete firstlist;
    delete secondlist;
  }
}

