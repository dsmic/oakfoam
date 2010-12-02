#ifndef DEF_OAKFOAM_UCT_H
#define DEF_OAKFOAM_UCT_H

#define UCT_TERMINAL_URGENCY 10

#include <cmath>
#include <list>
#include <string>
#include <sstream>
#include "Go.h"

namespace UCT
{ 
  class Tree
  {
    public:
      Tree(float uc, float ui, int rm, Go::Move mov = Go::Move(Go::EMPTY,Go::Move::RESIGN), UCT::Tree *p = NULL);
      ~Tree();
      
      UCT::Tree *getParent() { return parent; };
      std::list<UCT::Tree*> *getChildren() { return children; };
      Go::Move getMove() { return move; };
      bool isRoot() { return (parent==NULL); };
      bool isLeaf() { return (children->size()==0); };
      bool isTerminal();
      std::list<Go::Move> getMovesFromRoot();
      void divorceChild(UCT::Tree *child);
      bool isPrimary() { return (symmetryprimary==NULL); };
      UCT::Tree *getPrimary() { return symmetryprimary; };
      
      UCT::Tree *getChild(Go::Move move);
      int getPlayouts() { return playouts; };
      int getRAVEPlayouts() { return raveplayouts; };
      int getPriorPlayouts() { return priorplayouts; };
      float getRatio();
      float getRAVERatio();
      float getPriorRatio();
      float getBasePriorRatio();
      float getVal();
      float getUrgency();
      
      void addChild(UCT::Tree *node);
      void addWin();
      void addLose();
      void addPriorWins(int n);
      void addPriorLoses(int n);
      void addRAVEWin();
      void addRAVELose();
      void addRAVEWins(int n);
      void addRAVELoses(int n);
      
      std::string toSGFString(int boardsize, int numchildren);
      
    private:
      UCT::Tree *parent;
      std::list<UCT::Tree*> *children;
      UCT::Tree *symmetryprimary;
      
      Go::Move move;
      int playouts,raveplayouts,priorplayouts;
      int wins,ravewins,priorwins;
      int ravemoves;
      float ucbc,ucbinit;
      
      void passPlayoutUp(bool win);
      
      static float variance(int wins, int playouts);
  };
};
#endif
