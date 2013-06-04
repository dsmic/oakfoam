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
#include <cstring>
#include <boost/cstdint.hpp>
#include "Go.h"

/** Patterns.
 * Matching and manipulation of patterns.
 */
namespace Pattern
{
  class ThreeByThreeTable;
  
  /** Patterns of size 3x3.
   * Pattern hashes are represented by unsigned int's.
   * Assumes int is at least 16 bits.
   */
  class ThreeByThree
  {
    public:
      /** Make a hash given a set of colors. */
      static unsigned int makeHash(Go::Color colnw, Go::Color coln, Go::Color colne, Go::Color colw, Go::Color cole, Go::Color colsw, Go::Color cols, Go::Color colse);
      /** Make a hash from an array of colors. */
      static unsigned int makeHash(Go::Color colors[])
      {
        return Pattern::ThreeByThree::makeHash(
          colors[0], colors[1], colors[2],
          colors[3], colors[4],
          colors[5], colors[6], colors[7]
        );
      };
      /** Make a hash from a board position. */
      static unsigned int makeHash(Go::Board *board, int pos)
      {
        if (pos<0) return 0;
        int size=board->getSize();
        return Pattern::ThreeByThree::makeHash(
          board->getColor(pos+P_NW), board->getColor(pos+P_N), board->getColor(pos+P_NE),
          board->getColor(pos+P_W), board->getColor(pos+P_E),
          board->getColor(pos+P_SW), board->getColor(pos+P_S), board->getColor(pos+P_SE)
        );
      };
      
      /** Invert a hash.
       * Make black stones white and vica versa.
       */
      static unsigned int invert(unsigned int hash);
      /** Rotate a hash 90 degrees to the right. */
      static unsigned int rotateRight(unsigned int hash);
      /** Flip a hash horizontally. */
      static unsigned int flipHorizontal(unsigned int hash);
      
      /** Compute the smallest equivalent hash.
       * The smallest hash is the smallest of all the eight possible rotations and flips.
       */
      static unsigned int smallestEquivalent(unsigned int hash);
    
    private:
      static int hashColor(Go::Color col);
  };

  /** Patterns of a 5x5 area not covered by the 3x3 patterns.
   * Pattern hashes are represented by unsigned long's.
   * Assumes long is at least 32 bits.
   */
  class FiveByFiveBorder
  {
    public:
      /** Make a hash from a board position. */
      static unsigned long makeHash(Go::Board *board, int pos);
    private:
      static int hashColor(Go::Color col);
  };      
  
  /** Table of 3x3 pattern hashes. */
  class ThreeByThreeTable
  {
    public:
      ThreeByThreeTable() : table((unsigned char *)malloc(PATTERN_3x3_TABLE_BYTES))
      {
        for (int i=0;i<PATTERN_3x3_TABLE_BYTES;i++)
          table[i]=0;
      };
      ~ThreeByThreeTable() { free(table); };
      
      /** Add a hash to the table. */
      inline void addPattern(unsigned int hash) { table[byteNum(hash)]|=(1<<bitNum(hash)); };
      /** Remove a hash from the table. */
      inline void clearPattern(unsigned int hash) { table[byteNum(hash)]&=(~(1<<bitNum(hash))); };
      /** Determine if a hash is in the table. */
      inline bool isPattern(unsigned int hash) const { return (table[byteNum(hash)]&(1<<bitNum(hash))); };
      /** Add or remove a hash.
       * @param add If set, add the hash, else remove it.
       * @param hash The hash to add or remove.
       */
      inline void updatePattern(bool add, unsigned int hash) { if (add) addPattern(hash); else clearPattern(hash); };
      /** Add or remove all possible rotations and flips of a pattern.
       * @param addpattern If set, add the hash, else remove it.
       * @param pattern The hash to add or remove.
       * @param addinverted If set, also add the inverted hashes.
       */
      void updatePatternTransformed(bool addpattern, unsigned int pattern, bool addinverted=true);
      
