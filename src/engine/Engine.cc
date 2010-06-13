#include "Engine.h"

Engine::Engine(Gtp::Engine *ge)
{
  gtpe=ge;
  
  std::srand(std::time(0));
  
  boardsize=9;
  currentboard=new Go::Board(boardsize);
  komi=5.5;
  
  this->addGtpCommands();
}

Engine::~Engine()
{
  delete currentboard;
}

void Engine::addGtpCommands()
{
  //gtpe->addFunctionCommand("boardsize",(Gtp::Engine::CommandFunction)(&(this->gtpBoardSize)));
}

void Engine::gtpBoardSize(Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  this->setBoardSize(9);
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->endResponse();
}

void Engine::generateMove(Go::Color col, Go::Move **move)
{
  //*move=new Go::Move(col,Go::Move::PASS);
  //this->randomValidMove(currentboard,col,move);
  //this->makeMove(**move);
  
  /*
  playoutboard=currentboard->copy();
  randomPlayout(playoutboard,col);
  playoutboard->print();
  printf("scoreable: %d\n",playoutboard->scoreable());
  delete playoutboard;
  */
  
  Util::MoveTree *movetree;
  Go::Board *playoutboard;
  Go::Move playoutmove;
  
  playoutmove=Go::Move();//Go::Move(col,Go:Move::PASS);
  movetree= new Util::MoveTree(playoutmove);
  movetree->addLose();
  
  for (int x=0;x<boardsize;x++)
  {
    for (int y=0;y<boardsize;y++)
    {
      playoutmove=Go::Move(col,x,y);
      if (currentboard->validMove(playoutmove))
      {
        Util::MoveTree *nmt=new Util::MoveTree(playoutmove);
        
        for (int i=0;i<PLAYOUTS_PER_MOVE;i++)
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
        
        movetree->addSibling(nmt);
      }
    }
  }
  
  float bestratio=0;
  Go::Move bestmove;//=new Go::Move(col,Go:Move::PASS);
  bestratio=0;
  //bestmove=new Go::Move(col,Go:Move::PASS);
  
  Util::MoveTree *cmt=movetree;
  
  while (cmt!=NULL)
  {
    if (cmt->getPlayouts()>0)
    {
      float r=(float)cmt->getWins()/cmt->getPlayouts();
      //printf("ratio for (%d,%d) %f\n",cmt->getMove().getX(),cmt->getMove().getY(),r);
      if (r>bestratio)
      {
        bestratio=r;
        bestmove=cmt->getMove();
      }
    }
    cmt=cmt->getSibling();
  }
  
  //bestmove.print();
  
  if (bestratio==0)
    *move=new Go::Move(col,Go::Move::PASS);
  else
    *move=new Go::Move(col,bestmove.getX(),bestmove.getY());
  
  this->makeMove(**move);
  
  delete movetree;
}

bool Engine::isMoveAllowed(Go::Move move)
{
  return currentboard->validMove(move);
}

void Engine::makeMove(Go::Move move)
{
  currentboard->makeMove(move);
}

int Engine::getBoardSize()
{
  return currentboard->getSize();
}

void Engine::setBoardSize(int s)
{
  delete currentboard;
  currentboard = new Go::Board(s);
}

void Engine::clearBoard()
{
  int size=currentboard->getSize();
  delete currentboard;
  currentboard = new Go::Board(size);
}

Go::Board *Engine::getCurrentBoard()
{
  return currentboard;
}

float Engine::getKomi()
{
  return komi;
}

void Engine::setKomi(float k)
{
  komi=k;
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
  int passes=0; //not always true
  
  coltomove=col;
  
  //printf("random playout\n");
  
  while (!board->scoreable())
  {
    this->randomValidMove(board,coltomove,&move);
    //printf("random move at (%d,%d)\n",move->getX(),move->getY());
    board->makeMove(*move);
    coltomove=Go::otherColor(coltomove);
    if (move->isResign())
      break;
    if (move->isPass())
      passes++;
    else
      passes=0;
    if (passes>=2)
      break;
  }
}


