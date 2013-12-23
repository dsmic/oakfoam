#ifndef DEF_OAKFOAM_PARAMETERS_H
#define DEF_OAKFOAM_PARAMETERS_H

#include "config.h"
#include <string>
#include <list>
#include <boost/thread/thread.hpp>
//from "Engine.h":
class Engine;
//from "Random.h":
class Random;
#include "../gtp/Gtp.h"

/** Parameter managment.
 * It is generally accepted that most of these should be set via GTP.
 * A "clear_board" command should usually be issued after these changes, to clear the MCTS tree.
 */
class Parameters
{
  public:
    Parameters();
    ~Parameters();
    
    /** Initial random seed.
     * Set this to zero to select a new default seed.
     */
    unsigned long rand_seed;
    
    /** Engine associated with these parameters. */
    Engine *engine;
    /** Current board size in use. */
    int board_size;
    
    /** Move policies that the engine can follow. */
    enum MovePolicy
    {
      MP_PLAYOUT,
      MP_ONEPLY,
      MP_UCT
    };
    /** String representation of the current move policy. */
    std::string move_policy_string;
    /** Current move policy. */
    Parameters::MovePolicy move_policy;
   
    /** Whether to make use of the currently loaded opening book. */
    bool book_use;
    
    /** Number of threads to use. */
    int thread_count;
    /** Jobs that the threads can perform. */
    enum ThreadJob
    {
      TJ_GENMOVE,
      TJ_PONDER,
      TJ_DONPLTS
    };
    /** Current thread job. */
    Parameters::ThreadJob thread_job;

    /** Number of Tree instances in memory, used to keep track of approximate memory usage. */
    long tree_instances;
    /** Maximum amount of memory usage.
     * This is compared to the number of Tree instances, so it is only approximate.
     * Units are megabytes.
     */
    unsigned long memory_usage_max;
    
    /** Number of playouts per move to perform.
     * Not used if time setting are in use.
     */
    int playouts_per_move;
    /** Minimum number of playouts per move.
     * Used when time settings are in use.
     */
    int playouts_per_move_min;
    /** Maximum number of playouts per move.
     * Used when time setting are in use.
     */
    int playouts_per_move_max;
    
    /** Whether to use the atari heuristic in playouts.
     * If the last move is next to, or part of, a group now in atari, either capture that group or try to extend it.
     * Don't extend if the group would remain in atari.
     */
    bool playout_atari_enabled;
    /** Whether to use the lastcapture heuristic in playouts.
     * If the last move was an atari, try to capture an adjacent group.
     */
    bool playout_lastcapture_enabled;
    /** The probablity of using the pattern heuristic in playouts.
     * If any moves around the last two moves match a pattern, play one of them.
     */
    float playout_patterns_p;
    float playout_patterns_gammas_p;
    /** Whether to use the features heuristic in playouts.
     * The feature gamma of a move over the sum of the gammas is the probability of that move being played.
     */
    bool playout_features_enabled;
    /** Whether to try to update the feature gammas incrementally. */
    bool playout_features_incremental;
    /** The probability of using the lastatari heuristic in playouts.
     * If the last move was an atari, try to extend the group in atari.
     */
    float playout_lastatari_p;
    /** Whether to skip using the lastatari heuristic if two or more groups are in atari, and the group causing the atari is in atari itself. */
    bool playout_lastatari_leavedouble;
    /** The probability of capturing an attached group instead of extending. */
    float playout_lastatari_captureattached_p;
    /** Whether to use the nakade heuristic in playouts.
     * If the last move created an eye of size 3, play in the center of that eye.
     */
    bool playout_nakade_enabled;
    bool playout_nakade4_enabled;
    bool playout_nakade_bent4_enabled;
    bool playout_nakade5_enabled;
    /** Whether to use the fillboard heuristic in playouts.
     * Randomly select a position on the board and play there if there are no surrounding stones.
     */
    bool playout_fillboard_enabled;
    /** Number of times to try the fillboard heuristic before continuing. */
    int playout_fillboard_n;
    /** replace a fillboard move with a neighbouring circpattern move*/
    bool playout_circreplace_enabled;
    bool playout_fillboard_bestcirc_enabled;
    int playout_randomquick_bestcirc_n;
    int playout_random_weight_territory_n;
    float playout_random_weight_territory_f0;
    float playout_random_weight_territory_f1;
    float playout_random_weight_territory_f;
    /** Number of times to try the circpattern heuristic before continuing. */
    int playout_circpattern_n;
    /** The probability of using the anycapture heuristic in playouts.
     * If any groups are in atari, capture one at random.
     */
    float playout_anycapture_p;
    /** Whether to use the lgrf1 heuristic in playouts.
     * Play a move according to LGRF-1, if such a move is available and legal.
     */
    bool playout_lgrf1_enabled;
    bool playout_lgrf_local;
    bool playout_lgrf1_safe_enabled;

