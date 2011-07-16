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

namespace Go
{
  //typedef std::allocator<int> allocator_int;
  typedef boost::fast_pool_allocator<int> allocator_int;
  class Group;
  //typedef std::allocator<Go::Group*> allocator_groupptr;
  typedef boost::fast_pool_allocator<Go::Group*> allocator_groupptr;
  
  typedef std::list<int,Go::allocator_int> list_int;

  enum Color
  {
    EMPTY,
    BLACK,
    WHITE,
    OFFBOARD
  };
  
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
  
  inline static int circDist(int x1, int y1, int x2, int y2)
  {
    int dx=abs(x1-x2);
    int dy=abs(y1-y2);
    if (dx>dy)
      return dx+dy+dx;
    else
      return dx+dy+dy;
  };
  
  class Exception
  {
    public:
      Exception(std::string m = "undefined") : message(m) {}
      std::string msg() {return message;}
    
    private:
      std::string message;
  };
  
  class Position
  {
    public:
      static inline int xy2pos(int x, int y, int boardsize) { return 1+x+(y+1)*(boardsize+1); };
      static inline int pos2x(int pos, int boardsize) { return (pos-1)%(boardsize+1); };
      static inline int pos2y(int pos, int boardsize) { return (pos-1)/(boardsize+1)-1; };
      static std::string pos2string(int pos, int boardsize);
      static int string2pos(std::string str, int boardsize);
  };
  
  class BitBoard
  {
    public:
      BitBoard(int s);
      ~BitBoard();
      
      inline bool get(int pos) const { return data[pos]; };
      inline void set(int pos, bool val=true) { data[pos]=val; };
      inline void clear(int pos) { this->set(pos,false); };
      inline void fill(bool val) { for (int i=0;i<sizedata;i++) data[i]=val; };
      inline void clear() { this->fill(false); };
    
    private:
      const int size,sizesq,sizedata;
      bool *const data;
  };
  
  template<typename T>
  class ObjectBoard
  {
    public:
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
      
      inline T get(int pos) const { return data[pos]; };
      inline void set(int pos, const T val) { data[pos]=val; };
      inline void fill(T val) { for (int i=0;i<sizedata;i++) data[i]=val; };
    
    private:
      const int size,sizesq,sizedata;
      T *const data;
  };
  
  class Move
  {
    public:
      enum Type
      {
        NORMAL,
        PASS,
        RESIGN
      };
      
      inline Move() : color(Go::EMPTY), pos(-2) {};
      inline Move(Go::Color col, int p) : color(col), pos(p) {};
      inline Move(Go::Color col, int x, int y, int boardsize) : color(col), pos(x<0?x:Go::Position::xy2pos(x,y,boardsize)) {};
      inline Move(Go::Color col, Go::Move::Type type) : color(col), pos((type==PASS)?-1:-2)
      {
        if (type==NORMAL)
          throw Go::Exception("invalid type");
      };
      
      inline Go::Color getColor() const {return color;};
      inline int getPosition() const {return pos;};
      inline int getX(int boardsize) const {return (this->isPass()||this->isResign()?pos:Go::Position::pos2x(pos,boardsize));};
      inline int getY(int boardsize) const {return (this->isPass()||this->isResign()?pos:Go::Position::pos2y(pos,boardsize));};
      void setPosition(int p) {pos=p;};
      
      inline bool isPass() const {return (pos==-1);};
      inline bool isResign() const {return (pos==-2);};
      inline bool isNormal() const {return (pos!=-1) && (pos!=-2);};
      
      std::string toString(int boardsize) const;
      
      inline bool operator==(const Go::Move other) const { return (color==other.getColor() && pos==other.getPosition()); };
      inline bool operator!=(const Go::Move other) const { return !(*this == other); };
    
    private:
      Go::Color color;
      int pos;
  };
  
  typedef boost::uint_fast64_t ZobristHash;
  
  class ZobristTable
  {
    public:
      ZobristTable(Parameters *prms, int sz);
      ~ZobristTable();
      
      int getSize() const { return size; };
      Go::ZobristHash getHash(Go::Color col, int pos) const;
    
