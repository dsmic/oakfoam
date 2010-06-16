#include "Engine.h"

Engine::Engine(Gtp::Engine *ge)
{
  gtpe=ge;
  
  std::srand(std::time(0));
  
  boardsize=9;
  currentboard=new Go::Board(boardsize);
  komi=5.5;
  
  playoutspermove=PLAYOUTS_PER_MOVE;
  livegfx=LIVEGFX_ON;
  
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
  
  gtpe->addFunctionCommand("param",this,&Engine::gtpParam);
  gtpe->addFunctionCommand("showgroups",this,&Engine::gtpShowGroups);
  gtpe->addFunctionCommand("showliberties",this,&Engine::gtpShowLiberties);
  
  gtpe->addAnalyzeCommand("final_score","Final Score","string");
  gtpe->addAnalyzeCommand("showboard","Show Board","string");
  gtpe->addAnalyzeCommand("showgroups","Show Groups","sboard");
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
  
  gtpe->getOutput()->printfDebug("[genmove]: ratio:%.2f mean:%+.1f\n",ratio,mean);
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printVertex(vert);
  gtpe->getOutput()->endResponse();
}

void Engine::gtpShowBoard(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printString("\n");
  me->currentboard->print();
  gtpe->getOutput()->endResponse(true);
}

void Engine::gtpFinalScore(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  float score;
  
  if (me->currentboard->scoreable())
    score=me->currentboard->score()-me->komi;
  else
  {
    Go::Board *playoutboard;
    playoutboard=me->currentboard->copy();
    me->randomPlayout(playoutboard,me->currentboard->nextToMove());
    score=playoutboard->score()-me->komi;
    delete playoutboard;
  }
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printScore(score);
  gtpe->getOutput()->endResponse();
}

void Engine::gtpShowGroups(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
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
      int lib=me->currentboard->boardData()[y*me->boardsize+x].liberties;
      if (lib!=-1)
        gtpe->getOutput()->printf("\"%d\" ",lib);
      else
        gtpe->getOutput()->printf("\"\" ");
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
    gtpe->getOutput()->printf("[string] playouts_per_move %d\n",me->playoutspermove);
    gtpe->getOutput()->printf("[bool] live_gfx %d\n",me->livegfx);
    gtpe->getOutput()->endResponse(true);
  }
  else if (cmd->numArgs()==2)
  {
    std::string param=cmd->getStringArg(0);
    if (param=="playouts_per_move")
        me->playoutspermove=cmd->getIntArg(1);
    else if (param=="live_gfx")
        me->livegfx=(cmd->getIntArg(1)==1);
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

void Engine::generateMove(Go::Color col, Go::Move **move, float *ratio, float *mean)
{
  //*move=new Go::Move(col,Go::Move::PASS);
  //this->randomValidMove(currentboard,col,move);
  //this->makeMove(**move);
  
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
  else if (bestratio<=RESIGN_RATIO_THRESHOLD && std::fabs(bestmean)>RESIGN_MEAN_THRESHOLD)
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
}

void Engine::clearBoard()
{
  int size=currentboard->getSize();
  delete currentboard;
  currentboard = new Go::Board(size);
}

void Engine::randomValidMove(Go::Board *board, Go::Color col, Go::Move **move)
{
  int i,x,y;
  
  i=0;
  do
  {
    x=(int)(std::rand()/((double)RAND_MAX+1)*boardsize);
    y=(int)(std::rand()/((double)RAND_MAX+1)*boardsize);
    i++;
    if (i>(boardsize*boardsize*2))
    {
      *move=new Go::Move(col,Go::Move::PASS);
      return;
    }
  } while (!board->validMove(Go::Move(col,x,y)) || board->weakEye(col,x,y));
  
  *move=new Go::Move(col,x,y);
}

void Engine::randomPlayout(Go::Board *board, Go::Color col)
{
  Go::Color coltomove;
  Go::Move *move;
  int passes=board->getPassesPlayed(); //not always true
  if (passes>=2)
    return;
  
  coltomove=col;
  
  while (!board->scoreable())
  {
    bool resign,pass;
    this->randomValidMove(board,coltomove,&move);
    board->makeMove(*move);
    resign=move->isResign();
    pass=move->isPass();
    delete move;
    coltomove=Go::otherColor(coltomove);
    if (resign)
      break;
    if (pass)
      passes++;
    else
      passes=0;
    if (passes>=2)
      break;
  }
}


