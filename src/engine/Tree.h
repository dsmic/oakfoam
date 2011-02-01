#ifndef DEF_OAKFOAM_TREE_H
#define DEF_OAKFOAM_TREE_H

#define TREE_TERMINAL_URGENCY 100
// must be greater than 1+max(bias)

#include <list>
#include <string>
#include "Go.h"
//from "Parameters.h":
class Parameters;

class Tree
{
  public:
    Tree(Parameters *prms, Go::Move mov = Go::Move(Go::EMPTY,Go::Move::RESIGN), Tree *p = NULL);
    ~Tree();
    
    Tree *getParent() { return parent; };
    std::list<Tree*> *getChildren() { return children; };
    Go::Move getMove() { return move; };
    bool isRoot() { return (parent==NULL); };
    bool isLeaf() { return (children->size()==0); };
    bool isTerminal();
    bool isTerminalWin() { return (this->isTerminalResult() && wins>0); };
    bool isTerminalLose() { return (this->isTerminalResult() && wins<=0); };
    bool isTerminalResult() { return hasTerminalWinrate; };
    std::list<Go::Move> getMovesFromRoot();
    void divorceChild(Tree *child);
    bool isPrimary() { return (symmetryprimary==NULL); };
    Tree *getPrimary() { return symmetryprimary; };
    void setPrimary(Tree *p) { symmetryprimary=p; };
    void performSymmetryTransformParentPrimary();
    void performSymmetryTransform(Go::Board::SymmetryTransform trans);
    
    bool isPruned() { return pruned; };
    void setPruned(bool p) { pruned=p; };
    void pruneChildren();
    void setPruneFactor(float p) { prunefactor=p; };
    void checkForUnPruning();
    void unPruneNow();
    
    Tree *getChild(Go::Move move);
    float getPlayouts() { return playouts; };
    float getRAVEPlayouts() { return raveplayouts; };
    float getPriorPlayouts() { return priorplayouts; };
    float getRatio();
    float getRAVERatio();
    float getPriorRatio();
    float getBasePriorRatio();
    float getVal();
    float getUrgency();
    
    void addChild(Tree *node);
    void addWin(Tree *source=NULL);
    void addLose(Tree *source=NULL);
    void addPriorWins(int n);
    void addPriorLoses(int n);
    void addRAVEWin();
    void addRAVELose();
    void addRAVEWins(int n);
    void addRAVELoses(int n);
    void addPartialResult(float win, float playout);
    
    void expandLeaf();
    Tree *getRobustChild(bool descend=false);
    Tree *getUrgentChild();
    void updateRAVE(Go::Color wincol,Go::BitBoard *blacklist,Go::BitBoard *whitelist);
    
    void allowContinuedPlay();
    
    std::string toSGFString();
    
  private:
    Tree *parent;
    std::list<Tree*> *children;
    Tree *symmetryprimary;
    
    Go::Move move;
    float playouts,raveplayouts,priorplayouts;
    float wins,ravewins,priorwins;
    Parameters *params;
    bool hasTerminalWinrate;
    bool terminaloverride;
    bool pruned;
    unsigned int prunedchildren;
    float prunefactor;
    int lastunprune,unprunenextchildat;
    float unprunebase;
    
    void passPlayoutUp(bool win, Tree *source);
    
    void unPruneNextChild();
    int unPruneMetric();
    void updateUnPruneAt();
    
    static float variance(int wins, int playouts);
};
#endif
