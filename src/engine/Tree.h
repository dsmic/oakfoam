#ifndef DEF_OAKFOAM_TREE_H
#define DEF_OAKFOAM_TREE_H

#define TREE_TERMINAL_URGENCY 100
// must be greater than 1+max(bias)

#include "config.h"
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

/** MCTS Tree. */
class Tree
{
  public:
    /** Create a Tree instance.
     * @param prms  Parameters to use.
     * @param h     Zobrist hash of this position, if available.
     * @param mov   Move that creates this position.
     * @param p     Parent tree node.
     */
    Tree(Parameters *prms, Go::ZobristHash h, Go::Move mov = Go::Move(Go::EMPTY,Go::Move::RESIGN), Tree *p = NULL);
    ~Tree();
    
    /** Get the parent tree node. */
    Tree *getParent() const { return parent; };
    /** Get a list of the children of this tree. */
    std::list<Tree*> *getChildren() const { return children; };
    /** Get this node's move. */
    Go::Move getMove() const { return move; };
    /** Determine if this node is the root of a tree. */
    bool isRoot() const { return (parent==NULL); };
    /** Determine if this node is the leaf of a tree. */
    bool isLeaf() const { return !beenexpanded; };
    /** Determine if this node is a terminal node.
     * Terminal nodes are nodes where the minimax result is known with certainty.
     */
    bool isTerminal() const;
    /** Determine if this node is a terminal win. */
    bool isTerminalWin() const { return (this->isTerminalResult() && hasTerminalWin); };
    /** Determine if this node is a terminal lose. */
    bool isTerminalLose() const { return (this->isTerminalResult() && !hasTerminalWin); };
    /** Determine if this node is a terminal win or lose.
     * A node can be a terminal node and not yet have a result, such as after two passes.
     */
    bool isTerminalResult() const { return hasTerminalWinrate; };
    /** Get a list of moves from the root of this tree to this node. */
    std::list<Go::Move> getMovesFromRoot() const;
    /** Remove a child from this tree.
     * This allows the divorced child to become the new tree root, and the rest of the tree freed.
     */
    void divorceChild(Tree *child);
    /** Determine if this is the primary symmetrical transform for this tree level. */
    bool isPrimary() const { return (symmetryprimary==NULL); };
    /** Get the primary symmetrical transform for this tree level. */
    Tree *getPrimary() const { return symmetryprimary; };
    /** Set the primary symmetrical transform for this tree level. */
    void setPrimary(Tree *p) { symmetryprimary=p; };
    /** Transform this tree so that this node is now a primary node. */
    void performSymmetryTransformParentPrimary();
    /** Tranform this tree in the given manner. */
    void performSymmetryTransform(Go::Board::SymmetryTransform trans);
    
    /** Determine if this node has been soft-pruned in the tree.
     * Nodes can be soft-pruned due to progressive widening or superko violations.
     * Soft-pruned node should not be considered for MCTS purposes.
     */
    bool isPruned() const { return pruned; };
    /** Set the pruned status of this node. */
    void setPruned(bool p) { pruned=p; };
    /** Prune all the children of this node. */
    void pruneChildren();
    Tree *getWorstChild();
    /** Check if a child node should be unpruned, due to progressive widening. */
    void checkForUnPruning();
    /** Unprune a new node now, irrespective of the progrssive widening status. */
    void unPruneNow();
    /** Determnie if this node has children that can be unpruned for progressive widening. */
    bool hasPrunedChildren() const { return (prunedchildren-superkochildrenviolations)>0; };
    
    /** Set the feature gamma value for this node. */
    void setFeatureGamma(float g);
    /** Get the feature gamma value for this node. */
    float getFeatureGamma() const { return gamma; };
    /** Get the sum of the children's gamma values. */
    float getChildrenTotalFeatureGamma() const { return childrentotalgamma; };
    /** Get the largest of the children's gamma values. */
    float getMaxChildFeatureGamma() const { return maxchildgamma; };
    
    /** Compute the progressive bias value for this node. */
    float getProgressiveBias() const;
    /** Set the static progressive bias bonus.
     * This is used to promote pass moves when winning by far.
     */
    void setProgressiveBiasBonus(float b) { biasbonus=b; };
    
