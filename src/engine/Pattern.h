#ifndef DEF_OAKFOAM_PATTERN_H
#define DEF_OAKFOAM_PATTERN_H

/*
  SmallPattern Positions:
    NW  N   NE
    W   C   E
    SW  S   SE
*/

#define PATTERN_3x3_TABLE_BYTES (1<<16)/8
#define PATTERN_3x3_GAMMAS (1<<16)

#define PATTERN_CIRC_MAXSIZE 15
#define PATTERN_CIRC_32BITPARTS 10

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

#include <string>
#include <list>
#include <boost/cstdint.hpp>
#include "Go.h"

namespace Pattern
{
  class ThreeByThreeTable;
  
  class ThreeByThree
  {
    public:
      static unsigned int makeHash(Go::Color colnw, Go::Color coln, Go::Color colne, Go::Color colw, Go::Color cole, Go::Color colsw, Go::Color cols, Go::Color colse);
      static unsigned int makeHash(Go::Color colors[])
      {
        return Pattern::ThreeByThree::makeHash(
          colors[0], colors[1], colors[2],
          colors[3], colors[4],
          colors[5], colors[6], colors[7]
        );
      };
      static unsigned int makeHash(Go::Board *board, int pos)
      {
        int size=board->getSize();
        return Pattern::ThreeByThree::makeHash(
          board->getColor(pos+P_NW), board->getColor(pos+P_N), board->getColor(pos+P_NE),
          board->getColor(pos+P_W), board->getColor(pos+P_E),
          board->getColor(pos+P_SW), board->getColor(pos+P_S), board->getColor(pos+P_SE)
        );
      };
      
      static unsigned int invert(unsigned int hash);
      static unsigned int rotateRight(unsigned int hash);
      static unsigned int flipHorizontal(unsigned int hash);
      
      static unsigned int smallestEquivalent(unsigned int hash);
    
    private:
      static int hashColor(Go::Color col);
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
      
      inline void addPattern(unsigned int hash) { table[byteNum(hash)]|=(1<<bitNum(hash)); };
      inline void clearPattern(unsigned int hash) { table[byteNum(hash)]&=(~(1<<bitNum(hash))); };
      inline bool isPattern(unsigned int hash) { return (table[byteNum(hash)]&(1<<bitNum(hash))); };
      inline void updatePattern(bool add, unsigned int hash) { if (add) addPattern(hash); else clearPattern(hash); };
      
      void updatePatternTransformed(bool addpattern, unsigned int pattern, bool addinverted=true);
      
      bool loadPatternFile(std::string patternfilename);
      bool loadPatternString(std::string patternstring);
      bool loadPatternDefaults() { return this->loadPatternString(PATTERN_3x3_DEFAULTS); };
    
    private:
      unsigned char *table; //assume sizeof(char)==1
      
      inline int byteNum(unsigned int hash) { return (hash/8); };
      inline int bitNum(unsigned int hash) { return (hash%8); };
      
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
  
  class ThreeByThreeGammas
  {
    public:
      ThreeByThreeGammas()
      {
        gammas=(float *)malloc(sizeof(float)*PATTERN_3x3_GAMMAS);
        for (int i=0;i<PATTERN_3x3_GAMMAS;i++)
          gammas[i]=-1;
      };
      ~ThreeByThreeGammas() { free(gammas); };
      
      float getGamma(unsigned int hash) { return gammas[hash]; };
      void setGamma(unsigned int hash, float g) { gammas[hash]=g; };
      bool hasGamma(unsigned int hash) { return (gammas[hash]!=-1); };
    
    private:
      float *gammas;
  };
  
  class CircularDictionary
  {
    public:
      CircularDictionary();
      
      boost::uint_fast32_t rot_r2[PATTERN_CIRC_32BITPARTS];
      boost::uint_fast32_t rot_r4[PATTERN_CIRC_32BITPARTS];
      boost::uint_fast32_t rot_l6[PATTERN_CIRC_32BITPARTS];
      boost::uint_fast32_t rot_l12[PATTERN_CIRC_32BITPARTS];
      boost::uint_fast32_t flip_0[PATTERN_CIRC_32BITPARTS];
      boost::uint_fast32_t flip_r2[PATTERN_CIRC_32BITPARTS];
      boost::uint_fast32_t flip_r4[PATTERN_CIRC_32BITPARTS];
      boost::uint_fast32_t flip_r6[PATTERN_CIRC_32BITPARTS];
      boost::uint_fast32_t flip_r10[PATTERN_CIRC_32BITPARTS];
      boost::uint_fast32_t flip_r14[PATTERN_CIRC_32BITPARTS];
      boost::uint_fast32_t flip_l2[PATTERN_CIRC_32BITPARTS];
      boost::uint_fast32_t flip_l4[PATTERN_CIRC_32BITPARTS];
      boost::uint_fast32_t flip_l6[PATTERN_CIRC_32BITPARTS];
      boost::uint_fast32_t flip_l10[PATTERN_CIRC_32BITPARTS];
      boost::uint_fast32_t flip_l14[PATTERN_CIRC_32BITPARTS];
      
      std::list<int> *getXOffsetsForSize(int size) { return &(dictx[size]); }; // XXX: no bounds checking!
      std::list<int> *getYOffsetsForSize(int size) { return &(dicty[size]); }; // XXX: no bounds checking!
      int getBaseOffset(int size) { return baseoffset[size]; };
    
    private:
      std::list<int> dictx[PATTERN_CIRC_MAXSIZE+1];
      std::list<int> dicty[PATTERN_CIRC_MAXSIZE+1];
      int baseoffset[PATTERN_CIRC_MAXSIZE+1];
      
      void setTrans(boost::uint_fast32_t data[PATTERN_CIRC_32BITPARTS], int offset);
  };
  
  class Circular
  {
    public:
      Circular(Pattern::CircularDictionary *dict, Go::Board *board, int pos, int sz);
      
      int getSize() { return size; };
      boost::uint_fast32_t *getHash() { return hash; };
      
      Pattern::Circular copy() { return this->getSubPattern(NULL,size); };
      Pattern::Circular getSubPattern(Pattern::CircularDictionary *dict, int newsize);
      
      std::string toString(Pattern::CircularDictionary *dict);
      
      bool operator==(Pattern::Circular other);
      bool operator!=(Pattern::Circular other) { return !(*this == other); };
      
      void invert();
      void rotateRight(Pattern::CircularDictionary *dict);
      void flipHorizontal(Pattern::CircularDictionary *dict);
    
    private:
      Circular() {};
      
      int size;
      boost::uint_fast32_t hash[PATTERN_CIRC_32BITPARTS];
      
      static int hashColor(Go::Color col);
      void initColor(int offset, Go::Color col);
      void resetColor(int offset);
  };
};
#endif
