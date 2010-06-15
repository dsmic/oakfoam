#include "Util.h"

Util::MoveTree::MoveTree(Go::Move mov)
{
  children=new std::list<Util::MoveTree*>();
  move=mov;
  playouts=0;
  wins=0;
}

Util::MoveTree::~MoveTree()
{
  delete children;
}


