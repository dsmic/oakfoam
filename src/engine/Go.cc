#include "Go.h"

Go::BitBoard::BitBoard(int s)
{
  size=s;
  sizesq=s*s;
  sizedata=1+(s+1)*(s+2);
  data=new bool[sizedata];
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
  stonesboard->clear();
  libertiesboard=new Go::BitBoard(size);
  libertiesboard->clear();
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
      this->spreadScore(scoredata,pos+P_N,this->getColor(pos));
      this->spreadScore(scoredata,pos+P_S,this->getColor(pos));
      this->spreadScore(scoredata,pos+P_E,this->getColor(pos));
      this->spreadScore(scoredata,pos+P_W,this->getColor(pos));
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
  
  this->spreadScore(scoredata,pos+P_N,col);
  this->spreadScore(scoredata,pos+P_S,col);
  this->spreadScore(scoredata,pos+P_E,col);
  this->spreadScore(scoredata,pos+P_W,col);
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
  else if (touchingEmpty(move.getPosition())>0)
    return true;
  else
  {
    int pos=move.getPosition();
    Go::Color col=move.getColor();
    Go::Color othercol=Go::otherColor(col);
    int captures=0;
    
    if (this->getColor(pos+P_N)==col && this->getLiberties(pos+P_N)>1)
      return true;
    if (this->getColor(pos+P_S)==col && this->getLiberties(pos+P_S)>1)
      return true;
    if (this->getColor(pos+P_E)==col && this->getLiberties(pos+P_E)>1)
      return true;
    if (this->getColor(pos+P_W)==col && this->getLiberties(pos+P_W)>1)
      return true;
    
    if (this->getColor(pos+P_N)==othercol && this->getLiberties(pos+P_N)==1)
      captures+=this->getGroupSize(pos+P_N);
    if (this->getColor(pos+P_S)==othercol && this->getLiberties(pos+P_S)==1)
      captures+=this->getGroupSize(pos+P_S);
    if (this->getColor(pos+P_E)==othercol && this->getLiberties(pos+P_E)==1)
      captures+=this->getGroupSize(pos+P_E);
    if (this->getColor(pos+P_W)==othercol && this->getLiberties(pos+P_W)==1)
      captures+=this->getGroupSize(pos+P_W);
    
    if (captures>1)
      return true;
    else if (captures==1)
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
    if (this->validMoveCheck(Go::Move(Go::BLACK,kopos)))
      this->addValidMove(Go::Move(Go::BLACK,kopos));
    if (this->validMoveCheck(Go::Move(Go::WHITE,kopos)))
      this->addValidMove(Go::Move(Go::WHITE,kopos));
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
  
  passesplayed=0;
  
  Go::Color col=move.getColor();
  Go::Color othercol=Go::otherColor(col);
  int pos=move.getPosition();
  int adjpos;
  int posko=-1;
  
  std::list<Go::Group*> *friendlygroups = new std::list<Go::Group*>();
  std::list<Go::Group*> *enemygroups = new std::list<Go::Group*>();
  
  adjpos=pos+P_N;
  if (this->getColor(adjpos)==othercol)
  {
    if (this->getLiberties(adjpos)==1)
    {
      if (removeGroup(this->getGroup(adjpos))==1)
      {
        if (posko==-1)
          posko=adjpos;
        else
          posko=-2;
      }
    }
    else
      enemygroups->push_back(this->getGroup(adjpos));
  }
  else if (this->getColor(adjpos)==col)
  {
    bool found=false;
    for(std::list<Go::Group*>::iterator iter=friendlygroups->begin();iter!=friendlygroups->end();++iter)
    {
      if ((*iter)==this->getGroup(adjpos))
      {
        found=true;
        break;
      }
    }
    if (!found)
      friendlygroups->push_back(this->getGroup(adjpos));
  }
  
  adjpos=pos+P_S;
  if (this->getColor(adjpos)==othercol)
  {
    if (this->getLiberties(adjpos)==1)
    {
      if (removeGroup(this->getGroup(adjpos))==1)
      {
        if (posko==-1)
          posko=adjpos;
        else
          posko=-2;
      }
    }
    else
      enemygroups->push_back(this->getGroup(adjpos));
  }
  else if (this->getColor(adjpos)==col)
  {
    bool found=false;
    for(std::list<Go::Group*>::iterator iter=friendlygroups->begin();iter!=friendlygroups->end();++iter)
    {
      if ((*iter)==this->getGroup(adjpos))
      {
        found=true;
        break;
      }
    }
    if (!found)
      friendlygroups->push_back(this->getGroup(adjpos));
  }
  
  adjpos=pos+P_E;
  if (this->getColor(adjpos)==othercol)
  {
    if (this->getLiberties(adjpos)==1)
    {
      if (removeGroup(this->getGroup(adjpos))==1)
      {
        if (posko==-1)
          posko=adjpos;
        else
          posko=-2;
      }
    }
    else
      enemygroups->push_back(this->getGroup(adjpos));
  }
  else if (this->getColor(adjpos)==col)
  {
    bool found=false;
    for(std::list<Go::Group*>::iterator iter=friendlygroups->begin();iter!=friendlygroups->end();++iter)
    {
      if ((*iter)==this->getGroup(adjpos))
      {
        found=true;
        break;
      }
    }
    if (!found)
      friendlygroups->push_back(this->getGroup(adjpos));
  }
  
  adjpos=pos+P_W;
  if (this->getColor(adjpos)==othercol)
  {
    if (this->getLiberties(adjpos)==1)
    {
      if (removeGroup(this->getGroup(adjpos))==1)
      {
        if (posko==-1)
          posko=adjpos;
        else
          posko=-2;
      }
    }
    else
      enemygroups->push_back(this->getGroup(adjpos));
  }
  else if (this->getColor(adjpos)==col)
  {
    bool found=false;
    for(std::list<Go::Group*>::iterator iter=friendlygroups->begin();iter!=friendlygroups->end();++iter)
    {
      if ((*iter)==this->getGroup(adjpos))
      {
        found=true;
        break;
      }
    }
    if (!found)
      friendlygroups->push_back(this->getGroup(adjpos));
  }
  
  this->setColor(move.getPosition(),move.getColor());
  
  if (friendlygroups->size()>0)
  {
    Go::Group *firstgroup=friendlygroups->front();
    firstgroup->addStone(pos);
    this->setGroup(pos,firstgroup);
    this->addDirectLiberties(pos,firstgroup);
    
    if (friendlygroups->size()>1)
    {
      for(std::list<Go::Group*>::iterator iter=friendlygroups->begin();iter!=friendlygroups->end();++iter)
      {
        if ((*iter)==friendlygroups->front())
          continue;
        
        this->mergeGroups(firstgroup,(*iter));
      }
    }
    
    firstgroup->removeLiberty(pos);
  }
  else
  {
    Go::Group *newgroup = new Go::Group(col,size);
    newgroup->addStone(pos);
    this->setGroup(pos,newgroup);
    this->addDirectLiberties(pos,newgroup);
    groups.push_back(newgroup);
  }
  
  for(std::list<Go::Group*>::iterator iter=enemygroups->begin();iter!=enemygroups->end();++iter)
  {
    (*iter)->removeLiberty(pos);
    if ((*iter)->numOfLiberties()==1)
    {
      int liberty=(*iter)->getLibertiesList()->front();
      this->addValidMove(Go::Move(col,liberty));
      if (this->touchingEmpty(liberty)==0 && !this->validMoveCheck(Go::Move(othercol,liberty)))
        this->removeValidMove(Go::Move(othercol,liberty));
    }
  }
  
  this->removeValidMove(Go::Move(Go::BLACK,pos));
  this->removeValidMove(Go::Move(Go::WHITE,pos));
  if (this->getGroup(pos)->numOfLiberties()==1)
  {
    int liberty=this->getGroup(pos)->getLibertiesList()->front();
    if (this->touchingEmpty(liberty)==0 && !this->validMoveCheck(Go::Move(col,liberty)))
      this->removeValidMove(Go::Move(col,liberty));
    if (this->validMoveCheck(Go::Move(othercol,liberty)))
      this->addValidMove(Go::Move(othercol,liberty));
  }
  
  if (this->getColor(pos+P_N)==Go::EMPTY)
  {
    if (!this->validMoveCheck(Go::Move(othercol,pos+P_N)))
      this->removeValidMove(Go::Move(othercol,pos+P_N));
  }
  if (this->getColor(pos+P_S)==Go::EMPTY)
  {
    if (!this->validMoveCheck(Go::Move(othercol,pos+P_S)))
      this->removeValidMove(Go::Move(othercol,pos+P_S));
  }
  if (this->getColor(pos+P_E)==Go::EMPTY)
  {
    if (!this->validMoveCheck(Go::Move(othercol,pos+P_E)))
      this->removeValidMove(Go::Move(othercol,pos+P_E));
  }
  if (this->getColor(pos+P_W)==Go::EMPTY)
  {
    if (!this->validMoveCheck(Go::Move(othercol,pos+P_W)))
      this->removeValidMove(Go::Move(othercol,pos+P_W));
  }
  
  friendlygroups->resize(0);
  delete friendlygroups;
  enemygroups->resize(0);
  delete enemygroups;
  
  nexttomove=Go::otherColor(nexttomove);
  movesmade++;
  
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
  
  if (this->getColor(pos+P_N)==Go::EMPTY)
    lib++;
  if (this->getColor(pos+P_S)==Go::EMPTY)
    lib++;
  if (this->getColor(pos+P_E)==Go::EMPTY)
    lib++;
  if (this->getColor(pos+P_W)==Go::EMPTY)
    lib++;
  
  return lib;
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
    
    this->spreadGroup(pos+P_N,group);
    this->spreadGroup(pos+P_S,group);
    this->spreadGroup(pos+P_E,group);
    this->spreadGroup(pos+P_W,group);
  }
}

