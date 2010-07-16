#include "Util.h"

Util::MoveTree::MoveTree(float uc, float ui, int rm, Go::Move mov, Util::MoveTree *p)
{
  parent=p;
  children=new std::list<Util::MoveTree*>();
  move=mov;
  playouts=0;
  wins=0;
  raveplayouts=0;
  ravewins=0;
  ravemoves=rm;
  ucbc=uc;
  ucbinit=ui;
}

Util::MoveTree::~MoveTree()
{
  for(std::list<Util::MoveTree*>::iterator iter=children->begin();iter!=children->end();++iter) 
  {
    delete (*iter);
  }
  delete children;
}

void Util::MoveTree::addChild(Util::MoveTree *node)
{
  children->push_back(node);
  node->parent=this;
}

float Util::MoveTree::getRatio()
{
  if (playouts>0)
    return (float)wins/playouts;
  else
    return 0;
}

float Util::MoveTree::getRAVERatio()
{
  if (raveplayouts>0)
    return (float)ravewins/raveplayouts;
  else
    return 0;
}

void Util::MoveTree::addWin()
{
  wins++;
  playouts++;
  this->passPlayoutUp(true);
}

void Util::MoveTree::addLose()
{
  playouts++;
  this->passPlayoutUp(false);
}

void Util::MoveTree::addRAVEWin()
{
  ravewins++;
  raveplayouts++;
}

void Util::MoveTree::addRAVELose()
{
  raveplayouts++;
}

Util::MoveTree *Util::MoveTree::getChild(Go::Move move)
{
  for(std::list<Util::MoveTree*>::iterator iter=children->begin();iter!=children->end();++iter) 
  {
    if ((*iter)->getMove().getColor()==move.getColor() && (*iter)->getMove().getX()==move.getX() && (*iter)->getMove().getY()==move.getY()) 
      return (*iter);
  }
  return NULL;
}

void Util::MoveTree::passPlayoutUp(bool win)
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

float Util::MoveTree::getVal()
{
  if (ravemoves==0 || raveplayouts==0)
    return this->getRatio();
  else if (playouts==0)
    return this->getRAVERatio();
  else
  {
    //float alpha=(float)(ravemoves-playouts)/ravemoves;
    //if (alpha<0) alpha=0;
    
    float alpha=(float)raveplayouts/(raveplayouts + playouts + (float)(raveplayouts*playouts)/ravemoves);
    
    return this->getRAVERatio()*alpha + this->getRatio()*(1-alpha);
  }
}

float Util::MoveTree::getUrgency()
{
  float bias;
  if (playouts==0 && raveplayouts==0)
    return ucbinit;
  
  if (parent==NULL || ucbc==0)
    bias=0;
  else
  {
    if (parent->getPlayouts()>0 && playouts>0)
      bias=ucbc*sqrt(log((float)parent->getPlayouts())/(playouts));
    else if (parent->getPlayouts()>0)
      bias=ucbc*sqrt(log((float)parent->getPlayouts())/(1));
    else
      bias=0;
  }
  
  return this->getVal()+bias;
}

std::list<Go::Move> Util::MoveTree::getMovesFromRoot()
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

bool Util::MoveTree::isTerminal()
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

void Util::MoveTree::divorceChild(Util::MoveTree *child)
{
  children->remove(child);
  child->parent=NULL;
}

