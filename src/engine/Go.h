#ifndef DEF_OAKFOAM_GO_H
#define DEF_OAKFOAM_GO_H

#include <string>
#include <cstdio>
#include <list>
#include <sstream>

namespace Go
{
  enum Color
  {
    EMPTY,
    BLACK,
    WHITE
  };
  
  inline static Go::Color otherColor(Go::Color col)
  {
    if (col==Go::BLACK)
      return Go::WHITE;
    else if (col==Go::WHITE)
      return Go::BLACK;
    else
      return Go::EMPTY;
  };
  
  class Exception
  {
    public:
      Exception(std::string m = "undefined") : message(m) {}
      std::string msg() {return message;}
    
    private:
      std::string message;
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
      
      inline Move() {color=Go::EMPTY;x=-2;y=-2;};
      
      inline Move(Go::Color col, int ix, int iy) {color=col;x=ix;y=iy;};
      inline Move(Go::Color col, Go::Move::Type type)
      {
        if (type==NORMAL)
          throw Go::Exception("invalid type");
        color=col;
        int i=(type==PASS)?-1:-2;
        x=i;
        y=i;
      };
      
      inline Go::Color getColor() {return color;};
      inline int getX() {return x;};
      inline int getY() {return y;};
      
      inline bool isPass() {return (x==-1 && y==-1)?true:false;};
      inline bool isResign() {return (x==-2 && y==-2)?true:false;};
      
      std::string toString();
      
      inline bool operator==(Go::Move other) { return (color==other.getColor() && x==other.getX() && y==other.getY()); };
      inline bool operator!=(Go::Move other) { return !(*this == other); };
    
    private:
      Go::Color color;
      int x,y;
  };
  
  class BitBoard
  {
    public:
      BitBoard(int s);
      ~BitBoard();
      
      inline bool get(int x, int y) { return data[y*size+x]; };
      inline void set(int x, int y, bool val=true) { data[y*size+x]=val; };
      inline void clear(int x, int y) { this->set(x,y,false); };
      inline void fill(bool val) { for (int i=0;i<(size*size);i++) data[i]=val; };
      inline void clear() { this->fill(false); };
    
    private:
      int size;
      bool *data;
  };
  
  class Board
  {
    public:
      Board(int s);
      ~Board();
      
      class Group;
      
      struct Point
      {
        int x;
        int y;
      };
      
      struct Vertex
      {
        Go::Board::Point point;
        Go::Color color;
        Go::Board::Group *group;
      };
      
      class Group
      {
        public:
          Group(int size);
          ~Group();
          
          inline int numOfStones() { return stoneslist.size(); };
          inline int numOfLiberties() { return libertieslist.size(); };
          
          inline std::list<Go::Board::Vertex*> *getStonesList() { return &stoneslist; };
          inline std::list<Go::Board::Vertex*> *getLibertiesList() { return &libertieslist; };
          
          void addStone(Go::Board::Vertex *stone);
          void addLiberty(Go::Board::Vertex *liberty);
          void removeLiberty(Go::Board::Vertex *liberty);
        
        private:
          std::list<Go::Board::Vertex*> stoneslist;
          Go::BitBoard *stonesboard;
          std::list<Go::Board::Vertex*> libertieslist;
          Go::BitBoard *libertiesboard;
      };
      
      Go::Board *copy();
      
      Go::Board::Vertex *boardData() { return data; }; //must only be used for read-only access
      std::string toString();
      std::list<Go::Board::Group*> *getGroups() { return &groups; };
      
      int getSize() { return size; };
      int getPassesPlayed() { return passesplayed; };
      int getMovesMade() { return movesmade; };
      
      void makeMove(Go::Move move);
      int numOfValidMoves(Go::Color col) { return (col==Go::BLACK?blackvalidmovecount:whitevalidmovecount); };
      Go::BitBoard *getValidMoves(Go::Color col);
      bool validMove(Go::Move move);
      
      void setNextToMove(Go::Color col) { nexttomove=col; };
      Go::Color nextToMove() { return nexttomove; };
      
      int score();
      bool weakEye(Go::Color col, int x, int y);
    
    private:
      int size;
      Go::Board::Vertex *data;
      int koX, koY;
      std::list<Go::Board::Group*> groups;
      Go::Color nexttomove;
      int passesplayed;
      int movesmade;
      int blackvalidmovecount,whitevalidmovecount;
      Go::BitBoard *blackvalidmoves,*whitevalidmoves;
      
      struct ScoreVertex
      {
        bool touched;
        Go::Color color;
      };
      
      Go::Board::Vertex *vertexAt(int x, int y);
      Go::Color colorAt(int x, int y);
      void setColorAt(int x, int y, Go::Color col);
      Go::Board::Group *groupAt(int x, int y);
      void setGroupAt(int x, int y, Go::Board::Group *group);
      int libertiesAt(int x, int y);
      int groupSizeAt(int x, int y);
      
      void checkCoords(int x, int y);
      
      void refreshGroups();
      void spreadGroup(int x, int y, Go::Color col, Go::Board::Group *group);
      void addDirectLiberties(int x, int y, Go::Board::Group *group);
      int directLiberties(int x, int y);
      int removeGroup(Go::Board::Group *group);
      void mergeGroups(Go::Board::Group *first, Go::Board::Group *second);
      
      void refreshValidMoves(Go::Color col);
      bool validMoveCheck(Go::Move move);
      void addValidMove(Go::Move move);
      void removeValidMove(Go::Move move);
      
      void setKo(int x, int y);
      
      void spreadScore(Go::Board::ScoreVertex *scoredata, int x, int y, Go::Color col);
  };
};

#endif