    private:
      Parameters *const params;
      const int size,sizedata;
      Go::ZobristHash *const blackhashes,*const whitehashes;
      
      Go::ZobristHash getRandomHash();
  };
  
  class ZobristTree
  {
    public:
      ZobristTree();
      ~ZobristTree();
      
      void addHash(Go::ZobristHash hash);
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
  
  class Group;
  
  class Vertex
  {
    public:
      Go::Color color;
      Go::Group *group;
  };
  
  class Board;
  
  class Group
  {
    public:
      Group(Go::Board *brd, int pos);
      ~Group() {};
      
      Go::Color getColor() const {return color;};
      int getPosition() const {return position;};
      
      void setParent(Go::Group *p) { parent=p; };
      Go::Group *find() const
      {
        if (parent==NULL)
          return (Go::Group *)this;
        else
          //return (parent=parent->find());
          return parent->find();
      };
      void unionWith(Go::Group *othergroup) //expects to only be called with roots
      {
        othergroup->setParent(this);
        stonescount+=othergroup->stonescount;
        pseudoliberties+=othergroup->pseudoliberties;
        libpossum+=othergroup->libpossum;
        libpossumsq+=othergroup->libpossumsq;
        adjacentgroups.splice(adjacentgroups.end(),*othergroup->getAdjacentGroups());
      };
      inline bool isRoot() const { return (parent==NULL); };
      
      inline int numOfStones() const { return stonescount; };
      inline int numOfPseudoLiberties() const { return pseudoliberties; };
      
      inline void addPseudoLiberty(int pos) { pseudoliberties++; libpossum+=pos; libpossumsq+=pos*pos; };
      inline void removePseudoLiberty(int pos) { pseudoliberties--; libpossum-=pos; libpossumsq-=pos*pos; };
      inline bool inAtari() const { return (pseudoliberties>0 && (pseudoliberties*libpossumsq)==(libpossum*libpossum)); };
      inline int getAtariPosition() const { if (this->inAtari()) return libpossum/pseudoliberties; else return -1; };
      void addTouchingEmpties();
      bool isOneOfTwoLiberties(int pos) const;
      int getOtherOneOfTwoLiberties(int pos) const;
      std::list<int,Go::allocator_int> *getAdjacentGroups() { return &adjacentgroups; };
    
    private:
      Go::Board *const board;
      const Go::Color color;
      const int position;
      
      int stonescount;
      Go::Group *parent;
      
      int pseudoliberties;
      int libpossum;
      int libpossumsq;
      
      std::list<int,Go::allocator_int> adjacentgroups;
  };
  
  class Board
  {
    public:
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
      struct SymmetryTransform
      {
        bool invertX;
        bool invertY;
        bool swapXY;
      };
      
      Board(int s);
      ~Board();
      
      Go::Board *copy();
      void copyOver(Go::Board *copyboard) const;
      std::string toString() const;
      std::string toSGFString() const;
      const Go::Vertex *boardData() const { return data; }; //read-only
      std::list<Go::Group*,Go::allocator_groupptr> *getGroups() { return &groups; };
      
      int getSize() const { return size; };
      int getSimpleKo() const { return simpleko; };
      bool isCurrentSimpleKo() const { return (simpleko!=-1); };
      int getMovesMade() const { return movesmade; };
      int getPassesPlayed() const { return passesplayed; };
      void resetPassesPlayed() { passesplayed=0; };
      Go::Color nextToMove() const { return nexttomove; };
      void setNextToMove(Go::Color col) { nexttomove=col; }; //clear ko?
      int getPositionMax() const { return sizedata; };
      Go::Move getLastMove() const { return lastmove; };
      Go::Move getSecondLastMove() const { return secondlastmove; };
      int getStoneCapturesOf(Go::Color col) const { return (col==Go::BLACK?blackcaptures:whitecaptures); };
      void resetCaptures() { blackcaptures=0; whitecaptures=0; };
      
      inline Go::Color getColor(int pos) const { return data[pos].color; };
      inline Go::Group *getGroup(int pos) const { return data[pos].group->find(); };
      inline bool inGroup(int pos) const { return (data[pos].group!=NULL); };
      inline bool onBoard(int pos) const { return (data[pos].color!=Go::OFFBOARD); };
      