    float playout_avoid_lbrf1_p;
    float playout_avoid_lbmf_p;
    float playout_avoid_lbrf1_p2;
    float playout_avoid_lbmf_p2;

    //own followup moves, independent of the inbetween move of opponent
    bool playout_lgrf1o_enabled;
    
    /** Whether to use the lgrf2 heuristic in playouts.
     * Play a move according to LGRF-2, if such a move is available and legal.
     */
    bool playout_lgrf2_enabled;
    bool playout_lgrf2_safe_enabled;

    bool playout_lgpf_enabled;
    
    /** Whether to use the mercy rule in playouts.
     * The mercy rule stops playouts where one color has captured many more prisoners and declares the playout a win for that color.
     */
    bool playout_mercy_rule_enabled;
    /** Factor between prisoners' difference and the board area.
     * @see playout_mercy_rule_enabled
     */
    float playout_mercy_rule_factor;

    bool dynkomi_enabled;

    bool mm_learn_enabled;
    float mm_learn_delta;
    int mm_learn_min_playouts;
    
/*    float test_p1;
    float test_p2;
    float test_p3;
    float test_p4;
    float test_p5;
    float test_p6;
    float test_p7;
    float test_p8;
    float test_p9;
    float test_p10;

     float test_p11;
    float test_p12;
    float test_p13;
    float test_p14;
    float test_p15;
    float test_p16;
    float test_p17;
    float test_p18;
    float test_p19;
    float test_p20;
    */

    
    /** Skip all playout heuristics with this probability.
     * Set to zero to disable.
     */
    float playout_random_chance;
    /** Probability of replacing a random  self-atari move with an approach move. */
    float playout_random_approach_p;
    /** Whether to use the last2libatari heuristic in playouts.
     * When the last move reduced a group to 2 liberties, and is near one of those liberties, play on one of them.
     */
    bool playout_last2libatari_enabled;
    /** Whether to try and play on the best of the 2 liberties.
     * @see playout_last2libatari_enabled
     */
    bool playout_last2libatari_complex;
    /** Whether to use the poolRAVE modification. */
    bool playout_poolrave_enabled;
    /** Whether to use the poolCriticality modification.
     * Similar to poolRAVE, but using criticality.
     */
    bool playout_poolrave_criticality;
    int playout_criticality_random_n;
    /** Chance of using poolRAVE.
     * @see playout_poolrave_enabled
     */
    float playout_poolrave_p;
    /** Pool size of poolRAVE.
     * @see playout_poolrave_enabled
     */
    int playout_poolrave_k;
    /** Minimum playouts for poolRAVE.
     * @see playout_poolrave_enabled
     */
    int playout_poolrave_min_playouts;
    /** Whether to use the avoid self-atari in playouts.
     * Avoid self-atari at almost all cost, like the eye-filling rule.
     */
    bool playout_avoid_selfatari;
    /** Only avoid self-atari of groups of this size or larger. */
    int playout_avoid_selfatari_size;
    bool playout_avoid_selfatari_complex;
    /** The useless move heuristic from the Crazy Stone paper. */
    bool playout_useless_move;
    /** Integer which allows different playout orders to be tested.
     * Currently under constuction.
     */
    int playout_order;

    /** Generate a move only within ~3 intersections of the last move.
     */
    bool playout_nearby_enabled;
    /** Fill weak eyes if there are no other moves (besides passing). */
    bool playout_fill_weak_eyes;
    
    /** UCB exploration constant. */
    float ucb_c;
    /** Initial UCB bias.
     * Used when no playouts have gone through a node.
     */
    float ucb_init;

    // Bernolli distribution experiments.
    // Under Construction
    float bernoulli_a;
    float bernoulli_b;
    float weight_score;
    float random_f;
    
