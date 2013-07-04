#ifndef DEF_OAKFOAM_FEATURES_H
#define DEF_OAKFOAM_FEATURES_H

#define PASS_LEVELS 2
#define CAPTURE_LEVELS 6
#define EXTENSION_LEVELS 2
#define SELFATARI_LEVELS 2
#define ATARI_LEVELS 3
#define BORDERDIST_LEVELS 4
#define LASTDIST_LEVELS 10
#define SECONDLASTDIST_LEVELS 10
#define CFGLASTDIST_LEVELS 10
#define CFGSECONDLASTDIST_LEVELS 10

#include <string>
#include <set>
#include <map>
#include <vector>

#include "Go.h"
//from "Parameters.h":
class Parameters;
//from "Pattern.h":
/*namespace Pattern
{
  class ThreeByThreeGammas;
  class Circular;
  class CircularDictionary;
};
*/
#include "Pattern.h"
#include "DecisionTree.h"

/** ELO Features.
 * Class to manage extracting, learning, and using feature weights.
 *
 * Typically, a move's gamma value is determined using these features.
 * This is done by checking if each of the feature classes match for the move.
 * Feature classes can have multiple levels.
 * When there are multiple feature levels that match, the largest level is used.
 * The gamma value for the move is the multiple of the matching features' gamma values.
 *
 * Feature level descriptions:
 *  - PASS
 *    - 1: The move is a pass.
 *    - 2: The move and previous move are passes.
 *  - CAPTURE
 *    - 1: The move is a capture.
 *    - 2: The move is a capture of a group in a broken ladder.
 *    - 3: The move is a capture that prevents the opponent playing a connection here.
 *    - 4: The move re-captures a group that just caused a capture.
 *    - 5: The move is a capture of a group adjacent to another group in atari.
 *    - 6: The move is a capture of a group, of 10 or more stones, adjacent to another group in atari.
 *  - EXTENSION
 *    - 1: The move extends a group in atari.
 *    - 2: The move extends a group in atari and a working ladder.
 *  - SELFATARI
 *    - 1: The move is a self-atari.
 *    - 2: The move is a self-atari that matches the playout_avoid_selfatari heuristic.
 *  - ATARI
 *    - 1: The move is an atari.
 *    - 2: The move is an atari and there is an active ko.
 *    - 3: The move is an atari on a group in a broken ladder.
 *  - BORDERDIST
 *    - x: The move is x away from the border.
 *  - LASTDIST
 *    - x: The move is x away from the last move (Circular distance).
 *  - SECONDLASTDIST
 *    - x: The move is x away from the second last move (Circular distance).
 *  - CFGLASTDIST
 *    - x: The move is x away from the last move (CFG distance).
 *  - CFGSECONDLASTDIST
 *    - x: The move is x away from the second last move (CFG distance).
 *  - PATTERN3X3
 *    - x: The move has a 3x3 pattern hash of x.
 *  - CIRCPATT
 *    - x: The move has the circular pattern x.
 */
