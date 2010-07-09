#include "Util.h"

Util::MoveTree::MoveTree(float uc, int rm, Go::Move mov, Util::MoveTree *p)
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

float Util::MoveTree::makeRAVEValue(float ratio, float raveratio, int playouts, int ravemoves)
{
  float alpha;
  if (ravemoves>0)
    alpha=(float)(ravemoves-playouts)/ravemoves;
  else
    alpha=0;
  
  if (alpha<0)
    alpha=0;
  return raveratio*alpha + ratio*(1-alpha);
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
  if (raveplayouts>0)
    return Util::MoveTree::makeRAVEValue(this->getRatio(),this->getRAVERatio(),playouts,ravemoves);
  else
    return this->getRatio();
}

float Util::MoveTree::getUrgency()
{
  float bias;
  if (playouts==0 && raveplayouts==0)
    return 10;
  
  if (parent==NULL)
    bias=0;
  else
  {
    if (parent->getPlayouts()>0 && playouts>0)
      bias=ucbc*sqrt(log((float)parent->getPlayouts())/(playouts));
    else if (parent->getPlayouts()>0)
      bias=ucbc*sqrt(log((float)parent->getPlayouts())/(1))*2;
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

