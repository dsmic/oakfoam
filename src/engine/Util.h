#ifndef DEF_OAKFOAM_UTIL_H
#define DEF_OAKFOAM_UTIL_H

#include <list>
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
      MoveTree(Go::Move mov = Go::Move(Go::EMPTY,Go::Move::PASS));
      ~MoveTree();
      
      std::list<Util::MoveTree*> *getChildren() { return children; };
      Go::Move getMove() { return move; };
      int getWins() { return wins; };
      int getPlayouts() { return playouts; };
      
      void addChild(Util::MoveTree *node) { children->push_back(node); };
      void addWin() { playouts++; wins++; };
      void addLose() { playouts++; };
      
    private:
      std::list<Util::MoveTree*> *children;
      
      Go::Move move;
      int playouts;
      int wins;
  };
};
#endif
