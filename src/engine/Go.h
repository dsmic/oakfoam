#ifndef DEF_OAKFOAM_GO_H
#define DEF_OAKFOAM_GO_H

#include <string>
#include <list>
#include <boost/pool/pool.hpp>
#include <boost/pool/pool_alloc.hpp>
#include <boost/pool/object_pool.hpp>
#include <boost/cstdint.hpp>
//from "Features.h":
class Features;
//from "Parameters.h":
class Parameters;
//from "Random.h":
class Random;

#include <set>
#define SYMMETRY_ONLYDEGRAGE false

#define P_N (-size-1)
#define P_S (size+1)
#define P_W (-1)
#define P_E (1)
#define P_NW (P_N+P_W)
#define P_NE (P_N+P_E)
#define P_SW (P_S+P_W)
#define P_SE (P_S+P_E)

#define foreach_adjacent(__pos, __adjpos, __body) \
  { \
    int __intpos = __pos; \
    int __adjpos; \
    __adjpos = __intpos+P_N; { __body }; \
    __adjpos = __intpos+P_S; { __body }; \
    __adjpos = __intpos+P_E; { __body }; \
    __adjpos = __intpos+P_W; { __body }; \
  }

#define foreach_onandadj(__pos, __adjpos, __body) \
  { \
    int __intpos = __pos; \
    int __adjpos; \
    __adjpos = __intpos; { __body }; \
    __adjpos = __intpos+P_N; { __body }; \
    __adjpos = __intpos+P_S; { __body }; \
    __adjpos = __intpos+P_E; { __body }; \
    __adjpos = __intpos+P_W; { __body }; \
  }

#define foreach_adjdiag(__pos, __adjpos, __body) \
  { \
    int __intpos = __pos; \
    int __adjpos; \
    __adjpos = __intpos+P_N; { __body }; \
    __adjpos = __intpos+P_S; { __body }; \
    __adjpos = __intpos+P_E; { __body }; \
    __adjpos = __intpos+P_W; { __body }; \
    __adjpos = __intpos+P_NE; { __body }; \
    __adjpos = __intpos+P_SE; { __body }; \
    __adjpos = __intpos+P_NW; { __body }; \
    __adjpos = __intpos+P_SW; { __body }; \
  }

#define foreach_diagonal(__pos, __adjpos, __body) \
  { \
    int __intpos = __pos; \
    int __adjpos; \
    __adjpos = __intpos+P_NE; { __body }; \
    __adjpos = __intpos+P_SE; { __body }; \
    __adjpos = __intpos+P_NW; { __body }; \
    __adjpos = __intpos+P_SW; { __body }; \
  }

/** Go-related objects. */
namespace Go
{
  /** The memory allocator used for int's. */
  typedef std::allocator<int> allocator_int;
  //typedef boost::fast_pool_allocator<int> allocator_int;
  class Group;
  /** The memory allocator used for Go::Group pointers. */
  typedef std::allocator<Go::Group*> allocator_groupptr;
  //typedef boost::fast_pool_allocator<Go::Group*> allocator_groupptr;
  /** A list of int's, using the specified memory allocator. */
  typedef std::list<int,Go::allocator_int> list_int;

  /** Go colors. */
  enum Color
  {
    EMPTY,
    BLACK,
    WHITE,
    OFFBOARD
  };
  
  /** Get the other Go color.
   * Black is converted to white and vica versa.
   */
  inline static Go::Color otherColor(Go::Color col)
  {
    switch (col)
    {
      case Go::BLACK:
        return Go::WHITE;
      case Go::WHITE:
        return Go::BLACK;
      default:
        return col;
    }
  };
  
  /** Get a char representation of given color. */
  inline static char colorToChar(Go::Color col)
  {
    switch (col)
    {
      case Go::BLACK:
        return 'B';
      case Go::WHITE:
        return 'W';
      case Go::EMPTY:
        return '.';
      default:
        return '-';
    }
  };
  
  /** Get the manhattan distance between two points. */
  inline static int rectDist(int x1, int y1, int x2, int y2)
  {
    int dx=abs(x1-x2);
    int dy=abs(y1-y2);
    return dx+dy;
  };

