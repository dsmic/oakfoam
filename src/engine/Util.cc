#include "Util.h"

Util::MoveTree::MoveTree(Go::Move mov)
{
  sibling=NULL;
  firstchild=NULL;
  move=mov;
  playouts=0;
  wins=0;
}

Util::MoveTree::~MoveTree()
{
  if (sibling!=NULL)
    delete sibling;
  
  if (firstchild!=NULL)
    delete firstchild;
}

void Util::MoveTree::addSibling(Util::MoveTree *node)
{
  if (sibling==NULL)
    sibling=node;
  else
    sibling->addSibling(node);
}

void Util::MoveTree::addChild(Util::MoveTree *node)
{
  if (firstchild==NULL)
    firstchild=node;
  else
    firstchild->addSibling(node);
}

Util::MoveList::MoveList(Go::Move firstmove)
{
  move=firstmove;
  next=NULL;
}

Util::MoveList::~MoveList()
{
  if (next!=NULL)
    delete next;
}

void Util::MoveList::addMove(Go::Move newmove)
{
  if (next==NULL)
    next=new Util::MoveList(newmove);
  else
    next->addMove(newmove);
}


