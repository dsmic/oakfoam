#include "Go.h"

Go::BitBoard::BitBoard(int s)
{
  size=s;
  sizesq=s*s;
  sizedata=1+(s+1)*(s+2);
  data=new bool[sizedata];
  for (int i=0;i<sizedata;i++)
    data[i]=false;
}

Go::BitBoard::~BitBoard()
{
  delete[] data;
}

std::string Go::Move::toString(int boardsize)
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
    ss<<"("<<this->getX(boardsize)<<","<<this->getY(boardsize)<<")\n";
    return ss.str();
  }
}

Go::Group::Group(Go::Color col, int size)
{
  color=col;
  stonesboard=new Go::BitBoard(size);
  libertiesboard=new Go::BitBoard(size);
}

Go::Group::~Group()
{
  delete stonesboard;
  delete libertiesboard;
}

void Go::Group::addStone(int pos)
{
  if (!stonesboard->get(pos))
  {
    stonesboard->set(pos);
    stoneslist.push_back(pos);
  }
}

void Go::Group::addLiberty(int pos)
{
  if (!libertiesboard->get(pos))
  {
    libertiesboard->set(pos);
    libertieslist.push_back(pos);
  }
}

void Go::Group::removeLiberty(int pos)
{
  if (libertiesboard->get(pos))
  {
    libertiesboard->clear(pos);
    libertieslist.remove(pos);
  }
}

Go::Board::Board(int s)
{
  size=s;
  sizesq=s*s;
  sizedata=1+(s+1)*(s+2);
  data=new Go::Vertex[sizedata];
  
  for (int p=0;p<sizedata;p++)
  {
    if (p<=(size) || p>=(sizedata-size-1) || (p%(size+1))==0)
      this->setColor(p,Go::OFFBOARD);
    else
      this->setColor(p,Go::EMPTY);
    this->setGroup(p,NULL);
  }
  
  movesmade=0;
  nexttomove=Go::BLACK;
  passesplayed=0;
  simpleko=-1;
  
  blackvalidmoves=new Go::BitBoard(size);
  whitevalidmoves=new Go::BitBoard(size);
  this->refreshValidMoves();
}

Go::Board::~Board()
{
  delete blackvalidmoves;
  delete whitevalidmoves;
  
  for(std::list<Go::Group*>::iterator iter=groups.begin();iter!=groups.end();++iter) 
  {
    delete (*iter);
  }
  groups.resize(0);
  
  delete[] data;
}

Go::Board *Go::Board::copy()
{
  Go::Board *copyboard;
  copyboard=new Go::Board(size);
  
  for (int p=0;p<sizedata;p++)
  {
    copyboard->data[p]=this->data[p];
  }
  
  copyboard->movesmade=this->movesmade;
  copyboard->nexttomove=this->nexttomove;
  copyboard->passesplayed=this->passesplayed;
  copyboard->simpleko=this->simpleko;
  
  copyboard->refreshGroups();
  
  return copyboard;
}

