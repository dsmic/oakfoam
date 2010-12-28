#ifndef DEF_OAKFOAM_FEATURES_H
#define DEF_OAKFOAM_FEATURES_H

#define PASS_LEVELS 2
#define CAPTURE_LEVELS 1
#define EXTENSION_LEVELS 1
#define SELFATARI_LEVELS 1
#define ATARI_LEVELS 2
#define BORDERDIST_LEVELS 4
#define LASTDIST_LEVELS 10
#define SECONDLASTDIST_LEVELS 10

#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <iomanip>
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
      LASTDIST,
      SECONDLASTDIST,
      PATTERN3X3,
      INVALID
    };
    
    Features();
    ~Features();
    
    unsigned int matchFeatureClass(Features::FeatureClass featclass, Go::Board *board, Go::Move move);
    float getFeatureGamma(Features::FeatureClass featclass, unsigned int level);
    float getMoveGamma(Go::Board *board, Go::Move move);
    float getBoardGamma(Go::Board *board, Go::Color col);
    std::string getFeatureClassName(Features::FeatureClass featclass);
    Features::FeatureClass getFeatureClassFromName(std::string name);
    bool setFeatureGamma(Features::FeatureClass featclass, unsigned int level, float gamma);
    
    std::string getMatchingFeaturesString(Go::Board *board, Go::Move move, bool pretty=true);
    std::string getFeatureIdList();
    
    bool loadGammaLine(std::string line);
    bool loadGammaFile(std::string filename);
    //bool loadGammaDefaults(); //todo
    
  
  private:
    Pattern::ThreeByThreeGammas *patterngammas;
    Pattern::ThreeByThreeGammas *patternids;
    float gammas_pass[PASS_LEVELS];
    float gammas_capture[CAPTURE_LEVELS];
    float gammas_extension[EXTENSION_LEVELS];
    float gammas_selfatari[SELFATARI_LEVELS];
    float gammas_atari[ATARI_LEVELS];
    float gammas_borderdist[BORDERDIST_LEVELS];
    float gammas_lastdist[LASTDIST_LEVELS];
    float gammas_secondlastdist[SECONDLASTDIST_LEVELS];
    
    float *getStandardGamma(Features::FeatureClass featclass);
    void updatePatternIds();
  
};

#endif