void Go::Board::addDirectLiberties(int pos, Go::Group *group)
{
  if (this->getColor(pos+P_N)==Go::EMPTY)
    group->addLiberty(pos+P_N);
  if (this->getColor(pos+P_S)==Go::EMPTY)
    group->addLiberty(pos+P_S);
  if (this->getColor(pos+P_E)==Go::EMPTY)
    group->addLiberty(pos+P_E);
  if (this->getColor(pos+P_W)==Go::EMPTY)
    group->addLiberty(pos+P_W);
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
    
    if (this->getColor(pos+P_N)==othercol)
    {
      if (this->getGroup(pos+P_N)->numOfLiberties()==1)
      {
        int liberty=this->getGroup(pos+P_N)->getLibertiesList()->front();
        this->addValidMove(Go::Move(othercol,liberty));
        possiblesuicides->push_back(liberty);
      }
      this->getGroup(pos+P_N)->addLiberty(pos);
    }
    if (this->getColor(pos+P_S)==othercol)
    {
      if (this->getGroup(pos+P_S)->numOfLiberties()==1)
      {
        int liberty=this->getGroup(pos+P_S)->getLibertiesList()->front();
        this->addValidMove(Go::Move(othercol,liberty));
        possiblesuicides->push_back(liberty);
      }
      this->getGroup(pos+P_S)->addLiberty(pos);
    }
    if (this->getColor(pos+P_E)==othercol)
    {
      if (this->getGroup(pos+P_E)->numOfLiberties()==1)
      {
        int liberty=this->getGroup(pos+P_E)->getLibertiesList()->front();
        this->addValidMove(Go::Move(othercol,liberty));
        possiblesuicides->push_back(liberty);
      }
      this->getGroup(pos+P_E)->addLiberty(pos);
    }
    if (this->getColor(pos+P_W)==othercol)
    {
      if (this->getGroup(pos+P_W)->numOfLiberties()==1)
      {
        int liberty=this->getGroup(pos+P_W)->getLibertiesList()->front();
        this->addValidMove(Go::Move(othercol,liberty));
        possiblesuicides->push_back(liberty);
      }
      this->getGroup(pos+P_W)->addLiberty(pos);
    }
    
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
    if (this->getColor(pos+P_N)!=Go::OFFBOARD && (this->getColor(pos+P_N)!=col || this->getLiberties(pos+P_N)<2))
      return false;
    if (this->getColor(pos+P_S)!=Go::OFFBOARD && (this->getColor(pos+P_S)!=col || this->getLiberties(pos+P_S)<2))
      return false;
    if (this->getColor(pos+P_E)!=Go::OFFBOARD && (this->getColor(pos+P_E)!=col || this->getLiberties(pos+P_E)<2))
      return false;
    if (this->getColor(pos+P_W)!=Go::OFFBOARD && (this->getColor(pos+P_W)!=col || this->getLiberties(pos+P_W)<2))
      return false;
    
    return true;
  }
}


