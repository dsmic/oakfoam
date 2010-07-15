#include "Go.h"

std::string Go::Move::toString()
{
  if (this->isPass())
    return "PASS\n";
  else if (this->isResign())
    return "RESIGN\n";
  else
  {
    std::ostringstream ss;
    if (color==Go::BLACK)
      ss<<"B";
    else if (color==Go::WHITE)
      ss<<"W";
    ss<<"("<<x<<","<<y<<")\n";
    return ss.str();
  }
}

Go::Board::Board(int s)
{
  size=s;
  data=new Go::Board::Vertex[size*size];
  for (int x=0;x<size;x++)
  {
    for (int y=0;y<size;y++)
    {
      Go::Board::Point pt={x,y};
      this->vertexAt(x,y)->point=pt;
      this->setColorAt(x,y,Go::EMPTY);
      this->setGroupAt(x,y,NULL);
    }
  }
  this->setKo(-1,-1);
  nexttomove=Go::BLACK;
  passesplayed=0;
  movesmade=0;
  
  blackvalidmoves=new Go::BitBoard(size);
  whitevalidmoves=new Go::BitBoard(size);
  
  this->refreshValidMoves(Go::BLACK);
  this->refreshValidMoves(Go::WHITE);
}

Go::Board::~Board()
{
  delete blackvalidmoves;
  delete whitevalidmoves;
  
  for(std::list<Go::Board::Group*>::iterator iter=groups.begin();iter!=groups.end();++iter) 
  {
    delete (*iter);
  }
  groups.resize(0);
  
  delete[] data;
}

inline void Go::Board::checkCoords(int x, int y)
{
  if (x<0 || y<0 || x>(size-1) || y>(size-1))
    throw Go::Exception("invalid coords");
}

inline Go::Board::Vertex *Go::Board::vertexAt(int x, int y)
{
  return &data[y*size+x];
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

inline Go::Board::Group *Go::Board::groupAt(int x, int y)
{
  //this->checkCoords(x,y);
  return data[y*size+x].group;
}

inline void Go::Board::setGroupAt(int x, int y, Go::Board::Group *group)
{
  //this->checkCoords(x,y);
  data[y*size+x].group=group;
}

inline int Go::Board::libertiesAt(int x, int y)
{
  //this->checkCoords(x,y);
  if (data[y*size+x].group==NULL)
    return 0;
  else
    return data[y*size+x].group->numOfLiberties();
}

inline int Go::Board::groupSizeAt(int x, int y)
{
  //this->checkCoords(x,y);
  if (data[y*size+x].group==NULL)
    return 0;
  else
    return data[y*size+x].group->numOfStones();
}

inline void Go::Board::setKo(int x, int y)
{
  koX=x;
  koY=y;
}

Go::Board *Go::Board::copy()
{
  Go::Board *copyboard;
  copyboard=new Go::Board(size);
  
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

bool Go::Board::validMoveCheck(Go::Move move)
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

inline int Go::Board::directLiberties(int x, int y)
{
  int lib=0;
  //this->checkCoords(x,y);
  
  if (x>0 && this->colorAt(x-1,y)==Go::EMPTY)
    ++lib;
  if (y>0 && this->colorAt(x,y-1)==Go::EMPTY)
    ++lib;
  if (x<(size-1) && this->colorAt(x+1,y)==Go::EMPTY)
    ++lib;
  if (y<(size-1) && this->colorAt(x,y+1)==Go::EMPTY)
    ++lib;
  
  return lib;
}

void Go::Board::refreshGroups()
{
  for (int x=0;x<size;x++)
  {
    for (int y=0;y<size;y++)
    {
      this->setGroupAt(x,y,NULL);
    }
  }
  
  for(std::list<Go::Board::Group*>::iterator iter=groups.begin();iter!=groups.end();++iter) 
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
        Go::Board::Group *newgroup = new Go::Board::Group(size);
        
        this->spreadGroup(x,y,this->colorAt(x,y),newgroup);
        groups.push_back(newgroup);
      }
    }
  }
  
  this->refreshValidMoves(Go::BLACK);
  this->refreshValidMoves(Go::WHITE);
}

void Go::Board::spreadGroup(int x, int y, Go::Color col, Go::Board::Group *group)
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

void Go::Board::addDirectLiberties(int x, int y, Go::Board::Group *group)
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

