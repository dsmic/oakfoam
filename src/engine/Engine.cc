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
  //*move=new Go::Move(col,Go::Move::PASS);
  this->randomValidMove(col,move);
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

void Engine::randomValidMove(Go::Color col, Go::Move **move)
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
  } while (!currentboard->validMove(Go::Move(col,x,y)));
  
  *move=new Go::Move(col,x,y);
}


