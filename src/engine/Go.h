#ifndef DEF_OAKFOAM_GO_H
#define DEF_OAKFOAM_GO_H

#include <string>
#include <cstdio>
#include <list>

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
      
      void print();
    
    private:
      Go::Color color;
      int x,y;
  };
  
  class Board
  {
    public:
      Board(int s);
      ~Board();
      
      struct Vertex
      {
        Go::Color color;
        int group;
        int liberties;
      };
      
      Go::Board *copy();
      Go::Board::Vertex *boardData(); //must only be used for read-only access
      
      int getSize();
      int getPassesPlayed() { return passesplayed; };
      int getMovesMade() { return movesmade; };
      
      bool validMove(Go::Move move);
      void makeMove(Go::Move move);
      
      Go::Color nextToMove() { return nexttomove; };
      
      void print();
      
      bool scoreable();
      int score();
      bool weakEye(Go::Color col, int x, int y);
      
      int size;
      Go::Board::Vertex *data;
      int koX, koY;
      int totalgroups;
      Go::Color nexttomove;
      int passesplayed;
      int movesmade;
      
    private:
      
      Go::Color colorAt(int x, int y);
      void setColorAt(int x, int y, Go::Color col);
      
      int groupAt(int x, int y);
      void setGroupAt(int x, int y, int group);
      
      int libertiesAt(int x, int y);
      void setLibertiesAt(int x, int y, int liberties);
      
      void checkCoords(int x, int y);
      
      void updateLiberties();
      int directLiberties(int x, int y);
      int directLiberties(int x, int y, bool dirty);
      int updateGroups();
      void spreadGroup(int x, int y, int group);
      int removeGroup(int group);
      
      int solidLinksFrom(int x, int y);
      int solidlyTouches(int x, int y);
      
      void setKo(int x, int y);
  };
  
  class IncrementalBoard
  {
    public:
      IncrementalBoard(int s);
      ~IncrementalBoard();
      
      class Group;
      
      struct Point
      {
        int x;
        int y;
      };
      
      struct Vertex
      {
        Go::IncrementalBoard::Point point;
        Go::Color color;
        Go::IncrementalBoard::Group *group;
      };
      
      class Group
      {
        public:
          int numOfStones() { return stones.size(); };
          int numOfLiberties() { return liberties.size(); };
          
          std::list<Go::IncrementalBoard::Vertex*> *getStones() { return &stones; };
          std::list<Go::IncrementalBoard::Vertex*> *getLiberties() { return &liberties; };
          
          void addStone(Go::IncrementalBoard::Vertex *stone);
          void addLiberty(Go::IncrementalBoard::Vertex *liberty);
          void removeLiberty(Go::IncrementalBoard::Vertex *liberty);
        
        private:
          std::list<Go::IncrementalBoard::Vertex*> stones;
          std::list<Go::IncrementalBoard::Vertex*> liberties;
      };
      
      Go::IncrementalBoard *copy();
      void import(Go::Board *board);
      
      Go::IncrementalBoard::Vertex *boardData() { return data; }; //must only be used for read-only access
      void print();
      
      int getSize() { return size; };
      int getPassesPlayed() { return passesplayed; };
      int getMovesMade() { return movesmade; };
      
      bool validMove(Go::Move move);
      void makeMove(Go::Move move);
      
      Go::Color nextToMove() { return nexttomove; };
      
      bool scoreable();
      int score();
      bool weakEye(Go::Color col, int x, int y);
    
    private:
      int size;
      Go::IncrementalBoard::Vertex *data;
      int koX, koY;
      std::list<Go::IncrementalBoard::Group*> groups;
      Go::Color nexttomove;
      int passesplayed;
      int movesmade;
      
      struct ScoreVertex
      {
        bool touched;
        Go::Color color;
      };
      
      Go::IncrementalBoard::Vertex *vertexAt(int x, int y);
      Go::Color colorAt(int x, int y);
      void setColorAt(int x, int y, Go::Color col);
      Go::IncrementalBoard::Group *groupAt(int x, int y);
      void setGroupAt(int x, int y, Go::IncrementalBoard::Group *group);
      int libertiesAt(int x, int y);
      int groupSizeAt(int x, int y);
      
      void checkCoords(int x, int y);
      
      void refreshGroups();
      void spreadGroup(int x, int y, Go::Color col, Go::IncrementalBoard::Group *group);
      void addDirectLiberties(int x, int y, Go::IncrementalBoard::Group *group);
      int directLiberties(int x, int y);
      int removeGroup(Go::IncrementalBoard::Group *group);
      void mergeGroups(Go::IncrementalBoard::Group *first, Go::IncrementalBoard::Group *second);
      
      void setKo(int x, int y);
      
      void spreadScore(Go::IncrementalBoard::ScoreVertex *scoredata, int x, int y, Go::Color col);
  };
};

#endif