void Go::Board::makeMove(Go::Move move)
{
  if (koX!=-1 && koY!=-1)
  {
    int x=koX,y=koY;
    this->setKo(-1,-1);
    if (this->validMoveCheck(Go::Move(Go::BLACK,x,y)))
      this->addValidMove(Go::Move(Go::BLACK,x,y));
    if (this->validMoveCheck(Go::Move(Go::WHITE,x,y)))
      this->addValidMove(Go::Move(Go::WHITE,x,y));
  }
  
  if (move.isPass() || move.isResign())
  {
    if (move.isPass())
      passesplayed++;
    nexttomove=Go::otherColor(move.getColor());
    movesmade++;
    return;
  }
  
  this->checkCoords(move.getX(),move.getY());
  if (!this->validMove(move))
  {
    fprintf(stderr,"invalid move at %d,%d\n",move.getX(),move.getY());
    throw Go::Exception("invalid move");
  }
  
  int x=move.getX();
  int y=move.getY();
  Go::Color col=move.getColor();
  Go::Color othercol=Go::otherColor(col);
  int poskox=-1;
  int poskoy=-1;
  
  passesplayed=0;
  
  std::list<Go::Board::Group*> *friendlygroups = new std::list<Go::Board::Group*>();
  std::list<Go::Board::Group*> *enemygroups = new std::list<Go::Board::Group*>();
  
  if (x>0)
  {
    Go::Board::Point pt={x-1,y};
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
        enemygroups->push_back(this->groupAt(pt.x,pt.y));
    }
    else if (this->colorAt(pt.x,pt.y)==col)
    {
      bool found=false;
      for(std::list<Go::Board::Group*>::iterator iter=friendlygroups->begin();iter!=friendlygroups->end();++iter)
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
    Go::Board::Point pt={x,y-1};
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
        enemygroups->push_back(this->groupAt(pt.x,pt.y));
    }
    else if (this->colorAt(pt.x,pt.y)==col)
    {
      bool found=false;
      for(std::list<Go::Board::Group*>::iterator iter=friendlygroups->begin();iter!=friendlygroups->end();++iter)
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
    Go::Board::Point pt={x+1,y};
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
        enemygroups->push_back(this->groupAt(pt.x,pt.y));
    }
    else if (this->colorAt(pt.x,pt.y)==col)
    {
      bool found=false;
      for(std::list<Go::Board::Group*>::iterator iter=friendlygroups->begin();iter!=friendlygroups->end();++iter)
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
    Go::Board::Point pt={x,y+1};
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
        enemygroups->push_back(this->groupAt(pt.x,pt.y));
    }
    else if (this->colorAt(pt.x,pt.y)==col)
    {
      bool found=false;
      for(std::list<Go::Board::Group*>::iterator iter=friendlygroups->begin();iter!=friendlygroups->end();++iter)
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
  
  this->setColorAt(x,y,col);
  
  if (friendlygroups->size()>0)
  {
    Go::Board::Group *firstgroup=friendlygroups->front();
    firstgroup->addStone(this->vertexAt(x,y));
    this->setGroupAt(x,y,firstgroup);
    this->addDirectLiberties(x,y,firstgroup);
    
    if (friendlygroups->size()>1)
    {
      for(std::list<Go::Board::Group*>::iterator iter=friendlygroups->begin();iter!=friendlygroups->end();++iter)
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
    Go::Board::Group *newgroup = new Go::Board::Group(size);
    newgroup->addStone(this->vertexAt(x,y));
    this->setGroupAt(x,y,newgroup);
    this->addDirectLiberties(x,y,newgroup);
    groups.push_back(newgroup);
  }
  
  for(std::list<Go::Board::Group*>::iterator iter=enemygroups->begin();iter!=enemygroups->end();++iter)
  {
    (*iter)->removeLiberty(this->vertexAt(x,y));
    if ((*iter)->numOfLiberties()==1)
    {
      Go::Board::Vertex *liberty=(*iter)->getLibertiesList()->front();
      this->addValidMove(Go::Move(col,liberty->point.x,liberty->point.y));
      if (this->directLiberties(liberty->point.x,liberty->point.y)==0 && !this->validMoveCheck(Go::Move(othercol,liberty->point.x,liberty->point.y)))
        this->removeValidMove(Go::Move(othercol,liberty->point.x,liberty->point.y));
    }
  }
  
  this->removeValidMove(Go::Move(Go::BLACK,x,y));
  this->removeValidMove(Go::Move(Go::WHITE,x,y));
  if (this->groupAt(x,y)->numOfLiberties()==1)
  {
    Go::Board::Vertex *liberty=this->groupAt(x,y)->getLibertiesList()->front();
    if (this->directLiberties(liberty->point.x,liberty->point.y)==0 && !this->validMoveCheck(Go::Move(col,liberty->point.x,liberty->point.y)))
      this->removeValidMove(Go::Move(col,liberty->point.x,liberty->point.y));
    if (this->validMoveCheck(Go::Move(othercol,liberty->point.x,liberty->point.y)))
      this->addValidMove(Go::Move(othercol,liberty->point.x,liberty->point.y));
  }
  
  if (x>0 && this->colorAt(x-1,y)==Go::EMPTY)
  {
    if (!this->validMoveCheck(Go::Move(othercol,x-1,y)))
      this->removeValidMove(Go::Move(othercol,x-1,y));
  }
  if (y>0 && this->colorAt(x,y-1)==Go::EMPTY)
  {
    if (!this->validMoveCheck(Go::Move(othercol,x,y-1)))
      this->removeValidMove(Go::Move(othercol,x,y-1));
  }
  if (x<(size-1) && this->colorAt(x+1,y)==Go::EMPTY)
  {
    if (!this->validMoveCheck(Go::Move(othercol,x+1,y)))
      this->removeValidMove(Go::Move(othercol,x+1,y));
  }
  if (y<(size-1) && this->colorAt(x,y+1)==Go::EMPTY)
  {
    if (!this->validMoveCheck(Go::Move(othercol,x,y+1)))
      this->removeValidMove(Go::Move(othercol,x,y+1));
  }
  
  friendlygroups->resize(0);
  delete friendlygroups;
  enemygroups->resize(0);
  delete enemygroups;
  
  nexttomove=Go::otherColor(nexttomove);
  movesmade++;
  
  if (poskox>=0 && poskoy>=0)
  {
    this->setKo(poskox,poskoy);
    if (!this->validMoveCheck(Go::Move(othercol,koX,koY)))
      this->removeValidMove(Go::Move(othercol,koX,koY));
  }
}

int Go::Board::removeGroup(Go::Board::Group *group)
{
  int s=group->numOfStones();
  Go::Color groupcol=group->getStonesList()->front()->color;
  
  groups.remove(group);
  
  std::list<Go::Board::Point> *possiblesuicides = new std::list<Go::Board::Point>();
  
  for(std::list<Go::Board::Vertex*>::iterator iter=group->getStonesList()->begin();iter!=group->getStonesList()->end();++iter)
  {
    Go::Board::Vertex *vert=(*iter);
    
    Go::Color col=vert->color;
    Go::Color othercol=Go::otherColor(col);
    int x=vert->point.x,y=vert->point.y;
    
    if (x>0 && this->colorAt(x-1,y)==othercol)
    {
      if (this->groupAt(x-1,y)->numOfLiberties()==1)
      {
        Go::Board::Vertex *liberty=this->groupAt(x-1,y)->getLibertiesList()->front();
        this->addValidMove(Go::Move(othercol,liberty->point.x,liberty->point.y));
        possiblesuicides->push_back(liberty->point);
      }
      this->groupAt(x-1,y)->addLiberty(this->vertexAt(x,y));
    }
    if (y>0 && this->colorAt(x,y-1)==othercol)
    {
      if (this->groupAt(x,y-1)->numOfLiberties()==1)
      {
        Go::Board::Vertex *liberty=this->groupAt(x,y-1)->getLibertiesList()->front();
        this->addValidMove(Go::Move(othercol,liberty->point.x,liberty->point.y));
        possiblesuicides->push_back(liberty->point);
      }
      this->groupAt(x,y-1)->addLiberty(this->vertexAt(x,y));
    }
    if (x<(size-1) && this->colorAt(x+1,y)==othercol)
    {
      if (this->groupAt(x+1,y)->numOfLiberties()==1)
      {
        Go::Board::Vertex *liberty=this->groupAt(x+1,y)->getLibertiesList()->front();
        this->addValidMove(Go::Move(othercol,liberty->point.x,liberty->point.y));
        possiblesuicides->push_back(liberty->point);
      }
      this->groupAt(x+1,y)->addLiberty(this->vertexAt(x,y));
    }
    if (y<(size-1) && this->colorAt(x,y+1)==othercol)
    {
      if (this->groupAt(x,y+1)->numOfLiberties()==1)
      {
        Go::Board::Vertex *liberty=this->groupAt(x,y+1)->getLibertiesList()->front();
        this->addValidMove(Go::Move(othercol,liberty->point.x,liberty->point.y));
        possiblesuicides->push_back(liberty->point);
      }
      this->groupAt(x,y+1)->addLiberty(this->vertexAt(x,y));
    }
    
    this->addValidMove(Go::Move(othercol,x,y));
    this->addValidMove(Go::Move(col,x,y));
    
    vert->color=Go::EMPTY;
    vert->group=NULL;
  }
  
  for(std::list<Go::Board::Point>::iterator iter=possiblesuicides->begin();iter!=possiblesuicides->end();++iter)
  {
    if (!this->validMoveCheck(Go::Move(groupcol,(*iter).x,(*iter).y))) 
      this->removeValidMove(Go::Move(groupcol,(*iter).x,(*iter).y));
  }
  
  possiblesuicides->resize(0);
  delete possiblesuicides;
  
  delete group;
  
  return s;
}

void Go::Board::mergeGroups(Go::Board::Group *first, Go::Board::Group *second)
{
  groups.remove(second);
  
  for(std::list<Go::Board::Vertex*>::iterator iter=second->getStonesList()->begin();iter!=second->getStonesList()->end();++iter) 
  {
    (*iter)->group=first;
    first->addStone((*iter));
  }
  
  for(std::list<Go::Board::Vertex*>::iterator iter=second->getLibertiesList()->begin();iter!=second->getLibertiesList()->end();++iter) 
  {
    first->addLiberty((*iter));
  }
  
  delete second;
}

int Go::Board::score()
{
  Go::Board::ScoreVertex *scoredata;
  
  scoredata=new Go::Board::ScoreVertex[size*size];
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
        s++;
      else if (col==Go::WHITE)
        s--;
    }
  }
  
  delete[] scoredata;
  return s;
}