  /** Get the circular distance between two points. */
  inline static int circDist(int x1, int y1, int x2, int y2)
  {
    int dx=abs(x1-x2);
    int dy=abs(y1-y2);
    if (dx>dy)
      return dx+dy+dx;
    else
      return dx+dy+dy;
  };
  
  /** Go-related Exceptions.
   * For exceptions such as an illegal move being attempted.
   */
  class Exception
  {
    public:
      /** Create an exception instance with given message. */
      Exception(std::string m = "undefined") : message(m) {}
      /** Get the exception message. */
      std::string msg() {return message;}
    
    private:
      std::string message;
  };
  
  /** Go positions.
   * A position is an intersection on a Go Board, represented with an int.
   * This class is used to manipulate these positions.
   */
  class Position
  {
    public:
      /** Convert x-y coordinates to a position. */
      static inline int xy2pos(int x, int y, int boardsize) { return 1+x+(y+1)*(boardsize+1); };
      /** Convert a postion to an x coordinate. */
      static inline int pos2x(int pos, int boardsize) { return (pos-1)%(boardsize+1); };
      /** Convert a postion to an y coordinate. */
      static inline int pos2y(int pos, int boardsize) { return (pos-1)/(boardsize+1)-1; };
      /** Get the string representation of a position. */
      static std::string pos2string(int pos, int boardsize);
      /** Parse a string representation of a poistion. */
      static int string2pos(std::string str, int boardsize);
  };
  
  /** Board with a bit for each position. */
  class BitBoard
  {
    public:
      /** Create an instance with given board size. */
      BitBoard(int s);
      ~BitBoard();
      
      /** Get the value of a position. */
      inline bool get(int pos) const { return data[pos]; };
      /** Set the value of a position. */
      inline void set(int pos, bool val=true) { data[pos]=val; };
      /** Clear a position.
       * Set the position to false.
       */
      inline void clear(int pos) { this->set(pos,false); };
      /** Fill the whole board with a given value. */
      inline void fill(bool val) { for (int i=0;i<sizedata;i++) data[i]=val; };
      /** Clear the whole board. */
      inline void clear() { this->fill(false); };

      /** Create a copy of this board. */
      Go::BitBoard *copy() const;
      
    private:
      const int size,sizesq,sizedata;
      bool *const data;
  };
  
  /** Board with an object for each position. */
  template<typename T>
  class ObjectBoard
  {
    public:
      /** Create a board with the given size. */
      ObjectBoard(int s)
        : size(s),
          sizesq(s*s),
          sizedata(1+(s+1)*(s+2)),
          data(new T[sizedata])
      {};
      ~ObjectBoard()
      {
        delete[] data;
      };
      
      /** Get the value of a position. */
      inline T get(int pos) const { return data[pos]; };
      /** Set the value of a position. */
      inline void set(int pos, const T val) { data[pos]=val; };
      /** Fill the whole board with a set value. */
      inline void fill(T val) { for (int i=0;i<sizedata;i++) data[i]=val; };
    
    private:
      const int size,sizesq,sizedata;
      T *const data;
  };
  
  /** Go move.
   * This represents a move on a Go board.
   */
  class Move
  {
    public:
      /** The type of Go move. */
      enum Type
      {
        NORMAL,
        PASS,
        RESIGN
      };
      
      /** Create an empty move. */
      inline Move() : color(Go::EMPTY), pos(-2), useforlgrf(false) {};
      /** Create a move for given color and position. */
      inline Move(Go::Color col, int p) : color(col), pos(p), useforlgrf(false) {};
      /** Create a move for given color and x-y coordinates. */
      inline Move(Go::Color col, int x, int y, int boardsize) : color(col), pos(x<0?x:Go::Position::xy2pos(x,y,boardsize)) {};
      /** Create a move for given color and type. */
      inline Move(Go::Color col, Go::Move::Type type) : color(col), pos((type==PASS)?-1:-2)
      {
        if (type==NORMAL)
          throw Go::Exception("invalid type");
      };
      
