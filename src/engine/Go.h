#ifndef DEF_OAKFOAM_GO_H
#define DEF_OAKFOAM_GO_H

#include <string>

namespace Go
{
  enum Color{
    EMPTY,
    BLACK,
    WHITE
  };
  
  class Exception {
    public:
      Exception(std::string m = "undefined") : message(m) {}
      std::string msg() {return message;}
    
    private:
      std::string message;
  };
  
  class Move
  {
    public:
      enum Type{
        NORMAL,
        PASS,
        RESIGN
      };
      
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
    
    private:
      Go::Color color;
      int x,y;
  };
  
  class Board
  {
    public:
      Board(int s);
      ~Board();
      
      struct Vertex{
        Go::Color color;
        int group;
        int liberties;
      };
      
      Go::Board::Vertex *boardData();
      
      int getSize();
      
      Go::Board *copy();
      
      bool validMove(Go::Move move);
      
      void makeMove(Go::Move move);
    
    private:
      int size;
      Go::Board::Vertex *data;
      int koX, koY;
      
      Go::Color colorAt(int x, int y);
      void setColorAt(int x, int y, Go::Color col);
      
      int groupAt(int x, int y);
      void setGroupAt(int x, int y, int group);
      
      int libertiesAt(int x, int y);
      void setLibertiesAt(int x, int y, int liberties);
      
      void checkCoords(int x, int y);
      
      int directLiberties(int x, int y);
      int currentLiberties(int x, int y);
      
      void setKo(int x, int y);
  };
};

#endif
