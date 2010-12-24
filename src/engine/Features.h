#ifndef DEF_OAKFOAM_FEATURES_H
#define DEF_OAKFOAM_FEATURES_H

#include "Go.h"

class Features
{
  public:
    enum FeatureClass
    {
      PASS,
      CAPTURE,
      EXTENSION,
      //SELFATARI,
      ATARI
    };
    
    int matchFeatureClass(Features::FeatureClass featclass, Go::Board *board, Go::Move move);
    float getFeatureGamma(Features::FeatureClass featclass, int level);
    float getMoveGamma(Go::Board *board, Go::Move move);
    float getBoardGamma(Go::Board *board, Go::Color col);
  
  private:
    
  
};

#endif
