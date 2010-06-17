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
  nexttomove=Go::BLACK;
  passesplayed=0;
  movesmade=0;
}

Go::Board::~Board()
{
  delete[] data;
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
  copyboard->nexttomove=nexttomove;
  copyboard->passesplayed=passesplayed;
  copyboard->movesmade=movesmade;
  
  return copyboard;
}

inline void Go::Board::setKo(int x, int y)
{
  koX=x;
  koY=y;
}

bool Go::Board::validMove(Go::Move move)
{
  if (move.isPass() || move.isResign())
    return true;
  
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
  else if (directLiberties(move.getX(),move.getY())>0)
    return true;
  else
  {
    int x=move.getX(),y=move.getY();
    Go::Color col=move.getColor();
    Go::Color othercol=Go::otherColor(col);
    
    if (x>0 && this->libertiesAt(x-1,y)==1 && this->colorAt(x-1,y)==othercol)
      return true;
    
    if (y>0 && this->libertiesAt(x,y-1)==1 && this->colorAt(x,y-1)==othercol)
      return true;
    
    if (x<(size-1) && this->libertiesAt(x+1,y)==1 && this->colorAt(x+1,y)==othercol)
      return true;
    
    if (y<(size-1) && this->libertiesAt(x,y+1)==1 && this->colorAt(x,y+1)==othercol)
      return true;
    
    if (x>0 && this->libertiesAt(x-1,y)>1 && this->colorAt(x-1,y)==col)
      return true;
    
    if (y>0 && this->libertiesAt(x,y-1)>1 && this->colorAt(x,y-1)==col)
      return true;
    
    if (x<(size-1) && this->libertiesAt(x+1,y)>1 && this->colorAt(x+1,y)==col)
      return true;
    
    if (y<(size-1) && this->libertiesAt(x,y+1)>1 && this->colorAt(x,y+1)==col)
      return true;
    
    return false;
  }
}

void Go::Board::makeMove(Go::Move move)
{
  int x,y,poskox,poskoy;
  Go::Color col,othercol;
  bool removedagroup;
  
  if (move.isPass() || move.isResign())
  {
    if (move.isPass())
      passesplayed++;
    this->setKo(-1,-1);
    nexttomove=Go::otherColor(nexttomove);
    movesmade++;
    return;
  }
  
  this->checkCoords(move.getX(),move.getY());
  if (!this->validMove(move))
    throw Go::Exception("invalid move");
  
  x=move.getX();
  y=move.getY();
  col=move.getColor();
  othercol=Go::otherColor(col);
  poskox=-1;
  poskoy=-1;
  removedagroup=false;
  passesplayed=0;
  
  //fprintf(stderr,"[makeMove]:start\n");
  
  if (x>0 && this->libertiesAt(x-1,y)==1 && this->colorAt(x-1,y)==othercol)
  {
    //fprintf(stderr,"[makeMove]:remove a group\n");
    removedagroup=true;
    if (removeGroup(this->groupAt(x-1,y))==1)
    {
      if (poskox==-1 && poskoy==-1)
      {
        poskox=x-1;
        poskoy=y;
      }
      else if (!(poskox==-2 && poskoy==-2))
      {
        poskox=-2;
        poskoy=-2;
      }
    }
  }
  
  if (y>0 && this->libertiesAt(x,y-1)==1 && this->colorAt(x,y-1)==othercol)
  {
    //fprintf(stderr,"[makeMove]:remove a group\n");
    removedagroup=true;
    if (removeGroup(this->groupAt(x,y-1))==1)
    {
      if (poskox==-1 && poskoy==-1)
      {
        poskox=x;
        poskoy=y-1;
      }
      else if (!(poskox==-2 && poskoy==-2))
      {
        poskox=-2;
        poskoy=-2;
      }
    }
  }
  
  if (x<(size-1) && this->libertiesAt(x+1,y)==1 && this->colorAt(x+1,y)==othercol)
  {
    //fprintf(stderr,"[makeMove]:remove a group\n");
    removedagroup=true;
    if (removeGroup(this->groupAt(x+1,y))==1)
    {
      if (poskox==-1 && poskoy==-1)
      {
        poskox=x+1;
        poskoy=y;
      }
      else if (!(poskox==-2 && poskoy==-2))
      {
        poskox=-2;
        poskoy=-2;
      }
    }
  }
  
  if (y<(size-1) && this->libertiesAt(x,y+1)==1 && this->colorAt(x,y+1)==othercol)
  {
    //fprintf(stderr,"[makeMove]:remove a group\n");
    removedagroup=true;
    if (removeGroup(this->groupAt(x,y+1))==1)
    {
      if (poskox==-1 && poskoy==-1)
      {
        poskox=x;
        poskoy=y+1;
      }
      else if (!(poskox==-2 && poskoy==-2))
      {
        poskox=-2;
        poskoy=-2;
      }
    }
  }
  
  if (poskox>=0 && poskoy>=0)
    this->setKo(poskox,poskoy);
  else
    this->setKo(-1,-1);
  
  this->setColorAt(x,y,col);
  if (removedagroup)
    this->updateLiberties();
  else if (this->solidlyTouches(x,y)==0)
  {
    this->setGroupAt(x,y,totalgroups);
    totalgroups++;
    this->setLibertiesAt(x,y,this->directLiberties(x,y));
  }
  else
    this->updateLiberties();
  
  nexttomove=Go::otherColor(nexttomove);
  movesmade++;
}