      /** Load a file of patterns. */
      bool loadPatternFile(std::string patternfilename);
      /** Load a single pattern line. */
      bool loadPatternString(std::string patternstring);
      /** Load a the default patterns. */
      bool loadPatternDefaults() { return this->loadPatternString(PATTERN_3x3_DEFAULTS); };

    private:
      unsigned char *const table; //assume sizeof(char)==1
      
      inline int byteNum(unsigned int hash) const { return (hash/8); };
      inline int bitNum(unsigned int hash) const { return (hash%8); };
      
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
  
  /** Table of gamma values for 3x3 pattern hashes. */
  class ThreeByThreeGammas
  {
    public:
      ThreeByThreeGammas() : gammas((float *)malloc(sizeof(float)*PATTERN_3x3_GAMMAS))
      {
        count = 0;
        for (int i=0;i<PATTERN_3x3_GAMMAS;i++)
          gammas[i]=-1;
      };
      ~ThreeByThreeGammas() { free(gammas); };
      
      /** Get the gamma value for a given hash.
       * A gamma value of -1 implies one hasn't been set.
       */
      float getGamma(unsigned int hash) const { return gammas[hash]; };
      /** Set the gamma value for a given hash. */
      void setGamma(unsigned int hash, float g) { if (this->getGamma(hash)==-1) count++; gammas[hash]=g; if (g==-1) count--; };
      void learnGamma(unsigned int hash, float g) { if (this->getGamma(hash)==-1) count++; gammas[hash]+=g; if (gammas[hash]<=0) {gammas[hash]=0.0001;}  if (g==-1) count--; };
      /** Determine whether a hash has an associated gamma value. */
      bool hasGamma(unsigned int hash) const { return (gammas[hash]!=-1); };
      /** Get count of hashes that are set. */
      unsigned int getCount() { return count; };
    
    private:
      float *const gammas;
      unsigned int count;
  };
  
  /** Dictionary used for circular pattern hashing. */
  class CircularDictionary
  {
    public:
      CircularDictionary();
      
      /** @defgroup CircDictRotate Hash parts applicable when rotating.
       * @{
       */
      boost::uint_fast32_t rot_r2[PATTERN_CIRC_32BITPARTS]; /**< Shift right by 2. */
      boost::uint_fast32_t rot_r4[PATTERN_CIRC_32BITPARTS]; /**< Shift right by 4. */
      boost::uint_fast32_t rot_l6[PATTERN_CIRC_32BITPARTS]; /**< Shift left by 6. */
      boost::uint_fast32_t rot_l12[PATTERN_CIRC_32BITPARTS]; /**< Shift left by 12. */
      /** @} */
      /** @defgroup CircDictFlip Hash parts applicable when flipping.
       * @{
       */
      boost::uint_fast32_t flip_0[PATTERN_CIRC_32BITPARTS]; /**< Remain the same. */
      boost::uint_fast32_t flip_r2[PATTERN_CIRC_32BITPARTS]; /**< Shift right by 2. */
      boost::uint_fast32_t flip_r4[PATTERN_CIRC_32BITPARTS]; /**< Shift right by 4. */
      boost::uint_fast32_t flip_r6[PATTERN_CIRC_32BITPARTS]; /**< Shift right by 6. */
      boost::uint_fast32_t flip_r10[PATTERN_CIRC_32BITPARTS]; /**< Shift right by 10. */
      boost::uint_fast32_t flip_r14[PATTERN_CIRC_32BITPARTS]; /**< Shift right by 14. */
      boost::uint_fast32_t flip_l2[PATTERN_CIRC_32BITPARTS]; /**< Shift left by 2. */
      boost::uint_fast32_t flip_l4[PATTERN_CIRC_32BITPARTS]; /**< Shift left by 4. */
      boost::uint_fast32_t flip_l6[PATTERN_CIRC_32BITPARTS]; /**< Shift left by 6. */
      boost::uint_fast32_t flip_l10[PATTERN_CIRC_32BITPARTS]; /**< Shift left by 10. */
      boost::uint_fast32_t flip_l14[PATTERN_CIRC_32BITPARTS]; /**< Shift left by 14. */
      /** @} */
      
