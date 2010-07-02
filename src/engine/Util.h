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
      int getPlayouts() { return playouts; };
      float getRatio() { return ratio; };
      float getMean() { return mean; };
      Util::MoveTree *getChild(Go::Move move);
      
      void addChild(Util::MoveTree *node) { children->push_back(node); };
      void addWin(float score=0);
      void addLose(float score=0);
      
    private:
      std::list<Util::MoveTree*> *children;
      
      Go::Move move;
      int playouts;
      float ratio;
      float mean;
  };
};
#endif
