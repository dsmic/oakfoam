#include "Engine.h"

Engine::Engine(Gtp::Engine *ge)
{
  gtpe=ge;
  
  std::srand(std::time(0));
  
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
  ravemoves=RAVE_MOVES;
  uctexpandafter=UCT_EXPAND_AFTER;
  
  resignratiothreshold=RESIGN_RATIO_THRESHOLD;
  resignmovefactorthreshold=RESIGN_MOVE_FACTOR_THRESHOLD;
  
  timebuffer=TIME_BUFFER;
  timepercentageboard=TIME_PERCENTAGE_BOARD;
  timemovebuffer=TIME_MOVE_BUFFER;
  timefactor=TIME_FACTOR;
  timemoveminimum=TIME_MOVE_MINIMUM;
  
  timemain=0;
  timeblack=0;
  timewhite=0;
  
  this->addGtpCommands();
}

Engine::~Engine()
{
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
  
  gtpe->addFunctionCommand("time_settings",this,&Engine::gtpTimeSettings);
  gtpe->addFunctionCommand("time_left",this,&Engine::gtpTimeLeft);
  
  gtpe->addAnalyzeCommand("final_score","Final Score","string");
  gtpe->addAnalyzeCommand("showboard","Show Board","string");
  gtpe->addAnalyzeCommand("showliberties","Show Liberties","sboard");
  gtpe->addAnalyzeCommand("showvalidmoves","Show Valid Moves","sboard");
  gtpe->addAnalyzeCommand("param","Parameters","param");
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
  
  Go::Move move=Go::Move((gtpcol==Gtp::BLACK ? Go::BLACK : Go::WHITE),vert.x,vert.y);
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
  float ratio;
  me->generateMove((gtpcol==Gtp::BLACK ? Go::BLACK : Go::WHITE),&move,&ratio);
  
  Gtp::Vertex vert= {move->getX(),move->getY()};
  delete move;
  
  long timeleft=(gtpcol==Gtp::BLACK ? me->timeblack : me->timewhite);
  gtpe->getOutput()->printfDebug("[genmove]: r:%.2f tl:%ld ppmo:%d ppmi:%.3f\n",ratio,timeleft,me->playoutspermove,me->playoutspermilli);
  if (me->livegfx)
    gtpe->getOutput()->printfDebug("gogui-gfx: TEXT [genmove]: r:%.2f tl:%ld ppmo:%d ppmi:%.3f\n",ratio,timeleft,me->playoutspermove,me->playoutspermilli);
  
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
        if (me->currentboard->boardData()[y*me->boardsize+x].color!=Go::EMPTY)
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
      if (me->currentboard->boardData()[y*me->boardsize+x].group==NULL)
        gtpe->getOutput()->printf("\"\" ");
      else
      {
        int lib=me->currentboard->boardData()[y*me->boardsize+x].group->numOfLiberties();
        gtpe->getOutput()->printf("\"%d\" ",lib);
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
      if (me->currentboard->validMove(Go::Move(Go::BLACK,x,y)))
        gtpe->getOutput()->printf("B");
      if (me->currentboard->validMove(Go::Move(Go::WHITE,x,y)))
        gtpe->getOutput()->printf("W");
      gtpe->getOutput()->printf("\" ");
    }
    gtpe->getOutput()->printf("\n");
  }

  gtpe->getOutput()->endResponse(true);
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
    gtpe->getOutput()->printf("[string] rave_moves %d\n",me->ravemoves);
    gtpe->getOutput()->printf("[string] uct_expand_after %d\n",me->uctexpandafter);
    gtpe->getOutput()->printf("[string] resign_ratio_threshold %.3f\n",me->resignratiothreshold);
    gtpe->getOutput()->printf("[string] resign_move_factor_threshold %.2f\n",me->resignmovefactorthreshold);
    gtpe->getOutput()->printf("[string] time_buffer %ld\n",me->timebuffer);
    gtpe->getOutput()->printf("[string] time_percentage_board %.2f\n",me->timepercentageboard);
    gtpe->getOutput()->printf("[string] time_move_buffer %d\n",me->timemovebuffer);
    gtpe->getOutput()->printf("[string] time_factor %.2f\n",me->timefactor);
    gtpe->getOutput()->printf("[string] time_move_minimum %ld\n",me->timemoveminimum);
    gtpe->getOutput()->printf("[bool] live_gfx %d\n",me->livegfx);
    gtpe->getOutput()->printf("[string] live_gfx_update_playouts %d\n",me->livegfxupdateplayouts);
    gtpe->getOutput()->printf("[string] live_gfx_delay %.3f\n",me->livegfxdelay);
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
    else if (param=="rave_moves")
      me->ravemoves=cmd->getIntArg(1);
    else if (param=="uct_expand_after")
      me->uctexpandafter=cmd->getIntArg(1);
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
    else if (param=="time_buffer")
      me->timebuffer=cmd->getIntArg(1);
    else if (param=="time_percentage_board")
      me->timepercentageboard=cmd->getFloatArg(1);
    else if (param=="time_move_buffer")
      me->timemovebuffer=cmd->getIntArg(1);
    else if (param=="time_factor")
      me->timefactor=cmd->getFloatArg(1);
    else if (param=="time_move_minimum")
      me->timemoveminimum=cmd->getIntArg(1);
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

