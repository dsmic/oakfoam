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

Util::MoveSequence::MoveSequence(Go::Move firstmove)
{
  move=firstmove;
  next=NULL;
}

Util::MoveSequence::~MoveSequence()
{
  if (next!=NULL)
    delete next;
}

void Util::MoveSequence::addMove(Go::Move newmove)
{
  if (next==NULL)
    next=new Util::MoveSequence(newmove);
  else
    next->addMove(newmove);
}


