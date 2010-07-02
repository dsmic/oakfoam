#include "Util.h"

Util::MoveTree::MoveTree(Go::Move mov)
{
  children=new std::list<Util::MoveTree*>();
  move=mov;
  playouts=0;
  ratio=0;
  mean=0;
  raveplayouts=0;
  raveratio=0;
}

Util::MoveTree::~MoveTree()
{
  for(std::list<Util::MoveTree*>::iterator iter=children->begin();iter!=children->end();++iter) 
  {
    delete (*iter);
  }
  delete children;
}

void Util::MoveTree::addWin(float score)
{
  int wins=ratio*playouts;
  float totalscore=mean*playouts;
  wins++;
  totalscore+=score;
  playouts++;
  ratio=(float)wins/playouts;
  mean=(float)totalscore/playouts;
}

void Util::MoveTree::addLose(float score)
{
  int wins=ratio*playouts;
  float totalscore=mean*playouts;
  totalscore+=score;
  playouts++;
  ratio=(float)wins/playouts;
  mean=(float)totalscore/playouts;
}

void Util::MoveTree::addRAVEWin()
{
  int ravewins=raveratio*raveplayouts;
  ravewins++;
  raveplayouts++;
  raveratio=(float)ravewins/raveplayouts;
}

void Util::MoveTree::addRAVELose()
{
  int ravewins=raveratio*raveplayouts;
  raveplayouts++;
  raveratio=(float)ravewins/raveplayouts;
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

float Util::MoveTree::getRAVERatio(int ravemoves)
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

