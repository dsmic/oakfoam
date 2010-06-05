#include "Go.h"

Go::Board::Board(int s)
{
  size=s;
  data=new Go::Color[size*size];
  for (int x=0;x<size;x++)
  {
    for (int y=0;y<size;y++)
    {
      this->setColorAt(x,y,Go::EMPTY);
    }
  }
}

Go::Board::~Board()
{
  delete data;
}

int Go::Board::getSize()
{
  return size;
}

inline void Go::Board::checkCoords(int x, int y)
{
  if (x<0 || y<0 || x>(size-1) || y>(size-1))
    throw Go::Exception("invalid coords");
}

Go::Color Go::Board::colorAt(int x, int y)
{
  this->checkCoords(x,y);
  return data[y*size+x];
}

void Go::Board::setColorAt(int x, int y, Go::Color col)
{
  this->checkCoords(x,y);
  data[y*size+x]=col;
}

Go::Board *Go::Board::copy()
{
  Go::Board *copyboard;
  copyboard=new Go::Board(size);
  
  for (int x=0;x<size;x++)
  {
    for (int y=0;y<size;y++)
    {
      copyboard->setColorAt(x,y,colorAt(x,y));
    }
  }
  
  return copyboard;
}

bool Go::Board::validMove(Go::Move move)
{
  try
  {
    this->checkCoords(move.getX(),move.getY());
  }
  catch (Go::Exception e)
  {
    return false;
  }
  
  if (this->colorAt(move.getX(),move.getY())!=Go::EMPTY)
    return false;
  else
    return true;
}

void Go::Board::makeMove(Go::Move move)
{
  this->checkCoords(move.getX(),move.getY());
  this->setColorAt(move.getX(),move.getY(),move.getColor());
}