    /** Number of moves it takes for RAVE to decay?
     * Set to zero to disable RAVE.
     */
    int rave_moves;
    /** Number of wins added to the RAVE statistics initially. */
    int rave_init_wins;
    float uct_preset_rave_f;
    /** Probability that the RAVE heuristic is ignored.
     * If triggered, RAVE is ignored for the selection of an urgent child.
     */
    float rave_skip;
    /** Adjust the number of moves to be used for RAVE.
     * Only the first x moves are used where x = rave_moves_use*board_size^2.
     * Set to zero to disable.
     */
    float rave_moves_use;
    
    /** After this many playouts through a node, it will be expanded. */
    int uct_expand_after;
    /** Whether to keep the sub-tree when a move is made.
     * It does not make sense to enable pondering if this is not set.
     */
    bool uct_keep_subtree;
    /** Whether to make use of symmetry to reduce the initial branching factor. */
    bool uct_symmetry_use;
    /** Whether to add a virtual loss on descent and remove it after updating the tree.
     * This greatly improves performance in a multi-core setup.
     */
    bool uct_virtual_loss;
    /** Whether to use a lock-free concurrency strategy. */
    bool uct_lock_free;
    /** Number of prior wins to add to atari moves in the tree. */
    int uct_atari_prior;
    int uct_playoutmove_prior;
    /** Number of prior wins to add to moves in the tree that match a pattern. */
    int uct_pattern_prior;
    /** Whether to make use of progressive widening.
     * When enabled, all node children are initially soft-pruned and the slowly unpruned, or widened, as playout pass through the node.
     * @todo Include relevant formula.
     */
    bool uct_progressive_widening_enabled;
    /** Number of nodes to unprune when a node is expanded.
     * @see uct_progressive_widening_enabled
     */
    int uct_progressive_widening_init;
    /** Constant for progressive widening.
     * @see uct_progressive_widening_enabled
     */
    float uct_progressive_widening_a;
    /** Constant for progressive widening.
     * @see uct_progressive_widening_enabled
     */
    float uct_progressive_widening_b;
    /** Constant for progressive widening.
     * @see uct_progressive_widening_enabled
     */
    float uct_progressive_widening_c;
    /** Parameter for progressive widening.
     * @see uct_progressive_widening_enabled
     */
    bool uct_progressive_widening_count_wins;
    /** Constant for a bonus given to playouts, depending on the score difference.
     * Set to zero to disable.
     */
    float uct_points_bonus;
    /** Constant for a bonus given to playouts, depending on the game length.
     * Set to zero to disable.
     */
    float uct_length_bonus;
    /** Whether to make use of progressive bias.
     * When enabled, a bias is added to nodes' urgencies.
     * This bias decays as playouts pass through the node.
     * @todo Include relevant formula.
     */
    bool uct_progressive_bias_enabled;
    /** Constant for progressive bias.
     * @see uct_progressive_bias_enabled
     */
    float uct_progressive_bias_h;
    /** Constant for progressive bias.
     * @see uct_progressive_bias_enabled
     */
    bool uct_progressive_bias_scaled;
    /** Constant for progressive bias.
     * @see uct_progressive_bias_enabled
     */
    bool uct_progressive_bias_relative;
    /** Constant for adding a bias to urgency based on criticality.
     * Set to zero to disable.
     * @todo Include relevant formula.
     */

    float uct_progressive_bias_moves;
    float uct_progressive_bias_exponent;
    
    float uct_criticality_urgency_factor;
    /** Parameter for criticality urgency.
     * @see uct_criticality_urgency_factor
     */
    float uct_criticality_urgency_decay;
    /** Constant for adjusting progressive widening based on criticality.
     * Set to zero to disable.
     * @todo Include relevant formula.
     */
    float uct_criticality_unprune_factor;
    /** Parameter for criticality unpruning.
     * @see uct_criticality_unprune_factor
     */
    bool uct_criticality_unprune_multiply;
    /** Constant for criticality unpruning.
     * @see uct_criticality_unprune_factor
     */
    int uct_criticality_min_playouts;
    /** Whether criticality updates should also affect siblings of the path to the root. */
    bool uct_criticality_siblings;
    /** Whether terminal nodes should be propogated up the tree. */

    float uct_criticality_rave_unprune_factor;
    
