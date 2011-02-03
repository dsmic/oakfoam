#ifndef DEF_OAKFOAM_GO_H
#define DEF_OAKFOAM_GO_H

#include <string>
#include <list>
#include <boost/pool/pool.hpp>
#include <boost/pool/pool_alloc.hpp>
#include <boost/pool/object_pool.hpp>
//from "Features.h":
class Features;

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
  };
  
  class BitBoard
  {
    public:
      BitBoard(int s);
      ~BitBoard();
      
      inline bool get(int pos) { return data[pos]; };
      inline void set(int pos, bool val=true) { data[pos]=val; };
      inline void clear(int pos) { this->set(pos,false); };
      inline void fill(bool val) { for (int i=0;i<sizedata;i++) data[i]=val; };
      inline void clear() { this->fill(false); };
    
    private:
      int size,sizesq,sizedata;
      bool *data;
  };
  
  template<typename T>
  class ObjectBoard
  {
    public:
      ObjectBoard(int s)
      {
        size=s;
        sizesq=s*s;
        sizedata=1+(s+1)*(s+2);
        data=new T[sizedata];
      };
      ~ObjectBoard()
      {
        delete[] data;
      };
      
      inline T get(int pos) { return data[pos]; };
      inline void set(int pos, T val) { data[pos]=val; };
      inline void fill(T val) { for (int i=0;i<sizedata;i++) data[i]=val; };
    
    private:
      int size,sizesq,sizedata;
      T *data;
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
      
      inline Move() {color=Go::EMPTY;pos=-2;};
      inline Move(Go::Color col, int p) {color=col;pos=p;};
      inline Move(Go::Color col, int x, int y, int boardsize) {color=col;pos=(x<0?x:Go::Position::xy2pos(x,y,boardsize));};
      inline Move(Go::Color col, Go::Move::Type type)
      {
        if (type==NORMAL)
          throw Go::Exception("invalid type");
        color=col;
        pos=(type==PASS)?-1:-2;
      };
      
      inline Go::Color getColor() {return color;};
      inline int getPosition() {return pos;};
      inline int getX(int boardsize) {return (this->isPass()||this->isResign()?pos:Go::Position::pos2x(pos,boardsize));};
      inline int getY(int boardsize) {return (this->isPass()||this->isResign()?pos:Go::Position::pos2y(pos,boardsize));};
      void setPosition(int p) {pos=p;};
      
      inline bool isPass() {return (pos==-1);};
      inline bool isResign() {return (pos==-2);};
      
      std::string toString(int boardsize);
      
      inline bool operator==(Go::Move other) { return (color==other.getColor() && pos==other.getPosition()); };
      inline bool operator!=(Go::Move other) { return !(*this == other); };
    
    private:
      Go::Color color;
      int pos;
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
      
      Go::Color getColor() {return color;};
      int getPosition() {return position;};
      
      void setParent(Go::Group *p) { parent=p; };
      Go::Group *find()
      {
        if (parent==NULL)
          return this;
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
      };
      inline bool isRoot() { return (parent==NULL); };
      
      inline int numOfStones() { return stonescount; };
      inline int numOfPseudoLiberties() { return pseudoliberties; };
      
      inline void addPseudoLiberty(int pos) { pseudoliberties++; libpossum+=pos; libpossumsq+=pos*pos; };
      inline void removePseudoLiberty(int pos) { pseudoliberties--; libpossum-=pos; libpossumsq-=pos*pos; };
      inline bool inAtari() { return (pseudoliberties>0 && (pseudoliberties*libpossumsq)==(libpossum*libpossum)); };
      inline int getAtariPosition() { if (this->inAtari()) return libpossum/pseudoliberties; else return -1; };
      void addTouchingEmpties();
      bool isOneOfTwoLiberties(int pos);
    
    private:
      Go::Color color;
      int position;
      Go::Board *board;
      
      int stonescount;
      Go::Group *parent;
      
      int pseudoliberties;
      int libpossum;
      int libpossumsq;
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
      void copyOver(Go::Board *copyboard);
      std::string toString();
      std::string toSGFString();
      Go::Vertex *boardData() { return data; }; //read-only
      std::list<Go::Group*,Go::allocator_groupptr> *getGroups() { return &groups; };
      
      int getSize() { return size; };
      int getSimpleKo() { return simpleko; };
      bool isCurrentSimpleKo() { return (simpleko!=-1); };
      int getMovesMade() { return movesmade; };
      int getPassesPlayed() { return passesplayed; };
      void resetPassesPlayed() { passesplayed=0; };
      Go::Color nextToMove() { return nexttomove; };
      void setNextToMove(Go::Color col) { nexttomove=col; }; //clear ko?
      int getPositionMax() { return sizedata; };
      Go::Move getLastMove() { return lastmove; };
      Go::Move getSecondLastMove() { return secondlastmove; };
      
      inline Go::Color getColor(int pos) { return data[pos].color; };
      inline Go::Group *getGroup(int pos) { return data[pos].group->find(); };
      inline bool inGroup(int pos) { return (data[pos].group!=NULL); };
      inline bool onBoard(int pos) { return (data[pos].color!=Go::OFFBOARD); };
      
      void makeMove(Go::Move move);
      bool validMove(Go::Move move);
      
      int numOfValidMoves(Go::Color col) { return (col==Go::BLACK?blackvalidmovecount:whitevalidmovecount); };
      Go::BitBoard *getValidMoves(Go::Color col) { return (col==Go::BLACK?blackvalidmoves:whitevalidmoves); };
      
      int score();
      bool weakEye(Go::Color col, int pos);
      int touchingEmpty(int pos);
      int surroundingEmpty(int pos);
      void countAdjacentColors(int pos, int &empty, int &black, int &white, int &offboard);
      void countDiagonalColors(int pos, int &empty, int &black, int &white, int &offboard);
      void turnSymmetryOff() { symmetryupdated=false;currentsymmetry=NONE; };
      void turnSymmetryOn() { symmetryupdated=true;currentsymmetry=this->computeSymmetry(); };
      
      void setFeatures(Features *feat, bool inc) { features=feat; incfeatures=inc; markchanges=true; this->refreshFeatureGammas(); };
      float getFeatureTotalGamma() { return (nexttomove==Go::BLACK?blacktotalgamma:whitetotalgamma); };
      float getFeatureGamma(int pos) { return (nexttomove==Go::BLACK?blackgammas:whitegammas)->get(pos); };
      
      static bool isWinForColor(Go::Color col, float score);
      
      Go::Board::Symmetry computeSymmetry();
      Go::Board::Symmetry getSymmetry() { return currentsymmetry; };
      std::string getSymmetryString(Go::Board::Symmetry sym);
      int doSymmetryTransformToPrimary(Go::Board::Symmetry sym, int pos);
      Go::Board::SymmetryTransform getSymmetryTransformToPrimary(Go::Board::Symmetry sym, int pos);
      int doSymmetryTransform(Go::Board::SymmetryTransform trans, int pos, bool reverse=false);
      static int doSymmetryTransformStatic(Go::Board::SymmetryTransform trans, int size, int pos);
      static int doSymmetryTransformStaticReverse(Go::Board::SymmetryTransform trans, int size, int pos);
      static Go::Board::SymmetryTransform getSymmetryTransformBetweenPositions(int size, int pos1, int pos2);
      
      bool isCapture(Go::Move move);
      bool isExtension(Go::Move move);
      bool isSelfAtari(Go::Move move);
      bool isAtari(Go::Move move);
      int getDistanceToBorder(int pos);
      int getCircularDistance(int pos1, int pos2);
      Go::ObjectBoard<int> *getCFGFrom(int pos, int max=0);
      int getThreeEmptyChainCenterFrom(int pos);
    
    private:
      int size;
      int sizesq;
      int sizedata;
      Go::Vertex *data;
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
      
      inline Go::Group *getGroupWithoutFind(int pos) { return data[pos].group; };
      inline void setColor(int pos, Go::Color col) { data[pos].color=col; if (markchanges) { lastchanges->set(pos); } };
      inline void setGroup(int pos, Go::Group *grp) { data[pos].group=grp; };
      inline int getPseudoLiberties(int pos) { if (data[pos].group==NULL) return 0; else return data[pos].group->find()->numOfPseudoLiberties(); };
      inline int getGroupSize(int pos) { if (data[pos].group==NULL) return 0; else return data[pos].group->find()->numOfStones(); };
      
      bool touchingAtLeastOneEmpty(int pos);
      
      void refreshGroups();
      void spreadGroup(int pos, Go::Group *group);
      int removeGroup(Go::Group *group);
      void spreadRemoveStones(Go::Color col, int pos, std::list<int,Go::allocator_int> *possiblesuicides);
      void mergeGroups(Go::Group *first, Go::Group *second);
      
      bool validMoveCheck(Go::Move move);
      void refreshValidMoves();
      void refreshValidMoves(Go::Color col);
      void addValidMove(Go::Move move);
      void removeValidMove(Go::Move move);
      
      bool hasSymmetryVertical();
      bool hasSymmetryHorizontal();
      bool hasSymmetryDiagonalDown();
      bool hasSymmetryDiagonalUp();
      void updateSymmetry();
      int doSymmetryTransformPrimitive(Go::Board::Symmetry sym, int pos);
      
      struct ScoreVertex
      {
        bool touched;
        Go::Color color;
      };
      
      void spreadScore(Go::Board::ScoreVertex *scoredata, int pos, Go::Color col);
      
      void refreshFeatureGammas();
      void updateFeatureGammas();
      void updateFeatureGamma(int pos);
      void updateFeatureGamma(Go::Color col, int pos);
  };
};

#endif
