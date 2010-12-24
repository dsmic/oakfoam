#ifndef DEF_OAKFOAM_FEATURES_H
#define DEF_OAKFOAM_FEATURES_H

#include "Go.h"
#include "Pattern.h"

class Features
{
  public:
    enum FeatureClass
    {
      PASS,
      CAPTURE,
      EXTENSION,
      SELFATARI,
      ATARI,
      BORDERDIST,
      PATTERN3X3
    };
    
    unsigned int matchFeatureClass(Features::FeatureClass featclass, Go::Board *board, Go::Move move);
    float getFeatureGamma(Features::FeatureClass featclass, unsigned int level);
    float getMoveGamma(Go::Board *board, Go::Move move);
    float getBoardGamma(Go::Board *board, Go::Color col);
  
  private:
    
  
};

#endif
