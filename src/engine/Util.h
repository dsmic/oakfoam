#ifndef DEF_OAKFOAM_UTIL_H
#define DEF_OAKFOAM_UTIL_H

#include "Go.h"

namespace Util
{
  static bool isWinForColor(Go::Color col, float score)
  {
    float k=0;
    
    if (col==Go::BLACK)
      k=1;
    else if (col==Go::WHITE)
      k=-1;
    else
      return false;
    
    return ((score*k)>0);
  };
  
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
  
  class MoveList
  {
    public:
      MoveList(Go::Move firstmove);
      ~MoveList();
      
      Go::Move getMove() { return move; };
      Util::MoveList *getNext() { return next; };
      void addMove(Go::Move newmove);
    
    private:
      Go::Move move;
      Util::MoveList *next;
  };
};
#endif