      /** Get the color of this move. */
      inline Go::Color getColor() const {return color;};
      /** Get the position of this move. */
      inline int getPosition() const {return pos;};
      /** Get the x-coordinate of this move. */
      inline int getX(int boardsize) const {return (this->isPass()||this->isResign()?pos:Go::Position::pos2x(pos,boardsize));};
      /** Get the y-coordinate of this move. */
      inline int getY(int boardsize) const {return (this->isPass()||this->isResign()?pos:Go::Position::pos2y(pos,boardsize));};
      /** Set the position of this move. */
      void setPosition(int p) {pos=p;};
      
      /** Determine if this move is a pass. */
      inline bool isPass() const {return (pos==-1);};
      /** Determine if this move is a resign. */
      inline bool isResign() const {return (pos==-2);};
      /** Determine if this move is a normal move. */
      inline bool isNormal() const {return (pos!=-1) && (pos!=-2);};
      
      /** Get the string representation of this move. */
      std::string toString(int boardsize) const;
      
      /** Determine equality with another move. */
      inline bool operator==(const Go::Move other) const { return (color==other.getColor() && pos==other.getPosition()); };
      /** Determine inequality with another move. */
      inline bool operator!=(const Go::Move other) const { return !(*this == other); };
      inline void set_useforlgrf(bool u) {useforlgrf=u;}
      inline bool is_useforlgrf() {return useforlgrf;}
      
    private:
      Go::Color color;
      int pos;
      bool useforlgrf;
  };
  
  /** Zobrist hash type.
   * 64-bits should be good enough to avoid most false positives.
   */
  typedef boost::uint_fast64_t ZobristHash;
  
  /** Table of Zobrist hashes for each stone. */
  class ZobristTable
  {
    public:
      /** Create a table of given size, with given parameters and seed. */
      ZobristTable(Parameters *prms, int sz, unsigned long seed);
      ~ZobristTable();
      
      /** Get the size of the table. */
      int getSize() const { return size; };
      /** Get the hash for a stone, represented by a color and position. */
      Go::ZobristHash getHash(Go::Color col, int pos) const;
    
    private:
      Parameters *const params;
      const int size,sizedata;
      Random *const rand;
      Go::ZobristHash *const blackhashes,*const whitehashes;
      
      Go::ZobristHash getRandomHash();
  };
  
  /** Binary search tree for storing Zobrist hashes. */
  class ZobristTree
  {
    public:
      ZobristTree();
      ~ZobristTree();
      
      /** Add a hash to the tree. */
      void addHash(Go::ZobristHash hash);
      /** Determine if a hash is present in the tree. */
      bool hasHash(Go::ZobristHash hash) const;
    
    private:
      class Node
      {
        public:
          Node(Go::ZobristHash h);
          ~Node();
          
          Go::ZobristHash getHash() const { return hash; };
          void add(Go::ZobristHash h);
          Go::ZobristTree::Node *find(Go::ZobristHash h) const;
        
        private:
          const Go::ZobristHash hash;
          Go::ZobristTree::Node *left,*right;
      };
      
      Go::ZobristTree::Node *const tree;
  };
  
  /** Map of potential territory. */
  class TerritoryMap
  {
    public:
      /** Create an instance of given board size. */
      TerritoryMap(int sz);
      ~TerritoryMap();

      /** Increment the number of board that have been incorporated into these stats. */
      void incrementBoards() { boards++; };
      /** Add an owner for a specific position. */
      void addPositionOwner(int pos, Go::Color col);
      /** Get the owner for a specific position. */
      float getPositionOwner(int pos) const;
      /** Decay the statistics. */
      void decay(float factor);
      
    private:
      const int size, sizedata;
      float boards;
      ObjectBoard<float> *blackowns,*whiteowns;
  };
  
  class Group;
  
  /** Vertex on a Go board. */
  class Vertex
  {
    public:
      /** Color of the vertex. */
      Go::Color color;
      /** Group associated with this vertex. */
      Go::Group *group;
  };
  
  class Board;
  