    /** Get the child of this node specified by @p move. */
    Tree *getChild(Go::Move move) const;
    /** Get the number of playouts through this node. */
    float getPlayouts() const { return playouts; };
    /** Get the number of wins through this node, for this node's color. */
    float getWins() const { return wins; };
    /** Get the number of RAVE playouts through this node. */
    float getRAVEPlayouts() const { return raveplayouts; };
    float getRAVEWins() const { return ravewins; };
    float getEARLYRAVEPlayouts() const { return earlyraveplayouts; };
    /** Get the number of RAVE playouts for the other color through this node. */
    float getRAVEPlayoutsOther() const { return raveplayoutsother; };
    float getRAVEWinsOther() const { return ravewinsother; };
    float getRAVEPlayoutsOtherEarly() const { return earlyraveplayoutsother; };
    float getRAVEWinsOtherEarly() const { return earlyravewinsother; };
    /** Get the ratio of wins to playouts. */
    float getRatio() const;
    /** Get the unprune factor, used for determining the order to unprune nodes in. */
    float getUnPruneFactor(float *moveValues=NULL, float mean=0, int num=0) const;
    /** Get the score mean. */
    float getScoreMean() const;
    /** Get the score standard deviation. */
    float getScoreSD() const;

    /** Get the ratio of RAVE wins to playouts. */
    float getRAVERatio() const;
    float getEARLYRAVERatio() const;
    /** Get the ratio of RAVE wins to playouts for the other color. */
    float getRAVERatioOther() const;
    /** Get the ratio of RAVE wins to playouts used for poolRAVE (excluding any initial wins). */
    float getRAVERatioForPool() const;
    /** Get the ratio of RAVE wins to playouts for the other color used for poolRAVE (excluding any initial wins). */
    float getRAVERatioOtherForPool() const;
    /** Get the value for this node.
     * This is a combination of normal and RAVE values.
     */
    float getVal(bool skiprave=false) const;
    /** Get the urgency for this node.
     * This is the node value combined with a biases.
     * Biases can be from UCB, progressive bias or criticality bias.
     */
    float getUrgency(bool skiprave=false) const;
    
    /** Add a child to this node. */
    void addChild(Tree *node);
    /** Add a win to this node.
     * @param fscore The final score of the playout.
     * @param source The child that this result is coming from.
     */
    void addWin(int fscore, Tree *source=NULL);
    /** Add a loss to this node.
     * @param fscore The final score of the playout.
     * @param source The child that this result is coming from.
     */
    void addLose(int fscore, Tree *source=NULL);
    /** Add a virtual loss. */
    void addVirtualLoss();
    /** Remove a virtual loss from this node and the path up to the root. */
    void removeVirtualLoss();
    /** Add a number of prior wins to this node. */
    void addPriorWins(int n);
    /** Add a number of prior losses to this node. */
    void addPriorLoses(int n);
    /** Add a RAVE win to this node. */
    void addRAVEWin(bool early);
    /** Add a RAVE loss to this node. */
    void addRAVELose(bool early);
    /** Add a RAVE win for the other color to this node. */
    void addRAVEWinOther(bool early);
    /** Add a RAVE loss for the other color to this node. */
    void addRAVELoseOther(bool early);
    
    /** Add a number of RAVE wins to this node. */
    void addRAVEWins(int n,bool early);
    void presetRave(float ravew,float ravep);
    void presetRaveEarly(float ravew,float ravep);
    /** Add a number of RAVE losses to this node. */
    void addRAVELoses(int n,bool early);
    /** Add a partial result to this node.
     * A partial result is used for non-integer result rewards.
     */
    void addPartialResult(float win, float playout, bool invertwin=true);
    /** Add a decaying result. */
    void addDecayResult(float result);
    
    /** Expand this leaf node.
     * Returns true if the node has been expanded or false otherwise (only applicable in multi-core situations).
     */
    bool expandLeaf(Worker::Settings *settings);
    /** Get this robust child of this node.
     * The robust child is the child with the most playouts through it.
     * @param descend If set, descend down the tree to a leaf node, picking the robust child at each node.
     */
    Tree *getRobustChild(bool descend=false) const;
    /** Get the second most robust child.
     * @param firstchild Assume this node is the most robust.
     */
    Tree *getSecondRobustChild(const Tree *firstchild=NULL) const;
    /** Get the urgent child of this node.
     * The urgent child is the child with the largest urgency.
     */
    Tree *getUrgentChild(Worker::Settings *settings);
    /** Get the child with the best ratio. */
    Tree *getBestRatioChild(float playoutthreshold=0) const;
    /** Update RAVE values for the path from this node to the root of the tree. */
    void updateRAVE(Go::Color wincol,Go::BitBoard *blacklist,Go::BitBoard *whitelist,bool early);
    /** Prune any superko violations. */
    void pruneSuperkoViolations();
    
