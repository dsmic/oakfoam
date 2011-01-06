#include "UCT.h"

#include <cmath>
#include <sstream>

UCT::Tree::Tree(Parameters *prms, Go::Move mov, UCT::Tree *p)
{
  parent=p;
  children=new std::list<UCT::Tree*>();
  params=prms;
  move=mov;
  playouts=0;
  wins=0;
  raveplayouts=0;
  ravewins=0;
  priorplayouts=0;
  priorwins=0;
  symmetryprimary=NULL;
  hasTerminalWinrate=false;
  
  if (parent!=NULL)
  {
    Go::Move pmove=parent->getMove();
    if (!pmove.isPass() && !pmove.isResign())
    {
      if (pmove.getColor()==move.getColor())
        fprintf(stderr,"WARNING! unexpected parent color\n");
    }
  }
}

UCT::Tree::~Tree()
{
  for(std::list<UCT::Tree*>::iterator iter=children->begin();iter!=children->end();++iter) 
  {
    delete (*iter);
  }
  delete children;
}

void UCT::Tree::addChild(UCT::Tree *node)
{
  children->push_back(node);
  node->parent=this;
}

float UCT::Tree::getRatio()
{
  if (playouts>0)
    return (float)wins/playouts;
  else if (playouts==-1)
    return wins;
  else
    return 0;
}

float UCT::Tree::getRAVERatio()
{
  if (raveplayouts>0)
    return (float)ravewins/raveplayouts;
  else
    return 0;
}

float UCT::Tree::getPriorRatio()
{
  if (priorplayouts>0)
    return (float)priorwins/priorplayouts;
  else
    return 0;
}

float UCT::Tree::getBasePriorRatio()
{
  if (playouts>0 || priorplayouts>0)
    return (float)(wins+priorwins)/(playouts+priorplayouts);
  else
    return 0;
}

void UCT::Tree::addWin(UCT::Tree *source)
{
  wins++;
  playouts++;
  this->passPlayoutUp(true,source);
}

void UCT::Tree::addLose(UCT::Tree *source)
{
  playouts++;
  this->passPlayoutUp(false,source);
}

void UCT::Tree::addPriorWins(int n)
{
  priorwins+=n;
  priorplayouts+=n;
}

void UCT::Tree::addPriorLoses(int n)
{
  priorplayouts+=n;
}

void UCT::Tree::addRAVEWin()
{
  ravewins++;
  raveplayouts++;
}

void UCT::Tree::addRAVELose()
{
  raveplayouts++;
}

void UCT::Tree::addRAVEWins(int n)
{
  ravewins+=n;
  raveplayouts+=n;
}

void UCT::Tree::addRAVELoses(int n)
{
  raveplayouts+=n;
}

UCT::Tree *UCT::Tree::getChild(Go::Move move)
{
  for(std::list<UCT::Tree*>::iterator iter=children->begin();iter!=children->end();++iter) 
  {
    if ((*iter)->getMove()==move) 
      return (*iter);
  }
  return NULL;
}

void UCT::Tree::passPlayoutUp(bool win, UCT::Tree *source)
{
  if (this->isTerminal() && !this->isTerminalResult() && win)
  {
    wins=1;
    playouts=-1;
    hasTerminalWinrate=true;
  }
  
  if (source!=NULL && source->isTerminalResult() && win)
  {
    wins=1;
    playouts=-1;
    hasTerminalWinrate=true;
    //fprintf(stderr,"New Terminal Result %d!\n",win);
  }
  
  if (parent!=NULL)
  {
    //assume alternating colours going up
    if (win)
      parent->addLose(this);
    else
      parent->addWin(this);
  }
  
  if (this->isTerminal() && !this->isTerminalResult() && !win)
  {
    wins=0;
    playouts=-1;
    hasTerminalWinrate=true;
  }
  
  if (source!=NULL && source->isTerminalResult() && !win)
  {
    wins=0;
    playouts=-1;
    hasTerminalWinrate=true;
    //fprintf(stderr,"New Terminal Result %d!\n",win);
  }
}

float UCT::Tree::getVal()
{
  if (this->isTerminal())
  {
    if (playouts==-1)
      return wins;
    else
      return 1;
  }
  if (params->rave_moves==0 || raveplayouts==0)
    return this->getBasePriorRatio();
  else if (playouts==0)
    return this->getRAVERatio();
  else
  {
    //float alpha=(float)(ravemoves-playouts)/params->rave_moves;
    
    float alpha=(float)raveplayouts/(raveplayouts + playouts + (float)(raveplayouts*playouts)/params->rave_moves);
    
    if (alpha<=0)
      return this->getBasePriorRatio();
    else if (alpha>=1)
      return this->getRAVERatio();
    else
      //return this->getRAVERatio()*alpha + this->getRatio()*(1-alpha);
      return this->getRAVERatio()*alpha + this->getBasePriorRatio()*(1-alpha);
  }
}