    bool uct_terminal_handling;
    /** Constant for adjusting progressive widening based on RAVE.
     * Set to zero to disable.
     * @todo Include relevant formula.
     */
    float uct_prior_unprune_factor;
    float uct_rave_unprune_factor;
    float uct_earlyrave_unprune_factor;
    float uct_rave_unprune_decay;
    /** Parameter for RAVE unpruning.
     * @see uct_rave_unprune_factor
     */
    float uct_reprune_factor;
    float uct_factor_circpattern;
    float uct_factor_circpattern_exponent;
    int uct_circpattern_minsize;
    float uct_simple_pattern_factor;
    float uct_atari_unprune;
    float uct_atari_unprune_exp;
    float uct_danger_value;
   
    bool uct_rave_unprune_multiply;
    float uct_oldmove_unprune_factor;
    float uct_oldmove_unprune_factor_b;
    float uct_oldmove_unprune_factor_c;

    float uct_area_owner_factor_a;
    float uct_area_owner_factor_b;
    float uct_area_owner_factor_c;
    
    
    /** Constant for decaying tree statistics.
     * Set to one to disable.
     * @todo Include relevant formula.
     */
    float uct_decay_alpha;
    /** Constant for decaying tree statistics.
     * Set to one to disable.
     * @todo Include relevant formula.
     */
    float uct_decay_k;
    /** Constant for decaying tree statistics.
     * @see uct_decay_k
     */
    float uct_decay_m;
    
    /** Number of playouts between slow updates.
     * Slow updates are things like checking if the best move can possible change.
     */
    int uct_slow_update_interval;
    /** When the last slow update occured. */
    int uct_slow_update_last;

    /** Number of playouts between debug info.
     * Set to zero to disable. */
    int uct_slow_debug_interval;
    /** When the last slow debug occured. */
    int uct_slow_debug_last;
    
    /** Whether to stop early if the best move cannot change. */
    bool uct_stop_early;
    /** Whether the last search stopped early. */
    bool early_stop_occured;
    /** Most recent ration between the playouts of the best node and second best. */
    float uct_last_r2;
    /** Number of playouts already in tree when current search started. */
    int uct_initial_playouts;
    /** Whether we are in the cleanup phase of a game.
     * This occurs on KGS after a scoring dispute.
     */
    bool cleanup_in_progress;
    
    /** Constant for decaying the territory map. */
    float territory_decayfactor;
    /** Threshold for determining owners in the territory map. */
    float territory_threshold;
    
    /** Whether to enable pondering.
     * Pondering is thinking during the opponent's turn.
     */
    bool pondering_enabled;
    /** Maximum number of playouts that pondering can do.
     * This value is compared with the number of playouts in the tree.
     */
    int pondering_playouts_max;
    
    /** Whether GTP commands can be interrupted.
     * This occurs when "#gogui-interrupt" is sent when another command it working.
     */
    bool interrupts_enabled;
    
    /** Whether positional superko is in effect. */
    bool rules_positional_superko_enabled;
    /** Whether superko should only be checked in the top tree ply. */
    bool rules_superko_top_ply;
    /** Number of playouts after which superko should be checked. */
    int rules_superko_prune_after;
    /** Whether to check superko when a playout passes through a node. */
    bool rules_superko_at_playout;
    /** Whether all stones are treated as alive.
     * Turning this off means that the territory map is used to determine alive and dead stones.
     * @see rules_all_stones_alive_playouts
     */
    bool rules_all_stones_alive;
    /** Minimum number of playouts for considering alive and dead stones. */
    int rules_all_stones_alive_playouts;
    
    /** Threshold at which to assume that we have a "sure win." */
    float surewin_threshold;
    /** Whether a sure win is currently expected. */
    bool surewin_expected;
    /** Bonus given to pass moves when a sure win is expected. */
    float surewin_pass_bonus;
    /** Bonus given to moves that are adjacent to dead stones when a sure win is expected. */
    float surewin_touchdead_bonus;
    /** Penalty given to moves that are in the opponent's area when a sure win is expected. */
    float surewin_oppoarea_penalty;
    
    /** Minimum ratio before the game is resigned. */
    float resign_ratio_threshold;
    /** Minimum portion of a game before before resignation is considered. */
    float resign_move_factor_threshold;
    