std::string Go::Board::toString()
{
  std::ostringstream ss;
  
  for (int y=size-1;y>=0;y--)
  {
    for (int x=0;x<size;x++)
    {
      Go::Color col=this->getColor(Go::Position::xy2pos(x,y,size));
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

int Go::Board::score()
{
  Go::Board::ScoreVertex *scoredata;
  
  scoredata=new Go::Board::ScoreVertex[sizedata];
  for (int p=0;p<sizedata;p++)
  {
    if (this->getColor(p)==Go::OFFBOARD)
    {
      scoredata[p].touched=true;
      scoredata[p].color=Go::EMPTY;
    }
    else
    {
      scoredata[p].touched=false;
      scoredata[p].color=Go::EMPTY;
    }
  }
  
  for (int p=0;p<sizedata;p++)
  {
    if (!scoredata[p].touched && this->getColor(p)!=EMPTY)
    {
      this->spreadScore(scoredata,p,this->getColor(p));
    }
  }
  
  int s=0;
  
  for (int p=0;p<sizedata;p++)
  {
    Go::Color col=scoredata[p].color;
    if (col==Go::BLACK)
      s++;
    else if (col==Go::WHITE)
      s--;
  }
  
  delete[] scoredata;
  return s;
}

void Go::Board::spreadScore(Go::Board::ScoreVertex *scoredata, int pos, Go::Color col)
{
  bool wastouched=scoredata[pos].touched;
  
  if (this->getColor(pos)==Go::OFFBOARD)
    return;
  
  if (this->getColor(pos)!=Go::EMPTY && col==Go::EMPTY)
    return;
  
  if (this->getColor(pos)!=Go::EMPTY)
  {
    scoredata[pos].touched=true;
    scoredata[pos].color=this->getColor(pos);
    if (!wastouched)
    {
      foreach_adjacent(pos,{
        this->spreadScore(scoredata,p,this->getColor(pos));
      });
    }
    return;
  }
  
  if (scoredata[pos].touched && scoredata[pos].color==Go::EMPTY && col==Go::EMPTY)
    return;
  
  if (scoredata[pos].touched && scoredata[pos].color!=col)
    col=Go::EMPTY;
  
  if (scoredata[pos].touched && scoredata[pos].color==col)
    return;
  
  scoredata[pos].touched=true;
  scoredata[pos].color=col;
  
  foreach_adjacent(pos,{
    this->spreadScore(scoredata,p,col);
  });
}

bool Go::Board::validMove(Go::Move move)
{
  Go::BitBoard *validmoves=(move.getColor()==Go::BLACK?blackvalidmoves:whitevalidmoves);
  return move.isPass() || move.isResign() || validmoves->get(move.getPosition());
}

bool Go::Board::validMoveCheck(Go::Move move)
{
  if (move.isPass() || move.isResign())
    return true;
  
  if (this->getColor(move.getPosition())!=Go::EMPTY)
    return false;
  else if (touchingAtLeastOneEmpty(move.getPosition()))
    return true;
  else
  {
    int pos=move.getPosition();
    Go::Color col=move.getColor();
    Go::Color othercol=Go::otherColor(col);
    int captures=0;
    
    foreach_adjacent(pos,{
      if (this->getColor(p)==col && this->getLiberties(p)>1)
        return true;
      else if (this->getColor(p)==othercol && this->getLiberties(p)==1)
      {
        captures+=this->getGroupSize(p);
        if (captures>1)
          return true;
      }
    });
    
    if (captures==1)
    {
      if (pos==simpleko)
        return false;
      else
        return true;
    }
    
    return false;
  }
}

void Go::Board::makeMove(Go::Move move)
{
  if (simpleko!=-1)
  {
    int kopos=simpleko;
    simpleko=-1;
    if (this->validMoveCheck(Go::Move(move.getColor(),kopos)))
      this->addValidMove(Go::Move(move.getColor(),kopos));
  }
  
  if (move.isPass() || move.isResign())
  {
    if (move.isPass())
      passesplayed++;
    nexttomove=Go::otherColor(move.getColor());
    movesmade++;
    return;
  }
  
  if (!this->validMove(move))
  {
    fprintf(stderr,"invalid move at %d,%d\n",move.getX(size),move.getY(size));
    throw Go::Exception("invalid move");
  }
  
  movesmade++;
  nexttomove=Go::otherColor(nexttomove);
  passesplayed=0;
  
  Go::Color col=move.getColor();
  Go::Color othercol=Go::otherColor(col);
  int pos=move.getPosition();
  int posko=-1;
  
  this->setColor(move.getPosition(),move.getColor());
  
  Go::Group *thisgroup = new Go::Group(col,size);
  thisgroup->addStone(pos);
  this->setGroup(pos,thisgroup);
  this->addDirectLiberties(pos,thisgroup);
  groups.push_back(thisgroup);
  
  this->removeValidMove(Go::Move(Go::BLACK,pos));
  this->removeValidMove(Go::Move(Go::WHITE,pos));
  
  foreach_adjacent(pos,{
    if (this->getColor(p)!=Go::EMPTY && this->getColor(p)!=Go::OFFBOARD)
    {
      this->getGroup(p)->removeLiberty(pos);
      if (this->getColor(p)==col)
      {
        if (this->getGroup(p)->numOfStones()>thisgroup->numOfStones())
        {
          this->mergeGroups(this->getGroup(p),thisgroup);
          thisgroup=this->getGroup(p);
        }
        else
          this->mergeGroups(thisgroup,this->getGroup(p));
      }
      else if (this->getColor(p)==othercol)
      {
        if (this->getGroup(p)->numOfLiberties()==0)
        {
          if (removeGroup(this->getGroup(p))==1)
          {
            if (posko==-1)
              posko=p;
            else
              posko=-2;
          }
        }
        else if (this->getGroup(p)->numOfLiberties()==1)
        {
          int liberty=this->getGroup(p)->getLibertiesList()->front();
          this->addValidMove(Go::Move(col,liberty));
          if (this->touchingEmpty(liberty)==0 && !this->validMoveCheck(Go::Move(othercol,liberty)))
            this->removeValidMove(Go::Move(othercol,liberty));
        }
      }
    }
  });
  
  if (thisgroup->numOfLiberties()==1)
  {
    int liberty=thisgroup->getLibertiesList()->front();
    if (this->touchingEmpty(liberty)==0 && !this->validMoveCheck(Go::Move(col,liberty)))
      this->removeValidMove(Go::Move(col,liberty));
    if (this->validMoveCheck(Go::Move(othercol,liberty)))
      this->addValidMove(Go::Move(othercol,liberty));
  }
  
  foreach_adjacent(pos,{
    if (this->getColor(p)==Go::EMPTY)
    {
      if (!this->validMoveCheck(Go::Move(othercol,p)))
        this->removeValidMove(Go::Move(othercol,p));
    }
  });
  
  if (posko>=0)
  {
    simpleko=posko;
    if (!this->validMoveCheck(Go::Move(othercol,simpleko)))
      this->removeValidMove(Go::Move(othercol,simpleko));
  }
}

void Go::Board::refreshValidMoves()
{
  this->refreshValidMoves(Go::BLACK);
  this->refreshValidMoves(Go::WHITE);
}

void Go::Board::refreshValidMoves(Go::Color col)
{
  (col==Go::BLACK?blackvalidmoves:whitevalidmoves)->clear();
  (col==Go::BLACK?blackvalidmovecount:whitevalidmovecount)=0;
  
  for (int p=0;p<sizedata;p++)
  {
    if (this->validMoveCheck(Go::Move(col,p)))
    {
      this->addValidMove(Go::Move(col,p));
    }
  }
}

void Go::Board::addValidMove(Go::Move move)
{
  Go::BitBoard *validmoves=(move.getColor()==Go::BLACK?blackvalidmoves:whitevalidmoves);
  int *validmovecount=(move.getColor()==Go::BLACK?&blackvalidmovecount:&whitevalidmovecount);
  
  if (!validmoves->get(move.getPosition()))
  {
    validmoves->set(move.getPosition());
    (*validmovecount)++;
  }
}

void Go::Board::removeValidMove(Go::Move move)
{
  Go::BitBoard *validmoves=(move.getColor()==Go::BLACK?blackvalidmoves:whitevalidmoves);
  int *validmovecount=(move.getColor()==Go::BLACK?&blackvalidmovecount:&whitevalidmovecount);
  
  if (validmoves->get(move.getPosition()))
  {
    validmoves->clear(move.getPosition());
    (*validmovecount)--;
  }
}

int Go::Board::touchingEmpty(int pos)
{
  int lib=0;
  
  foreach_adjacent(pos,{
    if (this->getColor(p)==Go::EMPTY)
      lib++;
  });
  
  return lib;
}

bool Go::Board::touchingAtLeastOneEmpty(int pos)
{
  foreach_adjacent(pos,{
    if (this->getColor(p)==Go::EMPTY)
      return true;
  });
  
  return false;
}

void Go::Board::refreshGroups()
{
  for (int p=0;p<sizedata;p++)
  {
    this->setGroup(p,NULL);
  }
  
  for(std::list<Go::Group*>::iterator iter=groups.begin();iter!=groups.end();++iter) 
  {
    delete (*iter);
  }
  groups.resize(0);
  
  for (int p=0;p<sizedata;p++)
  {
    if (this->getColor(p)!=Go::EMPTY && this->getColor(p)!=Go::OFFBOARD && this->getGroup(p)==NULL)
    {
      Go::Group *newgroup = new Go::Group(this->getColor(p),size);
      
      this->spreadGroup(p,newgroup);
      groups.push_back(newgroup);
    }
  }
  
  this->refreshValidMoves();
}

void Go::Board::spreadGroup(int pos, Go::Group *group)
{
  if (this->getColor(pos)==group->getColor() && this->getGroup(pos)==NULL)
  {
    this->setGroup(pos,group);
    group->addStone(pos);
    this->addDirectLiberties(pos,group);
    
    foreach_adjacent(pos,{
      this->spreadGroup(p,group);
    });
  }
}

void Go::Board::addDirectLiberties(int pos, Go::Group *group)
{
  foreach_adjacent(pos,{
    if (this->getColor(p)==Go::EMPTY)
      group->addLiberty(p);
  });
}

int Go::Board::removeGroup(Go::Group *group)
{
  int s=group->numOfStones();
  Go::Color groupcol=group->getColor();
  
  groups.remove(group);
  
  std::list<int> *possiblesuicides = new std::list<int>();
  
  for(std::list<int>::iterator iter=group->getStonesList()->begin();iter!=group->getStonesList()->end();++iter)
  {
    int pos=(*iter);
    
    Go::Color col=this->getColor(pos);
    Go::Color othercol=Go::otherColor(col);
    
    foreach_adjacent(pos,{
      if (this->getColor(p)==othercol)
      {
        if (this->getGroup(p)->numOfLiberties()==1)
        {
          int liberty=this->getGroup(p)->getLibertiesList()->front();
          this->addValidMove(Go::Move(othercol,liberty));
          possiblesuicides->push_back(liberty);
        }
        this->getGroup(p)->addLiberty(pos);
      }
    });
    
    this->addValidMove(Go::Move(othercol,pos));
    this->addValidMove(Go::Move(col,pos));
    
    this->setColor(pos,Go::EMPTY);
    this->setGroup(pos,NULL);
  }
  
  for(std::list<int>::iterator iter=possiblesuicides->begin();iter!=possiblesuicides->end();++iter)
  {
    if (!this->validMoveCheck(Go::Move(groupcol,(*iter)))) 
      this->removeValidMove(Go::Move(groupcol,(*iter)));
  }
  
  possiblesuicides->resize(0);
  delete possiblesuicides;
  
  delete group;
  
  return s;
}

void Go::Board::mergeGroups(Go::Group *first, Go::Group *second)
{
  if (first==second)
    return;
  
  groups.remove(second);
  
  for(std::list<int>::iterator iter=second->getStonesList()->begin();iter!=second->getStonesList()->end();++iter) 
  {
    this->setGroup((*iter),first);
    first->addStone((*iter));
  }
  
  for(std::list<int>::iterator iter=second->getLibertiesList()->begin();iter!=second->getLibertiesList()->end();++iter) 
  {
    first->addLiberty((*iter));
  }
  
  delete second;
}

bool Go::Board::weakEye(Go::Color col, int pos)
{
  if (col==Go::EMPTY)
    return false;
  else
  {
    foreach_adjacent(pos,{
      if (this->getColor(p)!=Go::OFFBOARD && (this->getColor(p)!=col || this->getLiberties(p)<2))
        return false;
    });
    
    return true;
  }
}