class Features
{
  public:
    /** The different classes of features. */
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
      CFGLASTDIST,
      CFGSECONDLASTDIST,
      PATTERN3X3,
      CIRCPATT,
      INVALID
    };
    
    /** Create a Features object with the global parameters. */
    Features(Parameters *prms);
    ~Features();
    
    /** Check for a feature match and return the matched level.
     * The @p move is check for a match against @p featclass on @p board.
     * A return value of zero, means that the feature was not matched.
     */
    unsigned int matchFeatureClass(Features::FeatureClass featclass, Go::Board *board, Go::ObjectBoard<int> *cfglastdist, Go::ObjectBoard<int> *cfgsecondlastdist, Go::Move move, bool checkforvalidmove=true) const;
    /** Return the gamma weight for a specific feature and level. */
    float getFeatureGamma(Features::FeatureClass featclass, unsigned int level) const;
    /** Return the weight for a move.
     * The weight for a move is the product of matching feature weights for that move.
     */
    float getMoveGamma(Go::Board *board, Go::ObjectBoard<int> *cfglastdist, Go::ObjectBoard<int> *cfgsecondlastdist, DecisionTree::GraphCollection *graphs, Go::Move move, bool checkforvalidmove=true, bool withcircularpatterns=true) const;
    bool learnMovesGamma(Go::Board *board, Go::ObjectBoard<int> *cfglastdist, Go::ObjectBoard<int> *cfgsecondlastdist, std::map<float,Go::Move,std::greater<float> > ordervalue, std::map<int,float> move_gamma, float sum_gammas);
    bool learnMoveGamma(Go::Board *board, Go::ObjectBoard<int> *cfglastdist, Go::ObjectBoard<int> *cfgsecondlastdist, Go::Move move, float learn_diff);
    int learnMoveGammaC(Go::Board *board, Go::ObjectBoard<int> *cfglastdist, Go::ObjectBoard<int> *cfgsecondlastdist, Go::Move move, float learn_diff);
    /** Return the total of all gammas for the moves on a board. */
    float getBoardGamma(Go::Board *board, Go::ObjectBoard<int> *cfglastdist, Go::ObjectBoard<int> *cfgsecondlastdist, DecisionTree::GraphCollection *graphs, Go::Color col) const;
    /** Return the total of all gammas for the moves on a board and each move's weight in @p gammas. */
    float getBoardGammas(Go::Board *board, Go::ObjectBoard<int> *cfglastdist, Go::ObjectBoard<int> *cfgsecondlastdist, DecisionTree::GraphCollection *graphs, Go::Color col, Go::ObjectBoard<float> *gammas) const;
    /** Return the human-readable name for a feature class. */
    std::string getFeatureClassName(Features::FeatureClass featclass) const;
    /** Return the feature class, given a name. */
    Features::FeatureClass getFeatureClassFromName(std::string name) const;
    /** Set the gamma value for a specific feature and level. */
    bool setFeatureGamma(Features::FeatureClass featclass, unsigned int level, float gamma);
    void learnFeatureGammaMoves(Features::FeatureClass featclass, Go::Board *board, Go::ObjectBoard<int> *cfglastdist, Go::ObjectBoard<int> *cfgsecondlastdist, std::map<float,Go::Move,std::greater<float> > ordervalue, std::map<int,float> move_gamma, float sum_gammas);
    void learnFeatureGamma(Features::FeatureClass featclass, unsigned int level, float learn_diff);
    int learnFeatureGammaC(Features::FeatureClass featclass, unsigned int level, float learn_diff);
    
    /** Return a string of all the matching features for a move. */ 
    std::string getMatchingFeaturesString(Go::Board *board, Go::ObjectBoard<int> *cfglastdist, Go::ObjectBoard<int> *cfgsecondlastdist, DecisionTree::GraphCollection *graphs, Go::Move move, bool pretty=true) const;
    /** Return a list of all valid features and levels. */
    std::string getFeatureIdList() const;
    
    /** Load a gamma value from a line. */
    bool loadGammaLine(std::string line);
    /** Load a file of gamma values. */
    bool loadGammaFile(std::string filename);
    bool saveGammaFile(std::string filename);
    bool saveGammaFileInline(std::string filename);
    bool loadCircFile(std::string filename,int numlines);
    bool loadCircFileNot(std::string filename,int numlines);
    bool saveCircValueFile(std::string filename);
    bool loadCircValueFile(std::string filename);
    /** Load a number of lines of gamma values. */
    bool loadGammaString(std::string lines);
    /** Load the default gamma values. */
    void loadGammaDefaults();
    
    /** Return the CFG distances for the last and second last moves on a board. */
    void computeCFGDist(Go::Board *board, Go::ObjectBoard<int> **cfglastdist, Go::ObjectBoard<int> **cfgsecondlastdist);

    /** Return a structure with the gammas for the 3x3 patterns. */
    Pattern::ThreeByThreeGammas* getPatternGammas() {return patterngammas;}
    /** Return the circular dictionary. */
    Pattern::CircularDictionary *getCircDict() { return circdict; };
    /** Determine if a circular pattern is present. */
    bool hasCircPattern(Pattern::Circular *pc);

    bool isCircPattern(std::string circpattern) const;
    float valueCircPattern(std::string circpattern) const;
    void learnCircPattern(std::string circpattern,float delta);
    int getCircSize () {return circpatternsize;}
    
  private:
    Parameters *const params;
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
    float gammas_cfglastdist[CFGLASTDIST_LEVELS];
    float gammas_cfgsecondlastdist[CFGSECONDLASTDIST_LEVELS];

    Pattern::CircularDictionary *circdict; 
    std::map<Pattern::Circular,unsigned int> *circlevels;
    std::vector<std::string> *circstrings;
    std::vector<float> *circgammas;

    float *getStandardGamma(Features::FeatureClass featclass) const;
    void updatePatternIds();

    std::map<std::string,long int> circpatterns;
    std::map<std::string,long int> circpatternsnot;
    std::map<std::string,float> circpatternvalues;
    int circpatternsize;
    long int num_circmoves;
    long int num_circmoves_not;
};

#endif
