#ifndef DEF_OAKFOAM_UTIL_H
#define DEF_OAKFOAM_UTIL_H

#include <cmath>
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
      MoveTree(float uc, int rm, Go::Move mov = Go::Move(Go::EMPTY,Go::Move::PASS), Util::MoveTree *p = NULL);
      ~MoveTree();
      
      Util::MoveTree *getParent() { return parent; };
      std::list<Util::MoveTree*> *getChildren() { return children; };
      Go::Move getMove() { return move; };
      bool isRoot() { return (parent==NULL); };
      bool isLeaf() { return (children->size()==0); };
      std::list<Go::Move> getMovesFromRoot();
      
      Util::MoveTree *getChild(Go::Move move);
      int getPlayouts() { return playouts; };
      int getRAVEPlayouts() { return raveplayouts; };
      float getRatio() { return ratio; };
      float getRAVERatio() { return raveratio; };
      float getVal();
      float getUrgency();
      
      void addChild(Util::MoveTree *node);
      void addWin();
      void addLose();
      void addRAVEWin();
      void addRAVELose();
      
    private:
      Util::MoveTree *parent;
      std::list<Util::MoveTree*> *children;
      
      Go::Move move;
      int playouts,raveplayouts;
      float ratio,raveratio;
      int ravemoves;
      float ucbc;
      
      void passPlayoutUp(bool win);
      
      static float makeRAVEValue(float ratio, float raveratio, int playouts, int ravemoves);
  };
};
#endif