float UCT::Tree::getUrgency()
{
  float bias;
  if (this->isTerminalWin())
    return UCT_TERMINAL_URGENCY;
  else if (this->isTerminalLose())
    return -UCT_TERMINAL_URGENCY;
  
  if (playouts==0 && raveplayouts==0 && priorplayouts==0)
    return params->ucb_init;
  
  if (this->isTerminal())
  {
    if (playouts==0 || this->getVal()>0)
      return UCT_TERMINAL_URGENCY;
    else
      return -UCT_TERMINAL_URGENCY;
  }
  
  int plts=playouts+priorplayouts;
  
  if (parent==NULL || params->ucb_c==0)
    bias=0;
  else
  {
    if (parent->getPlayouts()>1 && plts>0)
      bias=params->ucb_c*sqrt(log((float)parent->getPlayouts())/(plts));
    else if (parent->getPlayouts()>1)
      bias=params->ucb_c*sqrt(log((float)parent->getPlayouts())/(1));
    else
      bias=params->ucb_c/2;
  }
  
  return this->getVal()+bias;
}

std::list<Go::Move> UCT::Tree::getMovesFromRoot()
{
  if (this->isRoot())
    return std::list<Go::Move>();
  else
  {
    std::list<Go::Move> list=parent->getMovesFromRoot();
    list.push_back(this->getMove());
    return list;
  }
}

bool UCT::Tree::isTerminal()
{
  if (hasTerminalWinrate)
    return true;
  else if (!this->isRoot())
  {
    if (this->getMove().isPass() && parent->getMove().isPass())
      return true;
    else
      return false;
  }
  else
    return false;
}

void UCT::Tree::divorceChild(UCT::Tree *child)
{
  children->remove(child);
  child->parent=NULL;
}

float UCT::Tree::variance(int wins, int playouts)
{
  return wins-(float)(wins*wins)/playouts;
}

std::string UCT::Tree::toSGFString()
{
  std::ostringstream ss;
  if (!this->isRoot())
  {
    ss<<"(;"<<Go::colorToChar(move.getColor())<<"[";
    if (!move.isPass()&&!move.isResign())
    {
      ss<<(char)(move.getX(params->board_size)+'a');
      ss<<(char)(params->board_size-move.getY(params->board_size)+'a'-1);
    }
    else if (move.isPass())
      ss<<"pass";
    ss<<"]C[";
    if (this->isTerminalWin())
      ss<<"Terminal Win";
    else if (this->isTerminalLose())
        ss<<"Terminal Lose";
    else
    {
      ss<<"Wins/Playouts: "<<wins<<"/"<<playouts<<"("<<this->getRatio()<<")\n";
      ss<<"RAVE Wins/Playouts: "<<ravewins<<"/"<<raveplayouts<<"("<<this->getRAVERatio()<<")";
    }
    ss<<"]";
  }
  
  UCT::Tree *bestchild=NULL;
  int besti=0;
  bool usedchild[children->size()];
  for (unsigned int i=0;i<children->size();i++)
    usedchild[i]=false;
  int childrendone=0;
  
  while (true)
  {
    int i=0;
    for(std::list<UCT::Tree*>::iterator iter=children->begin();iter!=children->end();++iter) 
    {
      if (usedchild[i]==false && ((*iter)->getPlayouts()>0 || (*iter)->isTerminal()))
      {
        if (bestchild==NULL || (*iter)->getPlayouts()>bestchild->getPlayouts() || (*iter)->isTerminalWin())
        {
          bestchild=(*iter);
          besti=i;
          if ((*iter)->isTerminalWin())
            break;
        }
      }
      i++;
    }
    if (bestchild==NULL)
      break;
    
    ss<<bestchild->toSGFString();
    usedchild[besti]=true;
    bestchild=NULL;
    besti=0;
    childrendone++;
    if (params->outputsgf_maxchildren!=0 && childrendone>=params->outputsgf_maxchildren)
      break;
  }
  if (!this->isRoot())
    ss<<")\n";
  return ss.str();
}

void UCT::Tree::performSymmetryTransform(Go::Board::SymmetryTransform trans)
{
  if (!move.isPass() && !move.isResign())
    move.setPosition(Go::Board::doSymmetryTransformStaticReverse(trans,params->board_size,move.getPosition()));
  
  for(std::list<UCT::Tree*>::iterator iter=children->begin();iter!=children->end();++iter) 
  {
    (*iter)->performSymmetryTransform(trans);
  }
}

void UCT::Tree::performSymmetryTransformParentPrimary()
{
  if (!this->isRoot() && !this->isPrimary())
  {
    Go::Board::SymmetryTransform trans=Go::Board::getSymmetryTransformBetweenPositions(params->board_size,move.getPosition(),symmetryprimary->getMove().getPosition());
    //fprintf(stderr,"transform: ix:%d iy:%d sxy:%d\n",trans.invertX,trans.invertY,trans.swapXY);
    parent->performSymmetryTransform(trans);
  }
}