  /** Group on a Go board.
   * These are disjoint sets.
   */
  class Group
  {
    public:
      /** Create a Group instance for given board, with given key stone position. */
      Group(Go::Board *brd, int pos);
      ~Group() {};
      
      /** Get the color of this group. */
      Go::Color getColor() const {return color;};
      /** Get the position of the key stone of this position. */
      int getPosition() const {return position;};
     
      /** Set the parent group for this group. */
      void setParent(Go::Group *p) { parent=p; };
      /** Get the root group for this group. */
      Go::Group *find() const
      {
        if (parent==NULL)
          return (Go::Group *)this;
        else
          //return (parent=parent->find());
          return parent->find();
      };
      /** Merge two groups.
       * Assumed that both groups are roots.
       */
      void unionWith(Go::Group *othergroup)
      {
        othergroup->setParent(this);
        stonescount+=othergroup->stonescount;
        pseudoliberties+=othergroup->pseudoliberties;
        pseudoends+=othergroup->pseudoends;
        pseudoborderdist+=othergroup->pseudoborderdist;
        libpossum+=othergroup->libpossum;
        libpossumsq+=othergroup->libpossumsq;
        adjacentgroups.splice(adjacentgroups.end(),*othergroup->getAdjacentGroups());
      };
      /** Determine if this group is a root. */
      inline bool isRoot() const { return (parent==NULL); };
      
      /** Get the number of stones in this group. */
      inline int numOfStones() const { return stonescount; };
      /** Get the number of pseudo liberties for this group. */
      inline int numOfPseudoLiberties() const { return pseudoliberties; };
      inline int numOfPseudoEnds() const { return pseudoends; };
      inline int numOfPseudoBorderDist() const { return pseudoborderdist; };
      
      /** Add a pseudo liberty to this group. */
      inline void addPseudoLiberty(int pos) { pseudoliberties++; libpossum+=pos; libpossumsq+=pos*pos; };
      inline void addPseudoEnd() { pseudoends++;};
      inline void addPseudoBorderDist(int dist) { pseudoborderdist+=dist;};
      /** Remove a pseudo liberty from this group. */
      inline void removePseudoLiberty(int pos) { pseudoliberties--; libpossum-=pos; libpossumsq-=pos*pos; };
      inline void removePseudoEnd() { pseudoends--;};
      /** Determine if this group is in atari. */
      inline bool inAtari() const { return (pseudoliberties>0 && (pseudoliberties*libpossumsq)==(libpossum*libpossum)); };
      /** Get the last remaining postion of a group in atari.
       * If the group is not in atari, -1 is returned.
       */
      inline int getAtariPosition() const { if (this->inAtari()) return libpossum/pseudoliberties; else return -1; };
      /** Add the orthogonally adjacent empty pseudo liberties. */
      void addTouchingEmpties();
      /** Determine if given position is one of the last two liberties. */
      bool isOneOfTwoLiberties(int pos) const;
      /** Get the other of the last two liberties.
       * If the group doesn't have two liberties, -1 is returned.
       */
      int getOtherOneOfTwoLiberties(int pos) const;
      /** Get a list of the adjacent groups to this group. */
      std::list<int,Go::allocator_int> *getAdjacentGroups() { return &adjacentgroups; };
    
    private:
      Go::Board *const board;
      const Go::Color color;
      const int position;
      
      int stonescount;
      Go::Group *parent;
      
      int pseudoliberties;
      int pseudoends;
      int pseudoborderdist;
      int libpossum;
      int libpossumsq;
      
      std::list<int,Go::allocator_int> adjacentgroups;
  };
  
  /** Go board. */
  class Board
  {
    public:
      /** Symmetry types for a board. */
      enum Symmetry
      {
        NONE,
        VERTICAL,
        HORIZONTAL,
        DIAGONAL_DOWN,
        DIAGONAL_UP,
        DIAGONAL_BOTH,
        VERTICAL_HORIZONTAL,
        FULL
      };
      /** Transform for flipping the board through a line of symmetry. */
      struct SymmetryTransform
      {
        bool invertX; /**< Should the x coordinate be inverted? */
        bool invertY; /**< Should the y coordinate be inverted? */
        bool swapXY; /**< Should the x and y coordinates be swapped? */
      };
      
