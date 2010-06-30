#include "Engine.h"

Engine::Engine(Gtp::Engine *ge)
{
  gtpe=ge;
  
  std::srand(std::time(0));
  
  boardsize=9;
  currentboard=new Go::Board(boardsize);
  komi=5.5;
  
  playoutspermilli=0;
  playoutspermove=PLAYOUTS_PER_MOVE;
  playoutspermoveinit=PLAYOUTS_PER_MOVE;
  livegfx=LIVEGFX_ON;
  
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
  //gtpe->addFunctionCommand("showgroups",this,&Engine::gtpShowGroups);
  gtpe->addFunctionCommand("showliberties",this,&Engine::gtpShowLiberties);
  
  gtpe->addFunctionCommand("time_settings",this,&Engine::gtpTimeSettings);
  gtpe->addFunctionCommand("time_left",this,&Engine::gtpTimeLeft);
  
  gtpe->addAnalyzeCommand("final_score","Final Score","string");
  gtpe->addAnalyzeCommand("showboard","Show Board","string");
  //gtpe->addAnalyzeCommand("showgroups","Show Groups","sboard");
  gtpe->addAnalyzeCommand("showliberties","Show Liberties","sboard");
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
  
  //assume that this will be a new game
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
  me->currentboard->print(); //TODO: redirect to gtpe output
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

/*void Engine::gtpShowGroups(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printString("\n");
  for (int y=me->boardsize-1;y>=0;y--)
  {
    for (int x=0;x<me->boardsize;x++)
    {
      int group=me->currentboard->boardData()[y*me->boardsize+x].group;
      if (group!=-1)
        gtpe->getOutput()->printf("\"%d\" ",group);
      else
        gtpe->getOutput()->printf("\"\" ");
    }
    gtpe->getOutput()->printf("\n");
  }

  gtpe->getOutput()->endResponse(true);
}*/

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

void Engine::gtpParam(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  if (cmd->numArgs()==0)
  {
    gtpe->getOutput()->startResponse(cmd);
    gtpe->getOutput()->printf("[bool] live_gfx %d\n",me->livegfx);
    gtpe->getOutput()->printf("[string] playouts_per_move %d\n",me->playoutspermove);
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
  //*move=new Go::Move(col,Go::Move::PASS);
  //this->randomValidMove(currentboard,col,move);
  //this->makeMove(**move);
  
  long *timeleft=(col==Go::BLACK ? &timeblack : &timewhite);
  long timestart=getCurrentTime();
  int totalplayouts=0;
  
  if (*timeleft>0 && playoutspermilli>0)
  {
    playoutspermove=playoutspermilli*this->getTimeAllowedThisTurn(col)/(boardsize*boardsize-currentboard->getMovesMade());
    if (playoutspermove<PLAYOUTS_PER_MOVE_MIN)
      playoutspermove=PLAYOUTS_PER_MOVE_MIN;
    else if (playoutspermove>PLAYOUTS_PER_MOVE_MAX)
      playoutspermove=PLAYOUTS_PER_MOVE_MAX;
  }
  
  Util::MoveTree *movetree;
  Go::Board *playoutboard;
  Go::Move playoutmove;
  
  movetree=new Util::MoveTree();
  
  if (livegfx)
    gtpe->getOutput()->printfDebug("gogui-gfx: TEXT [genmove]: starting...\n");
  
  playoutmove=Go::Move(col,Go::Move::PASS);
  Util::MoveTree *nmt=new Util::MoveTree(playoutmove);
  
  playoutboard=currentboard->copy();
  playoutboard->makeMove(playoutmove);
  playoutboard->makeMove(Go::Move(Go::otherColor(col),Go::Move::PASS));
  randomPlayout(playoutboard,col);
  if (Util::isWinForColor(col,playoutboard->score()-komi))
    nmt->addWin();
  else
    nmt->addLose();
  delete playoutboard;
  totalplayouts++;
  
  if (nmt->getRatio()==1)
  {
    for (int i=0;i<playoutspermove;i++)
    {
      playoutboard=currentboard->copy();
      playoutboard->makeMove(playoutmove);
      randomPlayout(playoutboard,Go::otherColor(col));
      if (Util::isWinForColor(col,playoutboard->score()-komi))
        nmt->addWin();
      else
        nmt->addLose();
      delete playoutboard;
      totalplayouts++;
    }
  }
  movetree->addChild(nmt);
  
  for (int x=0;x<boardsize;x++)
  {
    for (int y=0;y<boardsize;y++)
    {
      playoutmove=Go::Move(col,x,y);
      if (currentboard->validMove(playoutmove))
      {
        if (livegfx)
        {
          gtpe->getOutput()->printfDebug("gogui-gfx: TEXT [genmove]: evaluating move (%d,%d)\n",x,y);
          gtpe->getOutput()->printfDebug("gogui-gfx: VAR %c ",(col==Go::BLACK?'B':'W'));
          Gtp::Vertex vert={x,y};
          gtpe->getOutput()->printDebugVertex(vert);
          gtpe->getOutput()->printfDebug("\n");
        }
        Util::MoveTree *nmt=new Util::MoveTree(playoutmove);
        
        for (int i=0;i<playoutspermove;i++)
        {
          playoutboard=currentboard->copy();
          playoutboard->makeMove(playoutmove);
          randomPlayout(playoutboard,Go::otherColor(col));
          if (Util::isWinForColor(col,playoutboard->score()-komi))
            nmt->addWin(playoutboard->score()-komi);
          else
            nmt->addLose(playoutboard->score()-komi);
          delete playoutboard;
          totalplayouts++;
        }
        
        movetree->addChild(nmt);
      }
    }
  }
  
  float bestratio=0,bestmean=0;
  Go::Move bestmove;
  
  std::list<Util::MoveTree*>::iterator iter;
  
  for(iter=movetree->getChildren()->begin();iter!=movetree->getChildren()->end();++iter) 
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
  
  std::vector<Go::Move> validmoves;
  for (int x=0;x<boardsize;x++)
  {
    for (int y=0;y<boardsize;y++)
    {
      if (board->validMove(Go::Move(col,x,y)) && !board->weakEye(col,x,y))
      {
        validmoves.push_back(Go::Move(col,x,y));
      }
    }
  }
  
  if (validmoves.size()==0)
    *move=new Go::Move(col,Go::Move::PASS);
  else
  {
    int i=(int)(std::rand()/((double)RAND_MAX+1)*validmoves.size());
    *move=new Go::Move(col,validmoves.at(i).getX(),validmoves.at(i).getY());
  }
}

void Engine::randomPlayout(Go::Board *board, Go::Color col)
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
  movesleft+=timemovebuffer; //be able to play ~10 more moves
  long timepermove=timeleft/movesleft*timefactor; //allow more time in beginning
  if (timepermove<timemoveminimum)
    timepermove=timemoveminimum;
  return timepermove;
}