void Engine::generateMove(Go::Color col, Go::Move **move, float *ratio)
{
  if (movepolicy==Engine::MP_UCT || movepolicy==Engine::MP_ONEPLY)
  {
    long *timeleft=(col==Go::BLACK ? &timeblack : &timewhite);
    boost::timer timer;
    int totalplayouts=0;
    int livegfxupdate=0;
    
    if (*timeleft>0 && playoutspermilli>0)
    {
      playoutspermove=playoutspermilli*this->getTimeAllowedThisTurn(col);
      if (playoutspermove<playoutspermovemin)
        playoutspermove=playoutspermovemin;
      else if (playoutspermove>playoutspermovemax)
        playoutspermove=playoutspermovemax;
    }
    
    if (livegfx)
      gtpe->getOutput()->printfDebug("gogui-gfx: TEXT [genmove]: starting...\n");
    
    
    Util::MoveTree *movetree=new Util::MoveTree(ucbc,ravemoves);
    currentboard->setNextToMove(col);
    this->expandLeaf(movetree);
    
    Go::BitBoard *firstlist=new Go::BitBoard(boardsize);
    Go::BitBoard *secondlist=new Go::BitBoard(boardsize);
    
    for (int i=0;i<playoutspermove;i++)
    {
      Util::MoveTree *playouttree = this->getPlayoutTarget(movetree);
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
      
      bool playoutwin=Util::isWinForColor(playoutmoves.back().getColor(),playoutboard->score()-komi);
      if (playoutwin)
        playouttree->addWin();
      else
        playouttree->addLose();
      
      if (ravemoves>0)
      {
        for (int x=0;x<boardsize;x++)
        {
          for (int y=0;y<boardsize;y++)
          {
            if (firstlist->get(x,y))
            {
              Util::MoveTree *subtree=movetree->getChild(Go::Move(col,x,y));
              if (subtree!=NULL)
              {
                if (playoutwin)
                  subtree->addRAVEWin();
                else
                  subtree->addRAVELose();
              }
            }
          }
        }
        
        Util::MoveTree *secondtree=movetree->getChild(playoutmoves.front());
        if (!secondtree->isLeaf())
        {
          for (int x=0;x<boardsize;x++)
          {
            for (int y=0;y<boardsize;y++)
            {
              if (secondlist->get(x,y))
              {
                Util::MoveTree *subtree=secondtree->getChild(Go::Move(Go::otherColor(col),x,y));
                if (subtree!=NULL)
                {
                  if (playoutwin)
                    subtree->addRAVELose();
                  else
                    subtree->addRAVEWin();
                }
              }
            }
          }
        }
      }
      
      if (livegfx)
      {
        if (livegfxupdate>=livegfxupdateplayouts)
        {
          livegfxupdate=1; //for nice numbers
          
          gtpe->getOutput()->printfDebug("gogui-gfx:\n");
          gtpe->getOutput()->printfDebug("TEXT [genmove]: thinking... playouts:%d\n",totalplayouts);
          
          gtpe->getOutput()->printfDebug("INFLUENCE");
          int maxplayouts=0;
          for(std::list<Util::MoveTree*>::iterator iter=movetree->getChildren()->begin();iter!=movetree->getChildren()->end();++iter) 
          {
            if ((*iter)->getPlayouts()>maxplayouts)
              maxplayouts=(*iter)->getPlayouts();
          }
          float colorfactor=(col==Go::BLACK?1:-1);
          for(std::list<Util::MoveTree*>::iterator iter=movetree->getChildren()->begin();iter!=movetree->getChildren()->end();++iter) 
          {
            if (!(*iter)->getMove().isPass() && !(*iter)->getMove().isResign())
            {
              Gtp::Vertex vert={(*iter)->getMove().getX(),(*iter)->getMove().getY()};
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
                Gtp::Vertex vert={(*iter).getX(),(*iter).getY()};
                gtpe->getOutput()->printfDebug(" %c ",((*iter).getColor()==Go::BLACK?'B':'W'));
                gtpe->getOutput()->printDebugVertex(vert);
              }
            }
          }
          else
          {
            gtpe->getOutput()->printfDebug("SQUARE");
            for(std::list<Util::MoveTree*>::iterator iter=movetree->getChildren()->begin();iter!=movetree->getChildren()->end();++iter) 
            {
              if (!(*iter)->getMove().isPass() && !(*iter)->getMove().isResign())
              {
                if ((*iter)->getPlayouts()==maxplayouts)
                {
                  Gtp::Vertex vert={(*iter)->getMove().getX(),(*iter)->getMove().getY()};
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
    
    Util::MoveTree *besttree=this->getBestMoves(movetree,false);
    if (besttree==NULL)
      *move=new Go::Move(col,Go::Move::RESIGN);
    else if (besttree->getRatio()<resignratiothreshold && currentboard->getMovesMade()>(resignmovefactorthreshold*boardsize*boardsize))
      *move=new Go::Move(col,Go::Move::RESIGN);
    else
    {
      *move=new Go::Move(col,besttree->getMove().getX(),besttree->getMove().getY());
      if (ratio!=NULL)
        *ratio=besttree->getRatio();
    }
    
    this->makeMove(**move);
    
    delete movetree;
    
    if (livegfx)
      gtpe->getOutput()->printfDebug("gogui-gfx: CLEAR\n");
    
    long timeused=timer.elapsed()*1000;
    if (timeused>0)
      playoutspermilli=(float)totalplayouts/timeused;
    if (*timeleft!=0)
      *timeleft-=timeused;
  }
  else
  {
    this->randomPlayoutMove(currentboard,col,move);
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
}

void Engine::setBoardSize(int s)
{
  if (s<BOARDSIZE_MIN || s>BOARDSIZE_MAX)
    return;
  
  delete currentboard;
  currentboard = new Go::Board(s);
  boardsize=s;
}

void Engine::clearBoard()
{
  delete currentboard;
  currentboard = new Go::Board(boardsize);
}

void Engine::randomPlayoutMove(Go::Board *board, Go::Color col, Go::Move **move)
{
  std::vector<Go::Move> atarimoves;
  std::list<Go::Board::Group*> *groups=board->getGroups();
  
  for(std::list<Go::Board::Group*>::iterator iter=groups->begin();iter!=groups->end();++iter) 
  {
    if ((*iter)->numOfLiberties()==1)
    {
      Go::Board::Vertex *liberty=(*iter)->getLiberties()->front();
      if (board->validMove(Go::Move(col,liberty->point.x,liberty->point.y)))
        atarimoves.push_back(Go::Move(col,liberty->point.x,liberty->point.y));
    }
  }
  
  if (atarimoves.size()>0 && playoutatarichance>(std::rand()/((double)RAND_MAX+1)))
  {
    int i=(int)(std::rand()/((double)RAND_MAX+1)*atarimoves.size());
    *move=new Go::Move(col,atarimoves.at(i).getX(),atarimoves.at(i).getY());
    return;
  }
  
  for (int i=0;i<5;i++)
  {
    int x=(int)(std::rand()/((double)RAND_MAX+1)*boardsize);
    int y=(int)(std::rand()/((double)RAND_MAX+1)*boardsize);
    if (board->validMove(Go::Move(col,x,y)) && !board->weakEye(col,x,y))
    {
      *move=new Go::Move(col,x,y);
      return;
    }
  }
  
  std::vector<Go::Move> validmoves=this->getValidMoves(board,col);
  
  if (validmoves.size()==0)
    *move=new Go::Move(col,Go::Move::PASS);
  else
  {
    int i=(int)(std::rand()/((double)RAND_MAX+1)*validmoves.size());
    *move=new Go::Move(col,validmoves.at(i).getX(),validmoves.at(i).getY());
  }
}

void Engine::randomPlayout(Go::Board *board, std::list<Go::Move> startmoves, Go::Color colfirst, Go::BitBoard *firstlist, Go::BitBoard *secondlist)
{
  if (board->getPassesPlayed()>=2)
    return;
  
  for(std::list<Go::Move>::iterator iter=startmoves.begin();iter!=startmoves.end();++iter)
  {
    board->makeMove((*iter));
    if (((*iter).getColor()==colfirst?firstlist:secondlist)!=NULL && !(*iter).isPass() && !(*iter).isResign())
      ((*iter).getColor()==colfirst?firstlist:secondlist)->set((*iter).getX(),(*iter).getY());
    if (board->getPassesPlayed()>=2 || (*iter).isResign())
      return;
  }
  
  Go::Move *move;
  Go::Color coltomove=board->nextToMove();
  while (board->getPassesPlayed()<2)
  {
    bool resign;
    this->randomPlayoutMove(board,coltomove,&move);
    board->makeMove(*move);
    if ((coltomove==colfirst?firstlist:secondlist)!=NULL && !move->isPass() && !move->isResign())
      (coltomove==colfirst?firstlist:secondlist)->set(move->getX(),move->getY());
    resign=move->isResign();
    delete move;
    coltomove=Go::otherColor(coltomove);
    if (resign)
      break;
    if (board->getMovesMade()>(boardsize*boardsize*PLAYOUT_MAX_MOVE_FACTOR))
      break;
  }
}

long Engine::getTimeAllowedThisTurn(Go::Color col)
{
  long timeleft=(col==Go::BLACK ? timeblack : timewhite);
  timeleft-=timebuffer;
  if (timeleft<0)
    timeleft=1;
  int estimatedmovespergame=(float)boardsize*boardsize*timepercentageboard; //fill only some of the board
  int movesmade=currentboard->getMovesMade();
  int movesleft=estimatedmovespergame-movesmade;
  if (movesleft<0)
    movesleft=0;
  movesleft+=timemovebuffer; //be able to play more moves
  long timepermove=timeleft/movesleft*timefactor; //allow more time in beginning
  if (timepermove<timemoveminimum)
    timepermove=timemoveminimum;
  return timepermove;
}

std::vector<Go::Move> Engine::getValidMoves(Go::Board *board, Go::Color col)
{
  std::vector<Go::Move> validmovesvector;
  Go::BitBoard *validmovesbitboard;
  
  validmovesbitboard=board->getValidMoves(col);
  for (int x=0;x<boardsize;x++)
  {
    for (int y=0;y<boardsize;y++)
    {
      if (validmovesbitboard->get(x,y) && !board->weakEye(col,x,y))
        validmovesvector.push_back(Go::Move(col,x,y));
    }
  }
  
  return validmovesvector;
}

Util::MoveTree *Engine::getPlayoutTarget(Util::MoveTree *movetree)
{
  float besturgency=0;
  Util::MoveTree *besttree=NULL;
  
  for(std::list<Util::MoveTree*>::iterator iter=movetree->getChildren()->begin();iter!=movetree->getChildren()->end();++iter) 
  {
    float urgency;
    
    if ((*iter)->getPlayouts()==0 && (*iter)->getRAVEPlayouts()==0)
      urgency=(*iter)->getUrgency()+(std::rand()/((double)RAND_MAX+1)*10);
    else
      urgency=(*iter)->getUrgency();
    
    if (urgency>besturgency)
    {
      besttree=(*iter);
      besturgency=urgency;
    }
  }
  
  if (besttree==NULL)
    return NULL;
  
  if (movepolicy==Engine::MP_UCT && besttree->isLeaf())
  {
    if (besttree->getPlayouts()>uctexpandafter)
      this->expandLeaf(besttree);
  }
  
  if (besttree->isLeaf())
    return besttree;
  else
    return this->getPlayoutTarget(besttree);
}

void Engine::expandLeaf(Util::MoveTree *movetree)
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
  
  if (Util::isWinForColor(col,startboard->score()-komi))
  {
    Util::MoveTree *nmt=new Util::MoveTree(ucbc,ravemoves,Go::Move(col,Go::Move::PASS));
    nmt->addWin();
    movetree->addChild(nmt);
  }
  
  std::vector<Go::Move> validmoves=this->getValidMoves(startboard,col);
  for (int i=0;i<(int)validmoves.size();i++)
  {
    Util::MoveTree *nmt=new Util::MoveTree(ucbc,ravemoves,validmoves.at(i));
    movetree->addChild(nmt);
  }
  
  delete startboard;
}

Util::MoveTree *Engine::getBestMoves(Util::MoveTree *movetree, bool descend)
{
  float bestsims=0;
  Util::MoveTree *besttree=NULL;
  
  for(std::list<Util::MoveTree*>::iterator iter=movetree->getChildren()->begin();iter!=movetree->getChildren()->end();++iter) 
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