      /** Create a board of given size. */
      Board(int s);
      ~Board();
      
      /** Create a copy of this board. */
      Go::Board *copy() const;
      /** Copy this board over the given board. */
      void copyOver(Go::Board *copyboard) const;
      /** Get a string representation of this board. */
      std::string toString() const;
      /** Get a SGF representation of this board. */
      std::string toSGFString() const;
      /** Get the raw board data.
       * This should only be used for very special purposes, and usually in a read-only manner.
       */
      const Go::Vertex *boardData() const { return data; };
      /** Get a list of the groups on this board. */
      std::set<Go::Group*> *getGroups() { return &groups; };
      
      /** Get the size of this board. */
      int getSize() const { return size; };
      /** Get the position of a simple ko.
       * If there is no current simple ko, return -1.
       */
      int getSimpleKo() const { return simpleko; };
      /** Determine if there is currently a simple ko. */
      bool isCurrentSimpleKo() const { return (simpleko!=-1); };
      /** Get the number of moves made on this board. */
      int getMovesMade() const { return movesmade; };
      /** Get the number of consecutive passes that have been played before now. */
      int getPassesPlayed() const { return passesplayed; };
      /** Reset the passes that have been played.
       * Used to continue play after two passes.
       */
      void resetPassesPlayed() { passesplayed=0; };
      /** Get the next color to make a move. */
      Go::Color nextToMove() const { return nexttomove; };
      /** Set the next color to make a move. */
      void setNextToMove(Go::Color col) { nexttomove=col; }; //clear ko?
      /** Get the maximum position value for this board size. */
      int getPositionMax() const { return sizedata; };
      /** Get the last move made on this board. */
      Go::Move getLastMove() const { return lastmove; };
      /** Get the second-to-last move made on this board. */
      Go::Move getSecondLastMove() const { return secondlastmove; };
      /** Get the number of prisoners for a given color. */
      int getStoneCapturesOf(Go::Color col) const { return (col==Go::BLACK?blackcaptures:whitecaptures); };
      /** Reset the number of prisoners.
       * Used to make the mercy rule more robust.
       */
      void resetCaptures() { blackcaptures=0; whitecaptures=0; };
      /** Determine if the last move captured a group. */
      bool isLastCapture() { return lastcapture; };
      
      /** Get the color of the given position. */
      inline Go::Color getColor(int pos) const { return data[pos].color; };
      /** Get the group at the given position.
       * It is assumed there is a group at the position.
       */
      inline Go::Group *getGroup(int pos) const { return data[pos].group->find(); };
      /** Determine if the given position is part or a group. */
      inline bool inGroup(int pos) const { return (data[pos].group!=NULL); };
      /** Determine if the given position is actually on the board. */
      inline bool onBoard(int pos) const { return (data[pos].color!=Go::OFFBOARD); };
      
      /** Make a move on this board. */
      void makeMove(Go::Move move);
      /** Determine is the given move is a legal move. */
      bool validMove(Go::Move move) const;
      
      /** Get the number of legal moves currently available. */
      int numOfValidMoves(Go::Color col) const { return (col==Go::BLACK?blackvalidmovecount:whitevalidmovecount); };
      /** Get a board showing which moves are legal. */
      Go::BitBoard *getValidMoves(Go::Color col) const { return (col==Go::BLACK?blackvalidmoves:whitevalidmoves); };
      
