#include "Util.h"

Util::MoveTree::MoveTree(float uc, int rm, Go::Move mov, Util::MoveTree *p)
{
  parent=p;
  children=new std::list<Util::MoveTree*>();
  move=mov;
  playouts=0;
  ratio=0;
  raveplayouts=0;
  raveratio=0;
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

void Util::MoveTree::addWin()
{
  int wins=ratio*playouts;
  wins++;
  playouts++;
  ratio=(float)wins/playouts;
  this->passPlayoutUp(true);
}

void Util::MoveTree::addLose()
{
  int wins=ratio*playouts;
  playouts++;
  ratio=(float)wins/playouts;
  this->passPlayoutUp(false);
}

void Util::MoveTree::addRAVEWin()
{
  int ravewins=raveratio*raveplayouts;
  ravewins++;
  raveplayouts++;
  raveratio=(float)ravewins/raveplayouts;
  //this->passPlayoutUp();
}

void Util::MoveTree::addRAVELose()
{
  int ravewins=raveratio*raveplayouts;
  raveplayouts++;
  raveratio=(float)ravewins/raveplayouts;
  //this->passPlayoutUp();
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
    alpha=(ravemoves-playouts)/ravemoves;
  else
    alpha=0;
  
  if (alpha<0)
    alpha=0;
  return raveratio*alpha + ratio*(1-alpha);
}

void Util::MoveTree::updateFromChildPlayout(bool win)
{
  //assume alternating colours going up
  if (win)
    this->addLose();
  else
    this->addWin();
}

void Util::MoveTree::passPlayoutUp(bool win)
{
  if (parent!=NULL)
    parent->updateFromChildPlayout(win);
}

float Util::MoveTree::getVal()
{
  return Util::MoveTree::makeRAVEValue(ratio,raveratio,playouts,ravemoves);
}

float Util::MoveTree::getUrgency()
{
  float bias;
  if (playouts==0 && raveplayouts==0)
    return 10;
  
  if (parent!=NULL && playouts!=0)
    bias=ucbc*sqrt(log(parent->getPlayouts())/(playouts));
  else
    bias=ucbc*sqrt(log(parent->getPlayouts())/(1))*2;
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

