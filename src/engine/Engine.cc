#include "Engine.h"

Go::Board *currentboard;
float komi;
int boardsize;

void Engine::init()
{
  std::srand(std::time(0));
  
  boardsize=9;
  currentboard=new Go::Board(boardsize);
  komi=5.5;
}

void Engine::generateMove(Go::Color col, Go::Move **move)
{
  Go::Board *playoutboard;
  
  //playoutboard=currentboard->copy();
  //randomPlayout(playoutboard,col);
  //playoutboard->print();
  //delete playoutboard;
  
  //*move=new Go::Move(col,Go::Move::PASS);
  this->randomValidMove(currentboard,col,move);
  this->makeMove(**move);
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
  } while (!board->validMove(Go::Move(col,x,y)));
  
  *move=new Go::Move(col,x,y);
}

void Engine::randomPlayout(Go::Board *board, Go::Color col)
{
  Go::Color coltomove;
  Go::Move *move;
  int passes;
  
  coltomove=col;
  
  printf("random playout\n");
  
  while (!board->scoreable())
  {
    this->randomValidMove(board,coltomove,&move);
    printf("random move at (%d,%d)\n",move->getX(),move->getY());
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