void Go::Board::updateLiberties()
{
  int groups;
  
  groups=this->updateGroups();
  
  for (int x=0;x<size;x++)
  {
    for (int y=0;y<size;y++)
    {
      this->setLibertiesAt(x,y,-1);
    }
  }
  
  for (int g=0;g<groups;g++)
  {
    int lib=0;
    
    for (int x=0;x<size;x++)
    {
      for (int y=0;y<size;y++)
      {
        if (g==this->groupAt(x,y))
          lib+=this->directLiberties(x,y,true);
      }
    }
    
    for (int x=0;x<size;x++)
    {
      for (int y=0;y<size;y++)
      {
        if (g==this->groupAt(x,y))
          this->setLibertiesAt(x,y,lib);
        else if (this->libertiesAt(x,y)==-2)
          this->setLibertiesAt(x,y,-1);
      }
    }
  }
}

int Go::Board::directLiberties(int x, int y)
{
  return this->directLiberties(x,y,false);
}

int Go::Board::directLiberties(int x, int y, bool dirty)
{
  int lib;
  this->checkCoords(x,y);
  
  lib=0;
  
  if (x>0 && this->colorAt(x-1,y)==Go::EMPTY)
  {
    if (dirty)
    {
      if (this->libertiesAt(x-1,y)==-1)
      {
        lib++;
        this->setLibertiesAt(x-1,y,-2);
      }
    }
    else
      lib++;
  }
  if (y>0 && this->colorAt(x,y-1)==Go::EMPTY)
  {
    if (dirty)
    {
      if (this->libertiesAt(x,y-1)==-1)
      {
        lib++;
        this->setLibertiesAt(x,y-1,-2);
      }
    }
    else
      lib++;
  }
  if (x<(size-1) && this->colorAt(x+1,y)==Go::EMPTY)
  {
    if (dirty)
    {
      if (this->libertiesAt(x+1,y)==-1)
      {
        lib++;
        this->setLibertiesAt(x+1,y,-2);
      }
    }
    else
      lib++;
  }
  if (y<(size-1) && this->colorAt(x,y+1)==Go::EMPTY)
  {
    if (dirty)
    {
      if (this->libertiesAt(x,y+1)==-1)
      {
        lib++;
        this->setLibertiesAt(x,y+1,-2);
      }
    }
    else
      lib++;
  }
  
  return lib;
}

int Go::Board::solidLinksFrom(int x, int y)
{
  int links;
  Go::Color col;
  this->checkCoords(x,y);
  
  col=this->colorAt(x,y);
  links=0;
  
  if (x>0 && this->colorAt(x-1,y)==col)
    links++;
  if (y>0 && this->colorAt(x,y-1)==col)
    links++;
  if (x<(size-1) && this->colorAt(x+1,y)==col)
    links++;
  if (y<(size-1) && this->colorAt(x,y+1)==col)
    links++;
  
  return links;
}

int Go::Board::solidlyTouches(int x, int y)
{
  int links;
  Go::Color col;
  this->checkCoords(x,y);
  
  col=this->colorAt(x,y);
  links=0;
  
  if (x>0 && this->colorAt(x-1,y)!=Go::EMPTY)
    links++;
  if (y>0 && this->colorAt(x,y-1)!=Go::EMPTY)
    links++;
  if (x<(size-1) && this->colorAt(x+1,y)!=Go::EMPTY)
    links++;
  if (y<(size-1) && this->colorAt(x,y+1)!=Go::EMPTY)
    links++;
  
  return links;
}

