#include "Go.h"

#include <cstdio>
#include <sstream>
#include "Features.h"
#include "Parameters.h"
#include "Engine.h"

Go::BitBoard::BitBoard(int s)
  : size(s),
    sizesq(s*s),
    sizedata(1+(s+1)*(s+2)),
    data(new bool[sizedata])
{
  for (int i=0;i<sizedata;i++)
    data[i]=false;
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
  parent=NULL;
  pseudoliberties=0;
  libpossum=0;
  libpossumsq=0;
}

void Go::Group::addTouchingEmpties()
{
  int size=board->getSize();
  foreach_adjacent(position,p,{
    if (board->getColor(p)==Go::EMPTY)
      this->addPseudoLiberty(p);
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
  
  features=NULL;
  blacktotalgamma=0;
  whitetotalgamma=0;
  blackgammas=new Go::ObjectBoard<float>(s);
  whitegammas=new Go::ObjectBoard<float>(s);
  blackgammas->fill(0);
  whitegammas->fill(0);
  
  blackcaptures=0;
  whitecaptures=0;
  lastscoredata=NULL;
}

Go::Board::~Board()
{
  delete blackvalidmoves;
  delete whitevalidmoves;
  
  delete lastchanges;
  
  delete blackgammas;
  delete whitegammas;
  
  //XXX: memory will get freed when pool is destroyed
  /*for(std::list<Go::Group*,Go::allocator_groupptr>::iterator iter=groups.begin();iter!=groups.end();++iter) 
  {
    pool_group.destroy((*iter));
  }*/
  groups.resize(0);
  
  if (lastscoredata!=NULL)
    delete[] lastscoredata;
  
  delete[] data;
}

Go::Board *Go::Board::copy()
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
  copyboard->updateFeatureGammas();
  
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
    if (!scoredata[p].touched && this->getColor(p)!=Go::EMPTY)
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
    this->updateFeatureGammas();
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
  groups.push_back(thisgroup);
  
  thisgroup->addTouchingEmpties();
  
  foreach_adjacent(pos,p,{
    if (this->getColor(p)==col)
    {
      Go::Group *othergroup=this->getGroup(p);
      othergroup->removePseudoLiberty(pos);
      
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
      
      thisgroup->getAdjacentGroups()->push_back(p);
      othergroup->getAdjacentGroups()->push_back(pos);
      
      if (othergroup->numOfPseudoLiberties()==0)
      {
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
  if (features!=NULL)
    this->updateFeatureGammas();
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
  groups.resize(0);
  
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
      groups.push_back(newgroup);
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
    groups.push_back(thisgroup);
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
  
  groups.remove(group);
  
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
  
  groups.remove(second);
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

void Go::Board::updateFeatureGammas()
{
  if (markchanges && features!=NULL)
  {
    if (incfeatures)
    {
      //expand to 3x3 neighbourhood first
      //assume only the 3x3 neighbourhood is relevant
      //see: params->features_only_small
      Go::BitBoard *changes3x3=new Go::BitBoard(size);
      changes3x3->clear();
      for (int p=0;p<sizedata;p++)
      {
        if (lastchanges->get(p))
        {
          changes3x3->set(p);
          foreach_adjdiag(p,q,{
            if (this->onBoard(q))
              changes3x3->set(q);
          });
        }
      }
      
      for (int p=0;p<sizedata;p++)
      {
        if (changes3x3->get(p))
          this->updateFeatureGamma(p);
      }
      delete changes3x3;
      
      this->updateFeatureGamma(0); //pass
      
      lastchanges->clear();
    }
    else
    {
      (nexttomove==Go::BLACK?blacktotalgamma:whitetotalgamma)=0;
      (nexttomove==Go::BLACK?blackgammas:whitegammas)->fill(0);
      lastchanges->clear();
      
      for (int p=0;p<sizedata;p++)
      {
        this->updateFeatureGamma(nexttomove,p);
      }
    }
  }
}

void Go::Board::refreshFeatureGammas()
{
  blacktotalgamma=0;
  whitetotalgamma=0;
  blackgammas->fill(0);
  whitegammas->fill(0);
  lastchanges->clear();
  
  for (int p=0;p<sizedata;p++)
  {
    this->updateFeatureGamma(p);
  }
}

void Go::Board::updateFeatureGamma(int pos)
{
  this->updateFeatureGamma(Go::BLACK,pos);
  this->updateFeatureGamma(Go::WHITE,pos);
}

void Go::Board::updateFeatureGamma(Go::Color col, int pos)
{
  float oldgamma=(col==Go::BLACK?blackgammas:whitegammas)->get(pos);
  float gamma;
  if (pos==0) //pass
    gamma=features->getMoveGamma(this,Go::Move(col,Go::Move::PASS));
  else if (!this->weakEye(nexttomove,pos))
    gamma=features->getMoveGamma(this,Go::Move(col,pos));
  else
    gamma=0;
  (col==Go::BLACK?blackgammas:whitegammas)->set(pos,gamma);
  (col==Go::BLACK?blacktotalgamma:whitetotalgamma)+=(gamma-oldgamma);
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
  Go::Color col=move.getColor();
  int pos=move.getPosition();
  
  if (this->touchingEmpty(pos)>1)
    return false;
  
  bool foundgroupwith2libsorless=false;
  int libpos=-1;
  bool foundconnection=false;
  //Go::Group *grp2libs=NULL;
  foreach_adjacent(pos,p,{
    if (this->inGroup(p))
    {
      Go::Group *group=this->getGroup(p);
      if (col==group->getColor())
      {
        int otherlib=group->getOtherOneOfTwoLiberties(pos);
        if (otherlib!=-1)
        {
          if (!foundgroupwith2libsorless)
          {
            foundgroupwith2libsorless=true;
            //grp2libs=group;
            libpos=otherlib;
          }
          else if (libpos!=otherlib)
            foundconnection=true;
        }
        else if (!group->inAtari()) // more than 2 libs
          foundconnection=true;
      }
    }
  });
  if (!foundconnection)
    return true;
  else
    return false;
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
      //fprintf(stderr,"0x%016llx\n",hash);
    }
  }
  
  return hash;
}

Go::ZobristTable::ZobristTable(Parameters *prms, int sz)
  : params(prms),
    size(sz),
    sizedata(1+(sz+1)*(sz+2)),
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
  delete[] blackhashes;
  delete[] whitehashes;
}

Go::ZobristHash Go::ZobristTable::getRandomHash()
{
  return ((Go::ZobristHash)params->engine->getRandom()->getRandomInt() << 32) | ((Go::ZobristHash)params->engine->getRandom()->getRandomInt());
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

