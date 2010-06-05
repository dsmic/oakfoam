#include "Go.h"

Go::Board::Board(int s)
{
  size=s;
  data=new Go::Board::Vertex[size*size];
  for (int x=0;x<size;x++)
  {
    for (int y=0;y<size;y++)
    {
      this->setColorAt(x,y,Go::EMPTY);
      this->setGroupAt(x,y,-1);
      this->setLibertiesAt(x,y,-1);
    }
  }
  this->setKo(-1,-1);
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

Go::Board::Vertex *Go::Board::boardData()
{
  return data;
}

inline Go::Color Go::Board::colorAt(int x, int y)
{
  //this->checkCoords(x,y);
  return data[y*size+x].color;
}

inline void Go::Board::setColorAt(int x, int y, Go::Color col)
{
  //this->checkCoords(x,y);
  data[y*size+x].color=col;
}

inline int Go::Board::groupAt(int x, int y)
{
  //this->checkCoords(x,y);
  return data[y*size+x].group;
}

inline void Go::Board::setGroupAt(int x, int y, int group)
{
  //this->checkCoords(x,y);
  data[y*size+x].group=group;
}

inline int Go::Board::libertiesAt(int x, int y)
{
  //this->checkCoords(x,y);
  return data[y*size+x].liberties;
}

inline void Go::Board::setLibertiesAt(int x, int y, int liberties)
{
  //this->checkCoords(x,y);
  data[y*size+x].liberties=liberties;
}

Go::Board *Go::Board::copy()
{
  Go::Board *copyboard;
  copyboard=new Go::Board(size);
  
  for (int x=0;x<size;x++)
  {
    for (int y=0;y<size;y++)
    {
      copyboard->boardData()[y*size+x]=data[y*size+x];
    }
  }
  
  copyboard->setKo(koX,koY);
  
  return copyboard;
}

inline void Go::Board::setKo(int x, int y)
{
  koX=x;
  koY=y;
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
  else if (koX!=-1 && koY!=-1 && move.getX()==koX && move.getY()==koY)
    return false;
  else
    return true; //check if liberties avail
}

void Go::Board::makeMove(Go::Move move)
{
  this->checkCoords(move.getX(),move.getY());
  if (!this->validMove(move))
    throw Go::Exception("invalid move");
  
  //check if stones to be removed
  
  this->setColorAt(move.getX(),move.getY(),move.getColor());
  this->setLibertiesAt(move.getX(),move.getY(),-1);
  this->currentLiberties(move.getX(),move.getY());
}

int Go::Board::currentLiberties(int x, int y)
{
  int lib,group;
  this->checkCoords(x,y);
  
  if (this->colorAt(x,y)==Go::EMPTY)
    return -1;
  
  lib=this->libertiesAt(x,y);
  if (lib>=0)
    return lib;
  else
  {
    group=this->groupAt(x,y);
    lib=0;
    
    for (int ix=0;ix<size;ix++)
    {
      for (int iy=0;iy<size;iy++)
      {
        if (group==this->groupAt(ix,iy))
          lib+=directLiberties(ix,iy);
      }
    }
    
    for (int ix=0;ix<size;ix++)
    {
      for (int iy=0;iy<size;iy++)
      {
        if (group==this->groupAt(ix,iy))
          this->setLibertiesAt(ix,iy,lib);
      }
    }
    
    return lib;
  }
}

int Go::Board::directLiberties(int x, int y)
{
  int lib;
  this->checkCoords(x,y);
  
  if (this->colorAt(x,y)==Go::EMPTY)
    return -1;
  
  lib=0;
  
  if (x>0 && this->colorAt(x-1,y)==Go::EMPTY)
    lib++;
  if (y>0 && this->colorAt(x,y-1)==Go::EMPTY)
    lib++;
  if (x<(size-1) && this->colorAt(x+1,y)==Go::EMPTY)
    lib++;
  if (y<(size-1) && this->colorAt(x,y+1)==Go::EMPTY)
    lib++;
  
  return lib;
}


