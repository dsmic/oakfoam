#include "Go.h"

#include <cstdio>
#include <sstream>
#include "Features.h"
#include "Parameters.h"
#include "Engine.h"
#include "Random.h"

Go::BitBoard::BitBoard(int s)
  : size(s),
    sizesq(s*s),
    sizedata(1+(s+1)*(s+2)),
    data(new bool[sizedata])
{
  for (int i=0;i<sizedata;i++)
    data[i]=false;
}

Go::BitBoard *Go::BitBoard::copy() const
{
  Go::BitBoard *copyboard;
  copyboard=new Go::BitBoard(size);
  
  for (int i=0;i<sizedata;i++)
    copyboard->set(i,data[i]);  
  return copyboard;
}

Go::BitBoard::~BitBoard()
{
  delete[] data;
}

std::string Go::Position::pos2string(int pos, int boardsize)
{
  if (pos==-1)
    return "PASS";
  else if (pos==-2)
    return "RESIGN";
  else
  {
    std::ostringstream ss;
    int x=Go::Position::pos2x(pos,boardsize);
    int y=Go::Position::pos2y(pos,boardsize);
    char xletter='A'+x;
    if (x>=8)
      xletter++;
    ss<<xletter<<(y+1);
    return ss.str();
  }
}

int Go::Position::string2pos(std::string str, int boardsize)
{
  std::transform(str.begin(),str.end(),str.begin(),::tolower);
  if (str=="pass")
    return -1;
  else if (str=="resign")
    return -2;
  else
  {
    int x,y;
    x=str.at(0)-'a';
    if (str.at(0)>='i')
      x--;
    y=0;
    for (unsigned int i=1;i<str.length();i++)
    {
      y=(y*10)+(str.at(i)-'0');
    }
    return Go::Position::xy2pos(x,y-1,boardsize);
  }
}

std::string Go::Move::toString(int boardsize) const
{
  if (this->isPass())
  {
    if (color==Go::BLACK)
      return "B:PASS";
    else
      return "W:PASS";
  }
  else if (this->isResign())
    return "RESIGN";
  else
  {
    std::ostringstream ss;
    if (color==Go::BLACK)
      ss<<"B";
    else if (color==Go::WHITE)
      ss<<"W";
    char xletter='A'+this->getX(boardsize);
    if (this->getX(boardsize)>=8)
      xletter++;
    //ss<<"("<<xletter<<","<<(this->getY(boardsize)+1)<<")";
    ss<<":"<<xletter<<(this->getY(boardsize)+1);
    return ss.str();
  }
}

Go::Group::Group(Go::Board *brd, int pos)
  : board(brd),
    color(board->getColor(pos)),
    position(pos)
{
  stonescount=1;
  pseudoborderdist=brd->getPseudoDistanceToBorder(pos);
  parent=NULL;
  pseudoliberties=0;
  pseudoends=0;
  libpossum=0;
  libpossumsq=0;
}

void Go::Group::addTouchingEmpties()
{
  int size=board->getSize();
  foreach_adjacent(position,p,{
    if (board->getColor(p)==Go::EMPTY)
      this->addPseudoLiberty(p);
    if (board->getColor(p)!=board->getColor(position))
      this->addPseudoEnd();
  });
}

bool Go::Group::isOneOfTwoLiberties(int pos) const
{
  if (pseudoliberties>8 || this->inAtari())
    return false;
  
  int ps=pseudoliberties;
  int lps=libpossum;
  int lpsq=libpossumsq;
  int size=board->getSize();
  
  foreach_adjacent(pos,p,{
    if (board->inGroup(p) && board->getGroup(p)==this)
    {
      ps--;
      lps-=pos;
      lpsq-=pos*pos;
    }
  });
  
  return (ps>0 && (ps*lpsq)==(lps*lps));
}

int Go::Group::getOtherOneOfTwoLiberties(int pos) const
{
  if (pseudoliberties>8 || this->inAtari())
    return -1;
  
  int ps=pseudoliberties;
  int lps=libpossum;
  int lpsq=libpossumsq;
  int size=board->getSize();
  
  foreach_adjacent(pos,p,{
    if (board->inGroup(p) && board->getGroup(p)==this)
    {
      ps--;
      lps-=pos;
      lpsq-=pos*pos;
    }
  });
  
  if (ps>0 && (ps*lpsq)==(lps*lps))
    return lps/ps;
  else
    return -1;
}

Go::Board::Board(int s)
  : size(s),
    sizesq(s*s),
    sizedata(1+(s+1)*(s+2)),
    data(new Go::Vertex[sizedata])
{
  markchanges=false;
  lastchanges=new Go::BitBoard(size);
  lastcapture=false;
  
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
  
  lastmove=Go::Move(Go::BLACK,Go::Move::PASS);
  secondlastmove=Go::Move(Go::WHITE,Go::Move::PASS);
  
  blackvalidmoves=new Go::BitBoard(size);
  whitevalidmoves=new Go::BitBoard(size);
  blackvalidmovecount=0;
  whitevalidmovecount=0;
  for (int p=0;p<sizedata;p++)
  {
    if (this->getColor(p)!=Go::OFFBOARD)
    {
      blackvalidmoves->set(p);
      blackvalidmovecount++;
      whitevalidmoves->set(p);
      whitevalidmovecount++;
    }
    else
    {
      blackvalidmoves->clear(p);
      whitevalidmoves->clear(p);
    }
  }
  
  symmetryupdated=true;
  currentsymmetry=Go::Board::FULL;
  
  blackcaptures=0;
  whitecaptures=0;
  lastscoredata=NULL;
}

Go::Board::~Board()
{
  delete blackvalidmoves;
  delete whitevalidmoves;
  
  delete lastchanges;
  
  //XXX: memory will get freed when pool is destroyed
  /*for(std::list<Go::Group*,Go::allocator_groupptr>::iterator iter=groups.begin();iter!=groups.end();++iter) 
  {
    pool_group.destroy((*iter));
  }*/
  groups.clear();
  
  if (lastscoredata!=NULL)
    delete[] lastscoredata;
  
  delete[] data;
}

Go::Board *Go::Board::copy() const
{
  Go::Board *copyboard;
  copyboard=new Go::Board(size);
  
  this->copyOver(copyboard);
  
  return copyboard;
}

void Go::Board::copyOver(Go::Board *copyboard) const
{
  if (size!=copyboard->getSize())
    throw Go::Exception("cannot copy to a different size board");
  
  for (int p=0;p<sizedata;p++)
  {
    copyboard->data[p]=this->data[p];
  }
  
  copyboard->movesmade=this->movesmade;
  copyboard->nexttomove=this->nexttomove;
  copyboard->passesplayed=this->passesplayed;
  copyboard->simpleko=this->simpleko;
  
  copyboard->lastmove=this->lastmove;
  copyboard->secondlastmove=this->secondlastmove;
  
  copyboard->refreshGroups();
  
  copyboard->symmetryupdated=this->symmetryupdated;
  copyboard->currentsymmetry=this->currentsymmetry;
  
  copyboard->blackcaptures=this->blackcaptures;
  copyboard->whitecaptures=this->whitecaptures;
}

std::string Go::Board::toString() const
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

std::string Go::Board::toSGFString() const
{
  std::ostringstream ss,ssb,ssw;
  
  for (int p=0;p<sizedata;p++)
  {
    if (this->getColor(p)==Go::BLACK)
    {
      ssb<<"[";
      ssb<<(char)(Go::Position::pos2x(p,size)+'a');
      ssb<<(char)(size-Go::Position::pos2y(p,size)+'a'-1);
      ssb<<"]";
    }
  }
  if (ssb.str().length()>0)
    ss<<"AB"<<ssb.str();
  for (int p=0;p<sizedata;p++)
  {
    if (this->getColor(p)==Go::WHITE)
    {
      ssw<<"[";
      ssw<<(char)(Go::Position::pos2x(p,size)+'a');
      ssw<<(char)(size-Go::Position::pos2y(p,size)+'a'-1);
      ssw<<"]";
    }
  }
  if (ssw.str().length()>0)
    ss<<"AW"<<ssw.str();
  ss<<"PL["<<Go::colorToChar(this->nextToMove())<<"]";
  return ss.str();
}

