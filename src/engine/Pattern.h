#ifndef DEF_OAKFOAM_PATTERN_H
#define DEF_OAKFOAM_PATTERN_H

/*
  SmallPattern Positions:
    NW  N   NE
    W   C   E
    SW  S   SE
*/

#define PATTERN_3x3_TABLE_BYTES (1<<16)/8

#define PATTERN_3x3_DEFAULTS " \
+*BWBEE??? \
+*BWEEE?E? \
+*BW?BE?E? \
+BBWWEE?E? \
+*BW?W???? \
-*BW?WW?E? \
-*BW?WE?W? \
+*?B?WWwww \
+*BE?W?--- \
+*?B?bW--- \
+B?BW??_-- \
+W?BW?b--- \
+W?BWWB--- \
"

#include <cstdlib>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include "Go.h"

namespace Pattern
{
  class ThreeByThreeTable;
  
  class ThreeByThree
  {
    public:
      ThreeByThree(Go::Color colnw, Go::Color coln, Go::Color colne, Go::Color colw, Go::Color cole, Go::Color colsw, Go::Color cols, Go::Color colse)
      {
        this->colnw=colnw; this->coln=coln; this->colne=colne;
        this->colw=colw; this->cole=cole;
        this->colsw=colsw; this->cols=cols; this->colse=colse;
      };
      ThreeByThree(Go::Board *board, int pos)
      {
        int size=board->getSize();
        colnw=board->getColor(pos+P_NW); coln=board->getColor(pos+P_N); colne=board->getColor(pos+P_NE);
        colw=board->getColor(pos+P_W); cole=board->getColor(pos+P_E);
        colsw=board->getColor(pos+P_SW); cols=board->getColor(pos+P_S); colse=board->getColor(pos+P_SE);
      };
      ThreeByThree(Go::Color colors[])
      {
        colnw=colors[0]; coln=colors[1]; colne=colors[2];
        colw=colors[3]; cole=colors[4];
        colsw=colors[5]; cols=colors[6]; colse=colors[7];
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
      
      std::string toString();
    
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
      
      inline void addPattern(int hash) { table[byteNum(hash)]|=(1<<bitNum(hash)); };
      inline void clearPattern(int hash) { table[byteNum(hash)]&=(~(1<<bitNum(hash))); };
      inline bool isPattern(int hash) { return (table[byteNum(hash)]&(1<<bitNum(hash))); };
      inline void updatePattern(bool add, int hash) { if (add) addPattern(hash); else clearPattern(hash); };
      
      void updatePatternTransformed(bool addpattern, Pattern::ThreeByThree pattern, bool addinverted=true);
      
      bool loadPatternFile(std::string patternfilename);
      bool loadPatternString(std::string patternstring);
      bool loadPatternDefaults() { return this->loadPatternString(PATTERN_3x3_DEFAULTS); };
    
    private:
      unsigned char *table; //assume sizeof(char)==1
      
      inline int byteNum(int hash) { return (hash/8); };
      inline int bitNum(int hash) { return (hash%8); };
      
      enum Color
      {
        EMPTY,
        BLACK,
        WHITE,
        OFFBOARD,
        NOTBLACK,
        NOTWHITE,
        NOTEMPTY,
        ANY
      };
      
      void processFilePattern(std::string pattern);
      void processPermutations(bool addpatterns, Pattern::ThreeByThreeTable::Color toplay, Pattern::ThreeByThreeTable::Color colors[], int pos=0);
      
      Pattern::ThreeByThreeTable::Color fileCharToColor(char c)
      {
        if (c=='B')
          return Pattern::ThreeByThreeTable::BLACK;
        else if (c=='W')
          return Pattern::ThreeByThreeTable::WHITE;
        else if (c=='E')
          return Pattern::ThreeByThreeTable::EMPTY;
        else if (c=='-')
          return Pattern::ThreeByThreeTable::OFFBOARD;
        else if (c=='b')
          return Pattern::ThreeByThreeTable::NOTBLACK;
        else if (c=='w')
          return Pattern::ThreeByThreeTable::NOTWHITE;
        else if (c=='e')
          return Pattern::ThreeByThreeTable::NOTEMPTY;
        else
          return Pattern::ThreeByThreeTable::ANY;
      };
      Go::Color colorToGoColor(Pattern::ThreeByThreeTable::Color col)
      {
        switch (col)
        {
          case Pattern::ThreeByThreeTable::BLACK:
            return Go::BLACK;
          case Pattern::ThreeByThreeTable::WHITE:
            return Go::WHITE;
          case Pattern::ThreeByThreeTable::EMPTY:
            return Go::EMPTY;
          default:
            return Go::OFFBOARD;
        }
      };
  };
};
#endif
