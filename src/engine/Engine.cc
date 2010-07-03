#include "Engine.h"

Engine::Engine(Gtp::Engine *ge)
{
  gtpe=ge;
  
  std::srand(std::time(0));
  
  boardsize=9;
  currentboard=new Go::Board(boardsize);
  komi=5.5;
  
  livegfx=LIVEGFX_ON;
  
  playoutspermilli=0;
  playoutspermove=PLAYOUTS_PER_MOVE;
  playoutspermoveinit=PLAYOUTS_PER_MOVE;
  playoutspermovemax=PLAYOUTS_PER_MOVE_MAX;
  playoutspermovemin=PLAYOUTS_PER_MOVE_MIN;
  playoutatarichance=PLAYOUT_ATARI_CHANCE;
  
  ucbc=UCB_C;
  ravemoves=RAVE_MOVES;
  
  resignratiothreshold=RESIGN_RATIO_THRESHOLD;
  resignmeanthreshold=RESIGN_MEAN_THRESHOLD;
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
  float ratio,mean;
  me->generateMove((gtpcol==Gtp::BLACK ? Go::BLACK : Go::WHITE),&move,&ratio,&mean);
  
  Gtp::Vertex vert= {move->getX(),move->getY()};
  delete move;
  
  long timeleft=(gtpcol==Gtp::BLACK ? me->timeblack : me->timewhite);
  gtpe->getOutput()->printfDebug("[genmove]: r:%.2f m:%+.1f tl:%ld ppmo:%d ppmi:%.3f\n",ratio,mean,timeleft,me->playoutspermove,me->playoutspermilli);
  if (me->livegfx)
    gtpe->getOutput()->printfDebug("gogui-gfx: TEXT [genmove]: r:%.2f m:%+.1f tl:%ld ppmo:%d ppmi:%.3f\n",ratio,mean,timeleft,me->playoutspermove,me->playoutspermilli);
  
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
    gtpe->getOutput()->printf("[bool] live_gfx %d\n",me->livegfx);
    gtpe->getOutput()->printf("[string] playouts_per_move %d\n",me->playoutspermove);
    gtpe->getOutput()->printf("[string] playouts_per_move_max %d\n",me->playoutspermovemax);
    gtpe->getOutput()->printf("[string] playouts_per_move_min %d\n",me->playoutspermovemin);
    gtpe->getOutput()->printf("[string] playout_atari_chance %.2f\n",me->playoutatarichance);
    gtpe->getOutput()->printf("[string] ucb_c %.2f\n",me->ucbc);
    gtpe->getOutput()->printf("[string] rave_moves %d\n",me->ravemoves);
    gtpe->getOutput()->printf("[string] resign_ratio_threshold %.3f\n",me->resignratiothreshold);
    gtpe->getOutput()->printf("[string] resign_mean_threshold %.1f\n",me->resignmeanthreshold);
    gtpe->getOutput()->printf("[string] resign_move_factor_threshold %.2f\n",me->resignmovefactorthreshold);
    gtpe->getOutput()->printf("[string] time_buffer %ld\n",me->timebuffer);
    gtpe->getOutput()->printf("[string] time_percentage_board %.2f\n",me->timepercentageboard);
    gtpe->getOutput()->printf("[string] time_move_buffer %d\n",me->timemovebuffer);
    gtpe->getOutput()->printf("[string] time_factor %.2f\n",me->timefactor);
    gtpe->getOutput()->printf("[string] time_move_minimum %ld\n",me->timemoveminimum);
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
    else if (param=="live_gfx")
      me->livegfx=(cmd->getIntArg(1)==1);
    else if (param=="resign_ratio_threshold")
      me->resignratiothreshold=cmd->getFloatArg(1);
    else if (param=="resign_mean_threshold")
      me->resignmeanthreshold=cmd->getFloatArg(1);
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

void Engine::generateMove(Go::Color col, Go::Move **move, float *ratio, float *mean)
{
  long *timeleft=(col==Go::BLACK ? &timeblack : &timewhite);
  long timestart=getCurrentTime();
  int totalplayouts=0;
  
  if (*timeleft>0 && playoutspermilli>0)
  {
    playoutspermove=playoutspermilli*this->getTimeAllowedThisTurn(col);
    if (playoutspermove<PLAYOUTS_PER_MOVE_MIN)
      playoutspermove=PLAYOUTS_PER_MOVE_MIN;
    else if (playoutspermove>PLAYOUTS_PER_MOVE_MAX)
      playoutspermove=PLAYOUTS_PER_MOVE_MAX;
  }
  
  Util::MoveTree *movetree;
  
  movetree=new Util::MoveTree();
  
  if (livegfx)
    gtpe->getOutput()->printfDebug("gogui-gfx: TEXT [genmove]: starting...\n");
  
  Go::Board *passboard=currentboard->copy();
  passboard->makeMove(Go::Move(col,Go::Move::PASS));
  passboard->makeMove(Go::Move(Go::otherColor(col),Go::Move::PASS));
  if (Util::isWinForColor(col,passboard->score()-komi))
  {
    Util::MoveTree *nmt=new Util::MoveTree(Go::Move(col,Go::Move::PASS));
    nmt->addWin(passboard->score()-komi);
    movetree->addChild(nmt);
  }
  delete passboard;
  
  std::vector<Go::Move> validmoves=this->getValidMoves(currentboard,col);
  for (int i=0;i<(int)validmoves.size();i++)
  {
    Util::MoveTree *nmt=new Util::MoveTree(validmoves.at(i));
    movetree->addChild(nmt);
  }
  
  for (int i=0;i<playoutspermove;i++)
  {
    Util::MoveTree *playouttree = this->getPlayoutTarget(movetree,totalplayouts);
    if (playouttree==NULL)
      break;
    Go::Move playoutmove=playouttree->getMove();
    
    if (livegfx)
    {
      gtpe->getOutput()->printfDebug("gogui-gfx: TEXT [genmove]: eval (%d,%d) tp:%d r:%.2f\n",playoutmove.getX(),playoutmove.getY(),totalplayouts,playouttree->getRatio());
      gtpe->getOutput()->printfDebug("gogui-gfx: VAR %c ",(col==Go::BLACK?'B':'W'));
      Gtp::Vertex vert={playoutmove.getX(),playoutmove.getY()};
      gtpe->getOutput()->printDebugVertex(vert);
      gtpe->getOutput()->printfDebug("\n");
      for (long zzz=0;zzz<100000;zzz++) //slow down for display
        gtpe->getOutput()->printfDebug("");
    }
    
    Go::Board *playoutboard=currentboard->copy();
    Go::BitBoard *firstlist=new Go::BitBoard(boardsize);
    playoutboard->makeMove(playoutmove);
    this->randomPlayout(playoutboard,Go::otherColor(col),NULL,(ravemoves>0?firstlist:NULL));
    totalplayouts++;
    
    bool playoutwin=Util::isWinForColor(col,playoutboard->score()-komi);
    if (playoutwin)
      playouttree->addWin(playoutboard->score()-komi);
    else
      playouttree->addLose(playoutboard->score()-komi);
    
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
    
    delete firstlist;
    delete playoutboard;
  }
  
  float bestratio=0,bestmean=0;
  Go::Move bestmove;
  
  for(std::list<Util::MoveTree*>::iterator iter=movetree->getChildren()->begin();iter!=movetree->getChildren()->end();++iter) 
  {
    if ((*iter)->getPlayouts()>0)
    {
      if ((*iter)->getRatio()>bestratio)
      {
        bestmove=(*iter)->getMove();
        bestratio=(*iter)->getRatio();
        bestmean=(*iter)->getMean();
      }
    }
  }
  
  if (bestratio==0)
    *move=new Go::Move(col,Go::Move::RESIGN);
  else if (bestratio<=resignratiothreshold && std::fabs(bestmean)>resignmeanthreshold && currentboard->getMovesMade()>(resignmovefactorthreshold*boardsize*boardsize))
    *move=new Go::Move(col,Go::Move::RESIGN);
  else
    *move=new Go::Move(col,bestmove.getX(),bestmove.getY());
  
  if (ratio!=NULL)
    *ratio=bestratio;
  
  if (mean!=NULL)
    *mean=bestmean;
  
  this->makeMove(**move);
  
  delete movetree;
  if (livegfx)
    gtpe->getOutput()->printfDebug("gogui-gfx: CLEAR\n");
  
  long timeend=getCurrentTime();
  long timeused=timeend-timestart;
  if (timeused>0)
    playoutspermilli=(float)totalplayouts/timeused;
  if (*timeleft!=0)
    *timeleft-=timeused;
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

void Engine::randomValidMove(Go::Board *board, Go::Color col, Go::Move **move)
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

void Engine::randomPlayout(Go::Board *board, Go::Color col, Go::BitBoard *firstlist, Go::BitBoard *secondlist)
{
  Go::Color coltomove=col;
  Go::Move *move;
  int passes=board->getPassesPlayed();
  int moves=board->getMovesMade();
  
  while (passes<2)
  {
    bool resign,pass;
    this->randomValidMove(board,coltomove,&move);
    board->makeMove(*move);
    if ((coltomove==col?firstlist:secondlist)!=NULL && !move->isPass() && !move->isResign())
      (coltomove==col?firstlist:secondlist)->set(move->getX(),move->getY());
    resign=move->isResign();
    pass=move->isPass();
    delete move;
    coltomove=Go::otherColor(coltomove);
    moves++;
    if (resign)
      break;
    if (pass)
      passes++;
    else
      passes=0;
    if (moves>(boardsize*boardsize*PLAYOUT_MAX_MOVE_FACTOR))
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

Util::MoveTree *Engine::getPlayoutTarget(Util::MoveTree *movetree, int totalplayouts)
{
  float bestucbval=0;
  Util::MoveTree *besttree=NULL;
  
  for(std::list<Util::MoveTree*>::iterator iter=movetree->getChildren()->begin();iter!=movetree->getChildren()->end();++iter) 
  {
    if ((*iter)->getPlayouts()==0)
      return (*iter);
    else
    {
      float ratio=(*iter)->getRAVERatio(ravemoves);
      float ucbval=ratio + ucbc*sqrt(log(totalplayouts)/((*iter)->getPlayouts()));
      if (ucbval>bestucbval)
      {
        besttree=(*iter);
        bestucbval=ucbval;
      }
    }
  }
  
  return besttree;
}

