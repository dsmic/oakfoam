#include "UCT.h"

UCT::Tree::Tree(float uc, float ui, int rm, Go::Move mov, UCT::Tree *p)
{
  parent=p;
  children=new std::list<UCT::Tree*>();
  move=mov;
  playouts=0;
  wins=0;
  raveplayouts=0;
  ravewins=0;
  ravemoves=rm;
  ucbc=uc;
  ucbinit=ui;
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

void UCT::Tree::addWin()
{
  wins++;
  playouts++;
  this->passPlayoutUp(true);
}

void UCT::Tree::addLose()
{
  playouts++;
  this->passPlayoutUp(false);
}

void UCT::Tree::addNodeWins(int n)
{
  wins+=n;
  playouts+=n;
}

void UCT::Tree::addNodeLoses(int n)
{
  playouts+=n;
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

void UCT::Tree::passPlayoutUp(bool win)
{
  if (parent!=NULL)
  {
    //assume alternating colours going up
    if (win)
      parent->addLose();
    else
      parent->addWin();
  }
}

float UCT::Tree::getVal()
{
  if (ravemoves==0 || raveplayouts==0)
    return this->getRatio();
  else if (playouts==0)
    return this->getRAVERatio();
  else
  {
    //float alpha=(float)(ravemoves-playouts)/ravemoves;
    
    float alpha=(float)raveplayouts/(raveplayouts + playouts + (float)(raveplayouts*playouts)/ravemoves);
    
    if (alpha<=0)
      return this->getRatio();
    else if (alpha>=1)
      return this->getRAVERatio();
      //return this->getRAVERatio() + this->getRatio();
    else
      return this->getRAVERatio()*alpha + this->getRatio()*(1-alpha);
      //return this->getRAVERatio()*alpha + this->getRatio();
  }
}

float UCT::Tree::getUrgency()
{
  float bias;
  if (playouts==0 && raveplayouts==0)
    return ucbinit;
  
  if (this->isTerminal())
  {
    if (playouts==0 || this->getVal()>0)
      return UCT_TERMINAL_URGENCY;
    else
      return -UCT_TERMINAL_URGENCY;
  }
  
  if (parent==NULL || ucbc==0)
    bias=0;
  else
  {
    if (parent->getPlayouts()>1 && playouts>0)
      bias=ucbc*sqrt(log((float)parent->getPlayouts())/(playouts));
    else if (parent->getPlayouts()>1)
      bias=ucbc*sqrt(log((float)parent->getPlayouts())/(1));
    else
      bias=ucbc/2;
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
  if (!this->isRoot())
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

std::string UCT::Tree::toSGFString(int boardsize, int maxchildren)
{
  std::ostringstream ss;
  if (!this->isRoot())
  {
    ss<<"(;"<<Go::colorToChar(move.getColor())<<"[";
    if (!move.isPass()&&!move.isResign())
    {
      ss<<(char)(move.getX(boardsize)+'a');
      ss<<(char)(boardsize-move.getY(boardsize)+'a'-1);
    }
    ss<<"]C[";
    ss<<"Wins/Playouts: "<<wins<<"/"<<playouts<<"("<<this->getRatio()<<")\n";
    ss<<"RAVE Wins/Playouts: "<<ravewins<<"/"<<raveplayouts<<"("<<this->getRAVERatio()<<")";
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
      if (usedchild[i]==false && (*iter)->getPlayouts()>0)
      {
        if (bestchild==NULL || (*iter)->getPlayouts()>bestchild->getPlayouts())
        {
          bestchild=(*iter);
          besti=i;
        }
      }
      i++;
    }
    if (bestchild==NULL)
      break;
    
    ss<<bestchild->toSGFString(boardsize,maxchildren);
    usedchild[besti]=true;
    bestchild=NULL;
    besti=0;
    childrendone++;
    if (maxchildren!=0 && childrendone>=maxchildren)
      break;
  }
  if (!this->isRoot())
    ss<<")\n";
  return ss.str();
}

