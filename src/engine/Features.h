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

const std::string FEATURES_DEFAULT=
  "pass:1 0.0633337 \n"
  "pass:2 25.4255 \n"
  "capture:1 12.4151 \n"
  "extension:1 6.79605 \n"
  "selfatari:1 0.417892 \n"
  "atari:1 2.90135 \n"
  "atari:2 5.35719 \n"
  "borderdist:1 0.487576 \n"
  "borderdist:2 0.587389 \n"
  "borderdist:3 0.964131 \n"
  "borderdist:4 0.77074 \n"
  "lastdist:1 1 \n"
  "lastdist:2 3.94156 \n"
  "lastdist:3 2.19336 \n"
  "lastdist:4 1 \n"
  "lastdist:5 1 \n"
  "lastdist:6 1 \n"
  "lastdist:7 1 \n"
  "lastdist:8 1 \n"
  "lastdist:9 1 \n"
  "lastdist:10 1 \n"
  "secondlastdist:1 1 \n"
  "secondlastdist:2 2.38479 \n"
  "secondlastdist:3 1.76173 \n"
  "secondlastdist:4 1 \n"
  "secondlastdist:5 1 \n"
  "secondlastdist:6 1 \n"
  "secondlastdist:7 1 \n"
  "secondlastdist:8 1 \n"
  "secondlastdist:9 1 \n"
  "secondlastdist:10 1 \n"
  "pattern3x3:0x0000 0.560214 \n"
  "pattern3x3:0x0001 0.300812 \n"
  "pattern3x3:0x0002 0.575103 \n"
  "pattern3x3:0x0004 0.0440681 \n"
  "pattern3x3:0x0005 0.0342737 \n"
  "pattern3x3:0x0006 2.41131 \n"
  "pattern3x3:0x0008 0.778409 \n"
  "pattern3x3:0x0009 4.29016 \n"
  "pattern3x3:0x000a 0.125192 \n"
  "pattern3x3:0x0011 0.255391 \n"
  "pattern3x3:0x0012 0.536719 \n"
  "pattern3x3:0x0022 0.394319 \n"
  "pattern3x3:0x003f 0.00645382 \n"
  "pattern3x3:0x0044 0.0439393 \n"
  "pattern3x3:0x0046 2.16233 \n"
  "pattern3x3:0x0050 0.188748 \n"
  "pattern3x3:0x0056 1.44248 \n"
  "pattern3x3:0x0060 0.0970352 \n"
  "pattern3x3:0x0088 0.0341059 \n"
  "pattern3x3:0x0089 1.86381 \n"
  "pattern3x3:0x0090 0.839309 \n"
  "pattern3x3:0x00a0 0.515245 \n"
  "pattern3x3:0x00a9 0.692048 \n"
  "pattern3x3:0x0146 0.0732018 \n"
  "pattern3x3:0x0180 0.152934 \n"
  "pattern3x3:0x0195 0.149087 \n"
  "pattern3x3:0x0196 0.370403 \n"
  "pattern3x3:0x019a 0.206359 \n"
  "pattern3x3:0x01aa 0.206146 \n"
  "pattern3x3:0x0289 0.149411 \n"
  "pattern3x3:0x043f 0.0747944 \n"
  "pattern3x3:0x0528 0.429729 \n"
  "pattern3x3:0x0546 0.110487 \n"
  "pattern3x3:0x0564 0.452756 \n"
  "pattern3x3:0x0565 0.193772 \n"
  "pattern3x3:0x057f 0.539051 \n"
  "pattern3x3:0x0608 0.425 \n"
  "pattern3x3:0x0615 0.287897 \n"
  "pattern3x3:0x0649 0.224196 \n"
  "pattern3x3:0x083f 0.0512292 \n"
  "pattern3x3:0x0904 0.459621 \n"
  "pattern3x3:0x0918 1.58911 \n"
  "pattern3x3:0x092a 0.268186 \n"
  "pattern3x3:0x0986 0.273188 \n"
  "pattern3x3:0x0a89 0.308431 \n"
  "pattern3x3:0x0a98 0.197966 \n"
  "pattern3x3:0x0a9a 0.491793 \n"
  "pattern3x3:0x0abf 0.607719 \n"
  "pattern3x3:0x0cff 0.00491021 \n"
  "pattern3x3:0x0dc3 0.0442337 \n"
  "pattern3x3:0x0dc7 0.0517688 \n"
  "pattern3x3:0x0dd3 0.0367979 \n"
  "pattern3x3:0x0de3 0.793589 \n"
  "pattern3x3:0x0de7 2.17469 \n"
  "pattern3x3:0x0ec3 0.0569615 \n"
  "pattern3x3:0x0ecb 0.0628917 \n"
  "pattern3x3:0x0ed3 1.04942 \n"
  "pattern3x3:0x0edb 0.505529 \n"
  "pattern3x3:0x0ee3 0.0869913 \n"
  "pattern3x3:0x1146 0.106243 \n"
  "pattern3x3:0x193f 0.392318 \n"
  "pattern3x3:0x197f 0.875331 \n"
  "pattern3x3:0x19bf 3.97284 \n"
  "pattern3x3:0x263f 0.102137 \n"
  "pattern3x3:0x267f 1.08833 \n"
  "pattern3x3:0x443f 0.430603 \n"
  "pattern3x3:0x4456 0.519893 \n"
  "pattern3x3:0x4469 0.314501 \n"
  "pattern3x3:0x483f 0.295478 \n"
  "pattern3x3:0x492a 0.296867 \n"
  "pattern3x3:0x4cff 0.0822215 \n"
  "pattern3x3:0x4dd3 0.112411 \n"
  "pattern3x3:0x4ddb 1.44308 \n"
  "pattern3x3:0x4de3 0.757288 \n"
  "pattern3x3:0x4de7 2.23007 \n"
  "pattern3x3:0x4ee3 1.62963 \n"
  "pattern3x3:0x4eff 0.604023 \n"
  "pattern3x3:0x555a 0.619643 \n"
  "pattern3x3:0x5566 0.554974 \n"
  "pattern3x3:0x5965 0.204017 \n"
  "pattern3x3:0x597f 0.830255 \n"
  "pattern3x3:0x5aaa 1.61668 \n"
  "pattern3x3:0x5eff 0.656564 \n"
  "pattern3x3:0x6a22 0.364071 \n"
  "pattern3x3:0x6a3f 1.53226 \n"
  "pattern3x3:0x6abf 3.69554 \n"
  "pattern3x3:0x883f 0.203136 \n"
  "pattern3x3:0x8cff 0.0351649 \n"
  "pattern3x3:0x8ee3 0.144511 \n"
  "pattern3x3:0x8ee7 2.13367 \n"
  "pattern3x3:0x9eff 0.520427 \n"
  ;