    /** Time management constant.
     * Time used ofr a move is remaining time divided by this constant.
     */
    float time_k;
    /** Amount of time to keep aside.
     * This is used to mitigate lag problems that can occur in the last few moves of a game.
     * Units are seconds.
     */
    float time_buffer;
    /** Minimum time per move.
     * Units are seconds.
     */
    float time_move_minimum;
    /** Whether to ignore the time settings that are specified.
     * If set, time settings will be ignored.
     */
    bool time_ignore;
    /** Maximum time to use per move.
     * Units are seconds.
     */
    float time_move_max;
    
    /** Whether to output GoGui Live Gfx during thinking. */
    bool livegfx_on;
    /** Number of playouts between each Live Gfx update. */
    int livegfx_update_playouts;
    /** Time delay added after a Live Gfx update.
     * Used to mitigate a GoGui issue.
     */
    float livegfx_delay;
    
    /** Number of children to output to SGF.
     * Set to zero to output all.
     */
    int outputsgf_maxchildren;
    
    /** Whether to output debug info. */
    bool debug_on;

    /** Is undo naively supported, or are undo commands ignored? */
    bool undo_enable;
    
    /** Whether to use only small features.
     * Meant to aid incremental features for playouts.
     */
    bool features_only_small;
    /** Whether to output feature competitions after each move.
     * Used for training feature gammas.
     */
    float features_output_competitions;
    /** Whether to output feature competitions in a form for the MM tool.
     * @see features_output_competitions
     */
    bool features_output_competitions_mmstyle;
    /** Whether to output a comparison of each move made with the features' ordering.
     * Used to measure feature accuracy.
     */
    bool features_ordered_comparison;
    /** Whether to include the log evidence in the feature comparison. */
    bool features_ordered_comparison_log_evidence;
    /** Whether to include the move number in the feature comparison. */
    bool features_ordered_comparison_move_num;
    /** Whether to try take ladders into account with features. */
    bool features_ladders;
    /** Whether to enable tactical features. */
    bool features_tactical;
    /** Whether to only use history-agnostic features. */
    bool features_history_agnostic;
    /** Whether to use decision trees with features. */
    bool features_dt_use;
    /** Probability that the circular patterns are listed after a move. */
    float features_circ_list;
    /** Size of circular patterns that are listed after a move.
     * @see features_circ_list
     */
    int features_circ_list_size;

    bool features_pass_no_move_for_lastdist;

    /** Whether to automatically output an SGF when a game finishes. */
    bool auto_save_sgf;
    /** The filename prefix for the outputted SGFs. */
    std::string auto_save_sgf_prefix;

    /** Query selection policies for growing decision trees. */
    enum QuerySelectionPolicy
    {
      SP_WIN_LOSS_SEPARATE,
      SP_WEIGHTED_WIN_LOSS_SEPARATE,
      SP_WINRATE_ENTROPY,
      SP_WEIGHTED_WINRATE_ENTROPY,
      SP_CLASSIFICATION_SEPARATE,
      SP_SYMMETRIC_SEPARATE,
      SP_WEIGHTED_SYMMETRIC_SEPARATE,
      SP_ROBUST_DESCENT_SPLIT,
      SP_ROBUST_WIN_SPLIT,
      SP_ROBUST_LOSS_SPLIT,
      SP_ENTROPY_DESCENT_SPLIT,
      SP_ENTROPY_WIN_SPLIT,
      SP_ENTROPY_LOSS_SPLIT,
      SP_WINRATE_SPLIT,
      SP_DESCENT_SPLIT
    };
    /** String representation of the current query selection policy. */
    std::string dt_selection_policy_string;
    /** Current query selection policy. */
    Parameters::QuerySelectionPolicy dt_selection_policy;
    /** Probability that the decision trees are updated after a move. */
    float dt_update_prob;
    /** Number of descents that must occur before a decision tree node is split. */
    int dt_split_after;
    /** Whether to return only a single leaf node from each decision tree.
     * In practice, the leaf node with the smallest leaf id will be used.
     */
    bool dt_solo_leaf;
    /** Probability to output decision tree competitions after each move for the MM tool.
     * Used for training decision tree leaf weights.
     */
    float dt_output_mm;
    /** Whether to output a comparison of each move made with the decision trees' ordering.
     * Used to measure feature accuracy.
     */
    bool dt_ordered_comparison;
    