float Go::Board::score(Parameters* params)
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
    if (!scoredata[p].touched && this->getColor(p)!=Go::EMPTY)
    {
      this->spreadScore(scoredata,p,this->getColor(p));
    }
  }
  
  float s=0;
  
  for (int p=0;p<sizedata;p++)
  {
    Go::Color col=scoredata[p].color;
    float v=1.0;
//    if (params!=NULL && params->test_p3!=0.0)
//      v=(1.0-params->test_p3)+params->test_p3*pow(params->engine->getTerritoryMap()->getPositionOwner(p),2);
    if (col==Go::BLACK)
      s+=v;
    else if (col==Go::WHITE)
      s-=v;
  }
  
  if (lastscoredata!=NULL)
    delete[] lastscoredata;
  lastscoredata=scoredata;
  
  //delete[] scoredata;
  return s;
}

void Go::Board::spreadScore(Go::Board::ScoreVertex *scoredata, int pos, Go::Color col)
{
  bool wastouched;
  
  if (this->getColor(pos)==Go::OFFBOARD || col==Go::OFFBOARD)
    return;
  
  if (this->getColor(pos)!=Go::EMPTY && col==Go::EMPTY)
    return;
  
  wastouched=scoredata[pos].touched;
  
  if (this->getColor(pos)!=Go::EMPTY)
  {
    scoredata[pos].touched=true;
    scoredata[pos].color=this->getColor(pos);
    if (!wastouched)
    {
      foreach_adjacent(pos,p,{
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
  
  foreach_adjacent(pos,p,{
    this->spreadScore(scoredata,p,col);
  });
}

Go::Color Go::Board::getScoredOwner(int pos) const
{
  if (lastscoredata!=NULL)
    return lastscoredata[pos].color;
  else
    return Go::EMPTY;
}

bool Go::Board::validMove(Go::Move move) const
{
  Go::BitBoard *validmoves=(move.getColor()==Go::BLACK?blackvalidmoves:whitevalidmoves);
  return move.isPass() || move.isResign() || validmoves->get(move.getPosition());
}

bool Go::Board::validMoveCheck(Go::Move move) const
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
    bool isvalid=false;
    int captures=0;
    
    foreach_adjacent(pos,p,{
      if (this->getColor(p)==col || this->getColor(p)==othercol)
        this->getGroup(p)->removePseudoLiberty(pos);
      if (this->getColor(p)==col)
        this->getGroup(p)->removePseudoEnd();
    });
    
    foreach_adjacent(pos,p,{
      if (this->getColor(p)==col && this->getPseudoLiberties(p)>0)
        isvalid=true;
      else if (this->getColor(p)==othercol && this->getPseudoLiberties(p)==0)
      {
        captures+=this->getGroupSize(p);
        if (captures>1)
          isvalid=true;
      }
    });
    
    foreach_adjacent(pos,p,{
      if (this->getColor(p)==col || this->getColor(p)==othercol)
        this->getGroup(p)->addPseudoLiberty(pos);
      if (this->getColor(p)==col)
        this->getGroup(p)->addPseudoEnd();
    });
    
    if (isvalid)
      return true;
    
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
  if (nexttomove!=move.getColor())
  {
    fprintf(stderr,"WARNING! unexpected move color\n");
  }
  
  lastcapture = false;

  if (simpleko!=-1)
  {
    int kopos=simpleko;
    simpleko=-1;
    if (this->validMoveCheck(Go::Move(move.getColor(),kopos)))
      this->addValidMove(Go::Move(move.getColor(),kopos));
    if (markchanges)
      lastchanges->set(kopos);
  }
  
  if (move.isPass() || move.isResign())
  {
    if (move.isPass())
      passesplayed++;
    nexttomove=Go::otherColor(move.getColor());
    movesmade++;
    if (markchanges && !secondlastmove.isPass() && !secondlastmove.isResign())
      lastchanges->set(secondlastmove.getPosition());
    if (markchanges && !lastmove.isPass() && !lastmove.isResign())
      lastchanges->set(lastmove.getPosition());
    secondlastmove=lastmove;
    lastmove=move;
    if (symmetryupdated)
      this->updateSymmetry();
    return;
  }
  
  if (!this->validMove(move))
  {
    fprintf(stderr,"invalid move at %d,%d\n",move.getX(size),move.getY(size));
    throw Go::Exception("invalid move");
  }
  
  Go::Color col=move.getColor();
  Go::Color othercol=Go::otherColor(col);
  
  movesmade++;
  nexttomove=othercol;
  passesplayed=0;
  if (markchanges && !secondlastmove.isPass() && !secondlastmove.isResign())
    lastchanges->set(secondlastmove.getPosition());
  if (markchanges && !lastmove.isPass() && !lastmove.isResign())
    lastchanges->set(lastmove.getPosition());
  secondlastmove=lastmove;
  lastmove=move;
  
  int pos=move.getPosition();
  int posko=-1;
  
  this->setColor(pos,col);
  
  Go::Group *thisgroup=pool_group.construct(this,pos);
  this->setGroup(pos,thisgroup);
  groups.insert(thisgroup);
  
  thisgroup->addTouchingEmpties();
  
  foreach_adjacent(pos,p,{
    if (this->getColor(p)==col)
    {
      Go::Group *othergroup=this->getGroup(p);
      othergroup->removePseudoLiberty(pos);
      othergroup->removePseudoEnd();
      
      if (othergroup->numOfStones()>thisgroup->numOfStones())
        this->mergeGroups(othergroup,thisgroup);
      else
        this->mergeGroups(thisgroup,othergroup);
      thisgroup=thisgroup->find();
    }
  });
  
  foreach_adjacent(pos,p,{
    if (this->getColor(p)==othercol)
    {
      Go::Group *othergroup=this->getGroup(p);
      othergroup->removePseudoLiberty(pos);
      // not here, other group looses only lib but not end: othergroup->removePseudoEnd();
      
      thisgroup->getAdjacentGroups()->push_back(p);
      othergroup->getAdjacentGroups()->push_back(pos);
      
      if (othergroup->numOfPseudoLiberties()==0)
      {
        lastcapture=true;
        if (removeGroup(othergroup)==1)
        {
          if (posko==-1)
            posko=p;
          else
            posko=-2;
        }
      }
      else if (othergroup->numOfPseudoLiberties()<=4)
      {
        if (othergroup->inAtari())
        {
          int liberty=othergroup->getAtariPosition();
          this->addValidMove(Go::Move(col,liberty));
          if (this->touchingEmpty(liberty)==0 && !this->validMoveCheck(Go::Move(othercol,liberty)))
            this->removeValidMove(Go::Move(othercol,liberty));
        }
      }
    }
  });
  
  this->removeValidMove(Go::Move(Go::BLACK,pos));
  this->removeValidMove(Go::Move(Go::WHITE,pos));
  
  if (thisgroup->inAtari())
  {
    int liberty=thisgroup->getAtariPosition();
    if (this->touchingEmpty(liberty)==0 && !this->validMoveCheck(Go::Move(col,liberty)))
      this->removeValidMove(Go::Move(col,liberty));
    if (this->validMoveCheck(Go::Move(othercol,liberty)))
      this->addValidMove(Go::Move(othercol,liberty));
  }
  
  foreach_adjacent(pos,p,{
    if (this->getColor(p)==Go::EMPTY)
    {
      if (!this->validMoveCheck(Go::Move(othercol,p)))
        this->removeValidMove(Go::Move(othercol,p));
    }
  });
  
  if (posko>=0 && thisgroup->inAtari())
  {
    simpleko=posko;
    if (!this->validMoveCheck(Go::Move(othercol,simpleko)))
      this->removeValidMove(Go::Move(othercol,simpleko));
  }
  
  if (symmetryupdated)
    this->updateSymmetry();
}

void Go::Board::refreshValidMoves()
{
  this->refreshValidMoves(Go::BLACK);
  this->refreshValidMoves(Go::WHITE);
}

void Go::Board::refreshValidMoves(Go::Color col)
{
  (col==Go::BLACK?blackvalidmovecount:whitevalidmovecount)=0;
  Go::BitBoard *validmoves=(col==Go::BLACK?blackvalidmoves:whitevalidmoves);
  
  for (int p=0;p<sizedata;p++)
  {
    validmoves->clear(p);
    if (this->validMoveCheck(Go::Move(col,p)))
      this->addValidMove(Go::Move(col,p));
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

int Go::Board::touchingEmpty(int pos) const
{
  int lib=0;
  
  foreach_adjacent(pos,p,{
    if (this->getColor(p)==Go::EMPTY)
      lib++;
  });
  
  return lib;
}

int Go::Board::diagonalEmpty(int pos) const
{
  int lib=0;
  
  foreach_diagonal(pos,p,{
    if (this->getColor(p)==Go::EMPTY)
      lib++;
  });
  
  return lib;
}

int Go::Board::surroundingEmpty(int pos) const
{
  int lib=0;
  
  foreach_adjdiag(pos,p,{
    if (this->getColor(p)==Go::EMPTY)
      lib++;
  });
  
  return lib;
}

bool Go::Board::touchingAtLeastOneEmpty(int pos) const
{
  foreach_adjacent(pos,p,{
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
  
  //XXX: memory will get freed when pool is destroyed
  //for(std::list<Go::Group*,Go::allocator_groupptr>::iterator iter=groups.begin();iter!=groups.end();++iter) 
  //{
  //  pool_group.destroy((*iter));
  //}
  groups.clear();
  
  for (int p=0;p<sizedata;p++)
  {
    if (this->getColor(p)!=Go::EMPTY && this->getColor(p)!=Go::OFFBOARD && this->getGroupWithoutFind(p)==NULL)
    {
      Go::Group *newgroup=pool_group.construct(this,p);
      this->setGroup(p,newgroup);
      newgroup->addTouchingEmpties();
    
      foreach_adjacent(p,q,{
        this->spreadGroup(q,newgroup);
      });
      groups.insert(newgroup);
    }
  }
  
  for (int p=0;p<sizedata;p++)
  {
    if (this->inGroup(p))
    {
      Go::Group *group=this->getGroup(p);
      Go::Color othercol=Go::otherColor(group->getColor());
      foreach_adjacent(p,q,{
        if (this->getColor(q)==othercol)
        {
          //Go::Group *othergroup=this->getGroup(q);
          group->getAdjacentGroups()->push_back(q);
          //othergroup->getAdjacentGroups()->push_back(p);
        }
      });
    }
  }
  
  this->refreshValidMoves();
}

void Go::Board::spreadGroup(int pos, Go::Group *group)
{
  if (this->getColor(pos)==group->getColor() && this->getGroupWithoutFind(pos)==NULL)
  {
    Go::Group *thisgroup=pool_group.construct(this,pos);
    this->setGroup(pos,thisgroup);
    groups.insert(thisgroup);
    thisgroup->addTouchingEmpties();
    this->mergeGroups(group,thisgroup);
    
    foreach_adjacent(pos,p,{
      this->spreadGroup(p,group);
    });
  }
}

int Go::Board::removeGroup(Go::Group *group)
{
  group=group->find();
  int pos=group->getPosition();
  int s=group->numOfStones();
  Go::Color groupcol=group->getColor();
  
  groups.erase(group);
  
  std::list<int,Go::allocator_int> *possiblesuicides = new std::list<int,Go::allocator_int>();
  
  this->spreadRemoveStones(groupcol,pos,possiblesuicides);
  
  for(std::list<int,Go::allocator_int>::iterator iter=possiblesuicides->begin();iter!=possiblesuicides->end();++iter)
  {
    if (!this->validMoveCheck(Go::Move(groupcol,(*iter)))) 
      this->removeValidMove(Go::Move(groupcol,(*iter)));
  }
  
  possiblesuicides->resize(0);
  delete possiblesuicides;
  
  return s;
}

void Go::Board::spreadRemoveStones(Go::Color col, int pos, std::list<int,Go::allocator_int> *possiblesuicides)
{
  Go::Color othercol=Go::otherColor(col);
  //Go::Group *group=this->getGroupWithoutFind(pos); //see destroy() below
  
  this->setColor(pos,Go::EMPTY);
  this->setGroup(pos,NULL);
  if (col==Go::BLACK)
    blackcaptures++;
  else
    whitecaptures++;
  
  foreach_adjacent(pos,p,{
    if (this->getColor(p)==col)
      this->spreadRemoveStones(col,p,possiblesuicides);
    else if (this->getColor(p)==othercol)
    {
      Go::Group *othergroup=this->getGroup(p);
      if (othergroup->inAtari())
      {
        int liberty=othergroup->getAtariPosition();
        this->addValidMove(Go::Move(othercol,liberty));
        possiblesuicides->push_back(liberty);
      }
      othergroup->addPseudoLiberty(pos);
      // not here: othergroup->addPseudoEnd();
    }
  });
  
  this->addValidMove(Go::Move(othercol,pos));
  this->addValidMove(Go::Move(col,pos));
  
  //XXX: memory will get freed when pool is destroyed
  //pool_group.destroy(group);
}

void Go::Board::mergeGroups(Go::Group *first, Go::Group *second)
{
  if (first==second)
    return;
  
  groups.erase(second);
  first->unionWith(second);
}

bool Go::Board::weakEye(Go::Color col, int pos) const
{
  if (col==Go::EMPTY || col==Go::OFFBOARD || this->getColor(pos)!=Go::EMPTY)
    return false;
  else
  {
    bool onside=false;
    Go::Color othercol=Go::otherColor(col);
    int othercols=0;
    
    foreach_adjacent(pos,p,{
      if (this->getColor(p)==Go::OFFBOARD)
        onside=true;
      else if (this->getColor(p)!=col)
        return false;
    });
    
    foreach_diagonal(pos,p,{
      if (this->getColor(p)==othercol)
      {
        othercols++;
        if (onside || othercols>=2)
          return false;
      }
    });
    
    /*foreach_adjacent(pos,p,{
      if (this->getColor(p)!=Go::OFFBOARD && (this->getColor(p)!=col || this->getGroup(p)->inAtari()))
        return false;
    });*/
    
    return true;
  }
}

bool Go::Board::twoGroupEye(Go::Color col, int pos) const
{
  Go::Group *groups_used[4];
  int groups_used_num=0;
  foreach_adjacent(pos,p,{
    if (this->inGroup(p))
    {
      Go::Group *group=this->getGroup(p);
      if (col==group->getColor())
      {
        bool found=false;
        for (int i=0;i<groups_used_num;i++)
        {
          if (groups_used[i]==group)
            found=true;
        }
        if (!found)
        {
          groups_used[groups_used_num]=group;
          groups_used_num++;
        }
      }
    }
  });
  //now all groups attached to pos are in groups_used[]

  if (groups_used_num<2) return false; //can not be a two group eye

  for (int i=0;i<groups_used_num;i++)
  {
    Go::Group *group=groups_used[i];
    if (group->isOneOfTwoLiberties (pos))
    {
      //possible group attaching
      int otherlib=group->getOtherOneOfTwoLiberties(pos);
      //if otherlib has more than two libs together with first lib
      //it is shared!
      int found=0;
      Go::Group *groups_used_ol[4];
      int groups_used_num_ol=0;
      foreach_adjacent(otherlib,p2,{
        if (this->inGroup(p2))
        {
          Go::Group *group=this->getGroup(p2);
          bool found_ol=false;
          for (int i=0;i<groups_used_num_ol;i++)
          {
            if (groups_used_ol[i]==group)
              found_ol=true;
          }
          if (!found_ol)
          {
            groups_used_ol[groups_used_num_ol]=group;
            groups_used_num_ol++;
            if (col==group->getColor())
            {
              for (int j=0;j<groups_used_num;j++)
              {
                if (groups_used[j]==group)
                {
                  found++;
                  if (found>1)
                    return true;
                }
              }
            }
          }
        }
      });
    }
  }
  
  return false;
}
bool Go::Board::strongEye(Go::Color col, int pos) const
{
  if (col==Go::EMPTY || col==Go::OFFBOARD || this->getColor(pos)!=Go::EMPTY)
    return false;
  else
  {
    Go::Group *group=NULL;
    foreach_adjacent(pos,p,{
      if (this->getColor(p)!=col || this->getColor(p)==Go::EMPTY)
        return false;
      else if (this->inGroup(p))
      {
        if (group)
        {
          if (group!=this->getGroup(p))
            return false;
        }
        else
          group=this->getGroup(p);
      }
    });
    
    return true;
  }
}

bool Go::Board::isWinForColor(Go::Color col, float score)
{
  float k=0;
  
  if (col==Go::BLACK)
    k=1;
  else if (col==Go::WHITE)
    k=-1;
  else
    return false;
  
  return ((score*k)>0);
}

bool Go::Board::hasSymmetryVertical() const
{
  if (simpleko!=-1)
    return false;
  
  int xlimit=size/2+size%2-1;
  for (int x=0;x<xlimit;x++)
  {
    for (int y=0;y<size;y++)
    {
      int pos1=Go::Position::xy2pos(x,y,size);
      int pos2=this->doSymmetryTransformPrimitive(Go::Board::VERTICAL,pos1);
      if (this->getColor(pos1)!=this->getColor(pos2))
      {
        //fprintf(stderr,"mismatch at %d,%d (%d)\n",x,y,xlimit);
        return false;
      }
    }
  }
  
  return true;
}

bool Go::Board::hasSymmetryHorizontal() const
{
  if (simpleko!=-1)
    return false;
  
  int ylimit=size/2+size%2-1;
  for (int x=0;x<size;x++)
  {
    for (int y=0;y<ylimit;y++)
    {
      int pos1=Go::Position::xy2pos(x,y,size);
      int pos2=this->doSymmetryTransformPrimitive(Go::Board::HORIZONTAL,pos1);
      if (this->getColor(pos1)!=this->getColor(pos2))
      {
        //fprintf(stderr,"mismatch at %d,%d (%d)\n",x,y,ylimit);
        return false;
      }
    }
  }
  
  return true;
}

bool Go::Board::hasSymmetryDiagonalDown() const
{
  if (simpleko!=-1)
    return false;
  
  for (int x=0;x<size;x++)
  {
    for (int y=0;y<=(size-x-1);y++)
    {
      //fprintf(stderr,"pos: %d,%d %d,%d\n",x,y,size-y-1,size-x-1);
      int pos1=Go::Position::xy2pos(x,y,size);
      int pos2=this->doSymmetryTransformPrimitive(Go::Board::DIAGONAL_DOWN,pos1);
      if (this->getColor(pos1)!=this->getColor(pos2))
      {
        //fprintf(stderr,"mismatch at %d,%d %d,%d\n",x,y,size-y-1,size-x-1);
        return false;
      }
    }
  }
  
  return true;
}

bool Go::Board::hasSymmetryDiagonalUp() const
{
  if (simpleko!=-1)
    return false;
  
  for (int x=0;x<size;x++)
  {
    for (int y=0;y<=x;y++)
    {
      //fprintf(stderr,"pos: %d,%d %d,%d\n",x,y,y,x);
      int pos1=Go::Position::xy2pos(x,y,size);
      int pos2=this->doSymmetryTransformPrimitive(Go::Board::DIAGONAL_UP,pos1);
      if (this->getColor(pos1)!=this->getColor(pos2))
      {
        //fprintf(stderr,"mismatch at %d,%d %d,%d\n",x,y,y,x);
        return false;
      }
    }
  }
  
  return true;
}

Go::Board::Symmetry Go::Board::computeSymmetry()
{
  bool symvert=this->hasSymmetryVertical();
  bool symdiagup=this->hasSymmetryDiagonalUp();
  if (symvert && symdiagup)
    return Go::Board::FULL;
  else if (symvert && this->hasSymmetryHorizontal())
    return Go::Board::VERTICAL_HORIZONTAL;
  else if (symvert)
    return Go::Board::VERTICAL;
  else if (symdiagup && this->hasSymmetryDiagonalDown())
    return Go::Board::DIAGONAL_BOTH;
  else if (symdiagup)
    return Go::Board::DIAGONAL_UP;
  else if (this->hasSymmetryHorizontal())
    return Go::Board::HORIZONTAL;
  else if (this->hasSymmetryDiagonalDown())
    return Go::Board::DIAGONAL_DOWN;
  else
    return Go::Board::NONE;
}

std::string Go::Board::getSymmetryString(Go::Board::Symmetry sym) const
{
  if (sym==Go::Board::FULL)
    return "FULL";
  else if (sym==Go::Board::VERTICAL_HORIZONTAL)
    return "VERTICAL_HORIZONTAL";
  else if (sym==Go::Board::VERTICAL)
    return "VERTICAL";
  else if (sym==Go::Board::HORIZONTAL)
    return "HORIZONTAL";
  else if (sym==Go::Board::DIAGONAL_BOTH)
    return "DIAGONAL_BOTH";
  else if (sym==Go::Board::DIAGONAL_UP)
    return "DIAGONAL_UP";
  else if (sym==Go::Board::DIAGONAL_DOWN)
    return "DIAGONAL_DOWN";
  else
    return "NONE";
}

void Go::Board::updateSymmetry()
{
  #if SYMMETRY_ONLYDEGRAGE
    if (currentsymmetry==Go::Board::NONE)
      return;
    else
      currentsymmetry=this->computeSymmetry(); //can be optimized
  #else
    currentsymmetry=this->computeSymmetry(); //can be optimized (not used)
  #endif
}

int Go::Board::doSymmetryTransformPrimitive(Go::Board::Symmetry sym, int pos) const
{
  int x=Go::Position::pos2x(pos,size);
  int y=Go::Position::pos2y(pos,size);
  if (sym==Go::Board::VERTICAL)
    return Go::Position::xy2pos(size-x-1,y,size);
  else if (sym==Go::Board::HORIZONTAL)
    return Go::Position::xy2pos(x,size-y-1,size);
  else if (sym==Go::Board::DIAGONAL_UP)
    return Go::Position::xy2pos(y,x,size);
  else if (sym==Go::Board::DIAGONAL_DOWN)
    return Go::Position::xy2pos(size-y-1,size-x-1,size);
  else
    return -1;
}

int Go::Board::doSymmetryTransformToPrimary(Go::Board::Symmetry sym, int pos)
{
  return this->doSymmetryTransform(this->getSymmetryTransformToPrimary(sym,pos),pos);
}

Go::Board::SymmetryTransform Go::Board::getSymmetryTransformToPrimary(Go::Board::Symmetry sym, int pos) const
{
  return Go::Board::getSymmetryTransformToPrimaryStatic(size,sym,pos);
}

Go::Board::SymmetryTransform Go::Board::getSymmetryTransformToPrimaryStatic(int size, Go::Board::Symmetry sym, int pos)
{
  int x=Go::Position::pos2x(pos,size);
  int y=Go::Position::pos2y(pos,size);
  Go::Board::SymmetryTransform trans={false,false,false};
  
  if (sym==Go::Board::VERTICAL)
  {
    if (x>(size-x-1))
      trans.invertX=true;
  }
  else if (sym==Go::Board::HORIZONTAL)
  {
    if (y>(size-y-1))
      trans.invertY=true;
  }
  else if (sym==Go::Board::VERTICAL_HORIZONTAL)
  {
    if (x>(size-x-1))
      trans.invertX=true;
    if (y>(size-y-1))
      trans.invertY=true;
  }
  else if (sym==Go::Board::DIAGONAL_UP)
  {
    if (x>y)
      trans.swapXY=true;
  }
  else if (sym==Go::Board::DIAGONAL_DOWN)
  {
    if ((x+y)>((size-y-1)+(size-x-1)))
    {
      trans.invertX=true;
      trans.invertY=true;
      trans.swapXY=true;
    }
  }
  else if (sym==Go::Board::DIAGONAL_BOTH)
  {
    int nx=x;
    int ny=y;
    if ((x+y)>((size-y-1)+(size-x-1)))
    {
      trans.invertX=true;
      trans.invertY=true;
      trans.swapXY=true;
      nx=(size-y-1);
      ny=(size-x-1);
    }
    
    if (nx>ny)
      trans.swapXY=!trans.swapXY;
  }
  else if (sym==Go::Board::FULL)
  {
    int nx=x;
    int ny=y;
    
    if (x>(size-x-1))
    {
      trans.invertX=true;
      nx=(size-x-1);
    }
    
    if (y>(size-y-1))
    {
      trans.invertY=true;
      ny=(size-y-1);
    }
    
    if (nx>ny)
      trans.swapXY=true;
  }
  
  return trans;
}

int Go::Board::doSymmetryTransform(Go::Board::SymmetryTransform trans, int pos, bool reverse)
{
  if (reverse)
    return Go::Board::doSymmetryTransformStaticReverse(trans,size,pos);
  else
    return Go::Board::doSymmetryTransformStatic(trans,size,pos);
}

int Go::Board::doSymmetryTransformStatic(Go::Board::SymmetryTransform trans, int size, int pos)
{
  int x=Go::Position::pos2x(pos,size);
  int y=Go::Position::pos2y(pos,size);
  
  if (trans.invertX)
    x=(size-x-1);
  
  if (trans.invertY)
    y=(size-y-1);
  
  if (trans.swapXY)
  {
    int t=x;
    x=y;
    y=t;
  }
  
  return Go::Position::xy2pos(x,y,size);
}

int Go::Board::doSymmetryTransformStaticReverse(Go::Board::SymmetryTransform trans, int size, int pos)
{
  int x=Go::Position::pos2x(pos,size);
  int y=Go::Position::pos2y(pos,size);
  
  if (trans.swapXY)
  {
    int t=x;
    x=y;
    y=t;
  }
  
  if (trans.invertX)
    x=(size-x-1);
  
  if (trans.invertY)
    y=(size-y-1);
  
  return Go::Position::xy2pos(x,y,size);
}

Go::Board::SymmetryTransform Go::Board::getSymmetryTransformBetweenPositions(int size, int pos1, int pos2)
{
  int x1=Go::Position::pos2x(pos1,size);
  int y1=Go::Position::pos2y(pos1,size);
  int x2=Go::Position::pos2x(pos2,size);
  int y2=Go::Position::pos2y(pos2,size);
  Go::Board::SymmetryTransform trans={false,false,false};
  
  //int ix1=(size-x1-1);
  int ix2=(size-x2-1);
  int iy1=(size-y1-1);
  int iy2=(size-y2-1);
  
  if (x1==x2 && y1==y2)
    return trans;
  else if (x1==x2 && y1==iy2)
    trans.invertY=true;
  else if (y1==y2 && x1==ix2)
    trans.invertX=true;
  else if (x1==y2 && x2==y1)
    trans.swapXY=true;
  else if (x1==iy2 && x2==iy1)
  {
    trans.invertX=true;
    trans.invertY=true;
    trans.swapXY=true;
  }
  else if (x1==ix2 && y1==iy2)
  {
    trans.invertX=true;
    trans.invertY=true;
  }
  else if (x1==y2 && x2==iy1)
  {
    trans.invertY=true;
    trans.swapXY=true;
  }
  else if (x1==iy2 && x2==y1)
  {
    trans.invertX=true;
    trans.swapXY=true;
  }
  
  return trans;
}

std::list<Go::Board::SymmetryTransform> Go::Board::getSymmetryTransformsFromPrimary(Go::Board::Symmetry sym) const
{
  return Go::Board::getSymmetryTransformsFromPrimaryStatic(sym);
}

std::list<Go::Board::SymmetryTransform> Go::Board::getSymmetryTransformsFromPrimaryStatic(Go::Board::Symmetry sym)
{
  std::list<Go::Board::SymmetryTransform> list=std::list<Go::Board::SymmetryTransform>();
  Go::Board::SymmetryTransform transbase={false,false,false};
  list.push_back(transbase);
  
  if (sym==Go::Board::FULL)
  {
    Go::Board::SymmetryTransform trans={false,false,true};
    list.push_back(trans);
    trans.swapXY=false;
    trans.invertY=true;
    list.push_back(trans);
    trans.swapXY=true;
    list.push_back(trans);
    trans.swapXY=false;
    trans.invertY=false;
    trans.invertX=true;
    list.push_back(trans);
    trans.swapXY=true;
    list.push_back(trans);
    trans.swapXY=false;
    trans.invertY=true;
    list.push_back(trans);
    trans.swapXY=true;
    list.push_back(trans);
  }
  else if (sym==Go::Board::VERTICAL_HORIZONTAL)
  {
    Go::Board::SymmetryTransform trans={false,true,false};
    list.push_back(trans);
    trans.invertX=true;
    list.push_back(trans);
    trans.invertY=false;
    list.push_back(trans);
  }
  else if (sym==Go::Board::VERTICAL)
  {
    Go::Board::SymmetryTransform trans={true,false,false};
    list.push_back(trans);
  }
  else if (sym==Go::Board::HORIZONTAL)
  {
    Go::Board::SymmetryTransform trans={false,true,false};
    list.push_back(trans);
  }
  else if (sym==Go::Board::DIAGONAL_BOTH)
  {
    Go::Board::SymmetryTransform trans={false,false,true};
    list.push_back(trans);
    trans.invertX=true;
    trans.invertY=true;
    list.push_back(trans);
    trans.swapXY=false;
    list.push_back(trans);
  }
  else if (sym==Go::Board::DIAGONAL_UP)
  {
    Go::Board::SymmetryTransform trans={false,false,true};
    list.push_back(trans);
  }
  else if (sym==Go::Board::DIAGONAL_DOWN)
  {
    Go::Board::SymmetryTransform trans={true,true,true};
    list.push_back(trans);
  }
  
  return list;
}

bool Go::Board::isCapture(Go::Move move) const
{
  Go::Color col=move.getColor();
  int pos=move.getPosition();
  foreach_adjacent(pos,p,{
    if (this->inGroup(p))
    {
      Go::Group *group=this->getGroup(p);
      if (group->inAtari() && col!=group->getColor())
        return true;
    }
  });
  return false;
}

bool Go::Board::isExtension(Go::Move move) const
{
  Go::Color col=move.getColor();
  int pos=move.getPosition();
  bool foundgroupinatari=false;
  int foundconnectingliberties=0;
  int libpos=-1;
  foreach_adjacent(pos,p,{
    if (this->inGroup(p))
    {
      Go::Group *group=this->getGroup(p);
      if (col==group->getColor())
      {
        if (group->inAtari())
          foundgroupinatari=true;
        else if (foundconnectingliberties<2)
        {
          int otherlib=group->getOtherOneOfTwoLiberties(pos);
          if (otherlib!=-1)
          {
            if (foundconnectingliberties==0)
            {
              foundconnectingliberties=1;
              libpos=otherlib;
            }
            else if (libpos!=otherlib)
              foundconnectingliberties=2;
          }
          else
            foundconnectingliberties=2;
        }
      }
    }
    else if (this->onBoard(p) && foundconnectingliberties<2)
    {
      if (foundconnectingliberties==0)
      {
        foundconnectingliberties=1;
        libpos=p;
      }
      else if (libpos!=p)
        foundconnectingliberties=2;
    }
  });
  if (foundgroupinatari && foundconnectingliberties>=2)
    return true;
  else
    return false;
}

bool Go::Board::isSelfAtari(Go::Move move) const
{
  return this->isSelfAtariOfSize(move,0);
}

bool Go::Board::isSelfAtariOfSize(Go::Move move, int minsize, bool complex) const
{
  Go::Color col=move.getColor();
  int pos=move.getPosition();

  if (this->touchingEmpty(pos)>1)
    return false;

  int libpos=-1;
  int groupsize=1;
  int groupbent4indicator=this->getPseudoDistanceToBorder(pos);
  Go::Group *groups_used[4];
  int groups_used_num=0;
  int usedneighbours=0;
  int attach_group_pos=-1;
  int capturedstones=0;
  foreach_adjacent(pos,p,{
    if (this->inGroup(p))
    {
      Go::Group *group=this->getGroup(p);
      if (col==group->getColor())
      {
        attach_group_pos=p;
        usedneighbours++;
        if (!(group->inAtari() || group->isOneOfTwoLiberties(pos)))
          return false; // attached group has more than two libs
        bool found=false;
        for (int i=0;i<groups_used_num;i++)
        {
          if (groups_used[i]==group)
          {
            found=true;
            break;
          }
        }
        if (!found)
        {
          groups_used[groups_used_num]=group;
          groups_used_num++;
          int otherlib=group->getOtherOneOfTwoLiberties(pos);
          //fprintf(stderr,"otherlib %d\n",otherlib);
          if (otherlib!=-1)
          {
            if (libpos==-1)
              libpos=otherlib;
            else if (libpos!=otherlib)
              return false; // at least 2 libs
          }
          groupsize+=group->numOfStones();
          groupbent4indicator+=group->numOfPseudoBorderDist();
          //fprintf(stderr,"groupsize now %d %d\n",groupsize,groupbent4indicator);
        }
      }
      else
      {
        //more than on stone is captured, so not self atari
        if (group->inAtari())
          capturedstones+=group->numOfStones();
        if (capturedstones>1)
        {
          //fprintf(stderr,"group catched of size more than 1\n");
          return false;
        }
      }
    }
    else if (this->getColor(p)==Go::EMPTY)
    {
      if (libpos==-1)
        libpos=p;
      else if (libpos!=p)
        return false; // at least 2 libs
    }
  });
  
  if (groupsize>minsize)
    return true;
  else
  {
    if (!complex)
      return false;
    //complex
    if (groupsize<4) return false; //no complex handling necessary
    int pseudoends=4-usedneighbours-usedneighbours;
    for (int i=0;i<groups_used_num;i++)
    {
        pseudoends+=groups_used[i]->numOfPseudoEnds();
        //fprintf(stderr,"-- %d\n",groups_used[i]->numOfPseudoEnds());
    }
    //now we know the pseudoends of the group and can check if it is a good or bad form
    //fprintf(stderr,"complex self atari checked stones %d pseudoends %d bent4indicator %d\n",groupsize,pseudoends,groupbent4indicator);
    if (groupsize==5 && pseudoends!=10)
      return true; //do only play XXX
                   //             XX   form
    /*
     * 
     * bent 4 handling in the corner not ok     
    */ 
    //fprintf(stderr,"debug boardsize must be 9 selfatari 4 or 5 at %s with attached %s\n",move.toString (9).c_str(),Go::Position::pos2string(attach_group_pos,9).c_str());
    if (groupsize==4)
    {
      //here allow play of bent 4 in the corner
      if (groupbent4indicator==4)
        return false; //this is bent 4 in the corner or   
                      // XX
                      // XX in the corner, which is ok too
      if (attach_group_pos>=0) 
      {
        int nattached=0;
        foreach_adjacent(attach_group_pos,p,{
          if (this->getColor(p)==col)
            nattached++;
        });
        if (nattached!=2 && pseudoends!=8)
          return true; //do only play XX   XXX
                       //             XX    X
      }
     }
                                          
    return false;
  }
}

bool Go::Board::isAtari(Go::Move move) const
{
  Go::Color col=move.getColor();
  int pos=move.getPosition();
  foreach_adjacent(pos,p,{
    if (this->inGroup(p))
    {
      Go::Group *group=this->getGroup(p);
      if (col!=group->getColor() && group->isOneOfTwoLiberties(pos))
        return true;
    }
  });
  return false;
}

int Go::Board::getDistanceToBorder(int pos) const
{
  int x=Go::Position::pos2x(pos,size);
  int y=Go::Position::pos2y(pos,size);
  int ix=(size-x-1);
  int iy=(size-y-1);
  
  int dist=x;
  if (y<dist)
    dist=y;
  if (ix<dist)
    dist=ix;
  if (iy<dist)
    dist=iy;
  
  return dist;
}

int Go::Board::getRectDistance(int pos1, int pos2) const
{
  int x1=Go::Position::pos2x(pos1,size);
  int y1=Go::Position::pos2y(pos1,size);
  int x2=Go::Position::pos2x(pos2,size);
  int y2=Go::Position::pos2y(pos2,size);
  
  return Go::rectDist(x1,y1,x2,y2);
}

int Go::Board::getPseudoDistanceToBorder(int pos) const
{
  int x=Go::Position::pos2x(pos,size);
  int y=Go::Position::pos2y(pos,size);
  int ix=(size-x-1);
  int iy=(size-y-1);
  
  int distx=x;
  int disty=y;
  if (ix<distx)
    distx=ix;
  if (iy<disty)
    disty=iy;
  
  return distx+disty;
}

int Go::Board::getCircularDistance(int pos1, int pos2) const
{
  int x1=Go::Position::pos2x(pos1,size);
  int y1=Go::Position::pos2y(pos1,size);
  int x2=Go::Position::pos2x(pos2,size);
  int y2=Go::Position::pos2y(pos2,size);
  
  return Go::circDist(x1,y1,x2,y2);
}

Go::ObjectBoard<int> *Go::Board::getCFGFrom(int pos, int max) const
{
  Go::ObjectBoard<int> *cfgdist=new Go::ObjectBoard<int>(size);
  std::list<int> posqueue, distqueue;
  cfgdist->fill(-1);
  
  posqueue.push_back(pos);
  distqueue.push_back(0);
  while (!posqueue.empty())
  {
    int p=posqueue.front();posqueue.pop_front();
    int d=distqueue.front();distqueue.pop_front();
    
    if ((max==0 || d<=max) && (cfgdist->get(p)==-1 || cfgdist->get(p)>d))
    {
      Go::Color col=this->getColor(p);
      cfgdist->set(p,d);
      
      foreach_adjacent(p,q,{
        if (this->onBoard(q))
        {
          posqueue.push_back(q);
          if (col!=Go::EMPTY && col==this->getColor(q))
            distqueue.push_back(d);
          else
            distqueue.push_back(d+1);
        }
      });
    }
  }
  
  return cfgdist;
}

int Go::Board::getThreeEmptyGroupCenterFrom(int pos) const
{
  if (this->getColor(pos)!=Go::EMPTY)
    return -1;
  
  int empnei=this->touchingEmpty(pos);
  Go::Color col=Go::EMPTY;
  
  if (empnei==2) // possibly at center
  {
    int empty,black,white,offboard;
    foreach_adjacent(pos,p,{
      if (this->getColor(p)==Go::EMPTY)
      {
        if (this->touchingEmpty(p)!=1)
          return -1;
        else
        {
          this->countAdjacentColors(p,empty,black,white,offboard);
          if (col==Go::EMPTY)
          {
            if (black>0 && white>0)
              return -1;
            else if (black>0)
              col=Go::BLACK;
            else if (white>0)
              col=Go::WHITE;
          }
          else if (col==Go::BLACK && white>0)
            return -1;
          else if (col==Go::WHITE && black>0)
            return -1;
        }
      }
      else if (this->getColor(p)!=Go::OFFBOARD)
      {
        Go::Color col2=this->getColor(p);
        if (col==Go::EMPTY)
            col=col2;
        else if (col!=col2)
          return -1;
      }
    });
    return pos;
  }
  else if (empnei==1) // possibly at end
  {
    foreach_adjacent(pos,p,{
      if (this->getColor(p)==Go::EMPTY)
      {
        if (this->touchingEmpty(p)==2)
          return this->getThreeEmptyGroupCenterFrom(p);
        else
          return -1;
      }
    });
    return -1;
  }
  else
    return -1;
}

int Go::Board::getBent4EmptyGroupCenterFrom(int pos,bool onlycheck) const
{
  //fprintf(stderr,"test bent4 %s\n",Go::Position::pos2string(pos,size).c_str());
  if (this->getColor(pos)!=Go::EMPTY || this->getPseudoDistanceToBorder(pos)>2)
    return -1;
  
  int empnei=this->touchingEmpty(pos);
  Go::Color col=Go::EMPTY;
  
  if (empnei==2 && this->getPseudoDistanceToBorder(pos)==1) // possibly at center
  {
    int empty,black,white,offboard;
    foreach_adjacent(pos,p,{
      if (this->getColor(p)==Go::EMPTY)
      {
        if (this->touchingEmpty(p)>2)
          return -1;
        else
        {
          this->countAdjacentColors(p,empty,black,white,offboard);
          if (col==Go::EMPTY)
          {
            if (black>0 && white>0)
              return -1;
            else if (black>0)
              col=Go::BLACK;
            else if (white>0)
              col=Go::WHITE;
          }
          else if (col==Go::BLACK && white>0)
            return -1;
          else if (col==Go::WHITE && black>0)
            return -1;
        }
      }
      else if (this->getColor(p)!=Go::OFFBOARD)
      {
        Go::Color col2=this->getColor(p);
        if (col==Go::EMPTY)
            col=col2;
        else if (col!=col2)
          return -1;
      }
    });
    return pos;
  }
  else if (empnei==1) // possibly at end
  {
    if (onlycheck)
      return -1;
    foreach_adjdiag(pos,p,{
      if (this->getColor(p)==Go::EMPTY)
      {
        //fprintf(stderr,"test bent4 in %s\n",Go::Position::pos2string(p,size).c_str());
        if (this->touchingEmpty(p)==2 )
        {
          int retvalue=this->getBent4EmptyGroupCenterFrom(p,true);
          if (retvalue>-1)
            return retvalue;
        }
      }
    });
    return -1;
  }
  else
    return -1;
}

int Go::Board::getFourEmptyGroupCenterFrom(int pos) const
{
  // this handles the play of the 
  // XXX
  //  X
  //form,

  // the bent 4 is not ready yet!
  
  if (this->getColor(pos)!=Go::EMPTY)
    return -1;
  
  int empnei=this->touchingEmpty(pos);
  Go::Color col=Go::EMPTY;
  
  if (empnei==3) // possibly at center
  {
    int empty,black,white,offboard;
    foreach_adjacent(pos,p,{
      if (this->getColor(p)==Go::EMPTY)
      {
        if (this->touchingEmpty(p)!=1)
          return -1;
        else
        {
          this->countAdjacentColors(p,empty,black,white,offboard);
          if (col==Go::EMPTY)
          {
            if (black>0 && white>0)
              return -1;
            else if (black>0)
              col=Go::BLACK;
            else if (white>0)
              col=Go::WHITE;
          }
          else if (col==Go::BLACK && white>0)
            return -1;
          else if (col==Go::WHITE && black>0)
            return -1;
        }
      }
      else if (this->getColor(p)!=Go::OFFBOARD)
      {
        Go::Color col2=this->getColor(p);
        if (col==Go::EMPTY)
            col=col2;
        else if (col!=col2)
          return -1;
      }
    });
    //fprintf(stderr,"returned by bent4 %s\n",Go::Position::pos2string(pos,9).c_str());
    return pos;
  }
  else if (empnei==1) // possibly at end
  {
    foreach_adjacent(pos,p,{
      if (this->getColor(p)==Go::EMPTY)
      {
        if (this->touchingEmpty(p)==3)
          return this->getFourEmptyGroupCenterFrom(p);
        else
          return -1;
      }
    });
    return -1;
  }
  else
    return -1;
}


int Go::Board::getFiveEmptyGroupCenterFrom(int pos) const
{
  //fprintf(stderr,"test 5 %s\n",Go::Position::pos2string(pos,size).c_str());
  if (this->getColor(pos)!=Go::EMPTY)
    return -1;
  
  int empnei=this->touchingEmpty(pos);
  Go::Color col=Go::EMPTY;
  
  if (empnei==3) // possibly at center
  {
    if (this->diagonalEmpty(pos)!=1)
      return -1; //not exacly 5 empties
    int empty,black,white,offboard;
    this->countAdjacentColors(pos,empty,black,white,offboard);
    if (black>0 && white>0)
      return -1;
    else if (black>0)
      col=Go::BLACK;
    else if (white>0)
      col=Go::WHITE;
    int tmpempty,tmpblack,tmpwhite,tmpoffboard;
    foreach_adjdiag(pos,p,{
      if (this->getColor(p)==Go::EMPTY)
      {
        this->countAdjacentColors(p,tmpempty,tmpblack,tmpwhite,tmpoffboard);
        if (col==Go::EMPTY)
        {
          if (tmpblack>0 && tmpwhite>0)
            return -1;
          else if (tmpblack>0)
            col=Go::BLACK;
          else if (tmpwhite>0)
            col=Go::WHITE;
        }
        else if (col==Go::BLACK && tmpwhite>0)
          return -1;
        else if (col==Go::WHITE && tmpblack>0)
          return -1;
        black+=tmpblack;
        white+=tmpwhite;
        offboard+=tmpoffboard;
      }
    });
    if (black>0 && white>0)
      fprintf(stderr,"should not happen!\n");
    int numattached=black+white+offboard;
    // XXX
    // XX   Form has exactly 10 pseudo touches other forms not (hopefully:)
    if (numattached==10)
      return pos;
    else
      return -1;
  }
  else if (empnei<3) // possibly at end  can be optimized for empnei==1, but should not make to much difference!
  {
    foreach_adjdiag(pos,p,{
      if (this->getColor(p)==Go::EMPTY)
      {
        //fprintf(stderr,"now try at %s\n",Go::Position::pos2string(p,size).c_str());
        if (this->touchingEmpty(p)==3)
          return this->getFiveEmptyGroupCenterFrom(p);
      }
    });
    return -1;
  }
  else
    return -1;
}

void Go::Board::countAdjacentColors(int pos, int &empty, int &black, int &white, int &offboard) const
{
  empty=0;
  black=0;
  white=0;
  offboard=0;
  
  foreach_adjacent(pos,p,{
    switch (this->getColor(p))
    {
      case Go::EMPTY:
        empty++;
        break;
      case Go::BLACK:
        black++;
        break;
      case Go::WHITE:
        white++;
        break;
      default:
        offboard++;
        break;
    }
  });
}

void Go::Board::countDiagonalColors(int pos, int &empty, int &black, int &white, int &offboard) const
{
  empty=0;
  black=0;
  white=0;
  offboard=0;
  
  foreach_diagonal(pos,p,{
    switch (this->getColor(p))
    {
      case Go::EMPTY:
        empty++;
        break;
      case Go::BLACK:
        black++;
        break;
      case Go::WHITE:
        white++;
        break;
      default:
        offboard++;
        break;
    }
  });
}

Go::ZobristHash Go::Board::getZobristHash(Go::ZobristTable *table) const
{
  Go::ZobristHash hash=0;
  
  if (size!=table->getSize())
    return 0;
  
  for (int p=0;p<sizedata;p++)
  {
    if (this->getColor(p)!=Go::EMPTY && this->getColor(p)!=Go::OFFBOARD)
    {
      hash^=table->getHash(this->getColor(p),p);
    }
  }
  
  return hash;
}

Go::ZobristTable::ZobristTable(Parameters *prms, int sz, unsigned long seed)
  : params(prms),
    size(sz),
    sizedata(1+(sz+1)*(sz+2)),
    rand(new Random(seed)),
    blackhashes(new Go::ZobristHash[sizedata]),
    whitehashes(new Go::ZobristHash[sizedata])
{
  for (int i=0;i<sizedata;i++)
  {
    blackhashes[i]=this->getRandomHash();
    whitehashes[i]=this->getRandomHash();
  }
}

Go::ZobristTable::~ZobristTable()
{
  delete rand;
  delete[] blackhashes;
  delete[] whitehashes;
}

Go::ZobristHash Go::ZobristTable::getRandomHash()
{
  return ((Go::ZobristHash)rand->getRandomInt() << 32) | ((Go::ZobristHash)rand->getRandomInt());
}

Go::ZobristHash Go::ZobristTable::getHash(Go::Color col, int pos) const
{
  if (col==Go::BLACK)
    return blackhashes[pos];
  else if (col==Go::WHITE)
    return whitehashes[pos];
  else
    return 0;
}

Go::ZobristTree::ZobristTree() : tree(new Go::ZobristTree::Node(0))
{
}

Go::ZobristTree::~ZobristTree()
{
  delete tree;
}

void Go::ZobristTree::addHash(Go::ZobristHash hash)
{
  tree->add(hash);
}

bool Go::ZobristTree::hasHash(Go::ZobristHash hash) const
{
  return (tree->find(hash)!=NULL);
}

Go::ZobristTree::Node::Node(Go::ZobristHash h) : hash(h)
{
  left=NULL;
  right=NULL;
}

Go::ZobristTree::Node::~Node()
{
  if (left!=NULL)
    delete left;
  if (right!=NULL)
    delete right;
}

void Go::ZobristTree::Node::add(Go::ZobristHash h)
{
  if (h==hash)
    return;
  else if (h<hash)
  {
    if (left==NULL)
      left=new Go::ZobristTree::Node(h);
    else
      left->add(h);
  }
  else
  {
    if (right==NULL)
      right=new Go::ZobristTree::Node(h);
    else
      right->add(h);
  }
}

Go::ZobristTree::Node *Go::ZobristTree::Node::find(Go::ZobristHash h) const
{
  if (h==hash)
    return (Go::ZobristTree::Node *)this;
  else if (h<hash)
  {
    if (left==NULL)
      return NULL;
    else
      return left->find(h);
  }
  else
  {
    if (right==NULL)
      return NULL;
    else
      return right->find(h);
  }
}

Go::TerritoryMap::TerritoryMap(int sz)
  : size(sz),
    sizedata(1+(sz+1)*(sz+2))
{
  boards=0;
  blackowns=new ObjectBoard<float>(sz);
  whiteowns=new ObjectBoard<float>(sz);
  blackowns->fill(0);
  whiteowns->fill(0);
}

Go::TerritoryMap::~TerritoryMap()
{
  delete blackowns;
  delete whiteowns;
}

void Go::TerritoryMap::addPositionOwner(int pos, Go::Color col)
{
  switch (col)
  {
    case Go::BLACK:
      blackowns->set(pos,blackowns->get(pos)+1);
      break;
    case Go::WHITE:
      whiteowns->set(pos,whiteowns->get(pos)+1);
      break;
    default:
      break;
  }
}

float Go::TerritoryMap::getPositionOwner(int pos) const
{
  float b=blackowns->get(pos);
  float w=whiteowns->get(pos);
  float hb=boards/2;
  
  if (b>w) // possibly more black
  {
    if (b>hb) // more black
      return (b-hb)/hb;
    else
      return 0;
  }
  else // possibly more white
  {
    if (w>hb) // more white
      return -(w-hb)/hb;
    else
      return 0;
  }
}

void Go::Board::updateTerritoryMap(Go::TerritoryMap *tmap) const
{
  if (lastscoredata!=NULL)
  {
    for (int p=0;p<sizedata;p++)
    {
      tmap->addPositionOwner(p,lastscoredata[p].color);
    }
    tmap->incrementBoards();
  }
}

void Go::TerritoryMap::decay(float factor)
{
  for (int p=0;p<sizedata;p++)
  {
    blackowns->set(p,blackowns->get(p)*factor);
    whiteowns->set(p,whiteowns->get(p)*factor);
  }
  boards=boards*factor;
}

bool Go::Board::isAlive(Go::TerritoryMap *tmap, float threshold, int pos) const
{
  Go::Color col=this->getColor(pos);
  if (col==Go::BLACK)
    return (-tmap->getPositionOwner(pos))<threshold;
  else if (col==Go::WHITE)
    return (tmap->getPositionOwner(pos))<threshold;
  else
    return false;
}

int Go::Board::territoryScore(Go::TerritoryMap *tmap, float threshold)
{
  int score;
  Go::Board *tmpboard=this->copy();
  
  for (int p=0;p<sizedata;p++)
  {
    if (tmpboard->inGroup(p) && !tmpboard->isAlive(tmap,threshold,p))
      tmpboard->removeGroup(tmpboard->getGroup(p));
  }
  
  score=tmpboard->score();
  if (lastscoredata!=NULL)
    delete [] lastscoredata;
  lastscoredata=tmpboard->lastscoredata;
  tmpboard->lastscoredata=NULL;
  
  delete tmpboard;
  return score;
}

bool Go::Board::isLadder(Go::Group *group) const
{
  int libpos=group->getAtariPosition();
  if (libpos==-1)
    return false;

  return (this->touchingEmpty(libpos)<=2);
}

bool Go::Board::isLadderAfter(Go::Group *group, Go::Move move) const
{
  int libpos=group->getOtherOneOfTwoLiberties(move.getPosition());
  if (libpos==-1 || move.getColor()==group->getColor())
    return false;

  int liberties=0;
  foreach_adjacent(libpos,p,{
      if (this->getColor(p)==Go::EMPTY && p!=move.getPosition())
      {
        liberties++;
        if (liberties>2)
          return false;
      }
  });
  return true;
}

bool Go::Board::isProbableWorkingLadderAfter(Go::Group *group, Go::Move move) const
{
  int posA=group->getOtherOneOfTwoLiberties(move.getPosition());
  if (posA==-1 || move.getColor()==group->getColor())
    return false;
  else if (this->isSelfAtari(move))
    return false;
  else
    return this->isProbableWorkingLadder(group,posA,move.getPosition());
}

bool Go::Board::isProbableWorkingLadder(Go::Group *group) const
{
  int posA=group->getAtariPosition();
  if (posA==-1)
    return false;
  else
    return this->isProbableWorkingLadder(group,posA);
}

bool Go::Board::isProbableWorkingLadder(Go::Group *group, int posA, int movepos) const
{
  int dir=0,dir1=0,dir2=0;

  foreach_adjacent(posA,p,{
    if (this->inGroup(p))
    {
      if (this->getGroup(p)==group)
        dir1=posA-p;
      else if (this->getColor(p)==group->getColor())
        return false;
    }
  });

  foreach_adjacent(posA,p,{
    if (this->getColor(p)==Go::EMPTY && p!=movepos)
    {
      int dir2tmp=p-posA;
      if (dir1!=dir2tmp)
      {
        dir2=dir2tmp;
        dir=dir1+dir2;
      }
    }
  });

  //fprintf(stderr,"dir: %d\n",dir);
  if (dir==0)
    return false;

  //check for stones in atari
  Go::list_int *adjacentgroups=group->getAdjacentGroups();
  if (adjacentgroups->size()>(unsigned int)this->getPositionMax())
  {
    adjacentgroups->sort();
    adjacentgroups->unique();
  }
  for(Go::list_int::iterator iter=adjacentgroups->begin();iter!=adjacentgroups->end();++iter)
  {
    if (this->inGroup((*iter)) && this->getGroup((*iter))->inAtari())
      return false;
  }

  Go::Color col=group->getColor();
  Go::Color othercol=Go::otherColor(col);

  while (true)
  {
    Go::Color colB;
    Go::Color colC;
    if (this->getDistanceToBorder(posA)>2) // in middle
    {
      colB=this->getColor(posA+dir1);
      colC=this->getColor(posA+dir2);
    }
    else // near edge
    {
      colB=this->getColor(posA+dir2);
      colC=this->getColor(posA+dir1);
    }
    //fprintf(stderr,"ladder?: %s %s %c %c\n",Go::Position::pos2string(group->getPosition(),size).c_str(),Go::Position::pos2string(posA,size).c_str(),Go::colorToChar(colB),Go::colorToChar(colC));

    if (colB==Go::OFFBOARD)
      return (colC!=othercol);
    else if (colB!=Go::EMPTY)
      return (colB==othercol);
    else if (colC==Go::OFFBOARD)
      return true;
    else if (colC!=Go::EMPTY)
      return (colC==othercol);
    else
    {
      Go::Color colD=this->getColor(posA-dir2);
      if (colD!=Go::OFFBOARD) colD=this->getColor(posA-dir2-dir2);
      Go::Color colE=this->getColor(posA+dir1);
      if (colE!=Go::OFFBOARD) colE=this->getColor(posA+dir1-dir2);
      //fprintf(stderr,"ladder? 2: %s %c %c\n",Go::Position::pos2string(posA,size).c_str(),Go::colorToChar(colD),Go::colorToChar(colE));
      if (colD!=othercol && colE!=othercol && (colD==col || colE==col))
        return false;
    }

    posA+=dir2;
    if (this->getDistanceToBorder(posA)>2) // in middle
    {
      int dirtmp=dir1;
      dir1=dir2;
      dir2=dirtmp;
    }
  }

  return true;
}