      void makeMove(Go::Move move);
      bool validMove(Go::Move move) const;
      
      int numOfValidMoves(Go::Color col) const { return (col==Go::BLACK?blackvalidmovecount:whitevalidmovecount); };
      Go::BitBoard *getValidMoves(Go::Color col) const { return (col==Go::BLACK?blackvalidmoves:whitevalidmoves); };
      
      int score();
      bool weakEye(Go::Color col, int pos) const;
      int touchingEmpty(int pos) const;
      int surroundingEmpty(int pos) const;
      void countAdjacentColors(int pos, int &empty, int &black, int &white, int &offboard) const;
      void countDiagonalColors(int pos, int &empty, int &black, int &white, int &offboard) const;
      void turnSymmetryOff() { symmetryupdated=false;currentsymmetry=NONE; };
      void turnSymmetryOn() { symmetryupdated=true;currentsymmetry=this->computeSymmetry(); };
      
      void setFeatures(Features *feat, bool inc) { features=feat; incfeatures=inc; markchanges=true; this->refreshFeatureGammas(); };
      float getFeatureTotalGamma() const { return (nexttomove==Go::BLACK?blacktotalgamma:whitetotalgamma); };
      float getFeatureGamma(int pos) const { return (nexttomove==Go::BLACK?blackgammas:whitegammas)->get(pos); };
      
      static bool isWinForColor(Go::Color col, float score);
      
      Go::Board::Symmetry computeSymmetry();
      Go::Board::Symmetry getSymmetry() const { return currentsymmetry; };
      std::string getSymmetryString(Go::Board::Symmetry sym) const;
      int doSymmetryTransformToPrimary(Go::Board::Symmetry sym, int pos);
      Go::Board::SymmetryTransform getSymmetryTransformToPrimary(Go::Board::Symmetry sym, int pos) const;
      static Go::Board::SymmetryTransform getSymmetryTransformToPrimaryStatic(int size, Go::Board::Symmetry sym, int pos);
      int doSymmetryTransform(Go::Board::SymmetryTransform trans, int pos, bool reverse=false);
      static int doSymmetryTransformStatic(Go::Board::SymmetryTransform trans, int size, int pos);
      static int doSymmetryTransformStaticReverse(Go::Board::SymmetryTransform trans, int size, int pos);
      static Go::Board::SymmetryTransform getSymmetryTransformBetweenPositions(int size, int pos1, int pos2);
      std::list<Go::Board::SymmetryTransform> getSymmetryTransformsFromPrimary(Go::Board::Symmetry sym) const;
      static std::list<Go::Board::SymmetryTransform> getSymmetryTransformsFromPrimaryStatic(Go::Board::Symmetry sym);
      
      bool isCapture(Go::Move move) const;
      bool isExtension(Go::Move move) const;
      bool isSelfAtari(Go::Move move) const;
      bool isAtari(Go::Move move) const;
      int getDistanceToBorder(int pos) const;
      int getCircularDistance(int pos1, int pos2) const;
      Go::ObjectBoard<int> *getCFGFrom(int pos, int max=0) const;
      int getThreeEmptyGroupCenterFrom(int pos) const;
      
      Go::ZobristHash getZobristHash(Go::ZobristTable *table) const;
      
      Go::Color getScoredOwner(int pos) const;
    
    private:
      const int size;
      const int sizesq;
      const int sizedata;
      Go::Vertex *const data;
      std::list<Go::Group*,Go::allocator_groupptr> groups;
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
      Features *features;
      bool incfeatures;
      float blacktotalgamma;
      float whitetotalgamma;
      Go::ObjectBoard<float> *blackgammas;
      Go::ObjectBoard<float> *whitegammas;
      int blackcaptures,whitecaptures;
      
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
      
      void refreshFeatureGammas();
      void updateFeatureGammas();
      void updateFeatureGamma(int pos);
      void updateFeatureGamma(Go::Color col, int pos);
  };
};

#endif