void Go::Board::spreadScore(Go::Board::ScoreVertex *scoredata, int x, int y, Go::Color col)
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

bool Go::Board::weakEye(Go::Color col, int x, int y)
{
  if (col==Go::EMPTY)
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

std::string Go::Board::toString()
{
  std::ostringstream ss;
  
  for (int y=size-1;y>=0;y--)
  {
    for (int x=0;x<size;x++)
    {
      Go::Color col=this->colorAt(x,y);
      if (col==Go::BLACK)
        ss<<"X ";
      else if (col==Go::WHITE)
        ss<<"O ";
      else
        ss<<". ";
    }
    ss<<"\n";
  }
  
  return ss.str();
}

Go::Board::Group::Group(int size)
{
  stonesboard=new Go::BitBoard(size);
  stonesboard->clear();
  libertiesboard=new Go::BitBoard(size);
  libertiesboard->clear();
}

Go::Board::Group::~Group()
{
  delete stonesboard;
  delete libertiesboard;
}

void Go::Board::Group::addStone(Go::Board::Vertex *stone)
{
  if (!stonesboard->get(stone->point.x,stone->point.y))
  {
    stonesboard->set(stone->point.x,stone->point.y);
    stoneslist.push_back(stone);
  }
}

void Go::Board::Group::addLiberty(Go::Board::Vertex *liberty)
{
  if (!libertiesboard->get(liberty->point.x,liberty->point.y))
  {
    libertiesboard->set(liberty->point.x,liberty->point.y);
    libertieslist.push_back(liberty);
  }
}

void Go::Board::Group::removeLiberty(Go::Board::Vertex *liberty)
{
  if (libertiesboard->get(liberty->point.x,liberty->point.y))
  {
    libertiesboard->clear(liberty->point.x,liberty->point.y);
    libertieslist.remove(liberty);
  }
}

void Go::Board::refreshValidMoves(Go::Color col)
{
  (col==Go::BLACK?blackvalidmoves:whitevalidmoves)->clear();
  (col==Go::BLACK?blackvalidmovecount:whitevalidmovecount)=0;
  
  for (int x=0;x<size;x++)
  {
    for (int y=0;y<size;y++)
    {
      if (this->validMoveCheck(Go::Move(col,x,y)))
      {
        this->addValidMove(Go::Move(col,x,y));
      }
    }
  }
}

bool Go::Board::validMove(Go::Move move)
{
  Go::BitBoard *validmoves=(move.getColor()==Go::BLACK?blackvalidmoves:whitevalidmoves);
  return move.isPass() || move.isResign() || validmoves->get(move.getX(),move.getY());
}

Go::BitBoard *Go::Board::getValidMoves(Go::Color col)
{
  return (col==Go::BLACK?blackvalidmoves:whitevalidmoves);
}

void Go::Board::addValidMove(Go::Move move)
{
  Go::BitBoard *validmoves=(move.getColor()==Go::BLACK?blackvalidmoves:whitevalidmoves);
  int *validmovecount=(move.getColor()==Go::BLACK?&blackvalidmovecount:&whitevalidmovecount);
  
  if (!validmoves->get(move.getX(),move.getY()))
  {
    validmoves->set(move.getX(),move.getY());
    (*validmovecount)++;
  }
}

void Go::Board::removeValidMove(Go::Move move)
{
  Go::BitBoard *validmoves=(move.getColor()==Go::BLACK?blackvalidmoves:whitevalidmoves);
  int *validmovecount=(move.getColor()==Go::BLACK?&blackvalidmovecount:&whitevalidmovecount);
  
  if (validmoves->get(move.getX(),move.getY()))
  {
    validmoves->clear(move.getX(),move.getY());
    (*validmovecount)--;
  }
}

Go::BitBoard::BitBoard(int s)
{
  size=s;
  data=new bool[size*size];
}

Go::BitBoard::~BitBoard()
{
  delete[] data;
}

