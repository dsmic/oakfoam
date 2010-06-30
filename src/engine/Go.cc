#include "Go.h"

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

Go::IncrementalBoard::IncrementalBoard(int s)
{
  size=s;
  data=new Go::IncrementalBoard::Vertex[size*size];
  for (int x=0;x<size;x++)
  {
    for (int y=0;y<size;y++)
    {
      Go::IncrementalBoard::Point pt={x,y};
      this->vertexAt(x,y)->point=pt;
      this->setColorAt(x,y,Go::EMPTY);
      this->setGroupAt(x,y,NULL);
    }
  }
  this->setKo(-1,-1);
  nexttomove=Go::BLACK;
  passesplayed=0;
  movesmade=0;
}

Go::IncrementalBoard::~IncrementalBoard()
{
  for(std::list<Go::IncrementalBoard::Group*>::iterator iter=groups.begin();iter!=groups.end();++iter) 
  {
    delete (*iter);
  }
  groups.resize(0);
  delete[] data;
}

inline void Go::IncrementalBoard::checkCoords(int x, int y)
{
  if (x<0 || y<0 || x>(size-1) || y>(size-1))
    throw Go::Exception("invalid coords");
}

inline Go::IncrementalBoard::Vertex *Go::IncrementalBoard::vertexAt(int x, int y)
{
  return &data[y*size+x];
}

inline Go::Color Go::IncrementalBoard::colorAt(int x, int y)
{
  //this->checkCoords(x,y);
  return data[y*size+x].color;
}

inline void Go::IncrementalBoard::setColorAt(int x, int y, Go::Color col)
{
  //this->checkCoords(x,y);
  data[y*size+x].color=col;
}

inline Go::IncrementalBoard::Group *Go::IncrementalBoard::groupAt(int x, int y)
{
  //this->checkCoords(x,y);
  return data[y*size+x].group;
}

inline void Go::IncrementalBoard::setGroupAt(int x, int y, Go::IncrementalBoard::Group *group)
{
  //this->checkCoords(x,y);
  data[y*size+x].group=group;
}

inline int Go::IncrementalBoard::libertiesAt(int x, int y)
{
  //this->checkCoords(x,y);
  if (data[y*size+x].group==NULL)
    return 0;
  else
    return data[y*size+x].group->numOfLiberties();
}

inline int Go::IncrementalBoard::groupSizeAt(int x, int y)
{
  //this->checkCoords(x,y);
  if (data[y*size+x].group==NULL)
    return 0;
  else
    return data[y*size+x].group->numOfStones();
}

inline void Go::IncrementalBoard::setKo(int x, int y)
{
  koX=x;
  koY=y;
}

Go::IncrementalBoard *Go::IncrementalBoard::copy()
{
  Go::IncrementalBoard *copyboard;
  copyboard=new Go::IncrementalBoard(size);
  
  for (int x=0;x<size;x++)
  {
    for (int y=0;y<size;y++)
    {
      copyboard->data[y*size+x]=this->data[y*size+x];
    }
  }
  
  copyboard->setKo(this->koX,this->koY);
  copyboard->nexttomove=this->nexttomove;
  copyboard->passesplayed=this->passesplayed;
  copyboard->movesmade=this->movesmade;
  
  copyboard->refreshGroups();
  
  return copyboard;
}

