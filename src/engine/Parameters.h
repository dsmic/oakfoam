#ifndef DEF_OAKFOAM_PARAMETERS_H
#define DEF_OAKFOAM_PARAMETERS_H

#include <string>
#include <list>
#include <sstream>
#include <algorithm>
#include "../gtp/Gtp.h"

class Parameters
{
  public:
    Parameters();
    ~Parameters();
    
    unsigned long rand_seed;
    
    int board_size;
    
    enum MovePolicy
    {
      MP_PLAYOUT,
      MP_ONEPLY,
      MP_UCT
    };
    std::string move_policy_string;
    Parameters::MovePolicy move_policy;
    
    int playouts_per_move;
    int playouts_per_move_min;
    int playouts_per_move_max;
    
    bool playout_atari_enabled;
    bool playout_patterns_enabled;
    
    float ucb_c;
    float ucb_init;
    
    int rave_moves;
    int rave_init_wins;
    
    int uct_expand_after;
    bool uct_keep_subtree;
    bool uct_symmetry_use;
    int uct_atari_prior;
    int uct_pattern_prior;
    
    float surewin_threshold;
    bool surewin_expected;
    
    float resign_ratio_threshold;
    float resign_move_factor_threshold;
    
    float time_k;
    float time_buffer;
    float time_move_minimum;
    
    bool livegfx_on;
    int livegfx_update_playouts;
    float livegfx_delay;
    
    int outputsgf_maxchildren;
    
    bool debug_on;
    
    typedef void (*UpdateFunction)(void *instance, std::string id);
    
    void addParameter(std::string category, std::string id, int *ptr, int def, Parameters::UpdateFunction func=NULL, void *instance=NULL);
    void addParameter(std::string category, std::string id, float *ptr, float def, Parameters::UpdateFunction func=NULL, void *instance=NULL);
    void addParameter(std::string category, std::string id, bool *ptr, bool def, Parameters::UpdateFunction func=NULL, void *instance=NULL);
    void addParameter(std::string category, std::string id, std::string *ptr, std::string def, Parameters::UpdateFunction func=NULL, void *instance=NULL);
    void addParameter(std::string category, std::string id, std::string *ptr, std::list<std::string> *options, std::string def, Parameters::UpdateFunction func=NULL, void *instance=NULL);
    void addParameter(std::string category, std::string id, unsigned long *ptr, unsigned long def, Parameters::UpdateFunction func=NULL, void *instance=NULL);
    
    bool setParameter(std::string id, std::string val);
    void printParametersForGTP(Gtp::Engine *gtpe, std::string category="");
    void printParametersForDescription(Gtp::Engine *gtpe);
    
  private:
    enum ParameterType
    {
      INTEGER,
      FLOAT,
      BOOLEAN,
      STRING,
      LIST,
      UNSIGNED_LONG
    };
    
    struct Parameter
    {
      std::string category;
      std::string id;
      void *ptr;
      Parameters::ParameterType type;
      std::list<std::string> *options;
      Parameters::UpdateFunction func;
      void *instance;
    };
    
    std::list<Parameters::Parameter *> paramlist;
    
    bool setParameterInteger(Parameters::Parameter *param, std::string val);
    bool setParameterFloat(Parameters::Parameter *param, std::string val);
    bool setParameterBoolean(Parameters::Parameter *param, std::string val);
    bool setParameterString(Parameters::Parameter *param, std::string val);
    bool setParameterList(Parameters::Parameter *param, std::string val);
    bool setParameterUnsignedLong(Parameters::Parameter *param, std::string val);
    
    void printParameterForGTP(Gtp::Engine *gtpe, Parameters::Parameter *param);
    void printParameterIntegerForGTP(Gtp::Engine *gtpe, Parameters::Parameter *param);
    void printParameterFloatForGTP(Gtp::Engine *gtpe, Parameters::Parameter *param);
    void printParameterBooleanForGTP(Gtp::Engine *gtpe, Parameters::Parameter *param);
    void printParameterStringForGTP(Gtp::Engine *gtpe, Parameters::Parameter *param);
    void printParameterListForGTP(Gtp::Engine *gtpe, Parameters::Parameter *param);
    void printParameterUnsignedLongForGTP(Gtp::Engine *gtpe, Parameters::Parameter *param);
    
    void printParameterForDescription(Gtp::Engine *gtpe, Parameters::Parameter *param);
    void printParameterIntegerForDescription(Gtp::Engine *gtpe, Parameters::Parameter *param);
    void printParameterFloatForDescription(Gtp::Engine *gtpe, Parameters::Parameter *param);
    void printParameterBooleanForDescription(Gtp::Engine *gtpe, Parameters::Parameter *param);
    void printParameterStringForDescription(Gtp::Engine *gtpe, Parameters::Parameter *param);
    void printParameterListForDescription(Gtp::Engine *gtpe, Parameters::Parameter *param);
    void printParameterUnsignedLongForDescription(Gtp::Engine *gtpe, Parameters::Parameter *param);
};

#endif