int Go::Board::updateGroups()
{
  int group=0;
  
  for (int x=0;x<size;x++)
  {
    for (int y=0;y<size;y++)
    {
      this->setGroupAt(x,y,-1);
    }
  }
  
  for (int x=0;x<size;x++)
  {
    for (int y=0;y<size;y++)
    {
      if (this->colorAt(x,y)!=Go::EMPTY && this->groupAt(x,y)==-1)
      {
        this->spreadGroup(x,y,group);
        group++;
      }
    }
  }
  
  totalgroups=group;
  
  return group;
}

void Go::Board::spreadGroup(int x, int y, int group)
{
  Go::Color col;
  
  this->setGroupAt(x,y,group);
  col=this->colorAt(x,y);
  
  if (x>0 && this->colorAt(x-1,y)==col && this->groupAt(x-1,y)==-1)
    spreadGroup(x-1,y,group);
  if (y>0 && this->colorAt(x,y-1)==col && this->groupAt(x,y-1)==-1)
    spreadGroup(x,y-1,group);
  if (x<(size-1) && this->colorAt(x+1,y)==col && this->groupAt(x+1,y)==-1)
    spreadGroup(x+1,y,group);
  if (y<(size-1) && this->colorAt(x,y+1)==col && this->groupAt(x,y+1)==-1)
    spreadGroup(x,y+1,group);
}

int Go::Board::removeGroup(int group)
{
  int stones=0;
  
  for (int x=0;x<size;x++)
  {
    for (int y=0;y<size;y++)
    {
      if (this->groupAt(x,y)==group)
      {
        this->setColorAt(x,y,Go::EMPTY);
        this->setGroupAt(x,y,-1);
        this->setLibertiesAt(x,y,-1);
        stones++;
      }
    }
  }
  
  return stones;
}

void Go::Board::print()
{
  for (int y=size-1;y>=0;y--)
  {
    for (int x=0;x<size;x++)
    {
      Go::Color col=this->colorAt(x,y);
      if (col==Go::BLACK)
        printf("X ");
      else if (col==Go::WHITE)
        printf("O ");
      else
        printf(". ");
    }
    printf("\n");
  }
}

bool Go::Board::scoreable()
{
  //no more valid moves
  
  //printf("check scoreable\n");
  
  Go::Move move;
  for (int x=0;x<size;x++)
  {
    for (int y=0;y<size;y++)
    {
      //printf("test move at (%d,%d)\n",x,y);
      if (this->colorAt(x,y)==EMPTY)
      {
        move=Go::Move(Go::BLACK,x,y);
        if (this->validMove(move))
        {
          //printf("valid move B(%d,%d)\n",x,y);
          return false;
        }
        
        move=Go::Move(Go::WHITE,x,y);
        if (this->validMove(move))
        {
          //printf("valid move W(%d,%d)\n",x,y);
          return false;
        }
        
        //printf("invalid move at (%d,%d)\n",x,y);
      }
    }
  }
  
  //printf("scoreable!\n");
  
  return true;
}

int Go::Board::score()
{
  int s=0;
  
  for (int x=0;x<size;x++)
  {
    for (int y=0;y<size;y++)
    {
      Go::Color col=this->colorAt(x,y);
      if (col==Go::BLACK)
        s++;
      else if (col==Go::WHITE)
        s--;
      else
      {
        if (this->weakEye(Go::BLACK,x,y))
          s++;
        else if (this->weakEye(Go::WHITE,x,y))
          s--;
      }
    }
  }
  return s;
}

bool Go::Board::weakEye(Go::Color col, int x, int y)
{
  if (directLiberties(x,y)>0)
    return false;
  else
  {
    if (x>0 && this->colorAt(x-1,y)!=col)
      return false;
    
    if (y>0 && this->colorAt(x,y-1)!=col)
      return false;
    
    if (x<(size-1) && this->colorAt(x+1,y)!=col)
      return false;
    
    if (y<(size-1) && this->colorAt(x,y+1)!=col)
      return false;
    
    return true;
  }
}

void Go::Move::print()
{
  if (this->isPass())
    printf("PASS\n");
  else if (this->isResign())
    printf("RESIGN\n");
  else
  {
    if (color==Go::BLACK)
      printf("B");
    else if (color==Go::WHITE)
      printf("W");
    printf("(%d,%d)\n",x,y);
  }
}