      /** Compute the current score. */
      float score(Parameters* params=NULL);
      /** Determine if the given position is a weak eye for the given color. */
      bool weakEye(Go::Color col, int pos) const;
      /** Determine if the given position has an eye where two weak eyes are shared from two groups. */
      bool twoGroupEye(Go::Color col, int pos) const;
      /** Determine if the given position is surrounded by a single group of the given color. */
      bool strongEye(Go::Color col, int pos) const;
      /** Get the number of empty positions orthogonally adjacent to the given position. */
      int touchingEmpty(int pos) const;
      int diagonalEmpty(int pos) const;
      /** Get the number of empty positions in the eight positions surrounding the given position. */
      int surroundingEmpty(int pos) const;
      /** Get the number of each color in the orthogonally adjacent positions to the given position. */
      void countAdjacentColors(int pos, int &empty, int &black, int &white, int &offboard) const;
      /** Get the number of each color in the diagonally adjacent positions to the given position. */
      void countDiagonalColors(int pos, int &empty, int &black, int &white, int &offboard) const;
      /** Turn symmetry calculations off for this board. */
      void turnSymmetryOff() { symmetryupdated=false;currentsymmetry=NONE; };
      /** Turn symmetry calculations on for this board. */
      void turnSymmetryOn() { symmetryupdated=true;currentsymmetry=this->computeSymmetry(); };
      
      /** Determine if the given score is a win for the given color. */
      static bool isWinForColor(Go::Color col, float score);

      /** Compute the symmetry for this board. */
      Go::Board::Symmetry computeSymmetry();
      /** Get the computed symmetry for this board. */
      Go::Board::Symmetry getSymmetry() const { return currentsymmetry; };
      /** Get the string representation for the given symmetry. */
      std::string getSymmetryString(Go::Board::Symmetry sym) const;
      /** Transform a position to a primary position, given a symmetry. */
      int doSymmetryTransformToPrimary(Go::Board::Symmetry sym, int pos);
      /** Get the transform used to transform a position to its primary for a given symmetry. */
      Go::Board::SymmetryTransform getSymmetryTransformToPrimary(Go::Board::Symmetry sym, int pos) const;
      /** Get the transform used to transform a position to its primary for a given symmetry. */
      static Go::Board::SymmetryTransform getSymmetryTransformToPrimaryStatic(int size, Go::Board::Symmetry sym, int pos);
      /** Transform a position using the given symmetry transform. */
      int doSymmetryTransform(Go::Board::SymmetryTransform trans, int pos, bool reverse=false);
      /** Transform a position using the given symmetry transform. */
      static int doSymmetryTransformStatic(Go::Board::SymmetryTransform trans, int size, int pos);
      /** Reverse-transform a position using the given symmetry transform. */
      static int doSymmetryTransformStaticReverse(Go::Board::SymmetryTransform trans, int size, int pos);
      /** Get the transform to transform between two given positions. */
      static Go::Board::SymmetryTransform getSymmetryTransformBetweenPositions(int size, int pos1, int pos2);
      /** Get a list of applicable symmetry transforms for a given symmetry. */
      std::list<Go::Board::SymmetryTransform> getSymmetryTransformsFromPrimary(Go::Board::Symmetry sym) const;
      /** Get a list of applicable symmetry transforms for a given symmetry. */
      static std::list<Go::Board::SymmetryTransform> getSymmetryTransformsFromPrimaryStatic(Go::Board::Symmetry sym);
      
      /** Determine is the given move is a capture. */
      bool isCapture(Go::Move move) const;
      /** Determine is the given move is an extension. */
      bool isExtension(Go::Move move) const;
      /** Determine is the given move is a self-atari. */
      bool isSelfAtari(Go::Move move) const;
      /** Determine is the given move is a self-atari of a group of a minimum size. */
      bool isSelfAtariOfSize(Go::Move move, int minsize=0, bool complex=false) const;
      /** Determine is the given move is an atari. */
      bool isAtari(Go::Move move) const;
      /** Get the distance from the given position to the board edge. */
      int getDistanceToBorder(int pos) const;
      /** Get the manhattan distance between two positions. */
      int getRectDistance(int pos1, int pos2) const;
      int getPseudoDistanceToBorder(int pos) const;
      /** Get the circular distance between two positions. */
      int getCircularDistance(int pos1, int pos2) const;
      /** Get the CFG distances from the given position. */
      Go::ObjectBoard<int> *getCFGFrom(int pos, int max=0) const;
      /** Get the center liberty of a group of three.
       * Used for the nakade heuristic.
       * If there is not such position, -1 is returned.
       */
      int getThreeEmptyGroupCenterFrom(int pos) const;
      int getBent4EmptyGroupCenterFrom(int pos,bool onlycheck=false) const;
      int getFourEmptyGroupCenterFrom(int pos) const;
      int getFiveEmptyGroupCenterFrom(int pos) const;
      
