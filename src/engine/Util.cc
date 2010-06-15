#include "Util.h"

Util::MoveTree::MoveTree(Go::Move mov)
{
  children=new std::list<Util::MoveTree*>();
  move=mov;
  playouts=0;
  ratio=0;
  mean=0;
}

Util::MoveTree::~MoveTree()
{
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