bool Go::IncrementalBoard::validMove(Go::Move move)
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
  else if (directLiberties(move.getX(),move.getY())>0)
    return true;
  else
  {
    int x=move.getX(),y=move.getY();
    Go::Color col=move.getColor();
    Go::Color othercol=Go::otherColor(col);
    int captures=0;
    
    if (x>0 && this->libertiesAt(x-1,y)>1 && this->colorAt(x-1,y)==col)
      return true;
    
    if (y>0 && this->libertiesAt(x,y-1)>1 && this->colorAt(x,y-1)==col)
      return true;
    
    if (x<(size-1) && this->libertiesAt(x+1,y)>1 && this->colorAt(x+1,y)==col)
      return true;
    
    if (y<(size-1) && this->libertiesAt(x,y+1)>1 && this->colorAt(x,y+1)==col)
      return true;
    
    if (x>0 && this->libertiesAt(x-1,y)==1 && this->colorAt(x-1,y)==othercol)
      captures+=this->groupSizeAt(x-1,y);
    
    if (y>0 && this->libertiesAt(x,y-1)==1 && this->colorAt(x,y-1)==othercol)
      captures+=this->groupSizeAt(x,y-1);
    
    if (x<(size-1) && this->libertiesAt(x+1,y)==1 && this->colorAt(x+1,y)==othercol)
      captures+=this->groupSizeAt(x+1,y);
    
    if (y<(size-1) && this->libertiesAt(x,y+1)==1 && this->colorAt(x,y+1)==othercol)
      captures+=this->groupSizeAt(x,y+1);
    
    if (captures>1)
      return true;
    else if (captures==1)
    {
      if (koX!=-1 && koY!=-1 && move.getX()==koX && move.getY()==koY)
        return false;
      else
        return true;
    }
    
    return false;
  }
}