      /** Compute the Zobrist hash for this board. */
      Go::ZobristHash getZobristHash(Go::ZobristTable *table) const;
      
      /** Get the owner of given position, determined by the last score computed. */
      Go::Color getScoredOwner(int pos) const;
      /** Update the given territory map. */
      void updateTerritoryMap(Go::TerritoryMap *tmap) const;
      /** Determine if a stone is alive, according to the given territory map and threshold. */
      bool isAlive(Go::TerritoryMap *tmap, float threshold, int pos) const;
      /** Get the territory score using the given territory map and threshold. */
      int territoryScore(Go::TerritoryMap *tmap, float threshold);

      /** Determine if there is a ladder for the group.
       * A ladder is only detected if the group in question is currently in atari.
       */
      bool isLadder(Go::Group *group) const;
      /** Determine if there is a ladder for the group after the move is made. */
      bool isLadderAfter(Go::Group *group, Go::Move move) const;
      /** Determine if the ladder works or is broken in most cases. */
      bool isProbableWorkingLadder(Go::Group *group) const;
      /** Determine if the ladder that exists after the move is made works or is broken in most cases. */
      bool isProbableWorkingLadderAfter(Go::Group *group, Go::Move move) const;
   
    private:
      const int size;
      const int sizesq;
      const int sizedata;
      Go::Vertex *const data;
      std::set<Go::Group*> groups;
      int movesmade,passesplayed;
      Go::Color nexttomove;
      int simpleko;
      Go::Move lastmove,secondlastmove;
      bool symmetryupdated;
      Go::Board::Symmetry currentsymmetry;
      int blackvalidmovecount,whitevalidmovecount;
      Go::BitBoard *blackvalidmoves,*whitevalidmoves;
      boost::object_pool<Go::Group> pool_group;
      bool markchanges;
      Go::BitBoard *lastchanges;
      int blackcaptures,whitecaptures;
      bool lastcapture;
      
      struct ScoreVertex
      {
        bool touched;
        Go::Color color;
      };
      Go::Board::ScoreVertex *lastscoredata;
      
      inline Go::Group *getGroupWithoutFind(int pos) const { return data[pos].group; };
      inline void setColor(int pos, Go::Color col) { data[pos].color=col; if (markchanges) { lastchanges->set(pos); } };
      inline void setGroup(int pos, Go::Group *grp) { data[pos].group=grp; };
      inline int getPseudoLiberties(int pos) const { if (data[pos].group==NULL) return 0; else return data[pos].group->find()->numOfPseudoLiberties(); };
      inline int getGroupSize(int pos) const { if (data[pos].group==NULL) return 0; else return data[pos].group->find()->numOfStones(); };
      
      bool touchingAtLeastOneEmpty(int pos) const;
      
      void refreshGroups();
      void spreadGroup(int pos, Go::Group *group);
      int removeGroup(Go::Group *group);
      void spreadRemoveStones(Go::Color col, int pos, std::list<int,Go::allocator_int> *possiblesuicides);
      void mergeGroups(Go::Group *first, Go::Group *second);
      
      bool validMoveCheck(Go::Move move) const;
      void refreshValidMoves();
      void refreshValidMoves(Go::Color col);
      void addValidMove(Go::Move move);
      void removeValidMove(Go::Move move);
      
      bool hasSymmetryVertical() const;
      bool hasSymmetryHorizontal() const;
      bool hasSymmetryDiagonalDown() const;
      bool hasSymmetryDiagonalUp() const;
      void updateSymmetry();
      int doSymmetryTransformPrimitive(Go::Board::Symmetry sym, int pos) const;
      
      void spreadScore(Go::Board::ScoreVertex *scoredata, int pos, Go::Color col);
      bool isProbableWorkingLadder(Go::Group *group, int posA, int movepos=-1) const;
  };
};

#endif
