#ifndef DEF_OAKFOAM_TREE_H
#define DEF_OAKFOAM_TREE_H

#define TREE_TERMINAL_URGENCY 100
// must be greater than 1+max(bias)

#include <list>
#include <string>
#include <boost/thread/mutex.hpp>
#include "Go.h"
//from "Parameters.h":
class Parameters;
//from "Worker.h":
namespace Worker
{
  class Settings;
};

class Tree
{
  public:
    Tree(Parameters *prms, Go::ZobristHash h, Go::Move mov = Go::Move(Go::EMPTY,Go::Move::RESIGN), Tree *p = NULL);
    ~Tree();
    
    Tree *getParent() const { return parent; };
    std::list<Tree*> *getChildren() const { return children; };
    Go::Move getMove() const { return move; };
    bool isRoot() const { return (parent==NULL); };
    bool isLeaf() const { return !beenexpanded; };
    bool isTerminal() const;
    bool isTerminalWin() const { return (this->isTerminalResult() && hasTerminalWin); };
    bool isTerminalLose() const { return (this->isTerminalResult() && !hasTerminalWin); };
    bool isTerminalResult() const { return hasTerminalWinrate; };
    std::list<Go::Move> getMovesFromRoot() const;
    void divorceChild(Tree *child);
    bool isPrimary() const { return (symmetryprimary==NULL); };
    Tree *getPrimary() const { return symmetryprimary; };
    void setPrimary(Tree *p) { symmetryprimary=p; };
    void performSymmetryTransformParentPrimary();
    void performSymmetryTransform(Go::Board::SymmetryTransform trans);
    
    bool isPruned() const { return pruned; };
    void setPruned(bool p) { pruned=p; };
    void pruneChildren();
    void checkForUnPruning();
    void unPruneNow();
    bool hasPrunedChildren() const { return (prunedchildren-superkochildrenviolations)>0; };
    
    void setFeatureGamma(float g);
    float getFeatureGamma() const { return gamma; };
    float getChildrenTotalFeatureGamma() const { return childrentotalgamma; };
    float getMaxChildFeatureGamma() const { return maxchildgamma; };
    
    float getProgressiveBias() const;
    void setProgressiveBiasBonus(float b) { biasbonus=b; };
    
    Tree *getChild(Go::Move move) const;
    float getPlayouts() const { return playouts; };
    float getRAVEPlayouts() const { return raveplayouts; };
    float getPriorPlayouts() const { return priorplayouts; };
    float getRatio() const;
    float getRAVERatio() const;
    float getPriorRatio() const;
    float getBasePriorRatio() const;
    float getVal() const;
    float getUrgency() const;
    
    void addChild(Tree *node);
    void addWin(Tree *source=NULL);
    void addLose(Tree *source=NULL);
    void addVirtualLoss();
    void removeVirtualLoss();
    void addPriorWins(int n);
    void addPriorLoses(int n);
    void addRAVEWin();
    void addRAVELose();
    void addRAVEWins(int n);
    void addRAVELoses(int n);
    void addPartialResult(float win, float playout, bool invertwin=true);
    
    void expandLeaf();
    Tree *getRobustChild(bool descend=false) const;
    Tree *getSecondRobustChild(const Tree *firstchild=NULL) const;
    Tree *getUrgentChild(Worker::Settings *settings);
    Tree *getBestRatioChild(float playoutthreshold=0) const;
    void updateRAVE(Go::Color wincol,Go::BitBoard *blacklist,Go::BitBoard *whitelist);
    void pruneSuperkoViolations();
    
    void resetNode();
    void allowContinuedPlay();
    
    float secondBestPlayouts() const;
    float secondBestPlayoutRatio() const;
    float bestChildRatioDiff() const;
    
    Go::ZobristHash getHash() const { return hash; };
    void setHash(Go::ZobristHash h) { hash=h; };
    bool isSuperkoViolationWith(Go::ZobristHash h) const;
    bool isSuperkoViolation() const { return superkoviolation; };
    bool isSuperkoChecked() const { return superkochecked; };
    void doSuperkoCheck();
    
    void updateCriticality(Go::Board *board, Go::Color wincol);
    float getCriticality() const;
    
    float getTerritoryOwner() const;
    
    std::string toSGFString() const;
    
  private:
    Tree *parent;
    std::list<Tree*> *children;
    bool beenexpanded;
    Tree *symmetryprimary;
    
    Go::Move move;
    float playouts,raveplayouts,priorplayouts;
    float wins,ravewins,priorwins;
    Parameters *const params;
    bool hasTerminalWinrate,hasTerminalWin;
    bool terminaloverride;
    bool pruned;
    unsigned int prunedchildren;
    float gamma,childrentotalgamma,maxchildgamma;
    float lastunprune,unprunenextchildat;
    float unprunebase;
    int ownedblack,ownedwhite,ownedwinner;
    float biasbonus;
    bool superkoprunedchildren,superkoviolation,superkochecked;
    int superkochildrenviolations;
    Go::ZobristHash hash;
    boost::mutex expandmutex,updatemutex,unprunemutex,superkomutex;
    
    void passPlayoutUp(bool win, Tree *source);
    bool allChildrenTerminalLoses();
    bool hasOneUnprunedChildNotTerminalLoss();
    
    void unPruneNextChild();
    float unPruneMetric() const;
    void updateUnPruneAt();
    float getUnPruneFactor() const;
    
    void addCriticalityStats(bool winner, bool black, bool white);
    
    static float variance(int wins, int playouts);
};
#endif
