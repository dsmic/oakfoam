#ifndef DEF_OAKFOAM_UTIL_H
#define DEF_OAKFOAM_UTIL_H

#include "Go.h"

namespace Util
{
  class MoveTree
  {
    public:
      MoveTree(Go::Move mov);
      ~MoveTree();
      
      Util::MoveTree *getSibling() { return sibling; };
      Util::MoveTree *getFirstChild() { return firstchild; };
      Go::Move getMove() { return move; };
      int getWins() { return wins; };
      int getPlayouts() { return playouts; };
      
      void addSibling(Util::MoveTree *node);
      void addChild(Util::MoveTree *node);
      void addWin() { playouts++; wins++; };
      void addLose() { playouts++; };
      
    private:
      Util::MoveTree *sibling;
      Util::MoveTree *firstchild;
      
      Go::Move move;
      int playouts;
      int wins;
  };
  
  class MoveSequence
  {
    public:
      MoveSequence(Go::Move firstmove);
      ~MoveSequence();
      
      Go::Move getMove() { return move; };
      Util::MoveSequence *getNext() { return next; };
      void addMove(Go::Move newmove);
    
    private:
      Go::Move move;
      Util::MoveSequence *next;
  };
};
#endif