      /** Get a list of the x offsets applicable for a pattern of given size. */
      std::list<int> *getXOffsetsForSize(int size) { return &(dictx[size]); }; // XXX: no bounds checking!
      /** Get a list of the y offsets applicable for a pattern of given size. */
      std::list<int> *getYOffsetsForSize(int size) { return &(dicty[size]); }; // XXX: no bounds checking!
      /** Get the offest in the hash for the start of data specific to the given size. */
      int getBaseOffset(int size) { return baseoffset[size]; };
    
    private:
      std::list<int> dictx[PATTERN_CIRC_MAXSIZE+1];
      std::list<int> dicty[PATTERN_CIRC_MAXSIZE+1];
      int baseoffset[PATTERN_CIRC_MAXSIZE+1];
      
      void setTrans(boost::uint_fast32_t data[PATTERN_CIRC_32BITPARTS], int offset);
  };
  
  /** Circular Pattern. */
  class Circular
  {
    public:
      /** Create a pattern from a given board position. */
      Circular(Pattern::CircularDictionary *dict, const Go::Board *board, int pos, int sz);
      Circular(Pattern::CircularDictionary *dict, std::string fromString);
      Circular(boost::uint32_t hash_tmp[PATTERN_CIRC_32BITPARTS], int size_tmp):size(size_tmp) { memcpy(hash, hash_tmp, sizeof(boost::uint32_t)*PATTERN_CIRC_32BITPARTS); };
      
      /** Get the size of this pattern. */
      int getSize() const { return size; };
      /** Get the hash of this pattern. */
      boost::uint_fast32_t *getHash() const { return (boost::uint_fast32_t *)hash; };
      
      /** Make a copy of this pattern. */
      Pattern::Circular copy() { return this->getSubPattern(ldict,size); };
      /** Get a sub portion of this pattern. */
      Pattern::Circular getSubPattern(Pattern::CircularDictionary *dict, int newsize) const;
      
      /** Get a string representation of this pattern. */
      std::string toString(Pattern::CircularDictionary *dict) const;

      int sizefromString(std::string patternstring);
      
      /** Count the number of stones to get an idea of the uniqunes of the pattern */
      int countStones(Pattern::CircularDictionary *dict);
      
      /** Determine if two patterns are equal. */
      bool operator==(const Pattern::Circular other) const;
      /** Determine if two patterns are unequal. */
      bool operator!=(const Pattern::Circular other) const { return !(*this == other); };
      /** Determine if a pattern is smaller than another. */
      bool operator<(const Pattern::Circular other) const;
      /** Determine if a pattern is smaller than another. */
      bool operator<(const Pattern::Circular *other) const;
      
      /** Invert this pattern.
       * Black stones become white and vica versa. */
      void invert();
      /** Rotate this pattern 90 degress to the right. */
      void rotateRight(Pattern::CircularDictionary *dict);
      /** Flip this pattern horizontally. */
      void flipHorizontal(Pattern::CircularDictionary *dict);
      
      /** Compute the smallest equivalent pattern.
       * @see Pattern::ThreeByThree::smallestEquivalent()
       */
      void convertToSmallestEquivalent(Pattern::CircularDictionary *dict);
    
    private:
      Circular(int sz=0) : size(sz) {};
      
      const int size;
      boost::uint32_t hash[PATTERN_CIRC_32BITPARTS];  //the fast_uint_32_t can be 64 bit making toString not work
      
      static int hashColor(Go::Color col);
      static Go::Color hash2Color(int hash);
      void initColor(int offset, Go::Color col);
      Go::Color getColor(int offset);
      void resetColor(int offset);
      Pattern::CircularDictionary *ldict; //needed for copy?!
  };
};
#endif