#include "Go.h"
//from "Parameters.h":
class Parameters;
//from "Pattern.h":
namespace Pattern
{
  class ThreeByThreeGammas;
};

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
    
    Features(Parameters *prms);
    ~Features();
    
    unsigned int matchFeatureClass(Features::FeatureClass featclass, Go::Board *board, Go::Move move, bool checkforvalidmove=true);
    float getFeatureGamma(Features::FeatureClass featclass, unsigned int level);
    float getMoveGamma(Go::Board *board, Go::Move move, bool checkforvalidmove=true);
    float getBoardGamma(Go::Board *board, Go::Color col);
    float getBoardGammas(Go::Board *board, Go::Color col, Go::ObjectBoard<float> *gammas);
    std::string getFeatureClassName(Features::FeatureClass featclass);
    Features::FeatureClass getFeatureClassFromName(std::string name);
    bool setFeatureGamma(Features::FeatureClass featclass, unsigned int level, float gamma);
    
    std::string getMatchingFeaturesString(Go::Board *board, Go::Move move, bool pretty=true);
    std::string getFeatureIdList();
    
    bool loadGammaLine(std::string line);
    bool loadGammaFile(std::string filename);
    bool loadGammaString(std::string lines);
    void loadGammaDefaults() { this->loadGammaString(FEATURES_DEFAULT); };
  
  private:
    Parameters *params;
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
