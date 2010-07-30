#ifndef DEF_OAKFOAM_PATTERN_H
#define DEF_OAKFOAM_PATTERN_H

/*
  SmallPattern Positions:
    NW  N   NE
    W   C   E
    SW  S   SE
*/

#define PATTERN_3x3_TABLE_BYTES (2^16)/8

#include "Go.h"

namespace Pattern
{ 
  class ThreeByThree
  {
    public:
      ThreeByThree(Go::Color colnw, Go::Color coln, Go::Color colne, Go::Color colw, Go::Color cole, Go::Color colsw, Go::Color cols, Go::Color colse)
      {
        this->colnw=colnw; this->coln=coln; this->colne=colne;
        this->colnw=colw; this->colne=cole;
        this->colnw=colsw; this->coln=cols; this->colne=colse;
      };
      ThreeByThree(Go::Board *board, int pos)
      {
        int size=board->getSize();
        colnw=board->getColor(pos+P_NW); coln=board->getColor(pos+P_N); colne=board->getColor(pos+P_NE);
        colw=board->getColor(pos+P_W); cole=board->getColor(pos+P_E);
        colsw=board->getColor(pos+P_SW); cols=board->getColor(pos+P_S); colse=board->getColor(pos+P_SE);
      };
      ~ThreeByThree() {};
      
      inline bool operator==(Pattern::ThreeByThree other)
      {
        return (
          colnw==other.colnw && coln==other.coln && colne==other.colne && 
          colw==other.colw && cole==other.cole && 
          colsw==other.colsw && cols==other.cols && colse==other.colse
        );
      };
      inline bool operator!=(Pattern::ThreeByThree other) { return !(*this == other); };
      
      Pattern::ThreeByThree invert();
      Pattern::ThreeByThree rotateRight();
      Pattern::ThreeByThree flipHorizontal();
      
      int hash();
    
    private:
      Go::Color colnw,coln,colne,colw,cole,colsw,cols,colse;
      
      int hashColor(Go::Color col);
  };
  
  class ThreeByThreeTable
  {
    public:
      ThreeByThreeTable()
      {
        table=(unsigned char *)malloc(PATTERN_3x3_TABLE_BYTES);
        for (int i=0;i<PATTERN_3x3_TABLE_BYTES;i++)
          table[i]=0;
      };
      ~ThreeByThreeTable() { free(table); };
      
      void addPattern(int hash) { table[byteNum(hash)]|=(1<<bitNum(hash)); };
      void clearPattern(int hash) { table[byteNum(hash)]&=(~(1<<bitNum(hash))); };
      bool isPattern(int hash) { return (table[byteNum(hash)]&(1<<bitNum(hash))); };
    
    private:
      unsigned char *table; //assume sizeof(char)==1
      
      inline int byteNum(int hash) { return (hash/8); };
      inline int bitNum(int hash) { return (hash%8); };
  };
};
#endif