    #ifdef HAVE_MPI
      /** Period between MPI syncs.
       * Units are seconds.
       */
      double mpi_update_period;
      /** When the last MPI sync took place. */
      double mpi_last_update;
      /** Maximum tree depth to sync with MPI. */
      int mpi_update_depth;
      /** Threshold of playouts to sync with MPI.
       * Only node with at least this percentage of the total playouts are synced.
       */
      float mpi_update_threshold;
    #endif
    
    /** The function that is called when a parameter is updated. */
    typedef void (*UpdateFunction)(void *instance, std::string id);
    
    /** Add an int parameter. */
    void addParameter(std::string category, std::string id, int *ptr, int def, Parameters::UpdateFunction func=NULL, void *instance=NULL);
    /** Add a float parameter. */
    void addParameter(std::string category, std::string id, float *ptr, float def, Parameters::UpdateFunction func=NULL, void *instance=NULL);
    /** Add a double parameter. */
    void addParameter(std::string category, std::string id, double *ptr, double def, Parameters::UpdateFunction func=NULL, void *instance=NULL);
    /** Add a boolean  parameter. */
    void addParameter(std::string category, std::string id, bool *ptr, bool def, Parameters::UpdateFunction func=NULL, void *instance=NULL);
    /** Add a string parameter. */
    void addParameter(std::string category, std::string id, std::string *ptr, std::string def, Parameters::UpdateFunction func=NULL, void *instance=NULL);
    /** Add a list parameter. */
    void addParameter(std::string category, std::string id, std::string *ptr, std::list<std::string> *options, std::string def, Parameters::UpdateFunction func=NULL, void *instance=NULL);
    /** Add an unsigned long parameter. */
    void addParameter(std::string category, std::string id, unsigned long *ptr, unsigned long def, Parameters::UpdateFunction func=NULL, void *instance=NULL);
    
    /** Set the given parameter to the given value.
     * @return True if parameters was successfully set, otherwise false.
     */
    bool setParameter(std::string id, std::string val);
    /** Print parameters to the given Gtp engine for GoGui analyse commands. */
    void printParametersForGTP(Gtp::Engine *gtpe, std::string category="");
    /** Print parameters to the given Gtp engine as part of a description. */
    void printParametersForDescription(Gtp::Engine *gtpe);
    
  private:
    enum ParameterType
    {
      INTEGER,
      FLOAT,
      DOUBLE,
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
    bool setParameterDouble(Parameters::Parameter *param, std::string val);
    bool setParameterBoolean(Parameters::Parameter *param, std::string val);
    bool setParameterString(Parameters::Parameter *param, std::string val);
    bool setParameterList(Parameters::Parameter *param, std::string val);
    bool setParameterUnsignedLong(Parameters::Parameter *param, std::string val);
    
    void printParameterForGTP(Gtp::Engine *gtpe, Parameters::Parameter *param);
    void printParameterIntegerForGTP(Gtp::Engine *gtpe, Parameters::Parameter *param);
    void printParameterFloatForGTP(Gtp::Engine *gtpe, Parameters::Parameter *param);
    void printParameterDoubleForGTP(Gtp::Engine *gtpe, Parameters::Parameter *param);
    void printParameterBooleanForGTP(Gtp::Engine *gtpe, Parameters::Parameter *param);
    void printParameterStringForGTP(Gtp::Engine *gtpe, Parameters::Parameter *param);
    void printParameterListForGTP(Gtp::Engine *gtpe, Parameters::Parameter *param);
    void printParameterUnsignedLongForGTP(Gtp::Engine *gtpe, Parameters::Parameter *param);
    
    void printParameterForDescription(Gtp::Engine *gtpe, Parameters::Parameter *param);
    void printParameterIntegerForDescription(Gtp::Engine *gtpe, Parameters::Parameter *param);
    void printParameterFloatForDescription(Gtp::Engine *gtpe, Parameters::Parameter *param);
    void printParameterDoubleForDescription(Gtp::Engine *gtpe, Parameters::Parameter *param);
    void printParameterBooleanForDescription(Gtp::Engine *gtpe, Parameters::Parameter *param);
    void printParameterStringForDescription(Gtp::Engine *gtpe, Parameters::Parameter *param);
    void printParameterListForDescription(Gtp::Engine *gtpe, Parameters::Parameter *param);
    void printParameterUnsignedLongForDescription(Gtp::Engine *gtpe, Parameters::Parameter *param);
};

#endif