int Go::IncrementalBoard::directLiberties(int x, int y)
{
  int lib;
  this->checkCoords(x,y);
  
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

void Go::IncrementalBoard::refreshGroups()
{
  for (int x=0;x<size;x++)
  {
    for (int y=0;y<size;y++)
    {
      this->setGroupAt(x,y,NULL);
    }
  }
  
  for(std::list<Go::IncrementalBoard::Group*>::iterator iter=groups.begin();iter!=groups.end();++iter) 
  {
    delete (*iter);
  }
  groups.resize(0);
  
  for (int x=0;x<size;x++)
  {
    for (int y=0;y<size;y++)
    {
      if (this->colorAt(x,y)!=Go::EMPTY && this->groupAt(x,y)==NULL)
      {
        Go::IncrementalBoard::Group *newgroup = new Go::IncrementalBoard::Group();
        
        this->spreadGroup(x,y,this->colorAt(x,y),newgroup);
        groups.push_back(newgroup);
      }
    }
  }
}

void Go::IncrementalBoard::spreadGroup(int x, int y, Go::Color col, Go::IncrementalBoard::Group *group)
{
  if (this->colorAt(x,y)==col && this->groupAt(x,y)==NULL)
  {
    this->setGroupAt(x,y,group);
    group->addStone(this->vertexAt(x,y));
    this->addDirectLiberties(x,y,group);
    
    if (x>0)
      this->spreadGroup(x-1,y,col,group);
    if (y>0)
      this->spreadGroup(x,y-1,col,group);
    if (x<(size-1))
      this->spreadGroup(x+1,y,col,group);
    if (y<(size-1))
      this->spreadGroup(x,y+1,col,group);
  }
}

void Go::IncrementalBoard::addDirectLiberties(int x, int y, Go::IncrementalBoard::Group *group)
{
  if (x>0 && this->colorAt(x-1,y)==Go::EMPTY)
    group->addLiberty(this->vertexAt(x-1,y));
  if (y>0 && this->colorAt(x,y-1)==Go::EMPTY)
    group->addLiberty(this->vertexAt(x,y-1));
  if (x<(size-1) && this->colorAt(x+1,y)==Go::EMPTY)
    group->addLiberty(this->vertexAt(x+1,y));
  if (y<(size-1) && this->colorAt(x,y+1)==Go::EMPTY)
    group->addLiberty(this->vertexAt(x,y+1));
}

void Go::IncrementalBoard::makeMove(Go::Move move)
{
  int x,y,poskox,poskoy;
  Go::Color col,othercol;
  //bool removedagroup;
  
  if (move.isPass() || move.isResign())
  {
    if (move.isPass())
      passesplayed++;
    this->setKo(-1,-1);
    nexttomove=Go::otherColor(move.getColor());
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
  //removedagroup=false;
  passesplayed=0;
  
  std::list<Go::IncrementalBoard::Group*> *friendlygroups = new std::list<Go::IncrementalBoard::Group*>();
  
  if (x>0)
  {
    Go::IncrementalBoard::Point pt={x-1,y};
    if (this->colorAt(pt.x,pt.y)==othercol)
    {
      if (this->libertiesAt(pt.x,pt.y)==1)
      {
        if (removeGroup(this->groupAt(pt.x,pt.y))==1)
        {
          if (poskox==-1 && poskoy==-1)
          {
            poskox=pt.x;
            poskoy=pt.y;
          }
          else if (!(poskox==-2 && poskoy==-2))
          {
            poskox=-2;
            poskoy=-2;
          }
        }
      }
      else
        this->groupAt(pt.x,pt.y)->removeLiberty(this->vertexAt(x,y));
    }
    else if (this->colorAt(pt.x,pt.y)==col)
    {
      bool found=false;
      for(std::list<Go::IncrementalBoard::Group*>::iterator iter=friendlygroups->begin();iter!=friendlygroups->end();++iter)
      {
        if ((*iter)==this->groupAt(pt.x,pt.y))
        {
          found=true;
          break;
        }
      }
      if (!found)
        friendlygroups->push_back(this->groupAt(pt.x,pt.y));
    }
  }
  
  if (y>0)
  {
    Go::IncrementalBoard::Point pt={x,y-1};
    if (this->colorAt(pt.x,pt.y)==othercol)
    {
      if (this->libertiesAt(pt.x,pt.y)==1)
      {
        if (removeGroup(this->groupAt(pt.x,pt.y))==1)
        {
          if (poskox==-1 && poskoy==-1)
          {
            poskox=pt.x;
            poskoy=pt.y;
          }
          else if (!(poskox==-2 && poskoy==-2))
          {
            poskox=-2;
            poskoy=-2;
          }
        }
      }
      else
        this->groupAt(pt.x,pt.y)->removeLiberty(this->vertexAt(x,y));
    }
    else if (this->colorAt(pt.x,pt.y)==col)
    {
      bool found=false;
      for(std::list<Go::IncrementalBoard::Group*>::iterator iter=friendlygroups->begin();iter!=friendlygroups->end();++iter)
      {
        if ((*iter)==this->groupAt(pt.x,pt.y))
        {
          found=true;
          break;
        }
      }
      if (!found)
        friendlygroups->push_back(this->groupAt(pt.x,pt.y));
    }
  }
  
  if (x<(size-1))
  {
    Go::IncrementalBoard::Point pt={x+1,y};
    if (this->colorAt(pt.x,pt.y)==othercol)
    {
      if (this->libertiesAt(pt.x,pt.y)==1)
      {
        if (removeGroup(this->groupAt(pt.x,pt.y))==1)
        {
          if (poskox==-1 && poskoy==-1)
          {
            poskox=pt.x;
            poskoy=pt.y;
          }
          else if (!(poskox==-2 && poskoy==-2))
          {
            poskox=-2;
            poskoy=-2;
          }
        }
      }
      else
        this->groupAt(pt.x,pt.y)->removeLiberty(this->vertexAt(x,y));
    }
    else if (this->colorAt(pt.x,pt.y)==col)
    {
      bool found=false;
      for(std::list<Go::IncrementalBoard::Group*>::iterator iter=friendlygroups->begin();iter!=friendlygroups->end();++iter)
      {
        if ((*iter)==this->groupAt(pt.x,pt.y))
        {
          found=true;
          break;
        }
      }
      if (!found)
        friendlygroups->push_back(this->groupAt(pt.x,pt.y));
    }
  }
  
  if (y<(size-1))
  {
    Go::IncrementalBoard::Point pt={x,y+1};
    if (this->colorAt(pt.x,pt.y)==othercol)
    {
      if (this->libertiesAt(pt.x,pt.y)==1)
      {
        if (removeGroup(this->groupAt(pt.x,pt.y))==1)
        {
          if (poskox==-1 && poskoy==-1)
          {
            poskox=pt.x;
            poskoy=pt.y;
          }
          else if (!(poskox==-2 && poskoy==-2))
          {
            poskox=-2;
            poskoy=-2;
          }
        }
      }
      else
        this->groupAt(pt.x,pt.y)->removeLiberty(this->vertexAt(x,y));
    }
    else if (this->colorAt(pt.x,pt.y)==col)
    {
      bool found=false;
      for(std::list<Go::IncrementalBoard::Group*>::iterator iter=friendlygroups->begin();iter!=friendlygroups->end();++iter)
      {
        if ((*iter)==this->groupAt(pt.x,pt.y))
        {
          found=true;
          break;
        }
      }
      if (!found)
        friendlygroups->push_back(this->groupAt(pt.x,pt.y));
    }
  }
  
  if (poskox>=0 && poskoy>=0)
    this->setKo(poskox,poskoy);
  else
    this->setKo(-1,-1);
  
  this->setColorAt(x,y,col);
  
  if (friendlygroups->size()>0)
  {
    Go::IncrementalBoard::Group *firstgroup=friendlygroups->front();
    firstgroup->addStone(this->vertexAt(x,y));
    this->setGroupAt(x,y,firstgroup);
    this->addDirectLiberties(x,y,firstgroup);
    
    if (friendlygroups->size()>1)
    {
      for(std::list<Go::IncrementalBoard::Group*>::iterator iter=friendlygroups->begin();iter!=friendlygroups->end();++iter)
      {
        if ((*iter)==friendlygroups->front())
          continue;
        
        this->mergeGroups(firstgroup,(*iter));
      }
    }
    
    firstgroup->removeLiberty(this->vertexAt(x,y));
  }
  else
  {
    Go::IncrementalBoard::Group *newgroup = new Go::IncrementalBoard::Group();
    newgroup->addStone(this->vertexAt(x,y));
    this->setGroupAt(x,y,newgroup);
    this->addDirectLiberties(x,y,newgroup);
    groups.push_back(newgroup);
  }
  
  friendlygroups->resize(0);
  delete friendlygroups;
  
  nexttomove=Go::otherColor(nexttomove);
  movesmade++;
}

int Go::IncrementalBoard::removeGroup(Go::IncrementalBoard::Group *group)
{
  int s=group->numOfStones();
  
  groups.remove(group);
  
  for(std::list<Go::IncrementalBoard::Vertex*>::iterator iter=group->getStones()->begin();iter!=group->getStones()->end();++iter) 
  {
    Go::IncrementalBoard::Vertex *vert=(*iter);
    
    Go::Color othercol=Go::otherColor(vert->color);
    int x=vert->point.x,y=vert->point.y;
    
    if (x>0 && this->colorAt(x-1,y)==othercol)
      this->groupAt(x-1,y)->addLiberty(this->vertexAt(x,y));
    if (y>0 && this->colorAt(x,y-1)==othercol)
      this->groupAt(x,y-1)->addLiberty(this->vertexAt(x,y));
    if (x<(size-1) && this->colorAt(x+1,y)==othercol)
      this->groupAt(x+1,y)->addLiberty(this->vertexAt(x,y));
    if (y<(size-1) && this->colorAt(x,y+1)==othercol)
      this->groupAt(x,y+1)->addLiberty(this->vertexAt(x,y));
    
    vert->color=Go::EMPTY;
    vert->group=NULL;
  }
  
  delete group;
  
  return s;
}

void Go::IncrementalBoard::mergeGroups(Go::IncrementalBoard::Group *first, Go::IncrementalBoard::Group *second)
{
  groups.remove(second);
  
  for(std::list<Go::IncrementalBoard::Vertex*>::iterator iter=second->getStones()->begin();iter!=second->getStones()->end();++iter) 
  {
    (*iter)->group=first;
    first->addStone((*iter));
  }
  
  for(std::list<Go::IncrementalBoard::Vertex*>::iterator iter=second->getLiberties()->begin();iter!=second->getLiberties()->end();++iter) 
  {
    first->addLiberty((*iter));
  }
  
  delete second;
}

int Go::IncrementalBoard::score()
{
  Go::IncrementalBoard::ScoreVertex *scoredata;
  
  scoredata=new Go::IncrementalBoard::ScoreVertex[size*size];
  for (int x=0;x<size;x++)
  {
    for (int y=0;y<size;y++)
    {
      scoredata[y*size+x].touched=false;
      scoredata[y*size+x].color=Go::EMPTY;
    }
  }
  
  for (int x=0;x<size;x++)
  {
    for (int y=0;y<size;y++)
    {
      if (!scoredata[y*size+x].touched && this->colorAt(x,y)!=EMPTY)
      {
        this->spreadScore(scoredata,x,y,this->colorAt(x,y));
      }
    }
  }
  
  int s=0;
  
  for (int x=0;x<size;x++)
  {
    for (int y=0;y<size;y++)
    {
      Go::Color col=scoredata[y*size+x].color;
      if (col==Go::BLACK)
      {
        s++;
        //fprintf(stderr,"B");
      }
      else if (col==Go::WHITE)
      {
        s--;
        //fprintf(stderr,"W");
      }
      else
      {
        //fprintf(stderr,".");
      }
    }
    //fprintf(stderr,"\n");
  }
  
  delete[] scoredata;
  return s;
}

void Go::IncrementalBoard::spreadScore(Go::IncrementalBoard::ScoreVertex *scoredata, int x, int y, Go::Color col)
{
  bool wastouched=scoredata[y*size+x].touched;
  
  if (this->colorAt(x,y)!=Go::EMPTY && col==Go::EMPTY)
    return;
  
  if (this->colorAt(x,y)!=Go::EMPTY)
  {
    scoredata[y*size+x].touched=true;
    scoredata[y*size+x].color=this->colorAt(x,y);
    if (!wastouched)
    {
      if (x>0)
        this->spreadScore(scoredata,x-1,y,this->colorAt(x,y));
      
      if (y>0)
        this->spreadScore(scoredata,x,y-1,this->colorAt(x,y));
      
      if (x<(size-1))
        this->spreadScore(scoredata,x+1,y,this->colorAt(x,y));
      
      if (y<(size-1))
        this->spreadScore(scoredata,x,y+1,this->colorAt(x,y));
    }
    return;
  }
  
  if (scoredata[y*size+x].touched && scoredata[y*size+x].color==Go::EMPTY && col==Go::EMPTY)
    return;
  
  if (scoredata[y*size+x].touched && scoredata[y*size+x].color!=col)
    col=Go::EMPTY;
  
  if (scoredata[y*size+x].touched && scoredata[y*size+x].color==col)
    return;
  
  scoredata[y*size+x].touched=true;
  scoredata[y*size+x].color=col;
  
  if (x>0)
    this->spreadScore(scoredata,x-1,y,col);
  
  if (y>0)
    this->spreadScore(scoredata,x,y-1,col);
  
  if (x<(size-1))
    this->spreadScore(scoredata,x+1,y,col);
  
  if (y<(size-1))
    this->spreadScore(scoredata,x,y+1,col);
}

bool Go::IncrementalBoard::weakEye(Go::Color col, int x, int y)
{
  if (directLiberties(x,y)>0)
    return false;
  else
  {
    if (x>0 && (this->colorAt(x-1,y)!=col || this->libertiesAt(x-1,y)<2))
      return false;
    
    if (y>0 && (this->colorAt(x,y-1)!=col || this->libertiesAt(x,y-1)<2))
      return false;
    
    if (x<(size-1) && (this->colorAt(x+1,y)!=col || this->libertiesAt(x+1,y)<2))
      return false;
    
    if (y<(size-1) && (this->colorAt(x,y+1)!=col || this->libertiesAt(x,y+1)<2))
      return false;
    
    return true;
  }
}

void Go::IncrementalBoard::print()
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

void Go::IncrementalBoard::Group::addStone(Go::IncrementalBoard::Vertex *stone)
{
  for(std::list<Go::IncrementalBoard::Vertex*>::iterator iter=stones.begin();iter!=stones.end();++iter) 
  {
    if ((*iter)->point.x==stone->point.x && (*iter)->point.y==stone->point.y)
      return;
  }
  
  stones.push_back(stone);
}

void Go::IncrementalBoard::Group::addLiberty(Go::IncrementalBoard::Vertex *liberty)
{
  for(std::list<Go::IncrementalBoard::Vertex*>::iterator iter=liberties.begin();iter!=liberties.end();++iter) 
  {
    if ((*iter)->point.x==liberty->point.x && (*iter)->point.y==liberty->point.y)
      return;
  }
  
  liberties.push_back(liberty);
}

void Go::IncrementalBoard::Group::removeLiberty(Go::IncrementalBoard::Vertex *liberty)
{
  liberties.remove(liberty);
}