    /** Reset this node to an initial state. */
    void resetNode();
    /** Allow continued play from this node.
     * Used when play continues after two passes.
     */
    void allowContinuedPlay();
    
    /** Get the number of playouts of the sibling with the second most.
     * It is assumed that this is the node with the most.
     */
    float secondBestPlayouts() const;
    /** Get the ratio of playouts between this node and the sibling with the second most.
     * It is assumed that this is the node with the most.
     */
    float secondBestPlayoutRatio() const;
    /** Get the difference in ratio between this node and its best child. */
    float bestChildRatioDiff() const;
    
    /** Get the Zobrist hash for this position. */
    Go::ZobristHash getHash() const { return hash; };
    /** Set the Zobrist hash for this position. */
    void setHash(Go::ZobristHash h) { hash=h; };
    /** Determine if this node, or any of the nodes in the path to the root, have the same hash as @p h. */
    bool isSuperkoViolationWith(Go::ZobristHash h) const;
    /** Determine if this node is a superko violation. */
    bool isSuperkoViolation() const { return superkoviolation; };
    /** Determine if this node has been checked for a superko violation. */
    bool isSuperkoChecked() const { return superkochecked; };
    /** Check this node for a superko violation. */
    void doSuperkoCheck();
    
    /** Update the criticality for this node and the path to the root. */
    void updateCriticality(Go::Board *board, Go::Color wincol);
    /** Get the criticality for this node. */
    float getCriticality() const;
    
    /** Get the territory owner statistics for this node. */
    float getTerritoryOwner() const;
    
    /** Get a string representation for this node. */
    std::string toSGFString() const;
    
    /** The order in which this node was unpruned.
     * A zero signifies that this node has not yet been unpruned.
     * The first node to be unpruned will return a value of one.
     */
    int getUnprunedNum() const { return unprunednum; };
    /** Set the unprune order for this node.
     * @see Tree::getUnprunedNum()
     */
    void setUnprunedNum(int num) { unprunednum=num; };
    /** Get the number of unpruned children. */
    unsigned int getNumUnprunedChildren() const { return unprunedchildren; };

    #ifdef HAVE_MPI
      /** Get the difference between now and the last sync for MPI-shared stats. */
      void fetchMpiDiff(float &plts, float &wns);
      /** Add the given MPI-shared stats to this node. */
      void addMpiDiff(float plts, float wns);
      /** Reset and accumulated MPI-shared stats difference. */
      void resetMpiDiff();
    #endif

    std::list<Tree*> *getChildren() {return children;}
    
  private:
    Tree *parent;
    std::list<Tree*> *children;
    bool beenexpanded;
    Tree *symmetryprimary;
    
    Go::Move move;
    float playouts,raveplayouts,earlyraveplayouts;
    float wins,ravewins,earlyravewins;
    float raveplayoutsother;
    float ravewinsother;
    float earlyraveplayoutsother;
    float earlyravewinsother;
    float scoresum,scoresumsq;
    float decayedwins,decayedplayouts;
    Parameters *const params;
    bool hasTerminalWinrate,hasTerminalWin;
    bool terminaloverride;
    bool pruned;
    int unprunednum;
    unsigned int prunedchildren;
    unsigned int unprunedchildren;
    float gamma,childrentotalgamma,maxchildgamma;
    float lastunprune,unprunenextchildat;
    float unprunebase;
    int ownedblack,ownedwhite,ownedwinner;
    float biasbonus;
    bool superkoprunedchildren,superkoviolation,superkochecked;
    int superkochildrenviolations;
    Go::ZobristHash hash;
    boost::mutex expandmutex,updatemutex,unprunemutex,superkomutex;
    
    #ifdef HAVE_MPI
      float mpi_lastplayouts,mpi_lastwins;
    #endif
    
    void passPlayoutUp(int fscore, bool win, Tree *source);
    bool allChildrenTerminalLoses();
    bool hasOneUnprunedChildNotTerminalLoss();
    
    void unPruneNextChild();
    void unPruneNextChildNew();
    float unPruneMetric() const;
    void updateUnPruneAt();
    
    void addCriticalityStats(bool winner, bool black, bool white);
    
    static float variance(int wins, int playouts);
};
#endif
