#ifndef DEF_OAKFOAM_PARAMETERS_H
#define DEF_OAKFOAM_PARAMETERS_H

#include <string>
#include <list>
//from "Engine.h":
class Engine;
//from "Random.h":
class Random;
#include "../gtp/Gtp.h"

class Parameters
{
  public:
    Parameters();
    ~Parameters();
    
    unsigned long rand_seed;
    
    Engine *engine;
    int board_size;
    
    enum MovePolicy
    {
      MP_PLAYOUT,
      MP_ONEPLY,
      MP_UCT
    };
    std::string move_policy_string;
    Parameters::MovePolicy move_policy;
    
    bool book_use;
    
    int thread_count;
    enum ThreadJob
    {
      TJ_GENMOVE,
      TJ_PONDER,
      TJ_DONPLTS
    };
    Parameters::ThreadJob thread_job;
    
    int playouts_per_move;
    int playouts_per_move_min;
    int playouts_per_move_max;
    
    bool playout_atari_enabled;
    bool playout_lastcapture_enabled;
    bool playout_patterns_enabled;
    bool playout_features_enabled;
    bool playout_features_incremental;
    bool playout_lastatari_enabled;
    bool playout_nakade_enabled;
    bool playout_fillboard_enabled;
    int playout_fillboard_n;
    bool playout_anycapture_enabled;
    bool playout_lgrf1_enabled;
    bool playout_lgrf2_enabled;
    bool playout_mercy_rule_enabled;
    float playout_mercy_rule_factor;
    float playout_random_chance;
    
    float ucb_c;
    float ucb_init;
    
    int rave_moves;
    int rave_init_wins;
    
    int uct_expand_after;
    bool uct_keep_subtree;
    bool uct_symmetry_use;
    bool uct_virtual_loss;
    int uct_atari_prior;
    int uct_pattern_prior;
    bool uct_progressive_widening_enabled;
    float uct_progressive_widening_a;
    float uct_progressive_widening_b;
    float uct_progressive_widening_c;
    bool uct_progressive_widening_count_wins;
    float uct_points_bonus;
    float uct_length_bonus;
    bool uct_progressive_bias_enabled;
    float uct_progressive_bias_h;
    bool uct_progressive_bias_scaled;
    bool uct_progressive_bias_relative;
    float uct_criticality_urgency_factor;
    bool uct_criticality_urgency_decay;
    float uct_criticality_unprune_factor;
    bool uct_criticality_unprune_multiply;
    int uct_criticality_min_playouts;
    bool uct_criticality_siblings;
    
    int uct_slow_update_interval;
    int uct_slow_update_last;
    
    bool uct_stop_early;
    float uct_last_r2;
    
    bool pondering_enabled;
    int pondering_playouts_max;
    
    bool interrupts_enabled;
    
    bool rules_positional_superko_enabled;
    bool rules_superko_top_ply;
    int rules_superko_prune_after;
    bool rules_superko_at_playout;
    
    float surewin_threshold;
    bool surewin_expected;
    float surewin_pass_bonus;
    
    float resign_ratio_threshold;
    float resign_move_factor_threshold;
    
    float time_k;
    float time_buffer;
    float time_move_minimum;
    bool time_ignore;
    
    bool livegfx_on;
    int livegfx_update_playouts;
    float livegfx_delay;
    
    int outputsgf_maxchildren;
    
    bool debug_on;
    
    bool features_only_small;
    bool features_output_competitions;
    bool features_output_competitions_mmstyle;
    bool features_ordered_comparison;
    
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
