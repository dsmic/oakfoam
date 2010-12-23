#ifndef DEF_OAKFOAM_UCT_H
#define DEF_OAKFOAM_UCT_H

#define UCT_TERMINAL_URGENCY 100
// must be greater than 1+max(bias)

#include <cmath>
#include <list>
#include <string>
#include <sstream>
#include "Go.h"
#include "Parameters.h"

namespace UCT
{ 
  class Tree
  {
    public:
      Tree(Parameters *prms, Go::Move mov = Go::Move(Go::EMPTY,Go::Move::RESIGN), UCT::Tree *p = NULL);
      ~Tree();
      
      UCT::Tree *getParent() { return parent; };
      std::list<UCT::Tree*> *getChildren() { return children; };
      Go::Move getMove() { return move; };
      bool isRoot() { return (parent==NULL); };
      bool isLeaf() { return (children->size()==0); };
      bool isTerminal();
      bool isTerminalWin() { return (this->isTerminalResult() && wins>0); };
      bool isTerminalLose() { return (this->isTerminalResult() && wins<=0); };
      bool isTerminalResult() { return hasTerminalWinrate; };
      std::list<Go::Move> getMovesFromRoot();
      void divorceChild(UCT::Tree *child);
      bool isPrimary() { return (symmetryprimary==NULL); };
      UCT::Tree *getPrimary() { return symmetryprimary; };
      void setPrimary(UCT::Tree *p) { symmetryprimary=p; };
      void performSymmetryTransformParentPrimary();
      void performSymmetryTransform(Go::Board::SymmetryTransform trans);
      
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
      void addWin(UCT::Tree *source=NULL);
      void addLose(UCT::Tree *source=NULL);
      void addPriorWins(int n);
      void addPriorLoses(int n);
      void addRAVEWin();
      void addRAVELose();
      void addRAVEWins(int n);
      void addRAVELoses(int n);
      
      std::string toSGFString();
      
    private:
      UCT::Tree *parent;
      std::list<UCT::Tree*> *children;
      UCT::Tree *symmetryprimary;
      
      Go::Move move;
      int playouts,raveplayouts,priorplayouts;
      int wins,ravewins,priorwins;
      Parameters *params;
      bool hasTerminalWinrate;
      
      void passPlayoutUp(bool win, UCT::Tree *source);
      
      static float variance(int wins, int playouts);
  };
};
#endif
