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
  
  static Go::Color otherColor(Go::Color col)
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
      
      Move() {color=Go::EMPTY;x=-2;y=-2;};
      
      Move(Go::Color col, int ix, int iy) {color=col;x=ix;y=iy;};
      Move(Go::Color col, Go::Move::Type type)
      {
        if (type==NORMAL)
          throw Go::Exception("invalid type");
        color=col;
        int i=(type==PASS)?-1:-2;
        x=i;
        y=i;
      };
      
      Go::Color getColor() {return color;};
      int getX() {return x;};
      int getY() {return y;};
      
      bool isPass() {return (x==-1 && y==-1)?true:false;};
      bool isResign() {return (x==-2 && y==-2)?true:false;};
      
      std::string toString();
      
      bool operator==(Go::Move other) { return (color==other.getColor() && x==other.getX() && y==other.getY()); };
      bool operator!=(Go::Move other) { return !(*this == other); };
    
    private:
      Go::Color color;
      int x,y;
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
          int numOfStones() { return stones.size(); };
          int numOfLiberties() { return liberties.size(); };
          
          std::list<Go::Board::Vertex*> *getStones() { return &stones; };
          std::list<Go::Board::Vertex*> *getLiberties() { return &liberties; };
          
          void addStone(Go::Board::Vertex *stone);
          void addLiberty(Go::Board::Vertex *liberty);
          void removeLiberty(Go::Board::Vertex *liberty);
        
        private:
          std::list<Go::Board::Vertex*> stones;
          std::list<Go::Board::Vertex*> liberties;
      };
      
      Go::Board *copy();
      
      Go::Board::Vertex *boardData() { return data; }; //must only be used for read-only access
      std::string toString();
      
      int getSize() { return size; };
      int getPassesPlayed() { return passesplayed; };
      int getMovesMade() { return movesmade; };
      
      std::list<Go::Move> *getValidMoves(Go::Color col);
      bool validMove(Go::Move move);
      void makeMove(Go::Move move);
      
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
      std::list<Go::Move> blackvalidmoves,whitevalidmoves;
      
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
