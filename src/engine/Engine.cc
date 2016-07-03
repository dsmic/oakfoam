#include "Engine.h"

#include <cstdlib>
#include <cmath>
#include <vector>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <boost/timer.hpp>
#include <boost/lexical_cast.hpp>
#include <time.h>       /* clock_t, clock, CLOCKS_PER_SEC */


#ifdef HAVE_MPI
  #define MPIRANK0_ONLY(__body) {if (mpirank==0) { __body }}
#else
  #define MPIRANK0_ONLY(__body) { __body }
#endif
#include "Pattern.h"
#include "DecisionTree.h"
#ifdef HAVE_WEB
  #include "../web/Web.h"
#endif

//had trouble putting it as static variable into the Engine class, but should only be one Engine anyway!
#ifdef HAVE_CAFFE
#define num_nets 50
Net<float> *caffe_test_net[num_nets];
int caffe_test_net_input_dim[num_nets];
Net<float> *caffe_area_net;
bool disable_last_move_from_cnn;
bool double_second_last_move_from_cnn;
#endif		

Engine::Engine(Gtp::Engine *ge, std::string ln) : params(new Parameters())
{
  //cudaSetDeviceFlags(cudaDeviceBlockingSync);
  //Caffe::set_mode_gpu();
#ifdef HAVE_CAFFE
  Caffe::SetDevice(0);
  int t1=Caffe::mode();
  Caffe::set_mode(Caffe::GPU);
  int t2=Caffe::mode();
  fprintf(stderr,"%d %d\n",t1,t2);
  Caffe::DeviceQuery();
  //Caffe::set_phase(Caffe::TEST);
  caffe_area_net = NULL;
  for (int i=0;i<num_nets;i++)
    caffe_test_net[i] = NULL;
#endif
  
  ACcount=0;

  
  
  gtpe=ge;
  longname=ln;
  #ifdef HAVE_WEB
    web=NULL;
  #endif
  
  params->engine=this;
  
  #ifdef HAVE_MPI
    MPI::COMM_WORLD.Set_errhandler(MPI::ERRORS_ARE_FATAL);
    longname+=" (MPI)";
    mpiworldsize=MPI::COMM_WORLD.Get_size();
    mpirank=MPI::COMM_WORLD.Get_rank();
    mpihashtable.clear();
  #endif
  
  boardsize=9;
  params->board_size=boardsize;
  currentboard=new Go::Board(boardsize); //may be to early use of params, reload later with clear board?
  for (int i=0;i<historyboards_num;i++)
    historyboards[i]=NULL;
  komi=7.5;
  komi_handicap=0;
  recalc_dynkomi=0;

  deltawhiteoffset=boardsize*boardsize*(local_feature_num+hashto5num);
//  deltagammas = new std::atomic<float>[2*boardsize*boardsize*(local_feature_num+hashto5num)];
//  for (int i=0;i<2*boardsize*boardsize*(local_feature_num+hashto5num);i++) deltagammas[i]=1.0;
  deltagammaslocal = new float[2*boardsize*boardsize*(local_feature_num+hashto5num)];
  for (int i=0;i<2*boardsize*boardsize*(local_feature_num+hashto5num);i++) deltagammaslocal[i]=1.0;
  
  debug_solid_group=-1;

  params->tree_instances=0;
  
  zobristtable=new Go::ZobristTable(params,boardsize,ZOBRIST_HASH_SEED);
  
  params->addParameter("other","rand_seed",&(params->rand_seed),Random::makeSeed(),&Engine::updateParameterWrapper,this);
  
  std::list<std::string> *mpoptions=new std::list<std::string>();
  mpoptions->push_back("playout");
  mpoptions->push_back("1-ply");
  mpoptions->push_back("uct");
  mpoptions->push_back("cnn");
  params->addParameter("general","move_policy",&(params->move_policy_string),mpoptions,"uct",&Engine::updateParameterWrapper,this);
  params->move_policy=Parameters::MP_UCT;
  
  params->addParameter("general","book_use",&(params->book_use),BOOK_USE);
  
  params->addParameter("general","thread_count",&(params->thread_count),THREAD_COUNT,&Engine::updateParameterWrapper,this);
  params->addParameter("general","memory_usage_max",&(params->memory_usage_max),MEMORY_USAGE_MAX);
  
  params->addParameter("general","playouts_per_move",&(params->playouts_per_move),PLAYOUTS_PER_MOVE,&Engine::updateParameterWrapper,this);
  params->addParameter("general","playouts_per_move_max",&(params->playouts_per_move_max),PLAYOUTS_PER_MOVE_MAX);
  params->addParameter("general","playouts_per_move_min",&(params->playouts_per_move_min),PLAYOUTS_PER_MOVE_MIN);
  
  params->addParameter("playout","playout_criticality_random_n",&(params->playout_criticality_random_n),PLAYOUT_CRITICALITY_RANDOM_N);
  params->addParameter("playout","playout_poolrave_enabled",&(params->playout_poolrave_enabled),PLAYOUT_POOLRAVE_ENABLED);
  params->addParameter("playout","playout_poolrave_criticality",&(params->playout_poolrave_criticality),PLAYOUT_POOLRAVE_CRITICALITY);
  params->addParameter("playout","playout_poolrave_p",&(params->playout_poolrave_p),PLAYOUT_POOLRAVE_P);
  params->addParameter("playout","playout_poolrave_k",&(params->playout_poolrave_k),PLAYOUT_POOLRAVE_K);
  params->addParameter("playout","playout_poolrave_min_playouts",&(params->playout_poolrave_min_playouts),PLAYOUT_POOLRAVE_MIN_PLAYOUTS);
  params->addParameter("playout","playout_lgrf2_enabled",&(params->playout_lgrf2_enabled),PLAYOUT_LGRF2_ENABLED);
  params->addParameter("playout","playout_lgrf1_enabled",&(params->playout_lgrf1_enabled),PLAYOUT_LGRF1_ENABLED);
  params->addParameter("playout","playout_lgrf_local",&(params->playout_lgrf_local),PLAYOUT_LGRF_LOCAL);
  params->addParameter("playout","playout_lgrf2_safe_enabled",&(params->playout_lgrf2_safe_enabled),PLAYOUT_LGRF2_SAFE_ENABLED);
  params->addParameter("playout","playout_lgrf1_safe_enabled",&(params->playout_lgrf1_safe_enabled),PLAYOUT_LGRF1_SAFE_ENABLED);
  params->addParameter("playout","playout_lgrf1o_enabled",&(params->playout_lgrf1o_enabled),PLAYOUT_LGRF1O_ENABLED);
  params->addParameter("playout","playout_avoid_lbrf1_p",&(params->playout_avoid_lbrf1_p),PLAYOUT_AVOID_LBRF1_P);
  params->addParameter("playout","playout_avoid_lbmf_p",&(params->playout_avoid_lbmf_p),PLAYOUT_AVOID_LBMF_P);
  params->addParameter("playout","playout_avoid_lbrf1_p2",&(params->playout_avoid_lbrf1_p2),PLAYOUT_AVOID_LBRF1_P2);
  params->addParameter("playout","playout_avoid_lbmf_p2",&(params->playout_avoid_lbmf_p2),PLAYOUT_AVOID_LBMF_P2);
  params->addParameter("playout","playout_avoid_bpr_p",&(params->playout_avoid_bpr_p),PLAYOUT_AVOID_BPR_P);
  params->addParameter("playout","playout_avoid_bpr_p2",&(params->playout_avoid_bpr_p2),PLAYOUT_AVOID_BPR_P2);
  params->addParameter("playout","playout_lgpf_enabled",&(params->playout_lgpf_enabled),PLAYOUT_LGPF_ENABLED);
  params->addParameter("playout","playout_atari_enabled",&(params->playout_atari_enabled),PLAYOUT_ATARI_ENABLED);
  params->addParameter("playout","playout_lastatari_p",&(params->playout_lastatari_p),PLAYOUT_LASTATARI_P);
  params->addParameter("playout","playout_lastatari_leavedouble",&(params->playout_lastatari_leavedouble),PLAYOUT_LASTATARI_LEAVEDOUBLE);
  params->addParameter("playout","playout_lastatari_captureattached_p",&(params->playout_lastatari_captureattached_p),PLAYOUT_LASTATARI_CAPTUREATTACHED);
  params->addParameter("playout","playout_lastcapture_enabled",&(params->playout_lastcapture_enabled),PLAYOUT_LASTCAPTURE_ENABLED);
  params->addParameter("playout","playout_last2libatari_enabled",&(params->playout_last2libatari_enabled),PLAYOUT_LAST2LIBATARI_ENABLED);
  params->addParameter("playout","playout_last2libatari_complex",&(params->playout_last2libatari_complex),PLAYOUT_LAST2LIBATARI_COMPLEX);
  params->addParameter("playout","playout_last2libatari_allow_different_groups",&(params->playout_last2libatari_allow_different_groups),PLAYOUT_LAST2LIBATARI_ALLOW_DIFFERENT_GROUPS);
  params->addParameter("playout","playout_nakade_enabled",&(params->playout_nakade_enabled),PLAYOUT_NAKADE_ENABLED);
  params->addParameter("playout","playout_nakade2_enabled",&(params->playout_nakade2_enabled),1);
  params->addParameter("playout","playout_nakade4_enabled",&(params->playout_nakade4_enabled),PLAYOUT_NAKADE4_ENABLED);
  params->addParameter("playout","playout_nakade_bent4_enabled",&(params->playout_nakade_bent4_enabled),PLAYOUT_NAKADE_BENT4_ENABLED);
  params->addParameter("playout","playout_nakade5_enabled",&(params->playout_nakade5_enabled),PLAYOUT_NAKADE5_ENABLED);
  params->addParameter("playout","playout_nearby_enabled",&(params->playout_nearby_enabled),PLAYOUT_NEARBY_ENABLED);
  params->addParameter("playout","playout_fillboard_enabled",&(params->playout_fillboard_enabled),PLAYOUT_FILLBOARD_ENABLED);
  params->addParameter("playout","playout_fillboard_n",&(params->playout_fillboard_n),PLAYOUT_FILLBOARD_N);
  params->addParameter("playout","playout_fillboard_bestcirc_enabled",&(params->playout_fillboard_bestcirc_enabled),PLAYOUT_FILLBOARD_BESTCIRC_ENABLED);
  params->addParameter("playout","playout_randomquick_bestcirc_n",&(params->playout_randomquick_bestcirc_n),PLAYOUT_RANDOMQUICK_BESTCIRC_N);
  params->addParameter("playout","playout_random_weight_territory_n",&(params->playout_random_weight_territory_n),PLAYOUT_RANDOM_WEIGHT_TERRITORY_N);
  params->addParameter("playout","playout_random_weight_territory_f0",&(params->playout_random_weight_territory_f0),PLAYOUT_RANDOM_WEIGHT_TERRITORY_F0);
  params->addParameter("playout","playout_random_weight_territory_f1",&(params->playout_random_weight_territory_f1),PLAYOUT_RANDOM_WEIGHT_TERRITORY_F1);
  params->addParameter("playout","playout_random_weight_territory_f",&(params->playout_random_weight_territory_f),PLAYOUT_RANDOM_WEIGHT_TERRITORY_F);
  params->addParameter("playout","playout_circpattern_n",&(params->playout_circpattern_n),PLAYOUT_CIRCPATTERN_N);
  params->addParameter("playout","playout_patterns_p",&(params->playout_patterns_p),PLAYOUT_PATTERNS_P);
  params->addParameter("playout","playout_patterns_gammas_p",&(params->playout_patterns_gammas_p),PLAYOUT_PATTERNS_GAMMAS_P);
  params->addParameter("playout","playout_anycapture_p",&(params->playout_anycapture_p),PLAYOUT_ANYCAPTURE_P);
  params->addParameter("playout","playout_features_enabled",&(params->playout_features_enabled),PLAYOUT_FEATURES_ENABLED);
  params->addParameter("playout","playout_features_incremental",&(params->playout_features_incremental),PLAYOUT_FEATURES_INCREMENTAL);
  params->addParameter("playout","playout_random_chance",&(params->playout_random_chance),PLAYOUT_RANDOM_CHANCE);
  params->addParameter("playout","playout_random_approach_p",&(params->playout_random_approach_p),PLAYOUT_RANDOM_APPROACH_P);
  params->addParameter("playout","playout_defend_approach",&(params->playout_defend_approach),PLAYOUT_DEFEND_APPROACH);
  params->addParameter("playout","playout_avoid_selfatari",&(params->playout_avoid_selfatari),PLAYOUT_AVOID_SELFATARI);
  params->addParameter("playout","playout_avoid_selfatari_size",&(params->playout_avoid_selfatari_size),PLAYOUT_AVOID_SELFATARI_SIZE);
  params->addParameter("playout","playout_avoid_selfatari_complex",&(params->playout_avoid_selfatari_complex),PLAYOUT_AVOID_SELFATARI_COMPLEX);
  params->addParameter("playout","playout_useless_move",&(params->playout_useless_move),PLAYOUT_USELESS_MOVE);
  params->addParameter("playout","playout_order",&(params->playout_order),PLAYOUT_ORDER);
  params->addParameter("playout","playout_mercy_rule_enabled",&(params->playout_mercy_rule_enabled),PLAYOUT_MERCY_RULE_ENABLED);
  params->addParameter("playout","playout_mercy_rule_factor",&(params->playout_mercy_rule_factor),PLAYOUT_MERCY_RULE_FACTOR);
  params->addParameter("playout","playout_fill_weak_eyes",&(params->playout_fill_weak_eyes),PLAYOUT_FILL_WEAK_EYES);
  

  params->addParameter("test","test_p1",&(params->test_p1),0.0);
//  params->addParameter("test","test_p2",&(params->test_p2),0.0);
  params->addParameter("test","test_p3",&(params->test_p3),0.0);
  params->addParameter("test","test_p4",&(params->test_p4),0.0);
  params->addParameter("test","test_p5",&(params->test_p5),0.0);
  params->addParameter("test","test_p6",&(params->test_p6),0.0);
  params->addParameter("test","test_p7",&(params->test_p7),0.0);
  params->addParameter("test","test_p8",&(params->test_p8),0.0);
  params->addParameter("test","test_p9",&(params->test_p9),0.0);
  params->addParameter("test","test_p10",&(params->test_p10),0.0);
  params->addParameter("test","test_p11",&(params->test_p11),0.0);
  params->addParameter("test","test_p12",&(params->test_p12),0.0);
  params->addParameter("test","test_p13",&(params->test_p13),1.0);
  params->addParameter("test","test_p14",&(params->test_p14),1.0);
  params->addParameter("test","test_p15",&(params->test_p15),0.0);
  params->addParameter("test","test_p16",&(params->test_p16),0.0);
  params->addParameter("test","test_p17",&(params->test_p17),0.0);
  params->addParameter("test","test_p18",&(params->test_p18),0.0);
  params->addParameter("test","test_p19",&(params->test_p19),1.0);
  params->addParameter("test","test_p20",&(params->test_p20),0.0);
  params->addParameter("test","test_p20",&(params->test_p20),0.0);
  params->addParameter("test","test_p21",&(params->test_p21),0.0);
  params->addParameter("test","test_p22",&(params->test_p22),1.0);
  params->addParameter("test","test_p23",&(params->test_p23),0.0);
  params->addParameter("test","test_p24",&(params->test_p24),0.0);
  params->addParameter("test","test_p25",&(params->test_p25),0.0);
  params->addParameter("test","test_p26",&(params->test_p26),0.0);
  params->addParameter("test","test_p27",&(params->test_p27),0.0);
  params->addParameter("test","test_p28",&(params->test_p28),1.0);
  params->addParameter("test","test_p29",&(params->test_p29),0.0);
  params->addParameter("test","test_p30",&(params->test_p30),0.0);
  params->addParameter("test","test_p31",&(params->test_p31),0.0);
  params->addParameter("test","test_p32",&(params->test_p32),0.0);
  params->addParameter("test","test_p33",&(params->test_p33),1.0);
  params->addParameter("test","test_p34",&(params->test_p34),1.0);
  params->addParameter("test","test_p35",&(params->test_p35),1.0);
  params->addParameter("test","test_p36",&(params->test_p36),0.0);
  params->addParameter("test","test_p37",&(params->test_p37),0.0);
  params->addParameter("test","test_p38",&(params->test_p38),0.0);
  params->addParameter("test","test_p39",&(params->test_p39),0.0);
  
  params->addParameter("test","test_p40",&(params->test_p40),1.0);
  params->addParameter("test","test_p41",&(params->test_p41),0.0);
  params->addParameter("test","test_p42",&(params->test_p42),0.0);
  params->addParameter("test","test_p43",&(params->test_p43),0.0);
  params->addParameter("test","test_p44",&(params->test_p44),0.0);
  params->addParameter("test","test_p45",&(params->test_p45),1.0);
  params->addParameter("test","test_p46",&(params->test_p46),0.0);
  params->addParameter("test","test_p47",&(params->test_p47),0.0);
  params->addParameter("test","test_p48",&(params->test_p48),0.0);
  params->addParameter("test","test_p49",&(params->test_p49),0.0);
  
  params->addParameter("test","test_p50",&(params->test_p50),0.0);
  params->addParameter("test","test_p51",&(params->test_p51),1.0);
  params->addParameter("test","test_p52",&(params->test_p52),0.0);
  params->addParameter("test","test_p53",&(params->test_p53),0.0);
  params->addParameter("test","test_p54",&(params->test_p54),0.0);
  params->addParameter("test","test_p55",&(params->test_p55),0.0);
  params->addParameter("test","test_p56",&(params->test_p56),0.0);
  params->addParameter("test","test_p57",&(params->test_p57),0.0);
  params->addParameter("test","test_p58",&(params->test_p58),0.0);
  params->addParameter("test","test_p59",&(params->test_p59),0.0);
  
  params->addParameter("test","test_p60",&(params->test_p60),1.0);
  params->addParameter("test","test_p61",&(params->test_p61),0.0);
  params->addParameter("test","test_p62",&(params->test_p62),0.0);
  params->addParameter("test","test_p63",&(params->test_p63),0.0);
  params->addParameter("test","test_p64",&(params->test_p64),0.0);
  params->addParameter("test","test_p65",&(params->test_p65),0.0);
  params->addParameter("test","test_p66",&(params->test_p66),0.0);
  params->addParameter("test","test_p67",&(params->test_p67),0.0);
  params->addParameter("test","test_p68",&(params->test_p68),0.0);
  params->addParameter("test","test_p69",&(params->test_p69),1.0);

  params->addParameter("test","test_p70",&(params->test_p70),0.0);
  params->addParameter("test","test_p71",&(params->test_p71),0.0);
  params->addParameter("test","test_p72",&(params->test_p72),1.0);
  params->addParameter("test","test_p73",&(params->test_p73),0.0);
  params->addParameter("test","test_p74",&(params->test_p74),0.0);
  params->addParameter("test","test_p75",&(params->test_p75),0.0);
  params->addParameter("test","test_p76",&(params->test_p76),1.0);
  params->addParameter("test","test_p77",&(params->test_p77),0.0);
  params->addParameter("test","test_p78",&(params->test_p78),0.0);
  params->addParameter("test","test_p79",&(params->test_p79),0.0);
  
  params->addParameter("test","test_p80",&(params->test_p80),0.0);
  params->addParameter("test","test_p81",&(params->test_p81),0.0);
  params->addParameter("test","test_p82",&(params->test_p82),0.0);
  params->addParameter("test","test_p83",&(params->test_p83),0.0);
  params->addParameter("test","test_p84",&(params->test_p84),0.0);
  params->addParameter("test","test_p85",&(params->test_p85),0.0);
  params->addParameter("test","test_p86",&(params->test_p86),0.0);
  params->addParameter("test","test_p87",&(params->test_p87),0.0);
  params->addParameter("test","test_p88",&(params->test_p88),0.0);
  params->addParameter("test","test_p89",&(params->test_p89),0.0);
  
  params->addParameter("test","test_p90",&(params->test_p90),0.0);
  params->addParameter("test","test_p91",&(params->test_p91),0.0);
  params->addParameter("test","test_p92",&(params->test_p92),0.0);
  params->addParameter("test","test_p93",&(params->test_p93),20000.0);
  params->addParameter("test","test_p94",&(params->test_p94),0.0);
  params->addParameter("test","test_p95",&(params->test_p95),0.0);
  params->addParameter("test","test_p96",&(params->test_p96),0.0);
  params->addParameter("test","test_p97",&(params->test_p97),0.0);
  params->addParameter("test","test_p98",&(params->test_p98),0.0);
  params->addParameter("test","test_p99",&(params->test_p99),0.0);

  params->addParameter("test","test_p100",&(params->test_p100),0.0);
  params->addParameter("test","test_p101",&(params->test_p101),0.0);
  params->addParameter("test","test_p102",&(params->test_p102),0.0);
  params->addParameter("test","test_p103",&(params->test_p103),0.0);
  params->addParameter("test","test_p104",&(params->test_p104),0.0);
  params->addParameter("test","test_p105",&(params->test_p105),0.0);
  params->addParameter("test","test_p106",&(params->test_p106),0.0);
  params->addParameter("test","test_p107",&(params->test_p107),0.0);
  params->addParameter("test","test_p108",&(params->test_p108),0.0);
  params->addParameter("test","test_p109",&(params->test_p109),1.0);
  params->addParameter("test","test_p110",&(params->test_p110),0.0);
  
  params->addParameter("test","test_p111",&(params->test_p111),1.0);
  params->addParameter("test","test_p112",&(params->test_p112),0.0);
  params->addParameter("test","test_p113",&(params->test_p113),0.0);
  params->addParameter("test","test_p114",&(params->test_p114),0.0);
  params->addParameter("test","test_p115",&(params->test_p115),0.0);
  params->addParameter("test","test_p116",&(params->test_p116),0.0);
  params->addParameter("test","test_p117",&(params->test_p117),0.0);
  params->addParameter("test","test_p118",&(params->test_p118),0.0);
  params->addParameter("test","test_p119",&(params->test_p119),0.0);
  params->addParameter("test","test_p120",&(params->test_p120),0.0);

  params->addParameter("csplayout","csstyle_enabled",&(params->csstyle_enabled),false);
  params->addParameter("csplayout","csstyle_atatarigroup",&(params->csstyle_atatarigroup),1.0);
  params->addParameter("csplayout","csstyle_is2libgroup",&(params->csstyle_is2libgroup),1.0);
  params->addParameter("csplayout","csstyle_attachedpos",&(params->csstyle_attachedpos),1.0);
  params->addParameter("csplayout","csstyle_attachedposbutselfatari",&(params->csstyle_attachedposbutselfatari),1.0);
  params->addParameter("csplayout","csstyle_saveataricapture",&(params->csstyle_saveataricapture),1.0);
  params->addParameter("csplayout","csstyle_saveataricapturebutselfatari",&(params->csstyle_saveataricapturebutselfatari),1.0);
  params->addParameter("csplayout","csstyle_saveatariextention",&(params->csstyle_saveatariextention),1.0);
  params->addParameter("csplayout","csstyle_saveatariextentionbutselfatari",&(params->csstyle_saveatariextentionbutselfatari),1.0);
  params->addParameter("csplayout","csstyle_solvekocapture",&(params->csstyle_solvekocapture),1.0);
  params->addParameter("csplayout","csstyle_2libcapture",&(params->csstyle_2libcapture),1.0);
  params->addParameter("csplayout","csstyle_nakade",&(params->csstyle_nakade),1.0);
  params->addParameter("csplayout","csstyle_playonladder",&(params->csstyle_playonladder),1.0);
  params->addParameter("csplayout","csstyle_defendapproach",&(params->csstyle_defendapproach),1.0);
  params->addParameter("csplayout","csstyle_2libavoidcapture",&(params->csstyle_2libavoidcapture),1.0);
  params->addParameter("csplayout","csstyle_adaptiveplayouts",&(params->csstyle_adaptiveplayouts),false);
  params->addParameter("csplayout","csstyle_adaptiveplayouts_alpha",&(params->csstyle_adaptiveplayouts_alpha),0.01);
  params->addParameter("csplayout","csstyle_adaptiveplayouts_lambda",&(params->csstyle_adaptiveplayouts_lambda),0.001);
  params->addParameter("csplayout","csstyle_patterngammasnothing",&(params->csstyle_patterngammasnothing),1.0);
  params->addParameter("csplayout","csstyle_pattern_min_gamma_sort",&(params->csstyle_pattern_min_gamma_sort),1.0);
  params->addParameter("csplayout","csstyle_pattern_weak_gamma_pick",&(params->csstyle_pattern_weak_gamma_pick),10);
  params->addParameter("csplayout","csstyle_rate_of_gamma_moves",&(params->csstyle_rate_of_gamma_moves),1.0);
  params->addParameter("csplayout","csstyle_bad_move_reduce2libs",&(params->csstyle_bad_move_reduce2libs),true);
  params->addParameter("csplayout","csstyle_adaptiveplayouts_only_played",&(params->csstyle_adaptiveplayouts_only_played),false);
  params->addParameter("csplayout","csstyle_01",&(params->csstyle_01),0.0);
  params->addParameter("csplayout","csstyle_02",&(params->csstyle_02),0.0);
  params->addParameter("csplayout","csstyle_03",&(params->csstyle_03),0.0);
  params->addParameter("csplayout","csstyle_04",&(params->csstyle_04),0.0);
  params->addParameter("csplayout","csstyle_05",&(params->csstyle_05),0.0);
  params->addParameter("csplayout","csstyle_06",&(params->csstyle_06),0.0);
  params->addParameter("csplayout","csstyle_07",&(params->csstyle_07),0.0);
  params->addParameter("csplayout","csstyle_08",&(params->csstyle_08),0.0);
  params->addParameter("csplayout","csstyle_09",&(params->csstyle_09),0.0);

  params->addParameter("playout","localeval_01",&(params->localeval_01),0.0);
  params->addParameter("playout","localeval_02",&(params->localeval_02),0.0);
  params->addParameter("playout","localeval_03",&(params->localeval_03),0.0);
  params->addParameter("playout","localeval_04",&(params->localeval_04),0.0);
  params->addParameter("playout","localeval_05",&(params->localeval_05),0.0);
  params->addParameter("playout","localeval_06",&(params->localeval_06),0.0);
  params->addParameter("playout","localeval_07",&(params->localeval_07),0.0);
  params->addParameter("playout","localeval_08",&(params->localeval_08),0.0);
  params->addParameter("playout","localeval_09",&(params->localeval_09),0.0);
  
  params->addParameter("tree","ucb_c",&(params->ucb_c),UCB_C);
  params->addParameter("tree","ucb_init",&(params->ucb_init),UCB_INIT);

  params->addParameter("tree","bernoulli_a",&(params->bernoulli_a),BERNOULLI_A);
  params->addParameter("tree","bernoulli_b",&(params->bernoulli_b),BERNOULLI_B);
  params->addParameter("tree","kl_ucb_enabled",&(params->KL_ucb_enabled),KL_UCB_ENABLED );
  
  params->addParameter("tree","weight_score",&(params->weight_score),WEIGHT_SCORE);
  params->addParameter("tree","random_f",&(params->random_f),RANDOM_F);

  params->addParameter("tree","rave_moves",&(params->rave_moves),RAVE_MOVES);
  params->addParameter("tree","rave_init_wins",&(params->rave_init_wins),RAVE_INIT_WINS);
  params->addParameter("tree","uct_preset_rave_f",&(params->uct_preset_rave_f),UCT_PRESET_RAVE_F);
  params->addParameter("tree","rave_skip",&(params->rave_skip),RAVE_SKIP);
  params->addParameter("tree","rave_moves_use",&(params->rave_moves_use),RAVE_MOVES_USE);
  params->addParameter("tree","rave_only_first_move",&(params->rave_only_first_move),RAVE_ONLY_FIRST_MOVE);
  
  params->addParameter("tree","uct_expand_after",&(params->uct_expand_after),UCT_EXPAND_AFTER);
  params->addParameter("tree","uct_keep_subtree",&(params->uct_keep_subtree),UCT_KEEP_SUBTREE,&Engine::updateParameterWrapper,this);
  params->addParameter("tree","uct_symmetry_use",&(params->uct_symmetry_use),UCT_SYMMETRY_USE,&Engine::updateParameterWrapper,this);
  if (params->uct_symmetry_use)
    currentboard->turnSymmetryOn();
  else
    currentboard->turnSymmetryOff();
  params->addParameter("tree","uct_virtual_loss",&(params->uct_virtual_loss),UCT_VIRTUAL_LOSS);
  params->addParameter("tree","uct_lock_free",&(params->uct_lock_free),UCT_LOCK_FREE);
  params->addParameter("tree","uct_atari_prior",&(params->uct_atari_prior),UCT_ATARI_PRIOR);
  params->addParameter("tree","uct_playoutmove_prior",&(params->uct_playoutmove_prior),UCT_PLAYOUTMOVE_PRIOR);
  params->addParameter("tree","uct_pattern_prior",&(params->uct_pattern_prior),UCT_PATTERN_PRIOR);
  params->addParameter("tree","uct_progressive_widening_enabled",&(params->uct_progressive_widening_enabled),UCT_PROGRESSIVE_WIDENING_ENABLED);
  params->addParameter("tree","uct_progressive_widening_init",&(params->uct_progressive_widening_init),UCT_PROGRESSIVE_WIDENING_INIT);
  params->addParameter("tree","uct_progressive_widening_a",&(params->uct_progressive_widening_a),UCT_PROGRESSIVE_WIDENING_A);
  params->addParameter("tree","uct_progressive_widening_b",&(params->uct_progressive_widening_b),UCT_PROGRESSIVE_WIDENING_B);
  params->addParameter("tree","uct_progressive_widening_c",&(params->uct_progressive_widening_c),UCT_PROGRESSIVE_WIDENING_C);
  params->addParameter("tree","uct_progressive_widening_count_wins",&(params->uct_progressive_widening_count_wins),UCT_PROGRESSIVE_WIDENING_COUNT_WINS);
  params->addParameter("tree","uct_points_bonus",&(params->uct_points_bonus),UCT_POINTS_BONUS);
  params->addParameter("tree","uct_length_bonus",&(params->uct_length_bonus),UCT_LENGTH_BONUS);
  params->addParameter("tree","uct_progressive_bias_enabled",&(params->uct_progressive_bias_enabled),UCT_PROGRESSIVE_BIAS_ENABLED);
  params->addParameter("tree","uct_progressive_bias_h",&(params->uct_progressive_bias_h),UCT_PROGRESSIVE_BIAS_H);
  params->addParameter("tree","uct_progressive_bias_log_add",&(params->uct_progressive_bias_log_add),0.0);
  params->addParameter("tree","uct_progressive_bias_count_offset",&(params->uct_progressive_bias_count_offset),1.0);
  params->addParameter("tree","uct_progressive_bias_scaled",&(params->uct_progressive_bias_scaled),UCT_PROGRESSIVE_BIAS_SCALED);
  params->addParameter("tree","uct_progressive_bias_relative",&(params->uct_progressive_bias_relative),UCT_PROGRESSIVE_BIAS_RELATIVE);
  params->addParameter("tree","uct_progressive_bias_moves",&(params->uct_progressive_bias_moves),UCT_PROGRESSIVE_BIAS_MOVES);
  params->addParameter("tree","uct_progressive_bias_exponent",&(params->uct_progressive_bias_exponent),UCT_PROGRESSIVE_BIAS_EXPONENT);
  params->addParameter("tree","uct_criticality_urgency_factor",&(params->uct_criticality_urgency_factor),UCT_CRITICALITY_URGENCY_FACTOR);
  params->addParameter("tree","uct_criticality_urgency_decay",&(params->uct_criticality_urgency_decay),UCT_CRITICALITY_URGENCY_DECAY);
  params->addParameter("tree","uct_criticality_unprune_factor",&(params->uct_criticality_unprune_factor),UCT_CRITICALITY_UNPRUNE_FACTOR);
  params->addParameter("tree","uct_criticality_unprune_multiply",&(params->uct_criticality_unprune_multiply),UCT_CRITICALITY_UNPRUNE_MULTIPLY);
  params->addParameter("tree","uct_criticality_min_playouts",&(params->uct_criticality_min_playouts),UCT_CRITICALITY_MIN_PLAYOUTS);
  params->addParameter("tree","uct_criticality_siblings",&(params->uct_criticality_siblings),UCT_CRITICALITY_SIBLINGS);
  params->addParameter("tree","uct_criticality_rave_unprune_factor",&(params->uct_criticality_rave_unprune_factor),UCT_CRITICALITY_RAVE_UNPRUNE_FACTOR);
  params->addParameter("tree","uct_prior_unprune_factor",&(params->uct_prior_unprune_factor),UCT_PRIOR_UNPRUNE_FACTOR);
  params->addParameter("tree","uct_rave_unprune_factor",&(params->uct_rave_unprune_factor),UCT_RAVE_UNPRUNE_FACTOR);
  params->addParameter("tree","uct_rave_other_unprune_factor",&(params->uct_rave_other_unprune_factor),UCT_RAVE_OTHER_UNPRUNE_FACTOR);
  params->addParameter("tree","uct_earlyrave_unprune_factor",&(params->uct_earlyrave_unprune_factor),UCT_EARLYRAVE_UNPRUNE_FACTOR);
  params->addParameter("tree","uct_rave_unprune_decay",&(params->uct_rave_unprune_decay),UCT_RAVE_UNPRUNE_DECAY);
  params->addParameter("tree","uct_rave_unprune_multiply",&(params->uct_rave_unprune_multiply),UCT_RAVE_UNPRUNE_MULTIPLY);
  params->addParameter("tree","uct_oldmove_unprune_factor",&(params->uct_oldmove_unprune_factor),UCT_OLDMOVE_UNPRUNE_FACTOR);
  params->addParameter("tree","uct_oldmove_unprune_factor_b",&(params->uct_oldmove_unprune_factor_b),UCT_OLDMOVE_UNPRUNE_FACTOR_B);
  params->addParameter("tree","uct_oldmove_unprune_factor_c",&(params->uct_oldmove_unprune_factor_c),UCT_OLDMOVE_UNPRUNE_FACTOR_C);
  params->addParameter("tree","uct_area_owner_factor_a",&(params->uct_area_owner_factor_a),UCT_AREA_OWNER_FACTOR_A);
  params->addParameter("tree","uct_area_owner_factor_b",&(params->uct_area_owner_factor_b),UCT_AREA_OWNER_FACTOR_B);
  params->addParameter("tree","uct_area_owner_factor_c",&(params->uct_area_owner_factor_c),UCT_AREA_OWNER_FACTOR_C);
  params->addParameter("tree","uct_area_correlation_statistics",&(params->uct_area_correlation_statistics),UCT_AREA_CORRELATION_STATISTICS);
  params->addParameter("tree","uct_reprune_factor",&(params->uct_reprune_factor),UCT_REPRUNE_FACTOR);
  params->addParameter("tree","uct_factor_circpattern",&(params->uct_factor_circpattern),UCT_FACTOR_CIRCPATTERN);
  params->addParameter("tree","uct_factor_circpattern_exponent",&(params->uct_factor_circpattern_exponent),UCT_FACTOR_CIRCPATTERN_EXPONENT);
  params->addParameter("tree","uct_circpattern_minsize",&(params->uct_circpattern_minsize),UCT_CIRCPATTERN_MINSIZE);
  params->addParameter("tree","uct_simple_pattern_factor",&(params->uct_simple_pattern_factor),UCT_SIMPLE_PATTERN_FACTOR);
  params->addParameter("tree","uct_atari_unprune",&(params->uct_atari_unprune),UCT_ATARI_UNPRUNE);
  params->addParameter("tree","uct_atari_unprune_exp",&(params->uct_atari_unprune_exp),UCT_ATARI_UNPRUNE_EXP);
  params->addParameter("tree","uct_danger_value",&(params->uct_atari_unprune_exp),UCT_DANGER_VALUE);
  
  params->addParameter("tree","uct_slow_update_interval",&(params->uct_slow_update_interval),UCT_SLOW_UPDATE_INTERVAL);
  params->uct_slow_update_last=0;
  params->addParameter("tree","uct_slow_debug_interval",&(params->uct_slow_debug_interval),UCT_SLOW_DEBUG_INTERVAL);
  params->uct_slow_debug_last=0;
  params->addParameter("tree","uct_stop_early",&(params->uct_stop_early),UCT_STOP_EARLY);
  params->addParameter("tree","uct_terminal_handling",&(params->uct_terminal_handling),UCT_TERMINAL_HANDLING);
  
  params->addParameter("tree","surewin_threshold",&(params->surewin_threshold),SUREWIN_THRESHOLD);
  params->surewin_expected=false;
  params->addParameter("tree","surewin_pass_bonus",&(params->surewin_pass_bonus),SUREWIN_PASS_BONUS);
  params->addParameter("tree","surewin_touchdead_bonus",&(params->surewin_touchdead_bonus),SUREWIN_TOUCHDEAD_BONUS);
  params->addParameter("tree","surewin_oppoarea_penalty",&(params->surewin_oppoarea_penalty),SUREWIN_OPPOAREA_PENALTY);
  
  params->addParameter("tree","resign_ratio_threshold",&(params->resign_ratio_threshold),RESIGN_RATIO_THRESHOLD);
  params->addParameter("tree","resign_move_factor_threshold",&(params->resign_move_factor_threshold),RESIGN_MOVE_FACTOR_THRESHOLD);
  
  params->addParameter("tree","territory_decayfactor",&(params->territory_decayfactor),TERRITORY_DECAYFACTOR);
  params->addParameter("tree","territory_threshold",&(params->territory_threshold),TERRITORY_THRESHOLD);
  
  params->addParameter("tree","uct_decay_alpha",&(params->uct_decay_alpha),UCT_DECAY_ALPHA);
  params->addParameter("tree","uct_decay_k",&(params->uct_decay_k),UCT_DECAY_K);
  params->addParameter("tree","uct_decay_m",&(params->uct_decay_m),UCT_DECAY_M);

  params->addParameter("tree","features_ladders",&(params->features_ladders),FEATURES_LADDERS);
  params->addParameter("tree","features_dt_use",&(params->features_dt_use),false);
  params->addParameter("tree","features_pass_no_move_for_lastdist",&(params->features_pass_no_move_for_lastdist),FEATURES_PASS_NO_MOVE_FOR_LASTDIST);

  params->addParameter("tree","dynkomi_enabled",&(params->dynkomi_enabled),true);
  params->addParameter("tree","recalc_dynkomi_limit",&(params->recalc_dynkomi_limit),0);
  
  params->addParameter("tree","mm_learn_enabled",&(params->mm_learn_enabled),false);
  params->addParameter("tree","mm_learn_delta",&(params->mm_learn_delta),MM_LEARN_DELTA);
  params->addParameter("tree","mm_learn_min_playouts",&(params->mm_learn_min_playouts),MM_LEARN_MIN_PLAYOUTS);

  params->addParameter("rules","rules_positional_superko_enabled",&(params->rules_positional_superko_enabled),RULES_POSITIONAL_SUPERKO_ENABLED);
  params->addParameter("rules","rules_superko_top_ply",&(params->rules_superko_top_ply),RULES_SUPERKO_TOP_PLY);
  params->addParameter("rules","rules_superko_prune_after",&(params->rules_superko_prune_after),RULES_SUPERKO_PRUNE_AFTER);
  params->addParameter("rules","rules_superko_at_playout",&(params->rules_superko_at_playout),RULES_SUPERKO_AT_PLAYOUT);
  params->addParameter("rules","rules_all_stones_alive",&(params->rules_all_stones_alive),RULES_ALL_STONES_ALIVE);
  params->addParameter("rules","rules_all_stones_alive_playouts",&(params->rules_all_stones_alive_playouts),RULES_ALL_STONES_ALIVE_PLAYOUTS);
  
  params->addParameter("time","time_k",&(params->time_k),TIME_K);
  params->addParameter("time","time_buffer",&(params->time_buffer),TIME_BUFFER);
  params->addParameter("time","time_move_minimum",&(params->time_move_minimum),TIME_MOVE_MINIMUM);
  params->addParameter("time","time_ignore",&(params->time_ignore),false);
  params->addParameter("time","time_move_max",&(params->time_move_max),TIME_MOVE_MAX);
  
  params->addParameter("general","pondering_enabled",&(params->pondering_enabled),PONDERING_ENABLED,&Engine::updateParameterWrapper,this);
  params->addParameter("general","pondering_playouts_max",&(params->pondering_playouts_max),PONDERING_PLAYOUTS_MAX);
  
  params->addParameter("other","live_gfx",&(params->livegfx_on),LIVEGFX_ON);
  params->addParameter("other","live_gfx_update_playouts",&(params->livegfx_update_playouts),LIVEGFX_UPDATE_PLAYOUTS);
  params->addParameter("other","live_gfx_delay",&(params->livegfx_delay),LIVEGFX_DELAY);
  
  params->addParameter("other","outputsgf_maxchildren",&(params->outputsgf_maxchildren),OUTPUTSGF_MAXCHILDREN);
  
  params->addParameter("other","debug",&(params->debug_on),DEBUG_ON);
  params->addParameter("other","play_n_passes_first",&(params->play_n_passes_first),0);
  
  params->addParameter("other","interrupts_enabled",&(params->interrupts_enabled),INTERRUPTS_ENABLED,&Engine::updateParameterWrapper,this);
  params->addParameter("other","undo_enable",&(params->undo_enable),true);
  
  params->addParameter("other","features_only_small",&(params->features_only_small),false);
  params->addParameter("other","features_tactical",&(params->features_tactical),true);
  params->addParameter("other","features_history_agnostic",&(params->features_history_agnostic),false);
  params->addParameter("other","features_output_competitions",&(params->features_output_competitions),0.0);
  params->addParameter("other","features_output_for_playout",&(params->features_output_for_playout),false);
  params->addParameter("other","features_output_competitions_mmstyle",&(params->features_output_competitions_mmstyle),false);
  params->addParameter("other","features_ordered_comparison",&(params->features_ordered_comparison),false);
  params->addParameter("other","features_ordered_comparison_log_evidence",&(params->features_ordered_comparison_log_evidence),false);
  params->addParameter("other","features_ordered_comparison_move_num",&(params->features_ordered_comparison_move_num),false);
  params->addParameter("other","features_circ_list",&(params->features_circ_list),0.0);
  params->addParameter("other","features_circ_list_size",&(params->features_circ_list_size),0);
  params->addParameter("other","cnn_data",&(params->CNN_data),0.0);
  params->addParameter("other","cnn_data_predict_future",&(params->CNN_data_predict_future),0);

  params->addParameter("cnn","cnn_pass_probability",&(params->CNN_pass_probability),0.05);
  params->addParameter("cnn","cnn_data_playouts",&(params->CNN_data_playouts),0);
  params->addParameter("cnn","cnn_weak_gamma",&(params->cnn_weak_gamma),0);
  params->addParameter("cnn","cnn_lastmove_decay",&(params->cnn_lastmove_decay),0);
  params->addParameter("cnn","cnn_value_lambda",&(params->cnn_value_lambda),0);
  params->addParameter("cnn","cnn_random_for_only_cnn",&(params->cnn_random_for_only_cnn),0);
  params->addParameter("cnn","cnn_mutex_wait_lock",&(params->cnn_mutex_wait_lock),0);
  params->addParameter("cnn","cnn_num_of_gpus",&(params->cnn_num_of_gpus),0);
  params->addParameter("cnn","cnn_prior_values_treshhold",&(params->cnn_prior_values_treshhold),0.0);
  
  params->addParameter("other","auto_save_sgf",&(params->auto_save_sgf),false);
  params->addParameter("other","auto_save_sgf_prefix",&(params->auto_save_sgf_prefix),"game");
  params->addParameter("other","version_config_file",&(params->version_config_file),"1.0");

  std::list<std::string> *spoptions = new std::list<std::string>();
  spoptions->push_back("descent_split");
  spoptions->push_back("robust_descent_split");
  spoptions->push_back("robust_win_split");
  spoptions->push_back("robust_loss_split");
  spoptions->push_back("entropy_descent_split");
  spoptions->push_back("entropy_win_split");
  spoptions->push_back("entropy_loss_split");
  spoptions->push_back("winrate_split");
  spoptions->push_back("win_loss_separate");
  spoptions->push_back("weighted_win_loss_separate");
  spoptions->push_back("symmetric_separate");
  spoptions->push_back("weighted_symmetric_separate");
  spoptions->push_back("winrate_entropy");
  spoptions->push_back("weighted_winrate_entropy");
  spoptions->push_back("classification_separate");
  params->addParameter("other","dt_selection_policy",&(params->dt_selection_policy_string),spoptions,"descent_split",&Engine::updateParameterWrapper,this);
  params->dt_selection_policy = Parameters::SP_DESCENT_SPLIT;

  params->addParameter("other","dt_update_prob",&(params->dt_update_prob),0.00);
  params->addParameter("other","dt_split_after",&(params->dt_split_after),1000);
  params->addParameter("other","dt_solo_leaf",&(params->dt_solo_leaf),true);
  params->addParameter("other","dt_output_mm",&(params->dt_output_mm),0.00);
  params->addParameter("other","dt_ordered_comparison",&(params->dt_ordered_comparison),false);
  
  #ifdef HAVE_MPI
    params->addParameter("mpi","mpi_update_period",&(params->mpi_update_period),MPI_UPDATE_PERIOD);
    params->addParameter("mpi","mpi_update_depth",&(params->mpi_update_depth),MPI_UPDATE_DEPTH);
    params->addParameter("mpi","mpi_update_threshold",&(params->mpi_update_threshold),MPI_UPDATE_THRESHOLD);
  #endif
  
  patterntable=new Pattern::ThreeByThreeTable();
  patterntable->loadPatternDefaults2();
  
  features=new Features(params);
  features->loadGammaDefaults();
  
  book=new Book(params);
  
  time=new Time(params,0);
  
  playout=new Playout(params);
  
  movehistory=new std::list<Go::Move>();
  moveexplanations=new std::list<std::string>();
  hashtree=new Go::ZobristTree();
  
  territorymap=new Go::TerritoryMap(boardsize);
  area_correlation_map=new Go::TerritoryMap*[currentboard->getPositionMax()*2];
#warning "memory allocated for area_correlation_map"
  for (int i=0;i<currentboard->getPositionMax()*2;i++)
  {
    area_correlation_map[i]=new Go::TerritoryMap(boardsize);
  }
  probabilitymap=new Go::MoveProbabilityMap(boardsize);
  correlationmap=new Go::ObjectBoard<Go::CorrelationData>(boardsize);

  respondboard=new Go::RespondBoard(boardsize);
  
  blackOldMoves=new float[currentboard->getPositionMax()];
  whiteOldMoves=new float[currentboard->getPositionMax()];
  for (int i=0;i<currentboard->getPositionMax();i++)
  {
    blackOldMoves[i]=0;
    whiteOldMoves[i]=0;
  }
  blackOldMean=0.5;
  whiteOldMean=0.5;
  blackOldMovesNum=0;
  whiteOldMovesNum=0;
  
  this->addGtpCommands();
  
  movetree=NULL;
  this->clearMoveTree();

  isgamefinished=false;
  
  params->cleanup_in_progress=false;
  
  params->uct_last_r2=0;
  
  params->thread_job=Parameters::TJ_GENMOVE;
  threadpool = new Worker::Pool(params);
  
  #ifdef HAVE_MPI
    this->mpiBuildDerivedTypes();
    
    if (mpirank==0)
    {
      bool errorsyncing=false;
      
      for (int i=1;i<mpiworldsize;i++)
      {
        std::string data=this->mpiRecvString(i);
        if (data!=VERSION)
          errorsyncing=true;
      }
      
      if (errorsyncing)
        gtpe->getOutput()->printfDebug("FATAL ERROR! could not sync mpi\n");
      else
        gtpe->getOutput()->printfDebug("mpi synced world of size: %d\n",mpiworldsize);
      mpisynced=!errorsyncing;
    }
    else
    {
      this->mpiSendString(0,VERSION);
      mpisynced=true;
    }
  #endif
}

Engine::~Engine()
{
  this->gameFinished();
  #ifdef HAVE_MPI
    MPIRANK0_ONLY(this->mpiBroadcastCommand(Engine::MPICMD_QUIT););
    this->mpiFreeDerivedTypes();
  #endif
  delete threadpool;
  delete features;
  delete patterntable;
  if (movetree!=NULL)
    delete movetree;
  for (int i=0;i<currentboard->getPositionMax()*2;i++)
  {
    delete area_correlation_map[i];
  }
  delete[] area_correlation_map;
  delete currentboard;
  delete movehistory;
  delete moveexplanations;
  delete hashtree;
  delete params;
  delete time;
  delete book;
  delete zobristtable;
  delete playout;
  delete territorymap;
  delete probabilitymap;
  delete correlationmap;
  delete respondboard;
  delete[] blackOldMoves;
  delete[] whiteOldMoves;
//  delete[] deltagammas;
  delete[] deltagammaslocal;

  for (std::list<DecisionTree*>::iterator iter=decisiontrees.begin();iter!=decisiontrees.end();++iter)
  {
    delete (*iter);
  }
#ifdef HAVE_CAFFE
  for (int i=0;i<num_nets;i++) {
    if (caffe_test_net[i]!=NULL) delete caffe_test_net[i];
  }
  if (caffe_area_net!=NULL) delete caffe_area_net;
#endif
}

void Engine::getCNN(Go::Board *board, int thread_id, Go::Color col, float result[], int net_num, float *v)
{
#ifdef HAVE_CAFFE
	//if (board->getSize()!=19) {
	//	fprintf(stderr,"only 19x19 supported by CNN\n");
  //  for (int i=0;i<361;i++) result[i]=0.0;
	//	return;
	//}
  int size=board->getSize();
  int net_num_multi_gpu=net_num;
  if (params->cnn_num_of_gpus>0) {
    net_num_multi_gpu=thread_id; //only one gpu but mutliple threads at the moment
  }
	float *data;
  //board->calcSlowLibertyGroups();
	data= new float[caffe_test_net_input_dim[net_num]*size*size];
	//fprintf(stderr,"2\n");
	if (col==Go::BLACK) {
	  for (int j=0;j<size;j++)
	    for (int k=0;k<size;k++)
		  {
        for (int l=0;l<caffe_test_net_input_dim[net_num];l++) data[l*size*size+size*j+k]=0;
        //fprintf(stderr,"%d %d %d\n",i,j,k);
        int pos=Go::Position::xy2pos(j,k,size);
        int libs=0;
        if (board->inGroup(pos)) libs=board->getGroup(pos)->numRealLibs()-1;
        if (libs>3) libs=3; 
        if (board->getColor(pos)==Go::BLACK)
	          {
			  data[(0+libs)*size*size + size*j + k]=1.0;
			  //data[size*size+size*j+k]=0.0;
			  }
	      else if (board->getColor(pos)==Go::WHITE)
		      {
			  //data[j*size+k]=0.0;
			  data[(4+libs)*size*size + size*j + k]=1.0;
			  }
	      else if (board->getColor(Go::Position::xy2pos(j,k,size))==Go::EMPTY)
	      {
			    data[8*size*size + size*j + k]=1.0;
			  }
      }
	}
	if (col==Go::WHITE) {
	  for (int j=0;j<size;j++)
	    for (int k=0;k<size;k++)
		  {//fprintf(stderr,"%d %d %d\n",i,j,k);
        for (int l=0;l<caffe_test_net_input_dim[net_num];l++) data[l*size*size+size*j+k]=0;
        //fprintf(stderr,"%d %d %d\n",i,j,k);
        int pos=Go::Position::xy2pos(j,k,size);
        int libs=0;
        if (board->inGroup(pos)) libs=board->getGroup(pos)->numRealLibs()-1;
        if (libs>3) libs=3; 
        if (board->getColor(pos)==Go::BLACK)
	          {
			  data[(4+libs)*size*size + size*j + k]=1.0;
			  //data[size*size+size*j+k]=0.0;
			  }
	      else if (board->getColor(pos)==Go::WHITE)
		      {
			  //data[j*size+k]=0.0;
			  data[(0+libs)*size*size + size*j + k]=1.0;
			  }
	      else if (board->getColor(pos)==Go::EMPTY)
	      {
			    data[8*size*size + size*j + k]=1.0;
			  }
    }
	}
if (caffe_test_net_input_dim[net_num] == 20) {
int komiplane=-1;
if (col==Go::BLACK) {
  if (komi==0.5) komiplane=0;
  if (komi==6.5) komiplane=1;
  if (komi==7.5) komiplane=2;
}
if (col==Go::WHITE) {
  if (komi==0.5) komiplane=3;
  if (komi==6.5) komiplane=4;
  if (komi==7.5) komiplane=5;
}
if (komiplane>=0) {
  for (int j=0;j<size;j++)
    for (int k=0;k<size;k++) {
      data[(14+komiplane)*size*size+size*j+k]=1.0;
    }
}
}

if (caffe_test_net_input_dim[net_num] == 13 ) {
  if (double_second_last_move_from_cnn) {
  if (board->getSecondLastMove().isNormal()) {
    int j=Go::Position::pos2x(board->getSecondLastMove().getPosition(),size);
    int k=Go::Position::pos2y(board->getSecondLastMove().getPosition(),size);
    data[9*size*size+size*j+k]=1.0;
  }
  }
  else 
  {
    if (!disable_last_move_from_cnn) {
      if (board->getLastMove().isNormal()) {
        int j=Go::Position::pos2x(board->getLastMove().getPosition(),size);
        int k=Go::Position::pos2y(board->getLastMove().getPosition(),size);
        data[9*size*size+size*j+k]=1.0;
      }
    }
  }
  if (board->getSecondLastMove().isNormal()) {
    int j=Go::Position::pos2x(board->getSecondLastMove().getPosition(),size);
    int k=Go::Position::pos2y(board->getSecondLastMove().getPosition(),size);
    if (params->cnn_lastmove_decay>0)
      data[10*size*size+size*j+k]=exp(-0.0*params->cnn_lastmove_decay);
    else
      data[10*size*size+size*j+k]=1.0;
  }
  if (params->cnn_lastmove_decay>0) {
    if (board->getThirdLastMove().isNormal()) {
      int j=Go::Position::pos2x(board->getThirdLastMove().getPosition(),size);
      int k=Go::Position::pos2y(board->getThirdLastMove().getPosition(),size);
      data[9*size*size+size*j+k]=exp(-2.0*params->cnn_lastmove_decay);
    }
    if (board->getForthLastMove().isNormal()) {
      int j=Go::Position::pos2x(board->getForthLastMove().getPosition(),size);
      int k=Go::Position::pos2y(board->getForthLastMove().getPosition(),size);
      data[10*size*size+size*j+k]=exp(-2.0*params->cnn_lastmove_decay);
    }
    if (board->getFifthLastMove().isNormal()) {
      int j=Go::Position::pos2x(board->getFifthLastMove().getPosition(),size);
      int k=Go::Position::pos2y(board->getFifthLastMove().getPosition(),size);
      data[9*size*size+size*j+k]=exp(-4.0*params->cnn_lastmove_decay);
    }
    if (board->getSixLastMove().isNormal()) {
      int j=Go::Position::pos2x(board->getSixLastMove().getPosition(),size);
      int k=Go::Position::pos2y(board->getSixLastMove().getPosition(),size);
      data[10*size*size+size*j+k]=exp(-4.0*params->cnn_lastmove_decay);
    }
  }
  else {
    if (board->getThirdLastMove().isNormal()) {
      int j=Go::Position::pos2x(board->getThirdLastMove().getPosition(),size);
      int k=Go::Position::pos2y(board->getThirdLastMove().getPosition(),size);
      data[11*size*size+size*j+k]=1.0;
    }
    if (board->getForthLastMove().isNormal()) {
      int j=Go::Position::pos2x(board->getForthLastMove().getPosition(),size);
      int k=Go::Position::pos2y(board->getForthLastMove().getPosition(),size);
      data[12*size*size+size*j+k]=1.0;
    }
  }
}
else if (caffe_test_net_input_dim[net_num] == 14 || caffe_test_net_input_dim[net_num] == 20) {
  if (double_second_last_move_from_cnn) {
   fprintf(stderr," disable not supported!!\n");
  }
  if (board->getSimpleKo()>=0) {
    int j=Go::Position::pos2x(board->getSimpleKo(),size);
    int k=Go::Position::pos2y(board->getSimpleKo(),size);
    data[9*size*size+size*j+k]=1.0;
  }
  if (board->getLastMove().isNormal()) {
        int j=Go::Position::pos2x(board->getLastMove().getPosition(),size);
        int k=Go::Position::pos2y(board->getLastMove().getPosition(),size);
        data[10*size*size+size*j+k]=1.0;
      }
  if (board->getSecondLastMove().isNormal()) {
    int j=Go::Position::pos2x(board->getSecondLastMove().getPosition(),size);
    int k=Go::Position::pos2y(board->getSecondLastMove().getPosition(),size);
    if (params->cnn_lastmove_decay>0)
      data[11*size*size+size*j+k]=exp(-0.0*params->cnn_lastmove_decay);
    else
      data[11*size*size+size*j+k]=1.0;
  }
  if (params->cnn_lastmove_decay>0) {
    if (board->getThirdLastMove().isNormal()) {
      int j=Go::Position::pos2x(board->getThirdLastMove().getPosition(),size);
      int k=Go::Position::pos2y(board->getThirdLastMove().getPosition(),size);
      data[10*size*size+size*j+k]=exp(-2.0*params->cnn_lastmove_decay);
    }
    if (board->getForthLastMove().isNormal()) {
      int j=Go::Position::pos2x(board->getForthLastMove().getPosition(),size);
      int k=Go::Position::pos2y(board->getForthLastMove().getPosition(),size);
      data[11*size*size+size*j+k]=exp(-2.0*params->cnn_lastmove_decay);
    }
    if (board->getFifthLastMove().isNormal()) {
      int j=Go::Position::pos2x(board->getFifthLastMove().getPosition(),size);
      int k=Go::Position::pos2y(board->getFifthLastMove().getPosition(),size);
      data[10*size*size+size*j+k]=exp(-4.0*params->cnn_lastmove_decay);
    }
    if (board->getSixLastMove().isNormal()) {
      int j=Go::Position::pos2x(board->getSixLastMove().getPosition(),size);
      int k=Go::Position::pos2y(board->getSixLastMove().getPosition(),size);
      data[11*size*size+size*j+k]=exp(-4.0*params->cnn_lastmove_decay);
    }
  }
  else {
    if (board->getThirdLastMove().isNormal()) {
      int j=Go::Position::pos2x(board->getThirdLastMove().getPosition(),size);
      int k=Go::Position::pos2y(board->getThirdLastMove().getPosition(),size);
      data[12*size*size+size*j+k]=1.0;
    }
    if (board->getForthLastMove().isNormal()) {
      int j=Go::Position::pos2x(board->getForthLastMove().getPosition(),size);
      int k=Go::Position::pos2y(board->getForthLastMove().getPosition(),size);
      data[13*size*size+size*j+k]=1.0;
    }
  }
}
    

  Blob<float> *b=new Blob<float>(1,caffe_test_net_input_dim[net_num],size,size);
  b->set_cpu_data(data);
  vector<Blob<float>*> bottom;
  bottom.push_back(b); 
  //cudaSetDeviceFlags(cudaDeviceBlockingSync);
  Caffe::set_mode(Caffe::GPU);
  const vector<Blob<float>*>& rr =  caffe_test_net[net_num_multi_gpu]->Forward(bottom);
  //fprintf(stderr,"start\n");
  //clock_t tbegin = clock();
  //for (int i=0;i<50;i++) {
  //  const vector<Blob<float>*>& result =  caffe_test_net->Forward(bottom);
  //}
  //clock_t tend = clock();
  //fprintf(stderr,"end %f\n",double(tend - tbegin) / CLOCKS_PER_SEC);
  //for (int j=0;j<19;j++)
	//{
	//for (int k=0;k<19;k++)
	//	{
	//    fprintf(stderr,"%5.3f ",rr[0]->cpu_data()[j*19+k]);
	//	}
	//fprintf(stderr,"\n");
	//}
  for (int i=0;i<size*size;i++) {
	  result[i]=rr[0]->cpu_data()[i];
    if (result[i]<0.00001) result[i]=0.00001;
  }

//fprintf(stderr,"value cnn %f\n",value);
if (v!=NULL) {
  *v=1.0-rr[1]->cpu_data()[0];
}
  delete[] data;
  delete b;
#else
  fprintf(stderr," caffe not availible, compile with-caffe\n");
#endif
}

float Engine::getCNNwr(Go::Board *board,Go::Color col)
{
#ifdef HAVE_CAFFE
  if (board->getSize()!=19) {
		fprintf(stderr,"only 19x19 supported by CNN\n");
		return 0;
	}
	float *data;
	data= new float[3*19*19];
	//fprintf(stderr,"2\n");
	if (col==Go::BLACK) {
	  for (int j=0;j<19;j++)
	    for (int k=0;k<19;k++)
		  {//fprintf(stderr,"%d %d %d\n",i,j,k);
	      if (board->getColor(Go::Position::xy2pos(j,k,19))==Go::BLACK)
	          {
			  data[j*19+k]=1.0;
			  data[19*19+19*j+k]=0.0;
			  }
	      else if (board->getColor(Go::Position::xy2pos(j,k,19))==Go::WHITE)
		      {
			  data[j*19+k]=0.0;
			  data[19*19+19*j+k]=1.0;
			  }
	      else
	          {
			  data[j*19+k]=0.0;
			  data[19*19+19*j+k]=0.0;
			  }
        data[2*19*19+j*19+k]=komi;
	    }
	}
	if (col==Go::WHITE) {
	  for (int j=0;j<19;j++)
	    for (int k=0;k<19;k++)
		  {//fprintf(stderr,"%d %d %d\n",i,j,k);
	      if (board->getColor(Go::Position::xy2pos(j,k,19))==Go::BLACK)
	          {
			  data[j*19+k]=0.0;
			  data[19*19+19*j+k]=1.0;
			  }
	      else if (board->getColor(Go::Position::xy2pos(j,k,19))==Go::WHITE)
		      {
			  data[j*19+k]=1.0;
			  data[19*19+19*j+k]=0.0;
			  }
	      else
	          {
			  data[j*19+k]=0.0;
			  data[19*19+19*j+k]=0.0;
			  }
        data[2*19*19+j*19+k]=-komi;
    }
	}
  Blob<float> *b=new Blob<float>(1,3,19,19);
  b->set_cpu_data(data);
  vector<Blob<float>*> bottom;
  bottom.push_back(b); 
  const vector<Blob<float>*>& rr =  caffe_area_net->Forward(bottom);
  float wr_sum=0;
  for (int i=0;i<361;i++) {
    wr_sum+=rr[1]->cpu_data()[i];
  }
  
  delete[] data;
  delete b;
  return 1.0-wr_sum/361.0;
#else
  fprintf(stderr, " caffe is not availible, compile with-caffe\n");
  return 0;
#endif
}

void Engine::run(bool web_inf, std::string web_addr, int web_port)
{
  bool use_web=false;

  #ifdef HAVE_MPI
    if (mpirank==0)
    {
      if (mpisynced)
      {
        if (web_inf)
          use_web=true;
        else
          gtpe->run();
      }
    }
    else
      this->mpiCommandHandler();
  #else
    if (web_inf)
      use_web=true;
    else
      gtpe->run();
  #endif

  if (use_web)
  {
    #ifdef HAVE_WEB
      web=new Web(this,web_addr,web_port);
      web->run();
      delete web;
    #endif
  }
}

void Engine::postCmdLineArgs(bool book_autoload)
{
  params->rand_seed=threadpool->getThreadZero()->getSettings()->rand->getSeed();
  #ifdef HAVE_MPI
    gtpe->getOutput()->printfDebug("seed of rank %d: %lu\n",mpirank,params->rand_seed);
  #else
    gtpe->getOutput()->printfDebug("seed: %lu\n",params->rand_seed);
  #endif
  if (book_autoload)
  {
    bool loaded=false;
    if (!loaded)
    {
      std::string filename="book.dat";
      MPIRANK0_ONLY(gtpe->getOutput()->printfDebug("loading opening book from '%s'... ",filename.c_str()););
      loaded=book->loadFile(filename);
      MPIRANK0_ONLY(
        if (loaded)
          gtpe->getOutput()->printfDebug("done\n");
        else
          gtpe->getOutput()->printfDebug("error\n");
      );
    }
    #ifdef TOPSRCDIR
      if (!loaded)
      {
        std::string filename=TOPSRCDIR "/book.dat";
        MPIRANK0_ONLY(gtpe->getOutput()->printfDebug("loading opening book from '%s'... ",filename.c_str()););
        loaded=book->loadFile(filename);
        MPIRANK0_ONLY(
          if (loaded)
            gtpe->getOutput()->printfDebug("done\n");
          else
            gtpe->getOutput()->printfDebug("error\n");
        );
      }
    #endif
    
    MPIRANK0_ONLY(
      if (!loaded)
        gtpe->getOutput()->printfDebug("no opening book loaded\n");
    );
  }
}

void Engine::updateParameter(std::string id)
{
  if (id=="move_policy")
  {
    if (params->move_policy_string=="uct")
      params->move_policy=Parameters::MP_UCT;
    else if (params->move_policy_string=="1-ply")
      params->move_policy=Parameters::MP_ONEPLY;
    else if (params->move_policy_string=="playout")
      params->move_policy=Parameters::MP_PLAYOUT;
    else
      params->move_policy=Parameters::MP_CNN;
  }
  else if (id=="uct_keep_subtree")
  {
    this->clearMoveTree();
  }
  else if (id=="uct_symmetry_use")
  {
    if (params->uct_symmetry_use)
      currentboard->turnSymmetryOn();
    else
    {
      currentboard->turnSymmetryOff();
      this->clearMoveTree();
    }
  }
  else if (id=="rand_seed")
  {
    threadpool->setRandomSeeds(params->rand_seed);
    params->rand_seed=threadpool->getThreadZero()->getSettings()->rand->getSeed();
  }
  else if (id=="interrupts_enabled")
  {
    gtpe->setWorkerEnabled(params->interrupts_enabled);
  }
  else if (id=="pondering_enabled")
  {
    gtpe->setPonderEnabled(params->pondering_enabled);
  }
  else if (id=="thread_count")
  {
    if (threadpool->getSize()!=params->thread_count)
    {
      delete threadpool;
      threadpool = new Worker::Pool(params);
    }
  }
  else if (id=="playouts_per_move")
  {
    if (params->playouts_per_move>params->playouts_per_move_max)
      params->playouts_per_move_max=params->playouts_per_move;
    if (params->playouts_per_move<params->playouts_per_move_min)
      params->playouts_per_move_min=params->playouts_per_move;
  }
  else if (id=="dt_selection_policy")
  {
    if (params->dt_selection_policy_string == "win_loss_separate")
      params->dt_selection_policy = Parameters::SP_WIN_LOSS_SEPARATE;
    else if (params->dt_selection_policy_string == "weighted_win_loss_separate")
      params->dt_selection_policy = Parameters::SP_WEIGHTED_WIN_LOSS_SEPARATE;
    else if (params->dt_selection_policy_string == "winrate_entropy")
      params->dt_selection_policy = Parameters::SP_WINRATE_ENTROPY;
    else if (params->dt_selection_policy_string == "weighted_winrate_entropy")
      params->dt_selection_policy = Parameters::SP_WEIGHTED_WINRATE_ENTROPY;
    else if (params->dt_selection_policy_string == "symmetric_separate")
      params->dt_selection_policy = Parameters::SP_SYMMETRIC_SEPARATE;
    else if (params->dt_selection_policy_string == "weighted_symmetric_separate")
      params->dt_selection_policy = Parameters::SP_WEIGHTED_SYMMETRIC_SEPARATE;
    else if (params->dt_selection_policy_string == "classification_separate")
      params->dt_selection_policy = Parameters::SP_CLASSIFICATION_SEPARATE;
    else if (params->dt_selection_policy_string == "robust_descent_split")
      params->dt_selection_policy = Parameters::SP_ROBUST_DESCENT_SPLIT;
    else if (params->dt_selection_policy_string == "robust_win_split")
      params->dt_selection_policy = Parameters::SP_ROBUST_WIN_SPLIT;
    else if (params->dt_selection_policy_string == "robust_loss_split")
      params->dt_selection_policy = Parameters::SP_ROBUST_LOSS_SPLIT;
    else if (params->dt_selection_policy_string == "entropy_descent_split")
      params->dt_selection_policy = Parameters::SP_ENTROPY_DESCENT_SPLIT;
    else if (params->dt_selection_policy_string == "entropy_win_split")
      params->dt_selection_policy = Parameters::SP_ENTROPY_WIN_SPLIT;
    else if (params->dt_selection_policy_string == "entropy_loss_split")
      params->dt_selection_policy = Parameters::SP_ENTROPY_LOSS_SPLIT;
    else if (params->dt_selection_policy_string == "winrate_split")
      params->dt_selection_policy = Parameters::SP_WINRATE_SPLIT;
    else
      params->dt_selection_policy = Parameters::SP_DESCENT_SPLIT;
  }
}

void Engine::addGtpCommands()
{
  gtpe->addFunctionCommand("boardsize",this,&Engine::gtpBoardSize);
  gtpe->addFunctionCommand("clear_board",this,&Engine::gtpClearBoard);
  gtpe->addFunctionCommand("komi",this,&Engine::gtpKomi);
  gtpe->addFunctionCommand("play",this,&Engine::gtpPlay);
  gtpe->addFunctionCommand("genmove",this,&Engine::gtpGenMove);
  gtpe->addFunctionCommand("reg_genmove",this,&Engine::gtpRegGenMove);
  gtpe->addFunctionCommand("reg_ownerat",this,&Engine::gtpRegOwnerAt);
  gtpe->addFunctionCommand("sg_compare_float",this,&Engine::gtpSgCompareFloat);
  gtpe->addFunctionCommand("kgs-genmove_cleanup",this,&Engine::gtpGenMoveCleanup);
  gtpe->addFunctionCommand("showboard",this,&Engine::gtpShowBoard);
  gtpe->addFunctionCommand("final_score",this,&Engine::gtpFinalScore);
  gtpe->addFunctionCommand("final_status_list",this,&Engine::gtpFinalStatusList);
  gtpe->addFunctionCommand("undo",this,&Engine::gtpUndo);
  gtpe->addFunctionCommand("kgs-chat",this,&Engine::gtpChat);
  gtpe->addFunctionCommand("kgs-game_over",this,&Engine::gtpGameOver);
  gtpe->addFunctionCommand("echo",this,&Engine::gtpEcho);
  gtpe->addFunctionCommand("place_free_handicap",this,&Engine::gtpPlaceFreeHandicap);
  gtpe->addFunctionCommand("set_free_handicap",this,&Engine::gtpSetFreeHandicap);
  
  gtpe->addFunctionCommand("param",this,&Engine::gtpParam);
  gtpe->addFunctionCommand("showliberties",this,&Engine::gtpShowLiberties);
  gtpe->addFunctionCommand("showvalidmoves",this,&Engine::gtpShowValidMoves);
  gtpe->addFunctionCommand("showgroupsize",this,&Engine::gtpShowGroupSize);
  gtpe->addFunctionCommand("showpatternmatches",this,&Engine::gtpShowPatternMatches);
  gtpe->addFunctionCommand("loadpatterns",this,&Engine::gtpLoadPatterns);
  gtpe->addFunctionCommand("clearpatterns",this,&Engine::gtpClearPatterns);
  gtpe->addFunctionCommand("doboardcopy",this,&Engine::gtpDoBoardCopy);
  gtpe->addFunctionCommand("featurematchesat",this,&Engine::gtpFeatureMatchesAt);
  gtpe->addFunctionCommand("featureprobdistribution",this,&Engine::gtpFeatureProbDistribution);
  gtpe->addFunctionCommand("listallpatterns",this,&Engine::gtpListAllPatterns);
  gtpe->addFunctionCommand("loadfeaturegammas",this,&Engine::gtpLoadFeatureGammas);
  gtpe->addFunctionCommand("savefeaturegammas",this,&Engine::gtpSaveFeatureGammas);
  gtpe->addFunctionCommand("loadcnnp",this,&Engine::gtpLoadCNNp);
  gtpe->addFunctionCommand("loadcnnt",this,&Engine::gtpLoadCNNt);
  gtpe->addFunctionCommand("savefeaturecircbinarc",this,&Engine::gtpSaveFeatureCircularBinary);
  gtpe->addFunctionCommand("loadfeaturecircbinarc",this,&Engine::gtpLoadFeatureCircularBinary);
  gtpe->addFunctionCommand("loadcircpatterns",this,&Engine::gtpLoadCircPatterns);
  gtpe->addFunctionCommand("loadcircpatternsnot",this,&Engine::gtpLoadCircPatternsNot);
  gtpe->addFunctionCommand("savecircpatternvalues",this,&Engine::gtpSaveCircPatternValues);
  gtpe->addFunctionCommand("loadcircpatternvalues",this,&Engine::gtpLoadCircPatternValues);
  gtpe->addFunctionCommand("listfeatureids",this,&Engine::gtpListFeatureIds);
  gtpe->addFunctionCommand("showcfgfrom",this,&Engine::gtpShowCFGFrom);
  gtpe->addFunctionCommand("showcircdistfrom",this,&Engine::gtpShowCircDistFrom);
  gtpe->addFunctionCommand("listcircpatternsat",this,&Engine::gtpListCircularPatternsAt);
  gtpe->addFunctionCommand("listcircpatternsatsize",this,&Engine::gtpListCircularPatternsAtSize);
  gtpe->addFunctionCommand("listcircpatternsatsizenot",this,&Engine::gtpListCircularPatternsAtSizeNot);
  gtpe->addFunctionCommand("listallcircularpatterns",this,&Engine::gtpListAllCircularPatterns);
  gtpe->addFunctionCommand("listadjacentgroupsof",this,&Engine::gtpListAdjacentGroupsOf);
  
  gtpe->addFunctionCommand("time_settings",this,&Engine::gtpTimeSettings);
  gtpe->addFunctionCommand("time_left",this,&Engine::gtpTimeLeft);
  
  gtpe->addFunctionCommand("donplayouts",this,&Engine::gtpDoNPlayouts);
  gtpe->addFunctionCommand("solidgroupat",this,&Engine::gtpSolidGroupAt);
  gtpe->addFunctionCommand("donplayoutsaround",this,&Engine::gtpDoNPlayoutsAround);
  gtpe->addFunctionCommand("outputsgf",this,&Engine::gtpOutputSGF);
  gtpe->addFunctionCommand("playoutsgf",this,&Engine::gtpPlayoutSGF);
  gtpe->addFunctionCommand("playoutsgf_pos",this,&Engine::gtpPlayoutSGF_pos);
  gtpe->addFunctionCommand("gamesgf",this,&Engine::gtpGameSGF);
  
  gtpe->addFunctionCommand("explainlastmove",this,&Engine::gtpExplainLastMove);
  gtpe->addFunctionCommand("boardstats",this,&Engine::gtpBoardStats);
  gtpe->addFunctionCommand("showsymmetrytransforms",this,&Engine::gtpShowSymmetryTransforms);
  gtpe->addFunctionCommand("shownakadecenters",this,&Engine::gtpShowNakadeCenters);
  gtpe->addFunctionCommand("showtreelivegfx",this,&Engine::gtpShowTreeLiveGfx);
  gtpe->addFunctionCommand("describeengine",this,&Engine::gtpDescribeEngine);
  
  gtpe->addFunctionCommand("bookshow",this,&Engine::gtpBookShow);
  gtpe->addFunctionCommand("bookadd",this,&Engine::gtpBookAdd);
  gtpe->addFunctionCommand("bookremove",this,&Engine::gtpBookRemove);
  gtpe->addFunctionCommand("bookclear",this,&Engine::gtpBookClear);
  gtpe->addFunctionCommand("bookload",this,&Engine::gtpBookLoad);
  gtpe->addFunctionCommand("booksave",this,&Engine::gtpBookSave);
  
  gtpe->addFunctionCommand("showcurrenthash",this,&Engine::gtpShowCurrentHash);
  gtpe->addFunctionCommand("showsafepositions",this,&Engine::gtpShowSafePositions);
  gtpe->addFunctionCommand("dobenchmark",this,&Engine::gtpDoBenchmark);
  gtpe->addFunctionCommand("showcriticality",this,&Engine::gtpShowCriticality);
  gtpe->addFunctionCommand("showterritory",this,&Engine::gtpShowTerritory);
  gtpe->addFunctionCommand("showplayoutgammas",this,&Engine::gtpShowPlayoutGammas);
  gtpe->addFunctionCommand("showprobabilitycnn",this,&Engine::gtpShowProbabilityCNN);
  gtpe->addFunctionCommand("showterritorycnn",this,&Engine::gtpShowTerritoryCNN);
  gtpe->addFunctionCommand("showatarirespondat",this,&Engine::gtpShowAtariCaptureAttached);
  gtpe->addFunctionCommand("showterritoryat",this,&Engine::gtpShowTerritoryAt);
  gtpe->addFunctionCommand("showterritoryerror",this,&Engine::gtpShowTerritoryError);
  
  gtpe->addFunctionCommand("showmoveprobability",this,&Engine::gtpShowMoveProbability);
  gtpe->addFunctionCommand("showcorrelationmap",this,&Engine::gtpShowCorrelationMap);
  gtpe->addFunctionCommand("showratios",this,&Engine::gtpShowRatios);
  gtpe->addFunctionCommand("showreallibs",this,&Engine::gtpShowRealLibs);
  gtpe->addFunctionCommand("showtreeplayouts",this,&Engine::gtpShowTreePlayouts);
  gtpe->addFunctionCommand("showunprune",this,&Engine::gtpShowUnPrune);
  gtpe->addFunctionCommand("showunprunecolor",this,&Engine::gtpShowUnPruneColor);
  gtpe->addFunctionCommand("showownratios",this,&Engine::gtpShowOwnRatios);
  gtpe->addFunctionCommand("showraveratios",this,&Engine::gtpShowRAVERatios);
  gtpe->addFunctionCommand("showraveratioscolor",this,&Engine::gtpShowRAVERatiosColor);
  gtpe->addFunctionCommand("showraveratiosother",this,&Engine::gtpShowRAVERatiosOther);

  gtpe->addFunctionCommand("dtload",this,&Engine::gtpDTLoad);
  gtpe->addFunctionCommand("dtclear",this,&Engine::gtpDTClear);
  gtpe->addFunctionCommand("dtprint",this,&Engine::gtpDTPrint);
  gtpe->addFunctionCommand("dtat",this,&Engine::gtpDTAt);
  gtpe->addFunctionCommand("dtupdate",this,&Engine::gtpDTUpdate);
  gtpe->addFunctionCommand("dtsave",this,&Engine::gtpDTSave);
  gtpe->addFunctionCommand("dtset",this,&Engine::gtpDTSet);
  gtpe->addFunctionCommand("dtdistribution",this,&Engine::gtpDTDistribution);
  gtpe->addFunctionCommand("dtstats",this,&Engine::gtpDTStats);
  gtpe->addFunctionCommand("dtpath",this,&Engine::gtpDTPath);
  gtpe->addFunctionCommand("cputime",this,&Engine::gtpCPUtime);
  gtpe->addFunctionCommand("version",this,&Engine::gtpVERSION);
  
  //gtpe->addAnalyzeCommand("final_score","Final Score","string");
  //gtpe->addAnalyzeCommand("showboard","Show Board","string");
  gtpe->addAnalyzeCommand("boardstats","Board Stats","string");
  
  //gtpe->addAnalyzeCommand("showsymmetrytransforms","Show Symmetry Transforms","sboard");
  //gtpe->addAnalyzeCommand("showliberties","Show Liberties","sboard");
  //gtpe->addAnalyzeCommand("showvalidmoves","Show Valid Moves","sboard");
  //gtpe->addAnalyzeCommand("showgroupsize","Show Group Size","sboard");
  gtpe->addAnalyzeCommand("showcfgfrom %%p","Show CFG From","sboard");
  gtpe->addAnalyzeCommand("showcircdistfrom %%p","Show Circular Distance From","sboard");
  gtpe->addAnalyzeCommand("listcircpatternsat %%p","List Circular Patterns At","string");
  gtpe->addAnalyzeCommand("listadjacentgroupsof %%p","List Adjacent Groups Of","string");
  //gtpe->addAnalyzeCommand("showsafepositions","Show Safe Positions","gfx");
  gtpe->addAnalyzeCommand("showpatternmatches","Show Pattern Matches","sboard");
  gtpe->addAnalyzeCommand("showratios","Show Ratios","sboard");
  gtpe->addAnalyzeCommand("showreallibs","Show Real Libs","sboard");
  gtpe->addAnalyzeCommand("showtreeplayouts %%c","Show Tree Playouts","sboard");
  gtpe->addAnalyzeCommand("showunprune","Show UnpruneFactor","sboard");
  gtpe->addAnalyzeCommand("showunprunecolor","Show UnpruneFactor (color display)","cboard");
  gtpe->addAnalyzeCommand("showownratios","Show Own Ratios","sboard");
  gtpe->addAnalyzeCommand("showraveratios","Show RAVE Ratios","sboard");
  gtpe->addAnalyzeCommand("showraveratioscolor","Show RAVE Ratios (color display)","cboard");
  gtpe->addAnalyzeCommand("showraveratiosother","Show RAVE Ratios (other color)","sboard");
  //gtpe->addAnalyzeCommand("shownakadecenters","Show Nakade Centers","sboard");
  gtpe->addAnalyzeCommand("featurematchesat %%p","Feature Matches At","string");
  gtpe->addAnalyzeCommand("featureprobdistribution","Feature Probability Distribution","cboard");
  gtpe->addAnalyzeCommand("dtdistribution","Decision Tree Distribution","cboard");
  gtpe->addAnalyzeCommand("loadfeaturegammas %%r","Load Feature Gammas","none");
  gtpe->addAnalyzeCommand("showcriticality","Show Criticality","cboard");
  gtpe->addAnalyzeCommand("showterritory","Show Territory","dboard");
  gtpe->addAnalyzeCommand("showplayoutgammas %%c","Show Playout Gammas","sboard");
  gtpe->addAnalyzeCommand("showterritorycnn %%c","Show Territory CNN","dboard");
  gtpe->addAnalyzeCommand("showprobabilitycnn %%c","Show Probability CNN","dboard");
  gtpe->addAnalyzeCommand("showatarirespondat %%p %%c","Show Atarirespond At","sboard");
  gtpe->addAnalyzeCommand("showterritoryat %%p %%c","Show Territory At","dboard");
  gtpe->addAnalyzeCommand("showterritoryerror","Show Territory Error","dboard");
  gtpe->addAnalyzeCommand("showmoveprobability","Show Move Probability","dboard");
  gtpe->addAnalyzeCommand("showcorrelationmap","Show Correlation","dboard");
  gtpe->addAnalyzeCommand("showtreelivegfx","Show Tree Live Gfx","gfx");
  gtpe->addAnalyzeCommand("loadpatterns %%r","Load Patterns","none");
  gtpe->addAnalyzeCommand("clearpatterns","Clear Patterns","none");
  //gtpe->addAnalyzeCommand("doboardcopy","Do Board Copy","none");
  //gtpe->addAnalyzeCommand("showcurrenthash","Show Current Hash","string");
  
  gtpe->addAnalyzeCommand("param general","Parameters (General)","param");
  gtpe->addAnalyzeCommand("param tree","Parameters (Tree)","param");
  gtpe->addAnalyzeCommand("param playout","Parameters (Playout)","param");
  gtpe->addAnalyzeCommand("param csplayout","Parameters (CS - Playout)","param");
  gtpe->addAnalyzeCommand("param time","Parameters (Time)","param");
  gtpe->addAnalyzeCommand("param rules","Parameters (Rules)","param");
  gtpe->addAnalyzeCommand("param other","Parameters (Other)","param");
  gtpe->addAnalyzeCommand("param test","Parameters (Test)","param");
  gtpe->addAnalyzeCommand("param cnn","Parameters (CNN)","param");
  gtpe->addAnalyzeCommand("solidgroupat %%p","Solid Group At","none");
  gtpe->addAnalyzeCommand("donplayoutsaround %%s %%p","Do N Playouts around","none");
  gtpe->addAnalyzeCommand("donplayouts %%s","Do N Playouts","none");
  gtpe->addAnalyzeCommand("donplayouts 1","Do 1 Playout","none");
  gtpe->addAnalyzeCommand("donplayouts 100","Do 100 Playouts","none");
  gtpe->addAnalyzeCommand("donplayouts 1000","Do 1k Playouts","none");
  gtpe->addAnalyzeCommand("donplayouts 10000","Do 10k Playouts","none");
  gtpe->addAnalyzeCommand("donplayouts 100000","Do 100k Playouts","none");
  gtpe->addAnalyzeCommand("outputsgf %%w","Output SGF","none");
  gtpe->addAnalyzeCommand("playoutsgf %%w %%c","Playout to SGF (win of color)","none");
  gtpe->addAnalyzeCommand("playoutsgf_pos %%w %%c %%p","Playout to SGF (win of color at pos)","none");
 
  gtpe->addAnalyzeCommand("bookshow","Book Show","gfx");
  gtpe->addAnalyzeCommand("bookadd %%p","Book Add","none");
  gtpe->addAnalyzeCommand("bookremove %%p","Book Remove","none");
  gtpe->addAnalyzeCommand("bookclear","Book Clear","none");
  gtpe->addAnalyzeCommand("bookload %%r","Book Load","none");
  gtpe->addAnalyzeCommand("booksave %%w","Book Save","none");
  
  gtpe->setInterruptFlag(&stopthinking);
  gtpe->setPonderer(&Engine::ponderWrapper,this,&stoppondering);
  gtpe->setWorkerEnabled(params->interrupts_enabled);
  gtpe->setPonderEnabled(params->pondering_enabled);
}

void Engine::gtpBoardSize(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  if (cmd->numArgs()!=1)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("size is required");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  int newsize=cmd->getIntArg(0);
  
  if (newsize<BOARDSIZE_MIN || newsize>BOARDSIZE_MAX)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("unacceptable size");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  me->setBoardSize(newsize);
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->endResponse();
}

void Engine::gtpClearBoard(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  me->clearBoard();
  
  //assume that this will be the start of a new game
  me->time->resetAll();
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->endResponse();
}

void Engine::gtpKomi(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  if (cmd->numArgs()!=1)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("komi is required");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  me->setKomi(cmd->getFloatArg(0));
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->endResponse();
}

void Engine::gtpPlay(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  //gtpe->getOutput()->printfDebug("gtpPlay called\n");
  if (cmd->numArgs()!=2)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("move is required");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  Gtp::Color gtpcol = cmd->getColorArg(0);
  Gtp::Vertex vert = cmd->getVertexArg(1);
  
  if (gtpcol==Gtp::INVALID || (vert.x==-3 && vert.y==-3))
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("invalid move");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  Go::Move move=Go::Move((gtpcol==Gtp::BLACK ? Go::BLACK : Go::WHITE),vert.x,vert.y,me->boardsize);
  if (!me->isMoveAllowed(move))
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("illegal move");
    gtpe->getOutput()->endResponse();
    return;
  }
  me->makeMove(move);
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->endResponse();
}

void Engine::gtpGenMove(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  //gtpe->getOutput()->printfDebug("gtpGenMove called\n");
  
  if (cmd->numArgs()!=1)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("color is required");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  Gtp::Color gtpcol = cmd->getColorArg(0);
  
  if (gtpcol==Gtp::INVALID)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("invalid color");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  Go::Move *move;
  me->probabilitymap->decay(0);
  me->generateMove((gtpcol==Gtp::BLACK ? Go::BLACK : Go::WHITE),&move,true);
  Gtp::Vertex vert={move->getX(me->boardsize),move->getY(me->boardsize)};
  delete move;
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printVertex(vert);
  gtpe->getOutput()->endResponse();
}

void Engine::gtpGenMoveCleanup(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  if (cmd->numArgs()!=1)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("color is required");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  Gtp::Color gtpcol = cmd->getColorArg(0);
  
  if (gtpcol==Gtp::INVALID)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("invalid color");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  if (!me->params->cleanup_in_progress)
  {
    me->params->cleanup_in_progress=true;
    if (!me->params->rules_all_stones_alive)
      me->clearMoveTree();
  }
  
  Go::Move *move;
  me->generateMove((gtpcol==Gtp::BLACK ? Go::BLACK : Go::WHITE),&move,true);
  Gtp::Vertex vert={move->getX(me->boardsize),move->getY(me->boardsize)};
  delete move;
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printVertex(vert);
  gtpe->getOutput()->endResponse();
}

void Engine::gtpRegGenMove(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  if (cmd->numArgs()!=1)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("color is required");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  Gtp::Color gtpcol = cmd->getColorArg(0);
  
  if (gtpcol==Gtp::INVALID)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("invalid color");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  Go::Move *move;
  me->generateMove((gtpcol==Gtp::BLACK ? Go::BLACK : Go::WHITE),&move,false);
  Gtp::Vertex vert={move->getX(me->boardsize),move->getY(me->boardsize)};
  delete move;
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printVertexUpperCase(vert);
  gtpe->getOutput()->endResponse();
}

void Engine::gtpShowBoard(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printString("\n");
  gtpe->getOutput()->printString(me->currentboard->toString());
  gtpe->getOutput()->endResponse(true);
}

void Engine::gtpUndo(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  if (me->params->undo_enable && me->undo())
  {
    gtpe->getOutput()->startResponse(cmd);
    gtpe->getOutput()->endResponse();
  }
  else
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("cannot undo");
    gtpe->getOutput()->endResponse();
  }
}

float Engine::getScoreKomi() 
{ 
  //own test, did not look too bad!!!
  //float dynamic_komi=7.5*komi_handicap*exp(-5.0*sqrt(komi_handicap)*(float)currentboard->getMovesMade()/boardsize/boardsize);
  //  if (dynamic_komi<5)
  //    dynamic_komi=0;  //save the end game
  //Formula Petr Baudis dynamic komi (N=200 for 19x19 board scaled to smaller boards)
  float dynamic_komi=0;
  if (params->dynkomi_enabled)
  {
    dynamic_komi=7.0*komi_handicap*(1-(float)currentboard->getMovesMade()/(boardsize*boardsize*200.0/19.0/19.0));
    if (dynamic_komi<0)
      dynamic_komi=0;  //save the end game

    if (params->recalc_dynkomi_limit>0 && movetree!=NULL && movetree->getRobustChild()!=NULL)
    {
      switch (movetree->getRobustChild()->getMove().getColor())
      {
        case Go::BLACK:
          recalc_dynkomi=movetree->getRobustChild()->getScoreMean()*params->test_p11;
          //if (recalc_dynkomi<0) recalc_dynkomi=0;
          break;
        case Go::WHITE:
          recalc_dynkomi=-movetree->getRobustChild()->getScoreMean()*params->test_p11;
          //if (recalc_dynkomi>0) recalc_dynkomi=0;
          break;
        default:
          break;
      }
      if (recalc_dynkomi>30) recalc_dynkomi=params->recalc_dynkomi_limit;
      else
        if (recalc_dynkomi<-params->recalc_dynkomi_limit) recalc_dynkomi=-params->recalc_dynkomi_limit;
      return komi+komi_handicap+dynamic_komi+recalc_dynkomi;
    } 
    else
      return komi+komi_handicap+dynamic_komi;
  }
  return komi+komi_handicap;
}

float Engine::getHandiKomi() const
{ 
  return komi+komi_handicap; 
}

void Engine::gtpFinalScore(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  float score;
  
  if (!me->params->rules_all_stones_alive)
  {
    int plts=me->params->rules_all_stones_alive_playouts-me->movetree->getPlayouts();
    if (plts>0)
      me->doNPlayouts(plts);
  }
  
  if (me->params->rules_all_stones_alive || me->params->cleanup_in_progress)
    score=me->currentboard->score()-me->komi-me->komi_handicap;
  else
    score=me->currentboard->territoryScore(me->territorymap,me->params->territory_threshold)-me->komi-me->komi_handicap;
  
  gtpe->getOutput()->startResponse(cmd);
  if (score==0) // jigo
    gtpe->getOutput()->printf("0");
  else
    gtpe->getOutput()->printScore(score);
  gtpe->getOutput()->endResponse();
}

void Engine::gtpFinalStatusList(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  if (cmd->numArgs()!=1)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("argument is required");
    gtpe->getOutput()->endResponse();
    return;
  }

  std::string arg = cmd->getStringArg(0);
  std::transform(arg.begin(),arg.end(),arg.begin(),::tolower);

  if (!me->params->rules_all_stones_alive)
  {
    int plts=me->params->rules_all_stones_alive_playouts-me->movetree->getPlayouts();
    if (plts>0)
      me->doNPlayouts(plts);
  }
  
  if (arg=="dead")
  {
    if (me->params->rules_all_stones_alive || me->params->cleanup_in_progress)
    {
      gtpe->getOutput()->startResponse(cmd);
      gtpe->getOutput()->endResponse();
    }
    else
    {
      gtpe->getOutput()->startResponse(cmd);
      for (int x=0;x<me->boardsize;x++)
      {
        for (int y=0;y<me->boardsize;y++)
        {
          int pos=Go::Position::xy2pos(x,y,me->boardsize);
          if (me->currentboard->boardData()[pos].color!=Go::EMPTY)
          {
            if (!me->currentboard->isAlive(me->territorymap,me->params->territory_threshold,pos))
            {
              Gtp::Vertex vert={x,y};
              gtpe->getOutput()->printVertex(vert);
              gtpe->getOutput()->printf(" ");
            }
          }
        }
      }
      gtpe->getOutput()->endResponse();
    }
  }
  else if (arg=="alive")
  {
    gtpe->getOutput()->startResponse(cmd);
    for (int x=0;x<me->boardsize;x++)
    {
      for (int y=0;y<me->boardsize;y++)
      {
        int pos=Go::Position::xy2pos(x,y,me->boardsize);
        if (me->currentboard->boardData()[pos].color!=Go::EMPTY)
        {
          if (me->params->rules_all_stones_alive || me->params->cleanup_in_progress || me->currentboard->isAlive(me->territorymap,me->params->territory_threshold,pos))
          {
            Gtp::Vertex vert={x,y};
            gtpe->getOutput()->printVertex(vert);
            gtpe->getOutput()->printf(" ");
          }
        }
      }
    }
    gtpe->getOutput()->endResponse();
  }
  else
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("argument is not supported");
    gtpe->getOutput()->endResponse();
  }
}

void Engine::gtpShowLiberties(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printString("\n");
  for (int y=me->boardsize-1;y>=0;y--)
  {
    for (int x=0;x<me->boardsize;x++)
    {
      int pos=Go::Position::xy2pos(x,y,me->boardsize);
      if (me->currentboard->boardData()[pos].group==NULL)
        gtpe->getOutput()->printf("\"\" ");
      else
      {
        int lib=me->currentboard->boardData()[pos].group->find()->numOfPseudoLiberties();
        gtpe->getOutput()->printf("\"%d",lib);
        if (me->currentboard->boardData()[pos].group->find()->inAtari())
          gtpe->getOutput()->printf("!");
        gtpe->getOutput()->printf("\" ");
      }
    }
    gtpe->getOutput()->printf("\n");
  }

  gtpe->getOutput()->endResponse(true);
}

void Engine::gtpShowValidMoves(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printString("\n");
  for (int y=me->boardsize-1;y>=0;y--)
  {
    for (int x=0;x<me->boardsize;x++)
    {
      gtpe->getOutput()->printf("\"");
      if (me->currentboard->validMove(Go::Move(Go::BLACK,Go::Position::xy2pos(x,y,me->boardsize))))
        gtpe->getOutput()->printf("B");
      if (me->currentboard->validMove(Go::Move(Go::WHITE,Go::Position::xy2pos(x,y,me->boardsize))))
        gtpe->getOutput()->printf("W");
      gtpe->getOutput()->printf("\" ");
    }
    gtpe->getOutput()->printf("\n");
  }

  gtpe->getOutput()->endResponse(true);
}

void Engine::gtpShowGroupSize(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printString("\n");
  for (int y=me->boardsize-1;y>=0;y--)
  {
    for (int x=0;x<me->boardsize;x++)
    {
      int pos=Go::Position::xy2pos(x,y,me->boardsize);
      if (me->currentboard->boardData()[pos].group==NULL)
        gtpe->getOutput()->printf("\"\" ");
      else
      {
        int stones=me->currentboard->boardData()[pos].group->find()->numOfStones();
        gtpe->getOutput()->printf("\"%d\" ",stones);
      }
    }
    gtpe->getOutput()->printf("\n");
  }

  gtpe->getOutput()->endResponse(true);
}

void Engine::gtpShowPatternMatches(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printString("\n");
  for (int y=me->boardsize-1;y>=0;y--)
  {
    for (int x=0;x<me->boardsize;x++)
    {
      int pos=Go::Position::xy2pos(x,y,me->boardsize);
      gtpe->getOutput()->printf("\"");
      if (me->currentboard->validMove(Go::Move(Go::BLACK,pos)) && me->patterntable->isPattern(Pattern::ThreeByThree::makeHash(me->currentboard,pos)))
        gtpe->getOutput()->printf("B");
      if (me->currentboard->validMove(Go::Move(Go::WHITE,pos)) && me->patterntable->isPattern(Pattern::ThreeByThree::invert(Pattern::ThreeByThree::makeHash(me->currentboard,pos))))
        gtpe->getOutput()->printf("W");
      gtpe->getOutput()->printf("\" ");
    }
    gtpe->getOutput()->printf("\n");
  }

  gtpe->getOutput()->endResponse(true);
}

void Engine::gtpShowRatios(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  Go::Color col=me->currentboard->nextToMove();
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printString("\n");
  for (int y=me->boardsize-1;y>=0;y--)
  {
    for (int x=0;x<me->boardsize;x++)
    {
      int pos=Go::Position::xy2pos(x,y,me->boardsize);
      Go::Move move=Go::Move(col,pos);
      Tree *tree=me->movetree->getChild(move);
      if (tree!=NULL)
      {
        float ratio=tree->getRatio();
        gtpe->getOutput()->printf("\"%.2f\"",ratio);
      }
      else
        gtpe->getOutput()->printf("\"\"");
    }
    gtpe->getOutput()->printf("\n");
  }

  gtpe->getOutput()->endResponse(true);
}


void Engine::gtpShowAtariCaptureAttached(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  //me->respondboard->scale(0.2);
  //Go::Color col=me->currentboard->nextToMove();
  if (cmd->numArgs()!=2)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("color vertex is required");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  Gtp::Vertex vert = cmd->getVertexArg(0);
  Gtp::Color gtpcol = cmd->getColorArg(1);

  int pos=Go::Position::xy2pos(vert.x,vert.y,me->boardsize);
  int num=0;
  int capt=0;
  std::list<std::pair<int,int>> responds=me->respondboard->getMoves(pos,(gtpcol==Gtp::BLACK)?Go::BLACK : Go::WHITE,num,capt);
  fprintf(stderr,"allmoves %d %d\n",num,capt);
  float rb[me->currentboard->getPositionMax()];
  for (int i=0;i<me->currentboard->getPositionMax();i++)
      rb[i]=0;
  if (num>0) {
    for (std::list<std::pair<int,int>>::iterator it=responds.begin();it!=responds.end();++it) {
      fprintf(stderr,"respondmoves %d %d move %s  %f\n",it->first,it->second,Go::Move((gtpcol==Gtp::BLACK)?Go::WHITE : Go::BLACK,it->first).toString(me->boardsize).c_str(),(float)it->second/num);
      rb[it->first]=-(float)it->second/num;
    }
  }
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printString("\n");
  for (int y=me->boardsize-1;y>=0;y--)
  {
    for (int x=0;x<me->boardsize;x++)
    {
      int pos=Go::Position::xy2pos(x,y,me->boardsize);
      int val=(int)(rb[pos]*1000);
      if (val>0)
        gtpe->getOutput()->printf("\"%d\"",(int)(rb[pos]*1000));
      else
        gtpe->getOutput()->printf("\"\"");  
    }
    gtpe->getOutput()->printf("\n");
  }

  gtpe->getOutput()->endResponse(true);
}

void Engine::gtpShowRealLibs(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
//  Go::Color col=me->currentboard->nextToMove();
  me->currentboard->calcSlowLibertyGroups();
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printString("\n");
  for (int y=me->boardsize-1;y>=0;y--)
  {
    for (int x=0;x<me->boardsize;x++)
    {
      int pos=Go::Position::xy2pos(x,y,me->boardsize);
      int reallibs=0;
      if (me->currentboard->inGroup(pos)) {
        //reallibs=me->currentboard->getGroup(pos)->real_libs;
        reallibs=me->currentboard->getGroup(pos)->numRealLibs();
        if (me->currentboard->getGroup(pos)->real_libs!=reallibs)
          fprintf(stderr,"this is not correct, liberties are wrong:(\n");
      }
      gtpe->getOutput()->printf("\"%d\"",reallibs);
    }
    gtpe->getOutput()->printf("\n");
  }
  gtpe->getOutput()->endResponse(true);
}

void Engine::gtpShowTreePlayouts(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  Go::IntBoard *treeboardBlack=new Go::IntBoard(me->boardsize); treeboardBlack->clear();
  Go::IntBoard *treeboardWhite=new Go::IntBoard(me->boardsize); treeboardWhite->clear();
  me->movetree->fillTreeBoard (treeboardBlack,treeboardWhite);
  Gtp::Color gtpcol = cmd->getColorArg(0);
  if (gtpcol==Gtp::INVALID)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("invalid color");
    gtpe->getOutput()->endResponse();
    return;
  }
  Go::IntBoard *treeboard=((gtpcol==Gtp::BLACK)?treeboardBlack:treeboardWhite);
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printString("\n");
  for (int y=me->boardsize-1;y>=0;y--)
  {
    for (int x=0;x<me->boardsize;x++)
    {
      int pos=Go::Position::xy2pos(x,y,me->boardsize);
      gtpe->getOutput()->printf("\"%.2f\"",log(treeboard->get(pos)+1));
    }
    gtpe->getOutput()->printf("\n");
  }

  gtpe->getOutput()->endResponse(true);
  delete treeboardBlack;
  delete treeboardWhite;
}

void Engine::gtpShowUnPrune(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  Go::Color col=me->currentboard->nextToMove();
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printString("\n");
  for (int y=me->boardsize-1;y>=0;y--)
  {
    for (int x=0;x<me->boardsize;x++)
    {
      int pos=Go::Position::xy2pos(x,y,me->boardsize);
      Go::Move move=Go::Move(col,pos);
      Tree *tree=me->movetree->getChild(move);
      if (tree!=NULL)
      {
        float ratio=tree->getUnPruneFactor();
        gtpe->getOutput()->printf("\"%.1f\"",ratio);
      }
      else
        gtpe->getOutput()->printf("\"\"");
    }
    gtpe->getOutput()->printf("\n");
  }

  gtpe->getOutput()->endResponse(true);
}

void Engine::gtpShowOwnRatios(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  Go::Color col=me->currentboard->nextToMove();
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printString("\n");
  for (int y=me->boardsize-1;y>=0;y--)
  {
    for (int x=0;x<me->boardsize;x++)
    {
      int pos=Go::Position::xy2pos(x,y,me->boardsize);
      Go::Move move=Go::Move(col,pos);
      Tree *tree=me->movetree->getChild(move);
      if (tree!=NULL)
      {
        float ratio=tree->getSelfOwner(me->boardsize);
        gtpe->getOutput()->printf("\"%.2f\"",ratio);
      }
      else
        gtpe->getOutput()->printf("\"\"");
    }
    gtpe->getOutput()->printf("\n");
  }

  gtpe->getOutput()->endResponse(true);
}



void Engine::gtpShowRAVERatios(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  Go::Color col=me->currentboard->nextToMove();
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printString("\n");
  for (int y=me->boardsize-1;y>=0;y--)
  {
    for (int x=0;x<me->boardsize;x++)
    {
      int pos=Go::Position::xy2pos(x,y,me->boardsize);
      Go::Move move=Go::Move(col,pos);
      Tree *tree=me->movetree->getChild(move);
      if (tree!=NULL)
      {
        float ratio=tree->getRAVERatio();
        gtpe->getOutput()->printf("\"%.2f\"",ratio);
      }
      else
        gtpe->getOutput()->printf("\"\"");
    }
    gtpe->getOutput()->printf("\n");
  }

  gtpe->getOutput()->endResponse(true);
}

void Engine::gtpShowRAVERatiosColor(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  Go::Color col=me->currentboard->nextToMove();
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printString("\n");
  float min=1000000;
  float max=0;
  for (int y=me->boardsize-1;y>=0;y--)
  {
    for (int x=0;x<me->boardsize;x++)
    {
      int pos=Go::Position::xy2pos(x,y,me->boardsize);
      Go::Move move=Go::Move(col,pos);
      Tree *tree=me->movetree->getChild(move);
      if (tree!=NULL)
      {
        float ratio=tree->getRAVERatio();
        if (ratio<min) min=ratio;
        if (ratio>max) max=ratio;
      }
    }
  }
  for (int y=me->boardsize-1;y>=0;y--)
  {
    for (int x=0;x<me->boardsize;x++)
    {
      int pos=Go::Position::xy2pos(x,y,me->boardsize);
      Go::Move move=Go::Move(col,pos);
      Tree *tree=me->movetree->getChild(move);
      float ratio;
      float crit=0;
      if (tree!=NULL)
      {
        ratio=tree->getRAVERatio();
        crit=(ratio-min)/(max-min);
        //fprintf(stderr,"%f %f %f %f\n",min,max,ratio,crit);
      }
      if (crit==0 && (!move.isNormal()))
          gtpe->getOutput()->printf("\"\" ");
      else
      {
          float r,g,b;
          float x=crit;
          
          // scale from blue-red
          r=x;
          if (r>1)
            r=1;
          g=0;
          b=1-r;
          
          if (r<0)
            r=0;
          if (g<0)
            g=0;
          if (b<0)
            b=0;
          gtpe->getOutput()->printf("#%02x%02x%02x ",(int)(r*255),(int)(g*255),(int)(b*255));
          //gtpe->getOutput()->printf("#%06x ",(int)(prob*(1<<24)));
        }
    }
    gtpe->getOutput()->printf("\n");
  }

  gtpe->getOutput()->endResponse(true);
}

void Engine::gtpShowUnPruneColor(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  Go::Color col=me->currentboard->nextToMove();
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printString("\n");
  float min=1000000;
  float max=-1000000;
  for (int y=me->boardsize-1;y>=0;y--)
  {
    for (int x=0;x<me->boardsize;x++)
    {
      int pos=Go::Position::xy2pos(x,y,me->boardsize);
      Go::Move move=Go::Move(col,pos);
      Tree *tree=me->movetree->getChild(move);
      if (tree!=NULL)
      {
        float ratio=(tree->getUnPruneFactor());
        if (ratio<min) min=ratio;
        if (ratio>max) max=ratio;
      }
    }
  }
  for (int y=me->boardsize-1;y>=0;y--)
  {
    for (int x=0;x<me->boardsize;x++)
    {
      int pos=Go::Position::xy2pos(x,y,me->boardsize);
      Go::Move move=Go::Move(col,pos);
      Tree *tree=me->movetree->getChild(move);
      float ratio;
      float crit=0;
      if (tree!=NULL)
      {
        ratio=(tree->getUnPruneFactor());
        crit=(ratio-min)/(max-min);
        //fprintf(stderr,"%f %f %f %f\n",min,max,ratio,crit);
      }
      if (crit==0 && (!move.isNormal()))
          gtpe->getOutput()->printf("\"\" ");
      else
      {
          float r,g,b;
          float x=crit;
          
          // scale from blue-red
          r=x;
          if (r>1)
            r=1;
          g=0;
          b=1-r;
          
          if (r<0)
            r=0;
          if (g<0)
            g=0;
          if (b<0)
            b=0;
          gtpe->getOutput()->printf("#%02x%02x%02x ",(int)(r*255),(int)(g*255),(int)(b*255));
          //gtpe->getOutput()->printf("#%06x ",(int)(prob*(1<<24)));
        }
    }
    gtpe->getOutput()->printf("\n");
  }

  gtpe->getOutput()->endResponse(true);
}
        
void Engine::gtpShowRAVERatiosOther(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  Go::Color col=me->currentboard->nextToMove();
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printString("\n");
  for (int y=me->boardsize-1;y>=0;y--)
  {
    for (int x=0;x<me->boardsize;x++)
    {
      int pos=Go::Position::xy2pos(x,y,me->boardsize);
      Go::Move move=Go::Move(col,pos);
      Tree *tree=me->movetree->getChild(move);
      if (tree!=NULL)
      {
        float ratio=tree->getRAVERatioOther();
        gtpe->getOutput()->printf("\"%.2f\"",ratio);
      }
      else
        gtpe->getOutput()->printf("\"\"");
    }
    gtpe->getOutput()->printf("\n");
  }

  gtpe->getOutput()->endResponse(true);
}

void Engine::gtpLoadPatterns(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  if (cmd->numArgs()!=1)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printf("need 1 arg");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  std::string patternfile=cmd->getStringArg(0);
  
  delete me->patterntable;
  me->patterntable=new Pattern::ThreeByThreeTable();
  bool success=me->patterntable->loadPatternFile(patternfile);
  
  if (success)
  {
    #ifdef HAVE_MPI
      if (me->mpirank==0)
      {
        me->mpiBroadcastCommand(MPICMD_LOADPATTERNS);
        me->mpiBroadcastString(patternfile);
      }
    #endif
    
    gtpe->getOutput()->startResponse(cmd);
    gtpe->getOutput()->printf("loaded pattern file: %s",patternfile.c_str());
    gtpe->getOutput()->endResponse();
  }
  else
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printf("error loading pattern file: %s",patternfile.c_str());
    gtpe->getOutput()->endResponse();
  }
}

void Engine::gtpClearPatterns(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  delete me->patterntable;
  me->patterntable=new Pattern::ThreeByThreeTable();
  
  #ifdef HAVE_MPI
    if (me->mpirank==0)
      me->mpiBroadcastCommand(MPICMD_CLEARPATTERNS);
  #endif
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->endResponse();
}

void Engine::gtpDoBoardCopy(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  Go::Board *oldboard=me->currentboard;
  Go::Board *newboard=me->currentboard->copy();
  delete oldboard;
  me->currentboard=newboard;
  if (!me->params->uct_symmetry_use)
    me->currentboard->turnSymmetryOff();
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->endResponse();
}

void Engine::gtpPlayoutSGF(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;

  if (cmd->numArgs()!=2)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printf("need 2 arg");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  std::string sgffile=cmd->getStringArg(0);
  std::string who_wins=cmd->getStringArg(1);

  //can be used to get any win of playout if win=0
  int win=0;
  if (who_wins=="B"||who_wins=="b")
    win=1;
  if (who_wins=="W"||who_wins=="w")
    win=-1;

  bool success=false;
  bool foundwin=false;
  int i;
  float finalscore=0;
  for (i=0;i<1000;i++)
  {
    Go::Board *playoutboard=me->currentboard->copy();
    Go::Color col=me->currentboard->nextToMove();
    std::list<Go::Move> playoutmoves;
    std::list<std::string> movereasons;
    Tree *playouttree = me->movetree->getUrgentChild(me->threadpool->getThreadZero()->getSettings());
    float cnn_winrate=-2;
    me->playout->doPlayout(me->threadpool->getThreadZero()->getSettings(),playoutboard,finalscore,cnn_winrate,playouttree,playoutmoves,col,NULL,NULL,NULL,NULL,&movereasons);
    if (finalscore*win>=0)
    {
      foundwin=true;
      success=me->writeSGF(sgffile,me->currentboard,playoutmoves,&movereasons);
      break;
    }
  }

  if (!foundwin)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printf("No win found for %s",who_wins.c_str());
    gtpe->getOutput()->endResponse();
    return;
  }
  
  if (success)
  {
    gtpe->getOutput()->startResponse(cmd);
    gtpe->getOutput()->printf("wrote sgf file: %s (i was %d) finalscore %f",sgffile.c_str(),i,finalscore);
    gtpe->getOutput()->endResponse();
  }
  else
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printf("error writing sgf file: %s",sgffile.c_str());
    gtpe->getOutput()->endResponse();
  }
}

void Engine::gtpPlayoutSGF_pos(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;

  if (cmd->numArgs()!=3)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printf("need 3 arg");
    gtpe->getOutput()->endResponse();
    return;
  }

  std::string sgffile=cmd->getStringArg(0);
  std::string who_wins=cmd->getStringArg(1);
  std::string where_wins=cmd->getStringArg(2);

  int where=Go::Position::string2pos(where_wins,me->boardsize);

  int win=0;
  if (who_wins=="B"||who_wins=="b")
    win=1;
  if (who_wins=="W"||who_wins=="w")
    win=-1;

  bool success=false;
  bool foundwin=false;
  int how_often=0,from_often=0,numplayoutmoves=0;
  me->currentboard->updatePlayoutGammas(me->params, me->features);
  for (int i=0;i<1000+1000;i++)
  {
    Go::Board *playoutboard=me->currentboard->copy();
    if (me->debug_solid_group>=0 && playoutboard->inGroup(me->debug_solid_group)) {
      playoutboard->hasSolidGroups=true;
      Go::Group *thegroup=playoutboard->getGroup(me->debug_solid_group);
      thegroup->setSolid ();
    }
    Go::Color col=me->currentboard->nextToMove();
    float finalscore;
    std::list<Go::Move> playoutmoves;
    std::list<std::string> movereasons;
    Tree *playouttree = me->movetree; //me->movetree->getUrgentChild(me->threadpool->getThreadZero()->getSettings());
    float cnn_winrate=-2;
    me->playout->doPlayout(me->threadpool->getThreadZero()->getSettings(),playoutboard,finalscore,cnn_winrate,playouttree,playoutmoves,col,NULL,NULL,NULL,NULL,&movereasons);
    if (finalscore!=0 && i<1000) from_often++;
    playoutboard->score();
    //fprintf(stderr,"playoutres %d %d finalscore: %f\n",i,playoutboard->getScoredOwner(where),finalscore);
    if ((win==1  && playoutboard->getScoredOwner(where)==Go::BLACK) || (win==-1 && playoutboard->getScoredOwner(where)==Go::WHITE))
    {
      if (i<1000)
        how_often++;
      else
      {
        foundwin=true;
        success=me->writeSGF(sgffile,me->currentboard,playoutmoves,&movereasons);
        numplayoutmoves=playoutmoves.size();
        break;
      }
    }
  }

  if (!foundwin)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printf("No win found for %s",who_wins.c_str());
    gtpe->getOutput()->endResponse();
    return;
  }

  if (success)
  {
    gtpe->getOutput()->startResponse(cmd);
    gtpe->getOutput()->printf("wrote sgf file: %s  found within the first %d playouts: %d playoutmoves %d",sgffile.c_str(),from_often,how_often,numplayoutmoves);
    gtpe->getOutput()->endResponse();
  }
  else
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printf("error writing sgf file: %s",sgffile.c_str());
    gtpe->getOutput()->endResponse();
  }
}

void Engine::gtpRegOwnerAt(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  // usage reg_ownerat "Info String" treshhold Position
  Engine *me=(Engine*)instance;

  if (cmd->numArgs()!=2)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printf("need 2 arg");
    gtpe->getOutput()->endResponse();
    return;
  }

  std::string treshholdstring=cmd->getStringArg(0);
  std::string where_wins=cmd->getStringArg(1);

  int where=Go::Position::string2pos(where_wins,me->boardsize);

  float treshold;
  std::istringstream(treshholdstring)>>treshold;

  float ownership=me->territorymap->getPositionOwner(where);

  gtpe->getOutput()->printfDebug("values %f %f\n",treshold,ownership);
  std::string res;
  if (ownership<treshold)
    res="-1";
  else
    res="1";

  gtpe->getOutput()->startResponse(cmd,true);
  gtpe->getOutput()->printf("%s",res.c_str());
  gtpe->getOutput()->endResponse();
  return;

}

void Engine::gtpSgCompareFloat(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  // usage sg_compare_float float name
  Engine *me=(Engine*)instance;

  if (cmd->numArgs()!=2)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printf("need 2 arg");
    gtpe->getOutput()->endResponse();
    return;
  }

  std::string treshholdstring=cmd->getStringArg(0);
  std::string name=cmd->getStringArg(1);

  float treshhold;
  std::istringstream(treshholdstring)>>treshhold;

  gtpe->getOutput()->printfDebug("values %f %f\n",treshhold,me->movetree->getRobustChild()->getRatio());
  std::string res;
  if (name.compare("uct_value")!=0)
    res="name not supported";
  else
  {
    if (me->movetree->getRobustChild()->getRatio()<treshhold)
      res="-1";
    else
      res="1";
  }

  gtpe->getOutput()->startResponse(cmd,true);
  gtpe->getOutput()->printf("%s",res.c_str());
  gtpe->getOutput()->endResponse();
  return;

}

void Engine::gtpOutputSGF(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  if (cmd->numArgs()!=1)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printf("need 1 arg");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  std::string sgffile=cmd->getStringArg(0);
  bool success=me->writeSGF(sgffile,me->currentboard,me->movetree);
  
  if (success)
  {
    gtpe->getOutput()->startResponse(cmd);
    gtpe->getOutput()->printf("wrote sgf file: %s",sgffile.c_str());
    gtpe->getOutput()->endResponse();
  }
  else
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printf("error writing sgf file: %s",sgffile.c_str());
    gtpe->getOutput()->endResponse();
  }
}

void Engine::gtpGameSGF(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  if (cmd->numArgs()!=1)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printf("need 1 arg");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  std::string sgffile=cmd->getStringArg(0);
  bool success=me->writeGameSGF(sgffile);
  
  if (success)
  {
    gtpe->getOutput()->startResponse(cmd);
    gtpe->getOutput()->printf("wrote sgf file: %s",sgffile.c_str());
    gtpe->getOutput()->endResponse();
  }
  else
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printf("error writing sgf file: %s",sgffile.c_str());
    gtpe->getOutput()->endResponse();
  }
}

void Engine::gtpDoNPlayouts(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  if (cmd->numArgs()!=1)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printf("need 1 arg");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  int n=cmd->getIntArg(0);
  me->doNPlayouts(n);
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->endResponse();
}

void Engine::gtpSolidGroupAt(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  if (cmd->numArgs()!=1)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printf("need 1 arg");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  Gtp::Vertex vert = cmd->getVertexArg(0);
  int a_pos=Go::Position::xy2pos(vert.x,vert.y,me->boardsize);
  me->debug_solid_group=a_pos;
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->endResponse();
}


void Engine::gtpDoNPlayoutsAround(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  if (cmd->numArgs()!=2)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printf("need 2 arg");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  int n=cmd->getIntArg(0);
  Gtp::Vertex vert = cmd->getVertexArg(1);
  int a_pos=Go::Position::xy2pos(vert.x,vert.y,me->boardsize);
  me->clearMoveTree(a_pos);
  me->doNPlayouts(n);
  //me->clearMoveTree();
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->endResponse();
}

void Engine::gtpExplainLastMove(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  gtpe->getOutput()->startResponse(cmd);
  if (me->moveexplanations->size()>0)
    gtpe->getOutput()->printString(me->moveexplanations->back());
  gtpe->getOutput()->endResponse();
}

void Engine::gtpBoardStats(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  Go::Board *board=me->currentboard;
  int size=me->boardsize;
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printf("board stats:\n");
  gtpe->getOutput()->printf("komi: %.1f\n",me->komi);
  gtpe->getOutput()->printf("moves: %d\n",board->getMovesMade());
  gtpe->getOutput()->printf("next to move: %c\n",(board->nextToMove()==Go::BLACK?'B':'W'));
  gtpe->getOutput()->printf("passes: %d\n",board->getPassesPlayed());
  gtpe->getOutput()->printf("simple ko: ");
  int simpleko=board->getSimpleKo();
  if (simpleko==-1)
    gtpe->getOutput()->printf("NONE");
  else
  {
    Gtp::Vertex vert={Go::Position::pos2x(simpleko,me->boardsize),Go::Position::pos2y(simpleko,size)};
    gtpe->getOutput()->printVertex(vert);
  }
  gtpe->getOutput()->printf("\n");
  #if SYMMETRY_ONLYDEGRAGE
    gtpe->getOutput()->printf("stored symmetry: %s (degraded)\n",board->getSymmetryString(board->getSymmetry()).c_str());
  #else
    gtpe->getOutput()->printf("stored symmetry: %s\n",board->getSymmetryString(board->getSymmetry()).c_str());
  #endif
  gtpe->getOutput()->printf("computed symmetry: %s\n",board->getSymmetryString(board->computeSymmetry()).c_str());
  for (int p=0;p<board->getPositionMax();p++)
  {
    if (board->inGroup(p) && board->touchingEmpty(p)>0)
    {
      Go::Group *group=board->getGroup(p);
      if (board->isLadder(group))
        gtpe->getOutput()->printf("ladder at %s works: %d\n",Go::Position::pos2string(p,size).c_str(),board->isProbableWorkingLadder(group));
      else
      {
        Go::Color col=Go::otherColor(group->getColor());
        foreach_adjacent(p,q,{
          if (board->onBoard(q))
          {
            Go::Move move=Go::Move(col,q);
            if (board->validMove(move) && board->isLadderAfter(group,move))
              gtpe->getOutput()->printf("ladder at %s after %s works: %d\n",Go::Position::pos2string(p,size).c_str(),move.toString(size).c_str(),board->isProbableWorkingLadderAfter(group,move));
          }
        });
      }
    }
  }
  gtpe->getOutput()->endResponse(true);
}

void Engine::gtpFeatureMatchesAt(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  if (cmd->numArgs()!=1)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("vertex is required");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  Gtp::Vertex vert = cmd->getVertexArg(0);
  
  if (vert.x==-3 && vert.y==-3)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("invalid vertex");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  Go::Board *board=me->currentboard;
  Go::Color col=board->nextToMove();
  int pos=Go::Position::xy2pos(vert.x,vert.y,me->boardsize);
  Go::Move move=Go::Move(col,pos);
  
  Go::ObjectBoard<int> *cfglastdist=NULL;
  Go::ObjectBoard<int> *cfgsecondlastdist=NULL;
  me->features->computeCFGDist(board,&cfglastdist,&cfgsecondlastdist);
  DecisionTree::GraphCollection *graphs = new DecisionTree::GraphCollection(DecisionTree::getCollectionTypes(&(me->decisiontrees)),board);

  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printf("Feature Matches for %s:\n",move.toString(board->getSize()).c_str());
  gtpe->getOutput()->printf("PASS:              %u\n",me->features->matchFeatureClass(Features::PASS,board,cfglastdist,cfgsecondlastdist,move));
  gtpe->getOutput()->printf("CAPTURE:           %u\n",me->features->matchFeatureClass(Features::CAPTURE,board,cfglastdist,cfgsecondlastdist,move));
  gtpe->getOutput()->printf("EXTENSION:         %u\n",me->features->matchFeatureClass(Features::EXTENSION,board,cfglastdist,cfgsecondlastdist,move));
  gtpe->getOutput()->printf("SELFATARI:         %u\n",me->features->matchFeatureClass(Features::SELFATARI,board,cfglastdist,cfgsecondlastdist,move));
  gtpe->getOutput()->printf("ATARI:             %u\n",me->features->matchFeatureClass(Features::ATARI,board,cfglastdist,cfgsecondlastdist,move));
  gtpe->getOutput()->printf("BORDERDIST:        %u\n",me->features->matchFeatureClass(Features::BORDERDIST,board,cfglastdist,cfgsecondlastdist,move));
  gtpe->getOutput()->printf("LASTDIST:          %u\n",me->features->matchFeatureClass(Features::LASTDIST,board,cfglastdist,cfgsecondlastdist,move));
  gtpe->getOutput()->printf("SECONDLASTDIST:    %u\n",me->features->matchFeatureClass(Features::SECONDLASTDIST,board,cfglastdist,cfgsecondlastdist,move));
  gtpe->getOutput()->printf("CFGLASTDIST:       %u\n",me->features->matchFeatureClass(Features::CFGLASTDIST,board,cfglastdist,cfgsecondlastdist,move));
  gtpe->getOutput()->printf("CFGSECONDLASTDIST: %u\n",me->features->matchFeatureClass(Features::CFGSECONDLASTDIST,board,cfglastdist,cfgsecondlastdist,move));
  gtpe->getOutput()->printf("NAKADE:            %u\n",me->features->matchFeatureClass(Features::NAKADE,board,cfglastdist,cfgsecondlastdist,move));
  gtpe->getOutput()->printf("APPROACH:          %u\n",me->features->matchFeatureClass(Features::APPROACH,board,cfglastdist,cfgsecondlastdist,move));
  gtpe->getOutput()->printf("PATTERN3X3:        0x%04x\n",me->features->matchFeatureClass(Features::PATTERN3X3,board,cfglastdist,cfgsecondlastdist,move));
  gtpe->getOutput()->printf("CIRCPATT:          %u\n",me->features->matchFeatureClass(Features::CIRCPATT,board,cfglastdist,cfgsecondlastdist,move));
  float gamma=me->features->getMoveGamma(board,cfglastdist,cfgsecondlastdist,graphs,move);
  float total=me->features->getBoardGamma(board,cfglastdist,cfgsecondlastdist,graphs,col);
  float totallog=me->features->getBoardGamma(board,cfglastdist,cfgsecondlastdist,graphs,col,true);
  gtpe->getOutput()->printf("Gamma: %.2f/%.2f (%.2f) log %.2f\n",gamma,total,gamma/total,totallog);

  Tree *tree=me->movetree->getChild(move);
  if (tree) {
    gtpe->getOutput()->printf("SelfBlack: %.2f\n",tree->getOwnSelfBlack());
    gtpe->getOutput()->printf("SelfWhite: %.2f\n",tree->getOwnSelfWhite());
    gtpe->getOutput()->printf("OwnBlack: %.2f\n",tree->getOwnRatio(Go::BLACK));
    gtpe->getOutput()->printf("OwnWhite: %.2f\n",tree->getOwnRatio(Go::WHITE));
    gtpe->getOutput()->printf("WhiteSlope: %.5f\n",tree->getSlope(Go::WHITE));
    gtpe->getOutput()->printf("BlackSlope: %.5f\n",tree->getSlope(Go::BLACK));
  }
  gtpe->getOutput()->endResponse(true);
  
  if (cfglastdist!=NULL)
    delete cfglastdist;
  if (cfgsecondlastdist!=NULL)
    delete cfgsecondlastdist;
  delete graphs;
}

void Engine::gtpFeatureProbDistribution(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  Go::Board *board=me->currentboard;
  Go::Color col=board->nextToMove();
  
  Go::ObjectBoard<int> *cfglastdist=NULL;
  Go::ObjectBoard<int> *cfgsecondlastdist=NULL;
  me->features->computeCFGDist(board,&cfglastdist,&cfgsecondlastdist);
  DecisionTree::GraphCollection *graphs = new DecisionTree::GraphCollection(DecisionTree::getCollectionTypes(&(me->decisiontrees)),board);
  
  float totalgamma=me->features->getBoardGamma(board,cfglastdist,cfgsecondlastdist,graphs,col);
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printString("\n");
  for (int y=me->boardsize-1;y>=0;y--)
  {
    for (int x=0;x<me->boardsize;x++)
    {
      Go::Move move=Go::Move(col,Go::Position::xy2pos(x,y,me->boardsize)); 
      float gamma=me->features->getMoveGamma(board,cfglastdist,cfgsecondlastdist,graphs,move);
      if (gamma<=0)
        gtpe->getOutput()->printf("\"\" ");
      else
      {
        float prob=gamma/totalgamma;
        float point1=0.15;
        float point2=0.65;
        float r,g,b;
        // scale from blue-green-red-reddest?
        if (prob>=point2)
        {
          b=0;
          r=1;
          g=0;
        }
        else if (prob>=point1)
        {
          b=0;
          r=(prob-point1)/(point2-point1);
          g=1-r;
        }
        else
        {
          r=0;
          g=prob/point1;
          b=1-g;
        }
        if (r<0)
          r=0;
        if (g<0)
          g=0;
        if (b<0)
          b=0;
        gtpe->getOutput()->printf("#%02x%02x%02x ",(int)(r*255),(int)(g*255),(int)(b*255));
        //gtpe->getOutput()->printf("#%06x ",(int)(prob*(1<<24)));
      }
    }
    gtpe->getOutput()->printf("\n");
  }

  gtpe->getOutput()->endResponse(true);
  
  if (cfglastdist!=NULL)
    delete cfglastdist;
  if (cfgsecondlastdist!=NULL)
    delete cfgsecondlastdist;
  delete graphs;
}

void Engine::gtpListAllPatterns(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  Go::Board *board=me->currentboard;
  Go::Color col=board->nextToMove();
  
  gtpe->getOutput()->startResponse(cmd);
  for (int p=0;p<board->getPositionMax();p++)
  {
    if (me->currentboard->validMove(Go::Move(col,p)))
    {
      unsigned int hash=Pattern::ThreeByThree::makeHash(me->currentboard,p);
      if (col==Go::WHITE)
        hash=Pattern::ThreeByThree::invert(hash);
      hash=Pattern::ThreeByThree::smallestEquivalent(hash);
      gtpe->getOutput()->printf("0x%04x ",hash);
    }
  }

  gtpe->getOutput()->endResponse();
}

void Engine::gtpLoadFeatureGammas(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  if (cmd->numArgs()!=1)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printf("need 1 arg");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  std::string filename=cmd->getStringArg(0);
  me->learn_filename_features=filename;
  
  delete me->features;
  me->features=new Features(me->params);
  bool success=me->features->loadGammaFile(filename);
  
  if (success)
  {
    #ifdef HAVE_MPI
      if (me->mpirank==0)
      {
        me->mpiBroadcastCommand(MPICMD_LOADFEATUREGAMMAS);
        me->mpiBroadcastString(filename);
      }
    #endif
    
    gtpe->getOutput()->startResponse(cmd);
    gtpe->getOutput()->printf("loaded features gamma file: %s Attention, circ pattern files are removed by this!",filename.c_str());
    gtpe->getOutput()->endResponse();
  }
  else
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printf("error loading features gamma file: %s",filename.c_str());
    gtpe->getOutput()->endResponse();
  }
}

void Engine::gtpLoadCNNt(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
#ifdef HAVE_CAFFE
//  Engine *me=(Engine*)instance;
  
  if (cmd->numArgs()!=2)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printf("need 2 arg");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  std::string filename_net=cmd->getStringArg(0);
  std::string filename_parameters=cmd->getStringArg(1);

  bool success=true;
  //the library does not really seem to throw exceptions, but just exit :(
  try {
    if (caffe_area_net!=NULL) delete caffe_area_net;
    caffe_area_net = new Net<float>(filename_net,TEST);
    caffe_area_net->CopyTrainedLayersFrom(filename_parameters);
//    caffe_area_net->set_mode_gpu();
  }
  catch (int e) {
    gtpe->getOutput()->printf("try catch %d\n",e);
    success=false;
  }

  if (success)
  {
    #ifdef HAVE_MPI
      if (me->mpirank==0)
      {
        fprintf(stderr,"Attention, mpi not implemented yet!!!\n");
      }
    #endif
    
    gtpe->getOutput()->startResponse(cmd);
    gtpe->getOutput()->printf("loaded CNN file: %s and %s learned file",filename_net.c_str(),filename_parameters.c_str());
    gtpe->getOutput()->endResponse();
  }
  else
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printf("error loading features gamma file: %s",filename_net.c_str());
    gtpe->getOutput()->endResponse();
  }
#else
  gtpe->getOutput()->startResponse(cmd,false);
  gtpe->getOutput()->printf("caffe not availible, compile with-caffe");
  gtpe->getOutput()->endResponse();
#endif  
}

void Engine::gtpLoadCNNp(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
#ifdef HAVE_CAFFE
  Engine *me=(Engine*)instance;
  
  if (cmd->numArgs()!=2 && cmd->numArgs()!=3)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printf("need 2 or 3 arg");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  std::string filename_net=cmd->getStringArg(0);
  std::string filename_parameters=cmd->getStringArg(1);
  int net_num=0;
  if (cmd->numArgs()==3)
    net_num=cmd->getIntArg(2);
  disable_last_move_from_cnn=false;
  double_second_last_move_from_cnn=false;
  //net_num==1 is for weak gamma, it is not allowed to be combined with one net per thread at the moment!!
  
  if (net_num==2) {
    fprintf(stderr,"disabling last move for net 0!!!\n");
    net_num=0;
    disable_last_move_from_cnn=true;
  }
  if (net_num==3) {
    fprintf(stderr,"enable double second last move for net 0!!!\n");
    net_num=0;
    double_second_last_move_from_cnn=true;
  }
  if (net_num==0) {
    caffe_test_net_input_dim[0]=13; //since cudnn v4 supported caffe this did not work :(
  }
  if (net_num==4) {
    fprintf(stderr,"enable ko and 4 last moves for net 0!!!\n");
    net_num=0;
    caffe_test_net_input_dim[0]=14;
  }
  if (net_num==5) {
    fprintf(stderr,"enable ko and 4 last moves and komiplanes for net 0!!!\n");
    net_num=0;
    caffe_test_net_input_dim[0]=20;
  }
  bool success=true;
  //the library does not really seem to throw exceptions, but just exit :(
  try {
    if (caffe_test_net[net_num]!=NULL) delete caffe_test_net[net_num];
    fprintf(stderr,"ok, create net\n");
    caffe_test_net[net_num] = new Net<float>(filename_net,TEST);
    fprintf(stderr,"ok, created net\n");
    caffe_test_net[net_num]->CopyTrainedLayersFrom(filename_parameters);
    if (me->params->cnn_num_of_gpus>1) fprintf(stderr,"num of gpu >1 not yet supported\n");
    if (net_num==0 && me->params->cnn_num_of_gpus>0) {
      for (int j=1;j<me->params->thread_count;j++) {
        if (caffe_test_net[j]!=NULL) delete caffe_test_net[j];
        caffe_test_net[j] = new Net<float>(filename_net,TEST);
        caffe_test_net[j]->CopyTrainedLayersFrom(filename_parameters);
        fprintf(stderr,"additional net created %d\n",j);
      }
    }

    int t2=Caffe::mode();
    caffe_test_net_input_dim[net_num]=caffe_test_net[net_num]->input_blobs()[0]->shape()[1];
    caffe_test_net_input_dim[1]=9; //since cudnn v4 supported caffe this did not work :(
    fprintf(stderr,"Attention, fixed coded as caffe support broken?!   --->>> !!!!!!!!!!!!!!!!!!!!!!!! %d shape %d\n",t2,caffe_test_net_input_dim[net_num]);
    
//    caffe_test_net->set_mode_gpu();
  }
  catch (int e) {
    gtpe->getOutput()->printf("try catch %d\n",e);
    success=false;
  }

  if (success)
  {
    #ifdef HAVE_MPI
      if (me->mpirank==0)
      {
        fprintf(stderr,"Attention, mpi not implemented yet!!!\n");
      }
    #endif
    
    gtpe->getOutput()->startResponse(cmd);
    gtpe->getOutput()->printf("loaded CNN file: %s and %s learned file",filename_net.c_str(),filename_parameters.c_str());
    gtpe->getOutput()->endResponse();
  }
  else
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printf("error loading features gamma file: %s",filename_net.c_str());
    gtpe->getOutput()->endResponse();
  }
#else
  gtpe->getOutput()->startResponse(cmd,false);
  gtpe->getOutput()->printf("caffe not availible, compile with-caffe:");
  gtpe->getOutput()->endResponse();
#endif 
}

void Engine::gtpSaveFeatureGammas(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  if (cmd->numArgs()!=1)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printf("need 1 arg");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  std::string filename=cmd->getStringArg(0);
  
  bool success=me->features->saveGammaFile(filename);
  
  if (success)
  {
    gtpe->getOutput()->startResponse(cmd);
    gtpe->getOutput()->printf("saved features gamma file: %s",filename.c_str());
    gtpe->getOutput()->endResponse();
  }
  else
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printf("error saving features gamma file: %s",filename.c_str());
    gtpe->getOutput()->endResponse();
  }
}

void Engine::gtpSaveFeatureCircularBinary(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  if (cmd->numArgs()!=1)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printf("need 1 arg");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  std::string filename=cmd->getStringArg(0);
  
  bool success=me->features->saveCircularBinary(filename);
  
  if (success)
  {
    gtpe->getOutput()->startResponse(cmd);
    gtpe->getOutput()->printf("saveded binary circular pattern file: %s",filename.c_str());
    gtpe->getOutput()->endResponse();
  }
  else
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printf("error saveing binary circular pattern file: %s",filename.c_str());
    gtpe->getOutput()->endResponse();
  }
}

void Engine::gtpLoadFeatureCircularBinary(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  if (cmd->numArgs()!=1)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printf("need 1 arg");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  std::string filename=cmd->getStringArg(0);
  
  bool success=me->features->loadCircularBinary(filename);
  
  if (success)
  {
    gtpe->getOutput()->startResponse(cmd);
    gtpe->getOutput()->printf("loaded binary circular pattern file: %s",filename.c_str());
    gtpe->getOutput()->endResponse();
  }
  else
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printf("error loading binary circular pattern file: %s",filename.c_str());
    gtpe->getOutput()->endResponse();
  }
}

void Engine::gtpLoadCircPatterns(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  if (cmd->numArgs()!=2)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printf("need 2 arg (filename and number of lines)");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  std::string filename=cmd->getStringArg(0);
  int numlines=cmd->getIntArg(1);
  
  if (me->features==NULL)
    me->features=new Features(me->params);
  bool success=me->features->loadCircFile(filename,numlines);
  
  if (success)
  {
    //MPI code missing here!!!!
    //#ifdef HAVE_MPI
    //  if (me->mpirank==0)
    //  {
    //    me->mpiBroadcastCommand(MPICMD_LOADFEATUREGAMMAS);
    //    me->mpiBroadcastString(filename);
    //  }
    //#endif
    
    gtpe->getOutput()->startResponse(cmd);
    gtpe->getOutput()->printf("loaded circpatterns file: %s",filename.c_str());
    gtpe->getOutput()->endResponse();
  }
  else
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printf("error loading circpatterns file: %s",filename.c_str());
    gtpe->getOutput()->endResponse();
  }
}

void Engine::gtpLoadCircPatternsNot(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  if (cmd->numArgs()!=2)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printf("need 2 arg (filename and number of lines)");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  std::string filename=cmd->getStringArg(0);
  int numlines=cmd->getIntArg(1);
  
  if (me->features==NULL)
    me->features=new Features(me->params);
  bool success=me->features->loadCircFileNot(filename,numlines);
  
  if (success)
  {
    #ifdef HAVE_MPI
      if (me->mpirank==0)
      {
        me->mpiBroadcastCommand(MPICMD_LOADFEATUREGAMMAS);
        me->mpiBroadcastString(filename);
      }
    #endif
    
    gtpe->getOutput()->startResponse(cmd);
    gtpe->getOutput()->printf("loaded circpatterns file: %s",filename.c_str());
    gtpe->getOutput()->endResponse();
  }
  else
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printf("error loading circpatterns file: %s",filename.c_str());
    gtpe->getOutput()->endResponse();
  }
}

void Engine::gtpSaveCircPatternValues(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  if (cmd->numArgs()!=1)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printf("need 1 arg (filename and number of lines)");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  std::string filename=cmd->getStringArg(0);
  
  if (me->features==NULL)
    me->features=new Features(me->params);
  bool success=me->features->saveCircValueFile(filename);
  
  if (success)
  {
    #ifdef HAVE_MPI
      if (me->mpirank==0)
      {
        me->mpiBroadcastCommand(MPICMD_LOADFEATUREGAMMAS);
        me->mpiBroadcastString(filename);
      }
    #endif
    
    gtpe->getOutput()->startResponse(cmd);
    gtpe->getOutput()->printf("saved circvalue file: %s",filename.c_str());
    gtpe->getOutput()->endResponse();
  }
  else
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printf("error saving circpatterns file: %s",filename.c_str());
    gtpe->getOutput()->endResponse();
  }
}

void Engine::gtpLoadCircPatternValues(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  if (cmd->numArgs()!=1)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printf("need 1 arg (filename and number of lines)");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  std::string filename=cmd->getStringArg(0);
  me->learn_filename_circ_patterns=filename;
  
  if (me->features==NULL)
    me->features=new Features(me->params);
  bool success=me->features->loadCircValueFile(filename);
  
  if (success)
  {
    #ifdef HAVE_MPI
      if (me->mpirank==0)
      {
        me->mpiBroadcastCommand(MPICMD_LOADFEATUREGAMMAS);
        me->mpiBroadcastString(filename);
      }
    #endif
    
    gtpe->getOutput()->startResponse(cmd);
    gtpe->getOutput()->printf("loaded circvalue file: %s",filename.c_str());
    gtpe->getOutput()->endResponse();
  }
  else
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printf("error saving circpatterns file: %s",filename.c_str());
    gtpe->getOutput()->endResponse();
  }
}

void Engine::gtpListFeatureIds(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printf("\n%s",me->features->getFeatureIdList(me->params->features_output_for_playout).c_str());
  gtpe->getOutput()->endResponse(true);
}

void Engine::gtpShowCFGFrom(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  if (cmd->numArgs()!=1)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("vertex is required");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  Gtp::Vertex vert = cmd->getVertexArg(0);
  
  if (vert.x==-3 && vert.y==-3)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("invalid vertex");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  int pos=Go::Position::xy2pos(vert.x,vert.y,me->boardsize);
  Go::ObjectBoard<int> *cfgdist=me->currentboard->getCFGFrom(pos);
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printString("\n");
  for (int y=me->boardsize-1;y>=0;y--)
  {
    for (int x=0;x<me->boardsize;x++)
    {
      int p=Go::Position::xy2pos(x,y,me->boardsize);
      int dist=cfgdist->get(p);
      if (dist!=-1)
        gtpe->getOutput()->printf("\"%d\" ",dist);
      else
        gtpe->getOutput()->printf("\"\" ");
    }
    gtpe->getOutput()->printf("\n");
  }

  gtpe->getOutput()->endResponse(true);
  
  delete cfgdist;
}

void Engine::gtpShowCircDistFrom(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  if (cmd->numArgs()!=1)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("vertex is required");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  Gtp::Vertex vert = cmd->getVertexArg(0);
  
  if (vert.x==-3 && vert.y==-3)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("invalid vertex");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printString("\n");
  for (int y=me->boardsize-1;y>=0;y--)
  {
    for (int x=0;x<me->boardsize;x++)
    {
      int dist=Go::circDist(vert.x,vert.y,x,y);
      gtpe->getOutput()->printf("\"%d\" ",dist);
    }
    gtpe->getOutput()->printf("\n");
  }

  gtpe->getOutput()->endResponse(true);
}

void Engine::gtpListCircularPatternsAt(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  if (cmd->numArgs()!=1)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("vertex is required");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  Gtp::Vertex vert = cmd->getVertexArg(0);
  
  if (vert.x==-3 && vert.y==-3)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("invalid vertex");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  int pos=Go::Position::xy2pos(vert.x,vert.y,me->boardsize);
  Pattern::Circular pattcirc=Pattern::Circular(me->getCircDict(),me->currentboard,pos,PATTERN_CIRC_MAXSIZE);
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printf("Circular Patterns at %s:\n",Go::Position::pos2string(pos,me->boardsize).c_str());
  for (int s=2;s<=PATTERN_CIRC_MAXSIZE;s++)
  {
    gtpe->getOutput()->printf(" %s\n",pattcirc.getSubPattern(me->getCircDict(),s).toString(me->getCircDict()).c_str());
  }

  gtpe->getOutput()->printf("Smallest Equivalent:\n");
  pattcirc.convertToSmallestEquivalent(me->getCircDict());
  gtpe->getOutput()->printf(" %s\n",pattcirc.toString(me->getCircDict()).c_str());
  gtpe->getOutput()->endResponse(true);
}

void Engine::gtpListCircularPatternsAtSize(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  if (cmd->numArgs()!=3)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("vertex size and color is required");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  Gtp::Vertex vert = cmd->getVertexArg(0);
  int s=cmd->getIntArg(1);
  Gtp::Color gtpcol = cmd->getColorArg(2);
  
  if (vert.x==-3 && vert.y==-3)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("invalid vertex");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  int pos=Go::Position::xy2pos(vert.x,vert.y,me->boardsize);
  Pattern::Circular pattcirc=Pattern::Circular(me->getCircDict(),me->currentboard,pos,PATTERN_CIRC_MAXSIZE);
  if (gtpcol==Gtp::WHITE)
    pattcirc.invert();
  pattcirc.convertToSmallestEquivalent(me->getCircDict());
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printf(" %s\n",pattcirc.getSubPattern(me->getCircDict(),s).toString(me->getCircDict()).c_str());
  gtpe->getOutput()->endResponse(true);
}

void Engine::gtpListCircularPatternsAtSizeNot(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  if (cmd->numArgs()!=3)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("vertex size and color is required");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  Gtp::Vertex vert = cmd->getVertexArg(0);
  int s=cmd->getIntArg(1);
  Gtp::Color gtpcol = cmd->getColorArg(2);
  
  if (vert.x==-3 && vert.y==-3)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("invalid vertex");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  int pos=Go::Position::xy2pos(vert.x,vert.y,me->boardsize);
  fprintf(stderr,"played at %s\n",Go::Position::pos2string(pos,me->boardsize).c_str());
  
  Go::BitBoard *validmoves=me->currentboard->getValidMoves(gtpcol==Gtp::BLACK?Go::BLACK:Go::WHITE);
  Random rand;
  rand.makeSeed ();
  int r=rand.getRandomInt(me->currentboard->getPositionMax());
  int d=rand.getRandomInt(1)*2-1;
  for (int p=0;p<me->currentboard->getPositionMax();p++)
  {
    int rp=(r+p*d);
    if (rp<0) rp+=me->currentboard->getPositionMax();
    if (rp>=me->currentboard->getPositionMax()) rp-=me->currentboard->getPositionMax();
    if (pos!=rp && validmoves->get(rp))
    {
      pos=rp;
      break;
    }
  }

  fprintf(stderr,"circ pattern at %s\n",Go::Position::pos2string(pos,me->boardsize).c_str());
          
  Pattern::Circular pattcirc=Pattern::Circular(me->getCircDict(),me->currentboard,pos,PATTERN_CIRC_MAXSIZE);
  if (gtpcol==Gtp::WHITE)
    pattcirc.invert();
  pattcirc.convertToSmallestEquivalent(me->getCircDict());
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printf(" %s\n",pattcirc.getSubPattern(me->getCircDict(),s).toString(me->getCircDict()).c_str());
  gtpe->getOutput()->endResponse(true);
}

void Engine::gtpListAllCircularPatterns(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  int size=0;
  if (cmd->numArgs()>=1)
  {
    size=cmd->getIntArg(0);
  }
  
  Go::Board *board=me->currentboard;
  Go::Color col=board->nextToMove();
  
  gtpe->getOutput()->startResponse(cmd);
  for (int p=0;p<board->getPositionMax();p++)
  {
    if (me->currentboard->validMove(Go::Move(col,p)))
    {
      Pattern::Circular pattcirc=Pattern::Circular(me->getCircDict(),board,p,PATTERN_CIRC_MAXSIZE);
      if (col==Go::WHITE)
        pattcirc.invert();
      pattcirc.convertToSmallestEquivalent(me->getCircDict());
      if (size==0)
      {
        for (int s=4;s<=PATTERN_CIRC_MAXSIZE;s++)
        {
          gtpe->getOutput()->printf(" %s",pattcirc.getSubPattern(me->getCircDict(),s).toString(me->getCircDict()).c_str());
        }
      }
      else
        gtpe->getOutput()->printf(" %s",pattcirc.getSubPattern(me->getCircDict(),size).toString(me->getCircDict()).c_str());
    }
  }

  gtpe->getOutput()->endResponse();
}

void Engine::gtpListAdjacentGroupsOf(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  if (cmd->numArgs()!=1)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("vertex is required");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  Gtp::Vertex vert = cmd->getVertexArg(0);
  
  if (vert.x==-3 && vert.y==-3)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("invalid vertex");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  int pos=Go::Position::xy2pos(vert.x,vert.y,me->boardsize);
  Go::Group *group=NULL;
  if (me->currentboard->inGroup(pos))
    group=me->currentboard->getGroup(pos);
  
  if (group!=NULL)
  {
    Go::list_short *adjacentgroups=group->getAdjacentGroups();
    
    gtpe->getOutput()->startResponse(cmd);
    gtpe->getOutput()->printf("list of size %d:\n",adjacentgroups->size());
    for(auto iter=adjacentgroups->begin();iter!=adjacentgroups->end();++iter)
    {
      if (me->currentboard->inGroup((*iter)))
        gtpe->getOutput()->printf("%s\n",Go::Position::pos2string((*iter),me->boardsize).c_str());
    }
    gtpe->getOutput()->endResponse(true);
  }
  else
  {
    gtpe->getOutput()->startResponse(cmd);
    gtpe->getOutput()->endResponse();
  }
}

void Engine::gtpShowSymmetryTransforms(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  Go::Board::Symmetry sym=me->currentboard->getSymmetry();
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printString("\n");
  for (int y=me->boardsize-1;y>=0;y--)
  {
    for (int x=0;x<me->boardsize;x++)
    {
      int pos=Go::Position::xy2pos(x,y,me->boardsize);
      int transpos=me->currentboard->doSymmetryTransformToPrimary(sym,pos);
      if (transpos==pos)
        gtpe->getOutput()->printf("\"P\" ");
      else
      {
        Gtp::Vertex vert={Go::Position::pos2x(transpos,me->boardsize),Go::Position::pos2y(transpos,me->boardsize)};
        gtpe->getOutput()->printf("\"");
        gtpe->getOutput()->printVertex(vert);
        gtpe->getOutput()->printf("\" ");
      }
    }
    gtpe->getOutput()->printf("\n");
  }

  gtpe->getOutput()->endResponse(true);
}

void Engine::gtpShowNakadeCenters(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printString("\n");
  for (int y=me->boardsize-1;y>=0;y--)
  {
    for (int x=0;x<me->boardsize;x++)
    {
      int pos=Go::Position::xy2pos(x,y,me->boardsize);
      int centerpos=me->currentboard->getThreeEmptyGroupCenterFrom(pos);
      if (centerpos==-1)
        gtpe->getOutput()->printf("\"\" ");
      else
      {
        Gtp::Vertex vert={Go::Position::pos2x(centerpos,me->boardsize),Go::Position::pos2y(centerpos,me->boardsize)};
        gtpe->getOutput()->printf("\"");
        gtpe->getOutput()->printVertex(vert);
        gtpe->getOutput()->printf("\" ");
      }
    }
    gtpe->getOutput()->printf("\n");
  }

  gtpe->getOutput()->endResponse(true);
}

void Engine::gtpShowTreeLiveGfx(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  gtpe->getOutput()->startResponse(cmd);
  if (me->params->move_policy==Parameters::MP_UCT || me->params->move_policy==Parameters::MP_ONEPLY)
    me->displayPlayoutLiveGfx(-1,false);
  gtpe->getOutput()->endResponse();
}

void Engine::gtpDescribeEngine(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printf("%s\n",me->longname.c_str());
  #ifdef HAVE_MPI
    gtpe->getOutput()->printf("mpi world size: %d\n",me->mpiworldsize);
  #endif
  gtpe->getOutput()->printf("parameters:\n");
  me->params->printParametersForDescription(gtpe);
  gtpe->getOutput()->endResponse(true);
}

void Engine::gtpShowCurrentHash(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printf("Current hash: 0x%016llx",me->currentboard->getZobristHash(me->zobristtable));
  gtpe->getOutput()->endResponse();
}

void Engine::gtpBookShow(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printString(me->book->show(me->boardsize,me->movehistory));
  gtpe->getOutput()->endResponse(true);
}

void Engine::gtpBookAdd(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  if (cmd->numArgs()!=1)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("vertex is required");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  Gtp::Vertex vert = cmd->getVertexArg(0);
  
  if (vert.x==-3 && vert.y==-3)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("invalid vertex");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  int pos=Go::Position::xy2pos(vert.x,vert.y,me->boardsize);
  Go::Move move=Go::Move(me->currentboard->nextToMove(),pos);
  
  me->book->add(me->boardsize,me->movehistory,move);
  
  #ifdef HAVE_MPI
    if (me->mpirank==0)
    {
      unsigned int arg=pos;
      me->mpiBroadcastCommand(MPICMD_BOOKADD,&arg);
    }
  #endif
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->endResponse();
}

void Engine::gtpBookRemove(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  if (cmd->numArgs()!=1)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("vertex is required");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  Gtp::Vertex vert = cmd->getVertexArg(0);
  
  if (vert.x==-3 && vert.y==-3)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("invalid vertex");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  int pos=Go::Position::xy2pos(vert.x,vert.y,me->boardsize);
  Go::Move move=Go::Move(me->currentboard->nextToMove(),pos);
  
  me->book->remove(me->boardsize,me->movehistory,move);
  
  #ifdef HAVE_MPI
    if (me->mpirank==0)
    {
      unsigned int arg=pos;
      me->mpiBroadcastCommand(MPICMD_BOOKREMOVE,&arg);
    }
  #endif
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->endResponse();
}

void Engine::gtpBookClear(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  me->book->clear(me->boardsize);
  
  #ifdef HAVE_MPI
    if (me->mpirank==0)
      me->mpiBroadcastCommand(MPICMD_BOOKCLEAR);
  #endif
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->endResponse();
}

void Engine::gtpBookLoad(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  if (cmd->numArgs()!=1)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printf("need 1 arg");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  std::string filename=cmd->getStringArg(0);
  
  delete me->book;
  me->book=new Book(me->params);
  bool success=me->book->loadFile(filename);
  
  if (success)
  {
    #ifdef HAVE_MPI
      if (me->mpirank==0)
      {
        me->mpiBroadcastCommand(MPICMD_BOOKLOAD);
        me->mpiBroadcastString(filename);
      }
    #endif
    
    gtpe->getOutput()->startResponse(cmd);
    gtpe->getOutput()->printf("loaded book: %s",filename.c_str());
    gtpe->getOutput()->endResponse();
  }
  else
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printf("error loading book: %s",filename.c_str());
    gtpe->getOutput()->endResponse();
  }
}

void Engine::gtpBookSave(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  if (cmd->numArgs()!=1)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printf("need 1 arg");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  std::string filename=cmd->getStringArg(0);
  bool success=me->book->saveFile(filename);
  
  if (success)
  {
    gtpe->getOutput()->startResponse(cmd);
    gtpe->getOutput()->printf("saved book: %s",filename.c_str());
    gtpe->getOutput()->endResponse();
  }
  else
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printf("error saving book: %s",filename.c_str());
    gtpe->getOutput()->endResponse();
  }
}

void Engine::gtpShowSafePositions(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  Go::Board *board=me->currentboard;
  Benson *benson=new Benson(board);
  benson->solve();
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printf("BLACK");
  for (int p=0;p<board->getPositionMax();p++)
  {
    if (benson->getSafePositions()->get(p)==Go::BLACK)
      gtpe->getOutput()->printf(" %s",Go::Position::pos2string(p,me->boardsize).c_str());
  }
  gtpe->getOutput()->printf("\nWHITE");
  for (int p=0;p<board->getPositionMax();p++)
  {
    if (benson->getSafePositions()->get(p)==Go::WHITE)
      gtpe->getOutput()->printf(" %s",Go::Position::pos2string(p,me->boardsize).c_str());
  }
  gtpe->getOutput()->endResponse();
  
  delete benson;
}

void Engine::gtpDoBenchmark(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  int games;
  if (cmd->numArgs()==0)
    games=1000;
  else
    games=cmd->getIntArg(0);
  
  boost::posix_time::ptime time_start=me->timeNow();
  float finalscore;
  for (int i=0;i<games;i++)
  {
    Go::Board *board=new Go::Board(me->boardsize);
    std::list<Go::Move> playoutmoves;
    float cnn_winrate=-2;
    me->playout->doPlayout(me->threadpool->getThreadZero()->getSettings(),board,finalscore,cnn_winrate,NULL,playoutmoves,Go::BLACK,NULL,NULL,NULL,NULL);
    delete board;
  }
  
  float rate=(float)games/me->timeSince(time_start)/1000;
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printf("ppms: %.2f",rate);
  gtpe->getOutput()->endResponse();
}

void Engine::gtpShowCriticality(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  Go::Board *board=me->currentboard;
  Go::Color col=board->nextToMove();
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printString("\n");
  float min=1000000;
  float max=0;
  for (int y=me->boardsize-1;y>=0;y--)
  {
    for (int x=0;x<me->boardsize;x++)
    {
      int pos=Go::Position::xy2pos(x,y,me->boardsize);
      Go::Move move=Go::Move(col,pos);
      Tree *tree=me->movetree->getChild(move);
      if (tree!=NULL)
      {
        float ratio=tree->getCriticality();
        if (ratio<min) min=ratio;
        if (ratio>max) max=ratio;
      }
    }
  }
  for (int y=me->boardsize-1;y>=0;y--)
  {
    for (int x=0;x<me->boardsize;x++)
    {
      Go::Move move=Go::Move(col,Go::Position::xy2pos(x,y,me->boardsize)); 
      Tree *tree=me->movetree->getChild(move);
      if (tree==NULL || !tree->isPrimary())
        gtpe->getOutput()->printf("\"\" ");
      else
      {
        float crit=tree->getCriticality();
        float plts=(me->params->uct_criticality_siblings?me->movetree->getPlayouts():tree->getPlayouts());
        if (crit==0 && (!move.isNormal() || plts==0))
          gtpe->getOutput()->printf("\"\" ");
        else
        {
          crit=(crit-min)/(max-min);
          float r,g,b;
          float x=crit;
          
          // scale from blue-red
          r=x;
          if (r>1)
            r=1;
          g=0;
          b=1-r;
          
          if (r<0)
            r=0;
          if (g<0)
            g=0;
          if (b<0)
            b=0;
          gtpe->getOutput()->printf("#%02x%02x%02x ",(int)(r*255),(int)(g*255),(int)(b*255));
          //gtpe->getOutput()->printf("#%06x ",(int)(prob*(1<<24)));
        }
      }
    }
    gtpe->getOutput()->printf("\n");
  }

  gtpe->getOutput()->endResponse(true);
}

//#define wf(A)   ((A-0.5>0)?(sqrt(2*(A-0.5))+1)/2.0:(1.0-sqrt(-2*(A-0.5)))/2.0)
#define wf(A)   A
//#define wf(A) pow(A,0.5)
void Engine::gtpShowTerritory(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printString("\n");
  float territorycount=0;
  for (int y=me->boardsize-1;y>=0;y--)
  {
    for (int x=0;x<me->boardsize;x++)
    {
      int pos=Go::Position::xy2pos(x,y,me->boardsize);
      float tmp=me->territorymap->getPositionOwner(pos);
      //if (tmp>0.2) territorycount++;
      //if (tmp<-0.2) territorycount--;
      if (tmp<0)
        territorycount-=wf(-tmp);
      else
        territorycount+=wf(tmp);
      if (tmp<0)
        gtpe->getOutput()->printf("%.2f ",-wf(-tmp));
      else
        gtpe->getOutput()->printf("%.2f ",wf(tmp));  
    }
    gtpe->getOutput()->printf("\n");
  }

  if (territorycount-me->getHandiKomi()>0)
    gtpe->getOutput()->printf("Territory %.1f Komi %.1f B+%.1f (with ScoreKomi %.1f) (%.1f)\n",
      territorycount,me->getHandiKomi(),territorycount-me->getHandiKomi(),territorycount-me->getScoreKomi(),me->getScoreKomi());
  else
    gtpe->getOutput()->printf("Territory %.1f Komi %.1f W+%.1f (with ScoreKomi %.1f) (%.1f)\n",
      territorycount,me->getHandiKomi(),-(territorycount-me->getHandiKomi()),-(territorycount-me->getScoreKomi()),me->getScoreKomi());
    
  gtpe->getOutput()->endResponse(true);
}

void Engine::gtpShowProbabilityCNN(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  me->doNPlayouts(100);
  if (cmd->numArgs()!=1)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("color is required");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  Gtp::Color gtpcol = cmd->getColorArg(0);
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printString("\n");
  //if (me->boardsize !=19)
  //{
  //  gtpe->getOutput()->printString("This only works on 19x19!!!!\n");
  //  gtpe->getOutput()->endResponse(false);
  //  return;
  //}
  int bsize=me->boardsize;
  float result[bsize*bsize];
  float value=-1;
  me->getCNN(me->currentboard,0,(gtpcol==Gtp::BLACK)?Go::BLACK:Go::WHITE,result,0,&value);
  for (int y=me->boardsize-1;y>=0;y--)
  {
    for (int x=0;x<me->boardsize;x++)
    {
      float black=1.0/log(result[bsize*x+y]);
      if (black > -0.2) black=0;
      gtpe->getOutput()->printf("%.2f ",black);
    }
    gtpe->getOutput()->printf("\n");
  }
  fprintf(stderr,"value is %f\n",1.0-value);
  gtpe->getOutput()->endResponse(true);
}

void Engine::gtpShowTerritoryCNN(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
#ifdef HAVE_CAFFE
 Engine *me=(Engine*)instance;
  me->doNPlayouts(100);
  if (cmd->numArgs()!=1)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("color is required");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  Gtp::Color gtpcol = cmd->getColorArg(0);
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printString("\n");
  if (me->boardsize !=19)
  {
    gtpe->getOutput()->printString("This only works on 19x19!!!!\n");
    gtpe->getOutput()->endResponse(false);
    return;
  }
  float *data;
  //fprintf(stderr,"1\n");
  data= new float[3*19*19];
  for (int j=0;j<19;j++)
	  for (int k=0;k<19;k++)
		{
	    int pos=Go::Position::xy2pos(j,k,me->boardsize);
      Go::Color col=me->currentboard->getColor(pos);
		  if (gtpcol==Gtp::BLACK) {
        if (col==Go::BLACK)
          data[0*19*19+j*19+k]=1.0;
        else
          data[0*19*19+j*19+k]=0.0;
        if (col==Go::WHITE)  
		      data[1*19*19+j*19+k]=1.0;
        else
          data[1*19*19+j*19+k]=0.0;
      data[2*19*19+j*19+k]=me->komi;
      }
      else {
        if (col==Go::WHITE)
          data[0*19*19+j*19+k]=1.0;
        else
          data[0*19*19+j*19+k]=0.0;
        if (col==Go::BLACK)  
		      data[1*19*19+j*19+k]=1.0;
        else
          data[1*19*19+j*19+k]=0.0;
      data[2*19*19+j*19+k]=-me->komi;
      }
    }
  float result[361];
  float diffprob[121];
  float wr[361];
  Blob<float> *b=new Blob<float>(1,3,19,19);
  b->set_cpu_data(data);
  vector<Blob<float>*> bottom;
  bottom.push_back(b); 
  const vector<Blob<float>*>& rr =  caffe_area_net->Forward(bottom);
  for (int i=0;i<361;i++) {
	  wr[i]=rr[1]->cpu_data()[i];
    //gtpe->getOutput()->printf("wr%.3f",wr[i]);
  }
  //gtpe->getOutput()->printf("\n");
  for (int i=0;i<361;i++) {
	  result[i]=rr[2]->cpu_data()[i];
    //gtpe->getOutput()->printf("tr%.3f",result[i]);
  }
  //gtpe->getOutput()->printf("\n");
  for (int i=0;i<121;i++) {
    diffprob[i]=rr[0]->cpu_data()[i];
    //gtpe->getOutput()->printf("pd%.3f",diffprob[i]);
  }
  float territorycount=0;
  float norm=0;
  for (int y=me->boardsize-1;y>=0;y--)
  {
    for (int x=0;x<me->boardsize;x++)
    {
      int pos=Go::Position::xy2pos(x,y,me->boardsize);
      float territory=me->territorymap->getPositionOwner(pos);
      float black=result[19*x+y];
      if (gtpcol!=Gtp::BLACK)
        black=-black;
      norm+=pow(black-territory,2);
      //float white=result[1*19*19 +19*x+y];
      //this later if everything is working
      //if (black>white)
      //  gtpe->getOutput()->printf("%.2f ",black);
      //else
      //  gtpe->getOutput()->printf("%.2f ",-white);
      //if (gtpcol==Gtp::BLACK)
      //  gtpe->getOutput()->printf("%.2f ",black);
      //else
      //  gtpe->getOutput()->printf("%.2f ",-white);
      territorycount+=black;
      gtpe->getOutput()->printf("%.2f ",black);
    }
    gtpe->getOutput()->printf("\n");
  }
  float winprob=0;
  float komipos=60+me->getScoreKomi();
  if (gtpcol!=Gtp::BLACK)
    komipos=60-me->getScoreKomi();
  for (int i=0;i<121;i++) {
    gtpe->getOutput()->printf("%3.0f ",diffprob[i]*1000.0);
    if (i<komipos) winprob+=diffprob[i];
    if (i==komipos) winprob+=diffprob[i]/2.0;
  }
  gtpe->getOutput()->printf("\n");
  for (int i=0;i<121;i++) {
    gtpe->getOutput()->printf("%3d ",i-60);
  }
  float wr_sum=0,wr_sqrt=0;
  for (int i=0;i<361;i++) {
    wr_sum+=wr[i];
    wr_sqrt+=wr[i]*wr[i];
  }
  gtpe->getOutput()->printf("\nwinprob1: %.3f\n",1.0-winprob);
  gtpe->getOutput()->printf("winprob2: %.3f (%.3f)\n",1.0-wr_sum/361.0,wr_sqrt/361.0-(wr_sum*wr_sum/361.0/361.0));
  
  if (territorycount-me->getHandiKomi()>0)
    gtpe->getOutput()->printf("Territory %.1f Komi %.1f B+%.1f (with ScoreKomi %.1f) (%.1f) (norm %.2f)\n",
      territorycount,me->getHandiKomi(),territorycount-me->getHandiKomi(),territorycount-me->getScoreKomi(),me->getScoreKomi(),sqrt(norm));
  else
    gtpe->getOutput()->printf("Territory %.1f Komi %.1f W+%.1f (with ScoreKomi %.1f) (%.1f) (norm %.2f)\n",
      territorycount,me->getHandiKomi(),-(territorycount-me->getHandiKomi()),-(territorycount-me->getScoreKomi()),me->getScoreKomi(),sqrt(norm));
  gtpe->getOutput()->endResponse(true);
  delete[] data;
  delete b;
#else
  gtpe->getOutput()->printf("CAFFE not availible");
  gtpe->getOutput()->endResponse(true);
#endif
}

void Engine::gtpShowPlayoutGammas(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  if (cmd->numArgs()!=1)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("color is required");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  Gtp::Color gtpcol = cmd->getColorArg(0);
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printString("\n");
  me->currentboard->updatePlayoutGammas(me->params, me->features);
  for (int y=me->boardsize-1;y>=0;y--)
  {
    for (int x=0;x<me->boardsize;x++)
    {
      int pos=Go::Position::xy2pos(x,y,me->boardsize);
      float gammaval=0;
      if (gtpcol==Gtp::BLACK)
        gammaval=me->currentboard->blackgammas->get(pos);
      else
        gammaval=me->currentboard->whitegammas->get(pos);
        
      gtpe->getOutput()->printf("\"%.2f\"",gammaval);
    }
    gtpe->getOutput()->printf("\n");
  }
  gtpe->getOutput()->endResponse(true);
}


float Engine::getAreaCorrelation(Go::Move m)
{
  int showpos=m.getPosition();
  if (showpos<0) return 0;
  int color_offset=0;
  if (m.getColor()==Go::BLACK)
    color_offset=currentboard->getPositionMax();

  float sqrsum=0;
  float sqrsumcol=0;
  float playouts=area_correlation_map[showpos+color_offset]->getPlayouts();
  for (int y=boardsize-1;y>=0;y--)
  {
    for (int x=0;x<boardsize;x++)
    {
      int pos=Go::Position::xy2pos(x,y,boardsize);
      float tmp=area_correlation_map[showpos+color_offset]->getPositionOwner(pos)-territorymap->getPositionOwner(pos);
      tmp*=playouts/(playouts+params->test_p43); 
      sqrsum+=tmp*tmp;
      if ((m.getColor()==Go::BLACK && tmp>0) || (m.getColor()==Go::WHITE && tmp<0))
        sqrsumcol+=tmp*tmp;
    }
  }
  return sqrt(sqrsumcol)/boardsize/boardsize;
}

void Engine::gtpShowTerritoryAt(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  if (cmd->numArgs()!=2)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("color vertex is required");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  Gtp::Vertex vert = cmd->getVertexArg(0);
  Gtp::Color gtpcol = cmd->getColorArg(1);
  int color_offset=0;
  if (gtpcol==Gtp::BLACK)
    color_offset=me->currentboard->getPositionMax();
  
  if (vert.x==-3 && vert.y==-3)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("invalid vertex");
    gtpe->getOutput()->endResponse();
    return;
  }

  int showpos=Go::Position::xy2pos(vert.x,vert.y,me->boardsize);
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printString("\n");
  float territorycount=0;
  float sqrsum=0;
  float sqrsumcol=0;
  float playouts=me->area_correlation_map[showpos+color_offset]->getPlayouts();
  for (int y=me->boardsize-1;y>=0;y--)
  {
    for (int x=0;x<me->boardsize;x++)
    {
      int pos=Go::Position::xy2pos(x,y,me->boardsize);
      float tmp=me->area_correlation_map[showpos+color_offset]->getPositionOwner(pos)-me->territorymap->getPositionOwner(pos);
      tmp*=playouts/(playouts+me->params->test_p43); 
      sqrsum+=tmp*tmp;
      if ((gtpcol==Gtp::BLACK && tmp>0) || (gtpcol==Gtp::WHITE && tmp<0))
        sqrsumcol+=tmp*tmp;
      //if (tmp>0.2) territorycount++;
      //if (tmp<-0.2) territorycount--;
      if (tmp<0)
        territorycount-=wf(-tmp);
      else
        territorycount+=wf(tmp);
      if (tmp<0)
        gtpe->getOutput()->printf("%.2f ",-wf(-tmp));
      else
        gtpe->getOutput()->printf("%.2f ",wf(tmp));  
    }
    gtpe->getOutput()->printf("\n");
  }

  Go::Move m=Go::Move((gtpcol==Gtp::BLACK)?Go::BLACK:Go::WHITE,showpos);
    gtpe->getOutput()->printf("Playouts %f meandiff %f meandiff color %f check %f\n",
                 playouts,sqrt(sqrsum)/me->boardsize/me->boardsize,sqrt(sqrsumcol)/me->boardsize/me->boardsize,me->getAreaCorrelation(m));

    gtpe->getOutput()->endResponse(true);
}

void Engine::gtpShowTerritoryError(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;

  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printString("\n");

  float maxerror=0.00000001;
  for (int y=me->boardsize-1;y>=0;y--)
  {
    for (int x=0;x<me->boardsize;x++)
    {
      int pos=Go::Position::xy2pos(x,y,me->boardsize);
      Go::Move m1=Go::Move(Go::BLACK,pos);
      Go::Move m2=Go::Move(Go::WHITE,pos);
      
      float tmp=me->getAreaCorrelation(m1)+me->getAreaCorrelation(m2);
      if (tmp>maxerror) maxerror=tmp;
      
    }
  }

  for (int y=me->boardsize-1;y>=0;y--)
  {
    for (int x=0;x<me->boardsize;x++)
    {
      int pos=Go::Position::xy2pos(x,y,me->boardsize);
      Go::Move m1=Go::Move(Go::BLACK,pos);
      Go::Move m2=Go::Move(Go::WHITE,pos);
      
      float tmp1=me->getAreaCorrelation(m1);
      float tmp2=me->getAreaCorrelation(m2);
      if (tmp2>tmp1)
        gtpe->getOutput()->printf("%.2f ",-(tmp2+tmp1)/maxerror);
      else
        gtpe->getOutput()->printf("%.2f ",(tmp2+tmp1)/maxerror);
    }
    gtpe->getOutput()->printf("\n");
  }
  gtpe->getOutput()->printf("maxvalue %f\n",maxerror);
  gtpe->getOutput()->endResponse(true);
}

void Engine::gtpShowMoveProbability(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printString("\n");
  for (int y=me->boardsize-1;y>=0;y--)
  {
    for (int x=0;x<me->boardsize;x++)
    {
      int pos=Go::Position::xy2pos(x,y,me->boardsize);
      float tmp=me->probabilitymap->getMoveAs(pos)/me->currentboard->numOfValidMoves();
      gtpe->getOutput()->printf("%.2f ",tmp);  
    }
    gtpe->getOutput()->printf("\n");
  }

  gtpe->getOutput()->endResponse(true);
}

void Engine::gtpShowCorrelationMap(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printString("\n");
  for (int y=me->boardsize-1;y>=0;y--)
  {
    for (int x=0;x<me->boardsize;x++)
    {
      int pos=Go::Position::xy2pos(x,y,me->boardsize);
      float tmp=me->getCorrelation(pos);
      gtpe->getOutput()->printf("%.2f ",tmp);  
    }
    gtpe->getOutput()->printf("\n");
  }
  gtpe->getOutput()->endResponse(true);
}

void Engine::gtpParam(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  if (cmd->numArgs()==0)
  {
    gtpe->getOutput()->startResponse(cmd);
    me->params->printParametersForGTP(gtpe);
    gtpe->getOutput()->endResponse(true);
  }
  else if (cmd->numArgs()==1)
  {
    std::string category=cmd->getStringArg(0);
    
    gtpe->getOutput()->startResponse(cmd);
    me->params->printParametersForGTP(gtpe,category);
    gtpe->getOutput()->endResponse(true);
  }
  else if (cmd->numArgs()==2)
  {
    std::string param=cmd->getStringArg(0);
    std::transform(param.begin(),param.end(),param.begin(),::tolower);

    //these are gammas, they are found due to a : in the string
    //TODO: move this to a separate GTP command
    int pos=param.find(":");
    if (pos>0)
    {
      std::string tmp=cmd->getStringArg(0)+" "+cmd->getStringArg(1);
      //fprintf(stderr,"test %s %d\n",tmp.c_str(),pos);
      if (me->features->loadGammaLine(tmp))
      {
        //fprintf(stderr,"success\n");
        #ifdef HAVE_MPI
          if (me->mpirank==0)
          {
            me->mpiBroadcastCommand(MPICMD_SETPARAM);
            me->mpiBroadcastString(param);
            me->mpiBroadcastString(cmd->getStringArg(1));
          }
        #endif
      
      gtpe->getOutput()->startResponse(cmd);
      gtpe->getOutput()->endResponse();
      return;
      }
    }
    if (me->params->setParameter(param,cmd->getStringArg(1)))
    {
      #ifdef HAVE_MPI
        if (me->mpirank==0)
        {
          me->mpiBroadcastCommand(MPICMD_SETPARAM);
          me->mpiBroadcastString(param);
          me->mpiBroadcastString(cmd->getStringArg(1));
        }
      #endif
      
      gtpe->getOutput()->startResponse(cmd);
      gtpe->getOutput()->endResponse();
    }
    else
    {
      gtpe->getOutput()->startResponse(cmd,false);
      gtpe->getOutput()->printf("error setting parameter: %s",param.c_str());
      gtpe->getOutput()->endResponse();
    }
  }
  else if (cmd->numArgs()==3)
  {
    std::string category=cmd->getStringArg(0); //check this parameter in category?
    std::string param=cmd->getStringArg(1);
    std::transform(param.begin(),param.end(),param.begin(),::tolower);
    if (category.compare("feature")==0) //TODO: similar to above, should be moved to a separate GTP command
    {
      std::string tmp=cmd->getStringArg(1)+" "+cmd->getStringArg(2);
      fprintf(stderr,"test %s\n",tmp.c_str());
      me->features->loadGammaLine(tmp);
    }
    if (me->params->setParameter(param,cmd->getStringArg(2)))
    {
      #ifdef HAVE_MPI
        if (me->mpirank==0)
        {
          me->mpiBroadcastCommand(MPICMD_SETPARAM);
          me->mpiBroadcastString(param);
          me->mpiBroadcastString(cmd->getStringArg(2));
        }
      #endif
      
      gtpe->getOutput()->startResponse(cmd);
      gtpe->getOutput()->endResponse();
    }
    else
    {
      gtpe->getOutput()->startResponse(cmd,false);
      gtpe->getOutput()->printf("error setting parameter: %s",param.c_str());
      gtpe->getOutput()->endResponse();
    }
  }
  else
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printf("need 0 to 3 args");
    gtpe->getOutput()->endResponse();
  }
}

void Engine::gtpTimeSettings(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  if (cmd->numArgs()!=3)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("invalid arguments");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  delete me->time;
  me->time=new Time(me->params,cmd->getIntArg(0),cmd->getIntArg(1),cmd->getIntArg(2));
  
  #ifdef HAVE_MPI
    if (me->mpirank==0)
    {
      unsigned int arg1=cmd->getIntArg(0);
      unsigned int arg2=cmd->getIntArg(1);
      unsigned int arg3=cmd->getIntArg(2);
      me->mpiBroadcastCommand(MPICMD_TIMESETTINGS,&arg1,&arg2,&arg3);
    }
  #endif
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->endResponse();
}

void Engine::gtpTimeLeft(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;

//  if (me->time->isNoTiming())
//  {
//  gtpe->getOutput()->startResponse(cmd,false);
//   gtpe->getOutput()->printString("no time settings defined");
//   gtpe->getOutput()->endResponse();
// return;
// }
//else 
  if (cmd->numArgs()!=3)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("invalid arguments");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  Gtp::Color gtpcol = cmd->getColorArg(0);
  if (gtpcol==Gtp::INVALID)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("invalid color");
    gtpe->getOutput()->endResponse();
    return;
  }
  if (me->time->isNoTiming()) {
    me->time=new Time(me->params,cmd->getIntArg(1),0,0);
    fprintf(stderr,"time_left before time_setting\n");
  }
  Go::Color col=(gtpcol==Gtp::BLACK ? Go::BLACK : Go::WHITE);
  
  float oldtime=me->time->timeLeft(col);
  //int oldstones=me->time->stonesLeft(col);
  float newtime=(float)cmd->getIntArg(1);
  int newstones=cmd->getIntArg(2);
  
  if (newstones==0)
    gtpe->getOutput()->printfDebug("[time_left]: diff:%.3f\n",newtime-oldtime);
  else
    gtpe->getOutput()->printfDebug("[time_left]: diff:%.3f s:%d\n",newtime-oldtime,newstones);
  
  me->time->updateTimeLeft(col,newtime,newstones);
  
  #ifdef HAVE_MPI
    if (me->mpirank==0)
    {
      unsigned int arg1=(unsigned int)col;
      unsigned int arg2=(unsigned int)newtime;
      unsigned int arg3=newstones;
      me->mpiBroadcastCommand(MPICMD_TIMELEFT,&arg1,&arg2,&arg3);
    }
  #endif
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->endResponse();
}

void Engine::gtpChat(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  if (cmd->numArgs()<3)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("missing arguments");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  bool pm = (cmd->getStringArg(0)=="private");
  std::string name = cmd->getStringArg(1);
  std::string msg = cmd->getStringArg(2);
  for (unsigned int i=3;i<cmd->numArgs();i++)
  {
    msg+=" "+cmd->getStringArg(i);
  }

  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printf(me->chat(pm,name,msg));
  gtpe->getOutput()->endResponse();
}

void Engine::gtpDTLoad(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  if (cmd->numArgs()!=1)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printf("need 1 arg");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  std::string filename=cmd->getStringArg(0);
  
  std::list<DecisionTree*> *trees = DecisionTree::loadFile(me->params,filename);
  if (trees!=NULL)
  {
    for (std::list<DecisionTree*>::iterator iter=trees->begin();iter!=trees->end();++iter)
    {
      me->decisiontrees.push_back((*iter));
    }
    delete trees;

    gtpe->getOutput()->startResponse(cmd);
    gtpe->getOutput()->printf("loaded decision trees: %s",filename.c_str());
    gtpe->getOutput()->endResponse();
  }
  else
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printf("error loading decision trees: %s",filename.c_str());
    gtpe->getOutput()->endResponse();
  }
}

void Engine::gtpDTSave(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  if (cmd->numArgs()<1)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printf("need 1 arg");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  std::string filename=cmd->getStringArg(0);
  bool ignorestats = false;
  if (cmd->numArgs()>1)
    ignorestats = cmd->getIntArg(1)!=0;
  
  bool res = DecisionTree::saveFile(&(me->decisiontrees),filename,ignorestats);
  if (res)
  {
    gtpe->getOutput()->startResponse(cmd);
    gtpe->getOutput()->printf("saved decision trees: %s",filename.c_str());
    gtpe->getOutput()->endResponse();
  }
  else
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printf("error saving decision trees: %s",filename.c_str());
    gtpe->getOutput()->endResponse();
  }
}

void Engine::gtpDTClear(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  for (std::list<DecisionTree*>::iterator iter=me->decisiontrees.begin();iter!=me->decisiontrees.end();++iter)
  {
    delete (*iter);
  }
  me->decisiontrees.clear();
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printf("cleared decision trees");
  gtpe->getOutput()->endResponse();
}

void Engine::gtpDTPrint(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;

  bool ignorestats = false;
  if (cmd->numArgs()>=1)
    ignorestats = cmd->getIntArg(0)!=0;
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printf("decision trees:\n");
  int leafoffset = 0;
  for (std::list<DecisionTree*>::iterator iter=me->decisiontrees.begin();iter!=me->decisiontrees.end();++iter)
  {
    gtpe->getOutput()->printf("%s\n",(*iter)->toString(ignorestats,leafoffset).c_str());
    leafoffset += (*iter)->getLeafCount();
  }
  gtpe->getOutput()->endResponse(true);
}

void Engine::gtpDTAt(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  if (cmd->numArgs()!=1)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("vertex is required");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  Gtp::Vertex vert = cmd->getVertexArg(0);
  
  if (vert.x==-3 && vert.y==-3)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("invalid vertex");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  int pos=Go::Position::xy2pos(vert.x,vert.y,me->boardsize);
  Go::Move move=Go::Move(me->currentboard->nextToMove(),pos);

  DecisionTree::GraphCollection *graphs = new DecisionTree::GraphCollection(DecisionTree::getCollectionTypes(&(me->decisiontrees)),me->currentboard);
  float w = DecisionTree::getCollectionWeight(&(me->decisiontrees), graphs, move);
  delete graphs;

  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printf("weight for %s: %.2f",move.toString(me->boardsize).c_str(),w);
  gtpe->getOutput()->endResponse();
}

void Engine::gtpDTUpdate(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  DecisionTree::GraphCollection *graphs = new DecisionTree::GraphCollection(DecisionTree::getCollectionTypes(&(me->decisiontrees)),me->currentboard);
  DecisionTree::collectionUpdateDescent(&(me->decisiontrees),graphs,Go::Move(me->currentboard->nextToMove(),Go::Move::PASS));
  delete graphs;

  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printf("updated decision trees");
  gtpe->getOutput()->endResponse();
}

void Engine::gtpDTSet(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  if (cmd->numArgs()!=2)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("id and weight are required");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  int id = cmd->getIntArg(0);
  float weight = cmd->getFloatArg(1);
  
  DecisionTree::setCollectionLeafWeight(&(me->decisiontrees), id, weight);

  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printf("decision tree leaf weight updated");
  gtpe->getOutput()->endResponse();
}

void Engine::gtpDTDistribution(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  Go::Board *board = me->currentboard;
  Go::Color col = board->nextToMove();

  DecisionTree::GraphCollection *graphs = new DecisionTree::GraphCollection(DecisionTree::getCollectionTypes(&(me->decisiontrees)),board);
  float weightmax = 0;
  for (int y=me->boardsize-1;y>=0;y--)
  {
    for (int x=0;x<me->boardsize;x++)
    {
      Go::Move move = Go::Move(col,Go::Position::xy2pos(x,y,me->boardsize)); 
      float weight = DecisionTree::getCollectionWeight(&(me->decisiontrees), graphs, move);
      if (weight >= 0 && weight > weightmax)
        weightmax = weight;
    }
  }
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printString("\n");
  for (int y=me->boardsize-1;y>=0;y--)
  {
    for (int x=0;x<me->boardsize;x++)
    {
      Go::Move move = Go::Move(col,Go::Position::xy2pos(x,y,me->boardsize)); 
      float weight = DecisionTree::getCollectionWeight(&(me->decisiontrees), graphs, move);
      if (weight < 0)
        gtpe->getOutput()->printf("\"\" ");
      else
      {
        //float val = atan(weight)/asin(1);
        float val = weight/weightmax;
        float point1 = 0.15;
        float point2 = 0.65;
        float r,g,b;
        // scale from blue-green-red-reddest?
        if (val >= point2)
        {
          b = 0;
          r = 1;
          g = 0;
        }
        else if (val >= point1)
        {
          b = 0;
          r = (val-point1)/(point2-point1);
          g = 1 - r;
        }
        else
        {
          r = 0;
          g = val/point1;
          b = 1 - g;
        }
        if (r < 0)
          r = 0;
        if (g < 0)
          g = 0;
        if (b < 0)
          b = 0;
        gtpe->getOutput()->printf("#%02x%02x%02x ",(int)(r*255),(int)(g*255),(int)(b*255));
        //gtpe->getOutput()->printf("#%06x ",(int)(prob*(1<<24)));
      }
    }
    gtpe->getOutput()->printf("\n");
  }
  delete graphs;

  gtpe->getOutput()->endResponse(true);
}

void Engine::gtpDTStats(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;

  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printf("decision trees stats:");

  int forest_treenodes=0;
  int forest_leaves=0;
  int forest_maxdepth=0;
  int forest_sumdepth=0;
  int forest_sumsqdepth=0;
  int forest_maxnodes=0;
  int forest_sumnodes=0;
  int forest_sumsqnodes=0;
  float forest_sumlogweights=0;
  float forest_sumsqlogweights=0;

  for (std::list<DecisionTree*>::iterator iter=me->decisiontrees.begin();iter!=me->decisiontrees.end();++iter)
  {
    std::vector<std::string> *attrs = (*iter)->getAttrs();
    std::string attrstr = "";
    for (unsigned int i=0;i<attrs->size();i++)
    {
      if (i!=0)
        attrstr += "|";
      attrstr += attrs->at(i);
    }

    int treenodes;
    int leaves;
    int maxdepth;
    int sumdepth;
    int sumsqdepth;
    int maxnodes;
    int sumnodes;
    int sumsqnodes;
    float sumlogweights;
    float sumsqlogweights;
    (*iter)->getTreeStats(treenodes,leaves,maxdepth,sumdepth,sumsqdepth,maxnodes,sumnodes,sumsqnodes,sumlogweights,sumsqlogweights);
    forest_treenodes += treenodes;
    forest_leaves += leaves;
    forest_sumdepth += sumdepth;
    forest_sumsqdepth += sumsqdepth;
    forest_sumnodes += sumnodes;
    forest_sumsqnodes += sumsqnodes;
    forest_sumlogweights += sumlogweights;
    forest_sumsqlogweights += sumsqlogweights;
    if (maxdepth > forest_maxdepth)
      forest_maxdepth = maxdepth;
    if (maxnodes > forest_maxnodes)
      forest_maxnodes = maxnodes;

    float avgdepth = (float)sumdepth/leaves;
    float avgnodes = (float)sumnodes/leaves;

    float vardepth = (float)sumsqdepth/leaves - avgdepth*avgdepth;
    float varnodes = (float)sumsqnodes/leaves - avgnodes*avgnodes;

    float avglogweight = sumlogweights/leaves;
    float varlogweights = sumsqlogweights/leaves - avglogweight*avglogweight;

    gtpe->getOutput()->printf("\nStats for DT[%s]:\n",attrstr.c_str());
    gtpe->getOutput()->printf("  Nodes: %d\n",treenodes);
    gtpe->getOutput()->printf("  Leaves: %d\n",leaves);
    gtpe->getOutput()->printf("  Max Depth: %d\n",maxdepth);
    gtpe->getOutput()->printf("  Avg Depth: %.2f\n",avgdepth);
    gtpe->getOutput()->printf("  Var Depth: %.2f\n",vardepth);
    gtpe->getOutput()->printf("  Max Nodes: %d\n",maxnodes);
    gtpe->getOutput()->printf("  Avg Nodes: %.2f\n",avgnodes);
    gtpe->getOutput()->printf("  Var Nodes: %.2f\n",varnodes);
    gtpe->getOutput()->printf("  Var LW: %.3f\n",varlogweights);
  }

  float forest_avgdepth = (float)forest_sumdepth/forest_leaves;
  float forest_avgnodes = (float)forest_sumnodes/forest_leaves;

  float forest_vardepth = (float)forest_sumsqdepth/forest_leaves - forest_avgdepth*forest_avgdepth;
  float forest_varnodes = (float)forest_sumsqnodes/forest_leaves - forest_avgnodes*forest_avgnodes;

  float forest_avglogweight = forest_sumlogweights/forest_leaves;
  float forest_varlogweights = forest_sumsqlogweights/forest_leaves - forest_avglogweight*forest_avglogweight;

  gtpe->getOutput()->printf("\nStats for forest:\n");
  gtpe->getOutput()->printf("  Nodes: %d\n",forest_treenodes);
  gtpe->getOutput()->printf("  Leaves: %d\n",forest_leaves);
  gtpe->getOutput()->printf("  Max Depth: %d\n",forest_maxdepth);
  gtpe->getOutput()->printf("  Avg Depth: %.2f\n",forest_avgdepth);
  gtpe->getOutput()->printf("  Var Depth: %.2f\n",forest_vardepth);
  gtpe->getOutput()->printf("  Max Nodes: %d\n",forest_maxnodes);
  gtpe->getOutput()->printf("  Avg Nodes: %.2f\n",forest_avgnodes);
  gtpe->getOutput()->printf("  Var Nodes: %.2f\n",forest_varnodes);
  gtpe->getOutput()->printf("  Var LW: %.3f\n",forest_varlogweights);

  gtpe->getOutput()->endResponse(true);
}

void Engine::gtpDTPath(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  if (cmd->numArgs()!=1)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("leaf id required");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  int id = cmd->getIntArg(0);
  
  std::string path = DecisionTree::getCollectionLeafPath(&(me->decisiontrees), id);

  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printf("decision tree leaf %d path: %s",id,path.c_str());
  gtpe->getOutput()->endResponse();
}

void Engine::gtpGameOver(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;

  me->gameFinished();
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->endResponse();
}

void Engine::gtpEcho(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  gtpe->getOutput()->startResponse(cmd);
  for (unsigned int i=0; i<cmd->numArgs(); i++)
  {
    if (i!=0)
      gtpe->getOutput()->printf(" ");
    gtpe->getOutput()->printf(cmd->getStringArg(i));
  }
  gtpe->getOutput()->endResponse();
}

void Engine::gtpPlaceFreeHandicap(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  if (cmd->numArgs()!=1)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("argument is required");
    gtpe->getOutput()->endResponse();
    return;
  }

  int numHandicapstones=cmd->getIntArg (0);
  if (numHandicapstones<2||numHandicapstones>9)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("number of handicap stones not supported");
    gtpe->getOutput()->endResponse();
    return;
  }

  int sizem1=me->boardsize-1;
  int borderdist=3;
  if (me->boardsize<13)
    borderdist=2;
  int sizemb=sizem1-borderdist;
  Gtp::Vertex vert[9];
  vert[0].x=borderdist;   vert[0].y=borderdist;
  vert[1].x=sizemb;       vert[1].y=sizemb;
  vert[2].x=sizemb;       vert[2].y=borderdist;
  vert[3].x=borderdist;   vert[3].y=sizemb;
  vert[4].x=borderdist;   vert[4].y=sizem1/2;
  vert[5].x=sizemb;       vert[5].y=sizem1/2;
  vert[6].x=sizem1/2;     vert[6].y=borderdist;
  vert[7].x=sizem1/2;     vert[7].y=sizemb;
  vert[8].x=sizem1/2;     vert[8].y=sizem1/2;
  if (numHandicapstones>4 && numHandicapstones%2==1)
  {
    vert[numHandicapstones-1].x=sizem1/2; vert[numHandicapstones-1].y=sizem1/2;
  }
  gtpe->getOutput()->startResponse(cmd);
  for (int i=0;i<numHandicapstones;i++)
  {
    Go::Move move=Go::Move(Go::BLACK,vert[i].x,vert[i].y,me->boardsize);
    me->makeMove(move);
    gtpe->getOutput()->printVertex(vert[i]);
    gtpe->getOutput()->printf(" ");
  }
  me->setHandicapKomi(numHandicapstones);
  gtpe->getOutput()->endResponse();
}

void Engine::gtpSetFreeHandicap(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;

  int numVertices=cmd->numArgs();
  if (cmd->numArgs()<2)
  {
    gtpe->getOutput()->startResponse(cmd,false);
    gtpe->getOutput()->printString("At least 2 handicap stones required");
    gtpe->getOutput()->endResponse();
    return;
  }

  gtpe->getOutput()->startResponse(cmd);
  for (int x=0;x<numVertices;x++)
  {
    Gtp::Vertex vert=cmd->getVertexArg (x);
    Go::Move move=Go::Move(Go::BLACK,vert.x,vert.y,me->boardsize);
    if (!me->isMoveAllowed(move))
    {
      gtpe->getOutput()->startResponse(cmd,false);
      gtpe->getOutput()->printString("illegal move");
      gtpe->getOutput()->endResponse();
      return;
    }
    me->makeMove(move);
  
    //gtpe->getOutput()->printVertex(vert);
    //gtpe->getOutput()->printf(" ");
  }
  me->setHandicapKomi(numVertices);
  gtpe->getOutput()->endResponse();
}

void Engine::learnFromTree(Go::Board *tmpboard, Tree *learntree, std::ostringstream *ssun, int movenum)
{
  int num_unpruned=learntree->getNumUnprunedChildren();
  std::map<float,Go::Move,std::greater<float> > ordervalue;
  std::map<float,Tree*,std::greater<float> > orderlearntree;
  std::map<float,Go::Move,std::greater<float> > ordergamma;

  float forcesort=0;
  *ssun<<"\nun:"<<movenum<<"(";
  Go::ObjectBoard<int> *cfglastdist=NULL;
  Go::ObjectBoard<int> *cfgsecondlastdist=NULL;
  getFeatures()->computeCFGDist(tmpboard,&cfglastdist,&cfgsecondlastdist);
  DecisionTree::GraphCollection *graphs = new DecisionTree::GraphCollection(DecisionTree::getCollectionTypes(&decisiontrees),currentboard);

  for (int nn=1;nn<=num_unpruned;nn++)
  {
    for(std::list<Tree*>::iterator iter=learntree->getChildren()->begin();iter!=learntree->getChildren()->end();++iter) 
    {
      if ((*iter)->getUnprunedNum()==nn && (*iter)->isPrimary() && !(*iter)->isPruned())
      {
        *ssun<<(nn!=1?",":"")<<Go::Position::pos2string((*iter)->getMove().getPosition(),boardsize);
        //do not use getFeatureGamma of the tree, as this might be not exactly the order of the gammas to be trained
        ordergamma.insert(std::make_pair(getFeatures()->getMoveGamma(tmpboard,cfglastdist,cfgsecondlastdist,graphs,(*iter)->getMove())+forcesort,(*iter)->getMove()));
        ordervalue.insert(std::make_pair((*iter)->getPlayouts()+forcesort,(*iter)->getMove()));
        orderlearntree.insert(std::make_pair((*iter)->getPlayouts()+forcesort,(*iter)));
        forcesort+=0.001012321232123;
      }
    }
  }
  //include pruned into learning, as they all lost!!
  for(std::list<Tree*>::iterator iter=learntree->getChildren()->begin();iter!=learntree->getChildren()->end();++iter) 
  {
    if ((*iter)->isPrimary() && (*iter)->isPruned())
    {
      //ssun<<(nn!=1?",":"")<<Go::Position::pos2string((*iter)->getMove().getPosition(),boardsize);
      //do not use getFeatureGamma of the tree, as this might be not exactly the order of the gammas to be trained
      ordergamma.insert(std::make_pair(getFeatures()->getMoveGamma(tmpboard,cfglastdist,cfgsecondlastdist,graphs,(*iter)->getMove())+forcesort,(*iter)->getMove()));
      ordervalue.insert(std::make_pair(0.0+forcesort,(*iter)->getMove()));
      forcesort+=0.001012321232123;
    }
  }
  *ssun<<")";
  if (ordergamma.size()!=ordervalue.size())
    *ssun<<"\nthe ordering of gamma versus mc did not work correctly "<<ordergamma.size()<<" "<<ordervalue.size()<<"\n";
  //*ssun<<" ordermc:(";

  //for the moves (getPosition) the difference mc_position - gamma_position is calculated into numvalue_gamma
  std::map<int,int> mc_pos_move;
  std::map<int,int> gamma_move_pos;
  std::map<int,float> numvalue_gamma;
  std::map<int,float> move_gamma;
  float sum_gammas=0;
  std::map<float,Go::Move>::iterator it;
  int nn=1;
  for (it=ordervalue.begin();it!=ordervalue.end();++it)
  {
    //*ssun<<(nn!=1?",":"")<<Go::Position::pos2string(it->second.getPosition(),boardsize);
    mc_pos_move.insert(std::make_pair(nn,it->second.getPosition()));
    nn++;
  }
  //*ssun<<") ordergamma:(";
  nn=1;
#define sign(A) ((A>0)?1:((A<0)?-1:0))
#define gamma_from_mc_position(A) (move_gamma.find(mc_pos_move.find(A)->second)->second)
  for (it=ordergamma.begin();it!=ordergamma.end();++it)
  {
    //*ssun<<(nn!=1?",":"")<<Go::Position::pos2string(it->second.getPosition(),boardsize);
    gamma_move_pos.insert(std::make_pair(it->second.getPosition(),nn));
    move_gamma.insert(std::make_pair(it->second.getPosition(),it->first));
    sum_gammas+=it->first;
    nn++;
  }
  getFeatures()->learnMovesGamma(tmpboard,cfglastdist,cfgsecondlastdist,ordervalue,move_gamma,sum_gammas);
  *ssun<<")";
  std::map<float,Tree*>::iterator it_learntree;
  for (it_learntree=orderlearntree.begin();it_learntree!=orderlearntree.end();++it_learntree)
  {
    //check if enough playouts
    if (params->mm_learn_min_playouts>=it_learntree->second->getPlayouts ())
      break;
    //tmpboard must be copied
    Go::Board *nextboard=tmpboard->copy();
    //the move must be made first!
    nextboard->makeMove(it_learntree->second->getMove());
    *ssun<<"-"<<it_learntree->second->getMove().toString (boardsize)<<"-";
    //learnFromTree has to be called
    learnFromTree (nextboard,it_learntree->second,ssun,movenum+1);
  }

  if (cfglastdist!=NULL)
    delete cfglastdist;
  if (cfgsecondlastdist!=NULL)
    delete cfgsecondlastdist;
  delete graphs;
}

void Engine::generateMove(Go::Color col, Go::Move **move, bool playmove)
{
  clearStatistics();
  clearExpandStats();
  respondboard->scale(0.2);

  if (params->move_policy==Parameters::MP_CNN) {
    float *CNNresults=new float[boardsize*boardsize];
    float value=-1;
    params->engine->getCNN(currentboard,0,col,CNNresults,0,&value);
    std::string lastexplanation="cnn";
    std::ostringstream stringStream;
    std::vector<std::pair<float,int>> m;
    for (int x=0;x<boardsize;x++)
      for (int y=0;y<boardsize;y++) {
        int p=Go::Position::xy2pos(x,y,boardsize);
        if (currentboard->validMove (col,p) && !currentboard->strongEye(col,p) && !currentboard->isSelfAtariOfSize (Go::Move(col,p),6)) {
          m.push_back({-CNNresults[boardsize*x+y],p});
        }
      }
    int play_pos=0;
    if (m.size()>1) {
      std::sort(m.begin(),m.end());
      if (params->cnn_random_for_only_cnn>1) {
        int num=m.size();
        if (params->cnn_random_for_only_cnn<num) num=params->cnn_random_for_only_cnn;
        float sum=0;
        for (int i=0;i<num;i++)
          sum-=m[i].first;
        float tosum=sum*threadpool->getThreadZero()->getSettings()->rand->getRandomReal();
        float sum_now=0;
        while (play_pos<num) {
          sum_now-=m[play_pos].first;
          if (sum_now>tosum)
            break;
          play_pos++;
        }
        if (play_pos==num)
          play_pos=num-1;
      }
      *move=new Go::Move(col,m[play_pos].second);
      fprintf(stderr," move %s p: %f v: %f\n",(*move)->toString(boardsize).c_str(),-m[play_pos].first,1.0-value);
      stringStream << "cnn-move " << (*move)->toString(boardsize) << " p:" << -m[play_pos].first << " v:" << (1.0-value);
      lastexplanation=stringStream.str();
    }
    else
      *move=new Go::Move(col,Go::Move::PASS);
    if (playmove) {
      this->makeMove(**move);
      moveexplanations->back()=lastexplanation;
      gtpe->getOutput()->printfDebug("[genmove]: %s\n",lastexplanation.c_str());
    }
          
    return;
  }
  if (params->play_n_passes_first>0) {
    //for the MFGO1998 challange
    *move=new Go::Move(col,Go::Move::PASS);
    params->play_n_passes_first--;
    return;
  }
  if (params->book_use)
  {
    std::list<Go::Move> bookmoves=book->getMoves(boardsize,movehistory);
    
    if (bookmoves.size()>0)
    {
      int r=threadpool->getThreadZero()->getSettings()->rand->getRandomInt(bookmoves.size());
      int i=0;
      for (std::list<Go::Move>::iterator iter=bookmoves.begin();iter!=bookmoves.end();++iter)
      {
        if (i==r)
        {
          *move=new Go::Move(*iter);
          
          if (playmove)
            this->makeMove(**move);
            
          std::string lastexplanation="selected move from book";
          if (playmove)
            moveexplanations->back()=lastexplanation;
          
          gtpe->getOutput()->printfDebug("[genmove]: %s\n",lastexplanation.c_str());
          if (params->livegfx_on)
            gtpe->getOutput()->printfDebug("gogui-gfx: TEXT [genmove]: %s\n",lastexplanation.c_str());
          
          return;
        }
        i++;
      }
    }
  }
  if (params->move_policy==Parameters::MP_UCT || params->move_policy==Parameters::MP_ONEPLY)
  {
    boost::posix_time::ptime time_start=this->timeNow();
    int totalplayouts=0;
    float time_allocated;
    float playouts_per_milli;
    
    params->early_stop_occured=false;
    stopthinking=false;
    
    if (!time->isNoTiming())
    {
      if (params->time_ignore)
      {
        time_allocated=0;
        gtpe->getOutput()->printfDebug("[time_allowed]: ignoring time settings!\n");
      }
      else
      {
        time_allocated=time->getAllocatedTimeForNextTurn(col);
        gtpe->getOutput()->printfDebug("[time_allowed]: %.3f\n",time_allocated);
      }
    }
    else
      time_allocated=0;
    
    if (params->livegfx_on)
      gtpe->getOutput()->printfDebug("gogui-gfx: TEXT [genmove]: starting...\n");
    
    Go::Color expectedcol=currentboard->nextToMove();
    currentboard->setNextToMove(col);
    
    if (expectedcol!=col)
    {
      gtpe->getOutput()->printfDebug("WARNING! Unexpected color. Discarding tree.\n");
      this->clearMoveTree();
    }
    
    #ifdef HAVE_MPI
      MPIRANK0_ONLY(
        unsigned int tmp1=(unsigned int)col;
        this->mpiBroadcastCommand(MPICMD_GENMOVE,&tmp1);
      );
    #endif
    
    movetree->pruneSuperkoViolations();
    this->allowContinuedPlay();
    //no idea what this was for? seems buggy ...
    //this->updateTerritoryScoringInTree();
    params->uct_slow_update_last=0;
    params->uct_last_r2=-1;
    
    int startplayouts=(int)movetree->getPlayouts();
    #ifdef HAVE_MPI
      params->mpi_last_update=MPI::Wtime();
    #endif
    
    params->uct_initial_playouts=startplayouts;
    params->thread_job=Parameters::TJ_GENMOVE;
    threadpool->startAll();
    threadpool->waitAll();
    
    totalplayouts=(int)movetree->getPlayouts()-startplayouts;
    //fprintf(stderr,"tplts: %d\n",totalplayouts);
    
    if (movetree->isTerminalResult())
      gtpe->getOutput()->printfDebug("SOLVED! found 100%% sure result after %d plts!\n",totalplayouts);

    int num_unpruned=movetree->getNumUnprunedChildren();
    int sko=movetree->superKoChildViolation(); 
    std::ostringstream ssun;
    if (params->mm_learn_enabled)
      learnFromTree (currentboard,movetree,&ssun,1);
    else
    {
      ssun<<" un:(";
      for (int nn=1;nn<=num_unpruned;nn++)
      {
        for(std::list<Tree*>::iterator iter=movetree->getChildren()->begin();iter!=movetree->getChildren()->end();++iter) 
        {
          if ((*iter)->getUnprunedNum()==nn && (*iter)->isPrimary() && !(*iter)->isPruned())
          {
            ssun<<(nn!=1?",":"")<<Go::Position::pos2string((*iter)->getMove().getPosition(),boardsize);
          }
        }
      }
    ssun<<")";
    }
    ssun<<"st:(";
    for (int nn=0;nn<STATISTICS_NUM;nn++)
    {
      //ssun<<((nn!=0)?",":"");
      if (nn!=0) ssun<<",";
      //ssun<<nn;
      //ssun<<"-";
      ssun<<getStatistics (nn);
    }
    ssun<<")";
    ssun<< " ravepreset: " << (presetplayouts/presetnum);
    ssun<< " expand_num: "<<getExpandStats();
    if (params->csstyle_adaptiveplayouts) {
      float min=1,max=1;
      for (int i=0;i<2*boardsize*boardsize*(local_feature_num+hashto5num);i++) {
        if (deltagammaslocal[i]<min) min=deltagammaslocal[i];
        if (deltagammaslocal[i]>max) max=deltagammaslocal[i];
      }
      ssun<<" move finished min "<<min<<" max "<<max;
    }
    Tree *besttree=movetree->getRobustChild();
    if (besttree->isPruned()) {
      fprintf(stderr,"besttree is pruned but has %f playouts ?!\n",besttree->getPlayouts()); 
    }
    float scoresd=0;
    float scoremean=0;
    float bestratio=0;
    float cnn_value_display=-1;
    int   best_unpruned=0;
    float ratiodelta=-1;
    bool bestsame=false;
    int eq_moves=0;
    int eq_moves2=0;
    if (besttree==NULL)
    {
      fprintf(stderr,"WARNING! No move found!\n");
      *move=new Go::Move(col,Go::Move::RESIGN);
    }
    else if (!besttree->isTerminalWin() && besttree->getRatio()<params->resign_ratio_threshold && currentboard->getMovesMade()>(params->resign_move_factor_threshold*boardsize*boardsize))
    {
      *move=new Go::Move(col,Go::Move::RESIGN);
      bestratio=besttree->getRatio();
      cnn_value_display=besttree->getCNNvalue();
      eq_moves=besttree->countMoveCirc();
      eq_moves2=besttree->countMoveCirc2();
    }
    else
    {
      *move=new Go::Move(col,besttree->getMove().getPosition());
      bestratio=besttree->getRatio();
      cnn_value_display=besttree->getCNNvalue();
      eq_moves=besttree->countMoveCirc();
      eq_moves2=besttree->countMoveCirc2();
      scoresd=besttree->getScoreSD();
      scoremean=besttree->getScoreMean();
      best_unpruned=besttree->getUnprunedNum();
      
      ratiodelta=besttree->bestChildRatioDiff();
      bestsame=(besttree==(movetree->getBestRatioChild(10)));
    }
    
    if (params->uct_slow_update_last!=0)
      this->doSlowUpdate();
    
    /*if ((**move).isResign())
    {
      gtpe->getOutput()->printfDebug("[resign]: %s\n",besttree!=NULL?besttree->getMove().toString(boardsize).c_str():"NONE");
      this->writeSGF("debugresign.sgf",currentboard,movetree);
    }*/
    
    if (playmove)
      this->makeMove(**move);
    
    if (params->livegfx_on)
      gtpe->getOutput()->printfDebug("gogui-gfx: CLEAR\n");
    
    float time_used=this->timeSince(time_start);
    //fprintf(stderr,"tu: %f\n",time_used);
    if (time_used>0)
      playouts_per_milli=(float)totalplayouts/(time_used*1000);
    else
      playouts_per_milli=-1;
    if (!time->isNoTiming())
      time->useTime(col,time_used);
    
    std::ostringstream ss;
    ss << std::fixed;
    ss << "r:"<<std::setprecision(3)<<bestratio;
    if (cnn_value_display>-1)
      ss << " v:"<<std::setprecision(3)<<cnn_value_display;
    if (!time->isNoTiming())
    {
      ss << " tl:"<<std::setprecision(3)<<time->timeLeft(col);
      if (time->stonesLeft(col)>0)
        ss << " s:"<<time->stonesLeft(col);
    }
    //this was added because of a strange bug crashing some times in the following lines
    //I did not really found the problem?!
    //fprintf(stderr,"debug %f\n",scoresd);
    if (!time->isNoTiming() || params->early_stop_occured)
      ss << " plts:"<<totalplayouts;
    ss << " ppms:"<<std::setprecision(2)<<playouts_per_milli;
    ss << " rd:"<<std::setprecision(3)<<ratiodelta;
    ss << " r2:"<<std::setprecision(2)<<params->uct_last_r2;
    ss << " fs:"<<std::setprecision(2)<<scoremean;
    ss << " eq:"<<eq_moves;
    ss << " eq2:"<<eq_moves2;
    if (params->recalc_dynkomi_limit>0)  //do not accept loosing!
    {
      ss<< " dyn:"<<std::setprecision(1)<<recalc_dynkomi;
      /*
       * used if it is calculated from the last move
       switch ((*move)->getColor())
      {
        case Go::BLACK:
          //recalc_dynkomi+=scoremean/10.0;
          recalc_dynkomi=scoremean/2.0;
          if (recalc_dynkomi<0) recalc_dynkomi=0; //do not accept loosing
          break;
        case Go::WHITE:
          //recalc_dynkomi-=scoremean/10.0;
          recalc_dynkomi=-scoremean/2.0;
          if (recalc_dynkomi>0) recalc_dynkomi=0; //do not accept loosing
          break;
        default:
          break;
      }
      if (recalc_dynkomi>params->recalc_dynkomi_limit)
        recalc_dynkomi=params->recalc_dynkomi_limit;
      else
        if (recalc_dynkomi<-params->recalc_dynkomi_limit)
          recalc_dynkomi=-params->recalc_dynkomi_limit;
      */
    }
    ss << " fsd:"<<std::setprecision(2)<<scoresd;
    ss << " un:"<<best_unpruned<<"/"<<num_unpruned<<"(-"<<sko<<")";
    ss << " bs:"<<bestsame;

    
    
    Tree *pvtree=movetree->getRobustChild(true);
    if (pvtree!=NULL)
    {
      std::list<Go::Move> pvmoves=pvtree->getMovesFromRoot();
      ss<<" pv:(";
      for(std::list<Go::Move>::iterator iter=pvmoves.begin();iter!=pvmoves.end();++iter) 
      {
        ss<<(iter!=pvmoves.begin()?",":"")<<Go::Position::pos2string((*iter).getPosition(),boardsize);
      }
      ss<<")";
    }

    ss << " " << ssun.str();

    if (params->surewin_expected)
      ss << " surewin!";
    std::string lastexplanation=ss.str();

    if (playmove)
      moveexplanations->back()=lastexplanation;
    
    gtpe->getOutput()->printfDebug("[genmove]: %s\n",lastexplanation.c_str());
    if (params->livegfx_on)
      gtpe->getOutput()->printfDebug("gogui-gfx: TEXT [genmove]: %s\n",lastexplanation.c_str());
  }
  else
  {
    *move=new Go::Move(col,Go::Move::PASS);
    Go::Board *playoutboard=currentboard->copy();
    playoutboard->turnSymmetryOff();
    if (params->playout_features_enabled>0)
      playoutboard->setFeatures(features,params->playout_features_incremental,params->test_p8==0);
    if (params->csstyle_enabled) {
      if (params->playout_features_enabled)
        fprintf(stderr,"playout_features_enabled and csstyle_enabled can not be used together!!!!\n");
      playoutboard->updatePlayoutGammas(params, features);
    }
    critstruct *critarray=NULL;
    float *b_ravearray=NULL;
    float *w_ravearray=NULL;
    Tree *pooltree=movetree;
    if (pooltree!=NULL)
    {
      critarray=new critstruct[playoutboard->getPositionMax()];
      for (int i=0;i<playoutboard->getPositionMax ();i++)
          critarray[i]={0,0,0,0,0};
      b_ravearray=new float[playoutboard->getPositionMax()];
      w_ravearray=new float[playoutboard->getPositionMax()];
      //fprintf(stderr,"poolrave %f number children %d\n",pooltree->getRAVEPlayouts(),pooltree->getChildren()->size());
      for(std::list<Tree*>::iterator iter=pooltree->getChildren()->begin();iter!=pooltree->getChildren()->end();++iter) 
        {
          if (!(*iter)->getMove().isPass())
          {
            critarray[(*iter)->getMove().getPosition()].crit=(*iter)->getCriticality();
			      critarray[(*iter)->getMove().getPosition()].ownselfblack=(*iter)->getOwnSelfBlack();
			      critarray[(*iter)->getMove().getPosition()].ownselfwhite=(*iter)->getOwnSelfWhite();
			      critarray[(*iter)->getMove().getPosition()].ownblack=(*iter)->getOwnRatio(Go::BLACK);
			      critarray[(*iter)->getMove().getPosition()].ownwhite=(*iter)->getOwnRatio(Go::WHITE);
            critarray[(*iter)->getMove().getPosition()].slopeblack=(*iter)->getSlope(Go::BLACK);
			      critarray[(*iter)->getMove().getPosition()].slopewhite=(*iter)->getSlope(Go::WHITE);
            critarray[(*iter)->getMove().getPosition()].isbadblack=false;
            critarray[(*iter)->getMove().getPosition()].isbadwhite=false;
            
            if (params->debug_on)
            {
              fprintf(stderr,"move %s %d crit %f ownblack %f ownwhite %f ownrationb %f ownrationw %f slopeblack %f slopewhite %f\n",
                    (*iter)->getMove().toString(playoutboard->getSize()).c_str(),(*iter)->getMove().getPosition(),
                    critarray[(*iter)->getMove().getPosition()].crit,
                    critarray[(*iter)->getMove().getPosition()].ownselfblack,
                    critarray[(*iter)->getMove().getPosition()].ownselfwhite,
                    critarray[(*iter)->getMove().getPosition()].ownblack,
                    critarray[(*iter)->getMove().getPosition()].ownwhite,
                    critarray[(*iter)->getMove().getPosition()].slopeblack,
			              critarray[(*iter)->getMove().getPosition()].slopewhite
                      );
            (*iter)->displayOwnerCounts();
            }
            if ((*iter)->getMove().getColor()==Go::BLACK)
				    {
				      b_ravearray[(*iter)->getMove().getPosition()]=(*iter)->getRAVERatio();
				      w_ravearray[(*iter)->getMove().getPosition()]=(*iter)->getRAVERatioOther();
				    }
			      else
			      {
				      w_ravearray[(*iter)->getMove().getPosition()]=(*iter)->getRAVERatio();
				      b_ravearray[(*iter)->getMove().getPosition()]=(*iter)->getRAVERatioOther();
			      }
			 	  }
        }
    }

    playout->getPlayoutMove(threadpool->getThreadZero()->getSettings(),playoutboard,col,**move,critarray,(col==Go::BLACK)?b_ravearray:w_ravearray, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0,  NULL, NULL, NULL, 0);
    if (params->playout_useless_move)
      playout->checkUselessMove(threadpool->getThreadZero()->getSettings(),playoutboard,col,**move,(std::string *)NULL);
    delete playoutboard;
    //if (params->playout_defend_approach)
    //  currentboard->connectedAtariPos(**move,ACpos,ACcount);
    if (ACcount>0) 
    {
      for (int i=0;i<ACcount;i++)
        fprintf(stderr," %s ",Go::Position::pos2string(ACpos[i],currentboard->getSize()).c_str());
      fprintf(stderr,"---\n");
    }
    
    this->makeMove(**move);
    if (critarray)
      delete[] critarray;
    if (b_ravearray)
      delete[] b_ravearray;
    if (w_ravearray)
      delete[] w_ravearray;
  
  }
}

void Engine::getOnePlayoutMove(Go::Board *board, Go::Color col, Go::Move *move)
{
  Go::Board *playoutboard=board->copy();
  playoutboard->turnSymmetryOff();
  if (params->playout_features_enabled>0)
    playoutboard->setFeatures(features,params->playout_features_incremental,params->test_p8==0);
  if (params->csstyle_enabled) {
    if (params->playout_features_enabled)
      fprintf(stderr,"playout_features_enabled and csstyle_enabled can not be used together!!!!\n");
    playoutboard->updatePlayoutGammas(params, features);
  }
  playout->getPlayoutMove(threadpool->getThreadZero()->getSettings(),playoutboard,col,*move,NULL,NULL);
  if (params->playout_useless_move)
    playout->checkUselessMove(threadpool->getThreadZero()->getSettings(),playoutboard,col,*move,(std::string *)NULL);
  delete playoutboard;
}

bool Engine::isMoveAllowed(Go::Move move)
{
  return currentboard->validMove(move);
}

void Engine::makeMove(Go::Move move)
{
  #ifdef HAVE_MPI
    MPIRANK0_ONLY(
      unsigned int tmp1=(unsigned int)move.getColor();
      unsigned int tmp2=(unsigned int)move.getPosition();
      this->mpiBroadcastCommand(MPICMD_MAKEMOVE,&tmp1,&tmp2);
    );
  #endif

#define WITH_P(A) (A>=1.0 || (A>0 && threadpool->getThreadZero()->getSettings()->rand->getRandomReal()<A))  
  Engine *me=params->engine;
  bool playoutmove_triggered=true;
  if (params->features_output_for_playout)
  { 
    Go::Color col=move.getColor();
    Go::Move movetmp=Go::Move(col,Go::Move::PASS);
    Go::Board *playoutboard=currentboard->copy();
    playoutboard->turnSymmetryOff();
    if (params->playout_features_enabled>0)
      playoutboard->setFeatures(features,params->playout_features_incremental,params->test_p8==0);
    if (params->csstyle_enabled) {
      if (params->playout_features_enabled)
        fprintf(stderr,"playout_features_enabled and csstyle_enabled can not be used together!!!!\n");
      playoutboard->updatePlayoutGammas(params, features);
    }
    playout->getPlayoutMove(threadpool->getThreadZero()->getSettings(),playoutboard,col,movetmp,NULL,NULL);
    if (!movetmp.isPass())
      playoutmove_triggered=false;
    delete playoutboard;
  }  
  DecisionTree::GraphCollection *graphs = NULL;

  if (WITH_P(params->features_output_competitions)&& playoutmove_triggered)
  {
    bool isawinner=true;
    Go::ObjectBoard<int> *cfglastdist=NULL;
    Go::ObjectBoard<int> *cfgsecondlastdist=NULL;
    features->computeCFGDist(currentboard,&cfglastdist,&cfgsecondlastdist);
    float *result=NULL;
    if (params->test_p100>0) {
      result= new float[currentboard->getSize()*currentboard->getSize()];
      getCNN(currentboard,0,move.getColor(),result);
    }
    if (graphs == NULL)
      graphs = new DecisionTree::GraphCollection(DecisionTree::getCollectionTypes(&decisiontrees),currentboard);
    
    if (params->features_output_competitions_mmstyle)
    {
      int p=move.getPosition();
      std::string featurestring=features->getMatchingFeaturesString(currentboard,cfglastdist,cfgsecondlastdist,graphs,move,!params->features_output_competitions_mmstyle,params->features_output_for_playout, result);
      if (featurestring.length()>0)
      {
        gtpe->getOutput()->printfDebug("[features]:# competition (%d,%s)\n",(currentboard->getMovesMade()+1),Go::Position::pos2string(move.getPosition(),boardsize).c_str());
        gtpe->getOutput()->printfDebug("[features]:%s*",Go::Position::pos2string(p,boardsize).c_str());
        gtpe->getOutput()->printfDebug("%s",featurestring.c_str());
        gtpe->getOutput()->printfDebug("\n");
      }
      else
      { //should be not used at the moment, as all strings get a feature value, was used for calculating not attached feature value before
      if (!params->features_output_for_playout)
        isawinner=false;
      else
        {
          gtpe->getOutput()->printfDebug("[features]:# competition (%d,%s)\n",(currentboard->getMovesMade()+1),"NN");
          gtpe->getOutput()->printfDebug("[features]:%s*","NN");
          gtpe->getOutput()->printfDebug("%s"," 0");
          gtpe->getOutput()->printfDebug("\n");
        }
      }
    }
    else
      gtpe->getOutput()->printfDebug("[features]:# competition (%d,%s)\n",(currentboard->getMovesMade()+1),Go::Position::pos2string(move.getPosition(),boardsize).c_str());
    
    if (isawinner)
    {
      Go::Color col=move.getColor();
      std::string notnearbymove="[features]:NN: 0\n";
      for (int p=0;p<currentboard->getPositionMax();p++)
      {
        Go::Move m=Go::Move(col,p);
        if (currentboard->validMove(m) || m==move)
        {
          std::string featurestring=features->getMatchingFeaturesString(currentboard,cfglastdist,cfgsecondlastdist,graphs,m,!params->features_output_competitions_mmstyle,params->features_output_for_playout, result);
          if (featurestring.length()>0)
          {
            gtpe->getOutput()->printfDebug("[features]:%s",Go::Position::pos2string(p,boardsize).c_str());
            if (m==move)
              gtpe->getOutput()->printfDebug("*");
            else
              gtpe->getOutput()->printfDebug(":");
            gtpe->getOutput()->printfDebug("%s",featurestring.c_str());
            gtpe->getOutput()->printfDebug("\n");
          }
          else if (params->features_output_for_playout && m==move)
          { //should be not used at the moment, as all strings get a feature value, was used for calculating not attached feature value before
            //not nearby move is winner
            notnearbymove="[features]:" + Go::Position::pos2string(p,boardsize) + "* 0\n";
          }
        }
      }
      if (params->features_output_for_playout)
        gtpe->getOutput()->printfDebug(notnearbymove);  //the not nearby move is added with mm id 0(not used otherwize)
      {
        Go::Move m=Go::Move(col,Go::Move::PASS);
        if (currentboard->validMove(m) || m==move)
        {
          gtpe->getOutput()->printfDebug("[features]:PASS");
          if (m==move)
            gtpe->getOutput()->printfDebug("*");
          else
            gtpe->getOutput()->printfDebug(":");
          gtpe->getOutput()->printfDebug("%s",features->getMatchingFeaturesString(currentboard,cfglastdist,cfgsecondlastdist,graphs,m,!params->features_output_competitions_mmstyle,params->features_output_for_playout, result).c_str());
          gtpe->getOutput()->printfDebug("\n");
        }
      }
    }
    
    if (cfglastdist!=NULL)
      delete cfglastdist;
    if (cfgsecondlastdist!=NULL)
      delete cfgsecondlastdist;
    if (result!=NULL)
      delete[] result;
  }

  bool did_CNN=false;
  if (WITH_P(params->CNN_data) && move.isNormal())
  {
    did_CNN=true;
    //output for the CNN training, testing
    //one line per Position, all included in the board part, additionally the move in readable form
    // a position 0: empty 1: black stone 2: white stone 3: black next move 4: white next move
    Go::Board * useboard=currentboard;
    //gtpe->getOutput()->printfDebug("test");
    if (params->CNN_data_predict_future>0) {
      useboard=historyboards[params->CNN_data_predict_future-1];
    //  gtpe->getOutput()->printfDebug("_in_");
    }
    if (useboard!=NULL) {  //CNN_data_playouts not compatible with historyboards at the moment 
      int size=useboard->getSize ();
      useboard->calcSlowLibertyGroups();
      for (int x=0;x<size;x++) {
        for (int y=0;y<size;y++) {
          Go::Color c=useboard->getColor(Go::Position::xy2pos(x,y,size));
          switch (c) {
            case Go::BLACK:
              gtpe->getOutput()->printfDebug("1,");
              break;
            case Go::WHITE:
              gtpe->getOutput()->printfDebug("2,");
              break;
            default:
              int p=move.getPosition();
              if (Go::Position::pos2x(p,size)==x && Go::Position::pos2y(p,size)==y)
              {
                gtpe->getOutput()->printfDebug("3,");
              }
              else
                gtpe->getOutput()->printfDebug("0,");
          }
        }
      }
      for (int x=0;x<size;x++) {
        for (int y=0;y<size;y++) {
          int pos=Go::Position::xy2pos(x,y,size);
          if (useboard->inGroup(pos)) {
            gtpe->getOutput()->printfDebug("%d,",useboard->getGroup(pos)->real_libs);
          }
          else
            gtpe->getOutput()->printfDebug("0,");
        
        }
      }
      for (int x=0;x<size;x++) {
        for (int y=0;y<size;y++) {
          int pos=Go::Position::xy2pos(x,y,size);
          if (useboard->inGroup(pos)) {
            gtpe->getOutput()->printfDebug("%d,",useboard->getGroup(pos)->numOfStones());
          }
          else
            gtpe->getOutput()->printfDebug("0,");
        }
      }
      int p1=move.getPosition();
      int p2=currentboard->getLastMove().getPosition();
      int p3=currentboard->getSecondLastMove().getPosition();
      int p4=currentboard->getThirdLastMove().getPosition();
      int p5=currentboard->getForthLastMove().getPosition();
      int p6=currentboard->getFifthLastMove().getPosition();
      int p7=currentboard->getSixLastMove().getPosition();
      int sko=useboard->getSimpleKo();
      
        gtpe->getOutput()->printfDebug("%s,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",move.toString(size).c_str(),
          Go::Position::pos2x(p1,size),Go::Position::pos2y(p1,size),
          Go::Position::pos2x(p2,size),Go::Position::pos2y(p2,size),
          Go::Position::pos2x(p3,size),Go::Position::pos2y(p3,size),
          Go::Position::pos2x(p4,size),Go::Position::pos2y(p4,size),
          Go::Position::pos2x(p5,size),Go::Position::pos2y(p5,size),
          Go::Position::pos2x(p6,size),Go::Position::pos2y(p6,size),
          Go::Position::pos2x(p7,size),Go::Position::pos2y(p7,size),
          Go::Position::pos2x(sko,size),Go::Position::pos2y(sko,size)
                                     );
      if (params->CNN_data_playouts==0) 
        gtpe->getOutput()->printfDebug("\n");
      
    /* //Test code to check, if the predictor does the same as the scripts
       
       float result[361];
      getCNN(useboard,move.getColor(),result);
      int dd=0;
      gtpe->getOutput()->printfDebug("\n#",result[dd]);
        
      for (int x=0;x<size;x++) {
        for (int y=0;y<size;y++) {
          gtpe->getOutput()->printfDebug("%5.2f",result[dd]);
          dd++;
        }
        gtpe->getOutput()->printfDebug("\n#");
      }
      gtpe->getOutput()->printfDebug("\n");
      */
    }
  }
  if (params->CNN_data_predict_future>0) {
      if (historyboards[params->CNN_data_predict_future-1]!=NULL)
        delete historyboards[params->CNN_data_predict_future-1];
      for (int i=params->CNN_data_predict_future-1;i>0;i--)
        historyboards[i]=historyboards[i-1];
      historyboards[0]=currentboard->copy();
    }
  if (WITH_P(params->features_circ_list))
  {
    Go::Color col=currentboard->nextToMove();
    
    for (int p=0;p<currentboard->getPositionMax();p++)
    {
      if (currentboard->validMove(Go::Move(col,p)))
      {
        Pattern::Circular pattcirc=Pattern::Circular(this->getCircDict(),currentboard,p,PATTERN_CIRC_MAXSIZE);
        if (col==Go::WHITE)
          pattcirc.invert();
        pattcirc.convertToSmallestEquivalent(this->getCircDict());
        if (params->features_circ_list_size==0)
        {
          for (int s=3;s<=PATTERN_CIRC_MAXSIZE;s++)
          {
            gtpe->getOutput()->printfDebug("%s",pattcirc.getSubPattern(this->getCircDict(),s).toString(this->getCircDict()).c_str());
            if (s==PATTERN_CIRC_MAXSIZE)
              gtpe->getOutput()->printfDebug("\n");
            else
              gtpe->getOutput()->printfDebug(" ");
          }
        }
        else
        {
          bool found = false;
          for (int s=PATTERN_CIRC_MAXSIZE;s>params->features_circ_list_size;s--)
          {
            Pattern::Circular pc = pattcirc.getSubPattern(this->getCircDict(),s);
            if (features->hasCircPattern(&pc))
            {
              found = true;
              break;
            }
          }
          if (!found)
          {
            Pattern::Circular pc = pattcirc.getSubPattern(this->getCircDict(),params->features_circ_list_size);
            gtpe->getOutput()->printfDebug("%s\n",pc.toString(this->getCircDict()).c_str());
          }
        }
      }
    }
    }

  if (params->dt_update_prob > 0)
  {
    if (graphs == NULL)
      graphs = new DecisionTree::GraphCollection(DecisionTree::getCollectionTypes(&decisiontrees),currentboard);

    for (std::list<DecisionTree*>::iterator iter=decisiontrees.begin();iter!=decisiontrees.end();++iter)
    {
      if (WITH_P(params->dt_update_prob))
        (*iter)->updateDescent(graphs,move);
    }
  }

  if (WITH_P(params->dt_output_mm))
  {
    if (graphs == NULL)
      graphs = new DecisionTree::GraphCollection(DecisionTree::getCollectionTypes(&decisiontrees),currentboard);

    if (move.isNormal())
    {
      std::list<int> *ids = DecisionTree::getCollectionLeafIds(&decisiontrees,graphs,move);
      if (ids != NULL)
      {
        gtpe->getOutput()->printfDebug("[dt]:#\n");
        std::string idstring = "";
        for (std::list<int>::iterator iter=ids->begin();iter!=ids->end();++iter)
        {
          idstring += (iter==ids->begin()?"":" ") + boost::lexical_cast<std::string>((*iter));
        }
        if (idstring.size() > 0)
          gtpe->getOutput()->printfDebug("[dt]:%s\n",idstring.c_str());
        delete ids;

        Go::Color col=move.getColor();
        for (int p=0;p<currentboard->getPositionMax();p++)
        {
          Go::Move m=Go::Move(col,p);
          if (currentboard->validMove(m) || m==move)
          {
            std::list<int> *ids = DecisionTree::getCollectionLeafIds(&decisiontrees,graphs,m);
            if (ids != NULL)
            {
              std::string idstring = "";
              for (std::list<int>::iterator iter=ids->begin();iter!=ids->end();++iter)
              {
                idstring += (iter==ids->begin()?"":" ") + boost::lexical_cast<std::string>((*iter));
              }
              if (idstring.size() > 0)
                gtpe->getOutput()->printfDebug("[dt]:%s\n",idstring.c_str());
              delete ids;
            }
          }
        }
      }
    }
  }
  
  if (params->features_ordered_comparison)
  {
    bool usedpos[currentboard->getPositionMax()+1];
    for (int i=0;i<=currentboard->getPositionMax();i++)
      usedpos[i]=false;
    int posused=0;
    float bestgamma=-1;
    int bestpos=0;
    Go::Color col=move.getColor();
    int matchedat=0;
    
    Go::ObjectBoard<int> *cfglastdist=NULL;
    Go::ObjectBoard<int> *cfgsecondlastdist=NULL;
    features->computeCFGDist(currentboard,&cfglastdist,&cfgsecondlastdist);

    float *result=NULL;
    float passresult=1;
    if (params->test_p100>0) {
      result= new float[currentboard->getSize()*currentboard->getSize()];
      getCNN(currentboard,0,move.getColor(),result);
      passresult=0.000001;
    }
    
    if (graphs == NULL)
      graphs = new DecisionTree::GraphCollection(DecisionTree::getCollectionTypes(&decisiontrees),currentboard);

    float weights[currentboard->getPositionMax()+1]; // +1 for pass
    float sumweights = 0;
    for (int p=0;p<currentboard->getPositionMax();p++)
    {
      Go::Move m = Go::Move(col,p);
      if (currentboard->validMove(m) || m==move)
      {
        float w = features->getMoveGamma(currentboard,cfglastdist,cfgsecondlastdist,graphs,m);
        weights[p] = w;
        if (w != -1)
          sumweights += w;
      }
      else
        weights[p] = -1;
    }
    { //pass from francois
      int p = currentboard->getPositionMax();
      Go::Move m = Go::Move(col,Go::Move::PASS);
      if (currentboard->validMove(m) || m==move)
      {
        float w = features->getMoveGamma(currentboard,cfglastdist,cfgsecondlastdist,graphs,m);
        weights[p] = w;
        if (w != -1)
          sumweights += w;
      }
    }
    if (params->test_p100>99999) { // only cnn usage
      for (int p=0;p<currentboard->getPositionMax();p++)
      {
        int x=Go::Position::pos2x(p, currentboard->getSize());
        int y=Go::Position::pos2y(p, currentboard->getSize());
        float r=result[x*currentboard->getSize()+y];
        if (r<0.00000001) r=0.00000001;
        if (weights[p]>-1) weights[p]=r;
      }
    }
    else if (params->test_p100>0) {
      for (int p=0;p<currentboard->getPositionMax();p++)
      {
        int x=Go::Position::pos2x(p, currentboard->getSize());
        int y=Go::Position::pos2y(p, currentboard->getSize());
        float r=result[x*currentboard->getSize()+y];
        if (r<0.00000001) r=0.00000001;
        if (weights[p]>-1) weights[p]*=r;
      }
    }
    gtpe->getOutput()->printfDebug("[feature_comparison]:# comparison (%d,%s)\n",(currentboard->getMovesMade()+1),Go::Position::pos2string(move.getPosition(),boardsize).c_str());
    
    gtpe->getOutput()->printfDebug("[feature_comparison]:");
    while (true)
    {
      for (int p=0;p<currentboard->getPositionMax();p++)
      {
        if (!usedpos[p])
        {
          Go::Move m=Go::Move(col,p);
          if (currentboard->validMove(m) || m==move)
          {
            //float gamma=features->getMoveGamma(currentboard,cfglastdist,cfgsecondlastdist,m);
            float gamma = weights[p];
            if (gamma>bestgamma)
            {
              bestgamma=gamma;
              bestpos=p;
            }
          }
        }
      }
      
      {
        int p=currentboard->getPositionMax();
        if (!usedpos[p])
        {
          Go::Move m=Go::Move(col,Go::Move::PASS);
          if (currentboard->validMove(m) || m==move)
          {
            float gamma=weights[p];
            gamma*=passresult;
            if (gamma>bestgamma)
            {
              bestgamma=gamma;
              bestpos=p;
            }
          }
        }
      }
      
      if (bestgamma!=-1)
      {
        Go::Move m;
        if (bestpos==currentboard->getPositionMax())
          m=Go::Move(col,Go::Move::PASS);
        else
          m=Go::Move(col,bestpos);
        posused++;
        usedpos[bestpos]=true;
        gtpe->getOutput()->printfDebug(" %s",Go::Position::pos2string(m.getPosition(),boardsize).c_str());
        if (m==move)
        {
          gtpe->getOutput()->printfDebug("*");
          matchedat=posused;
        }
      }
      else
        break;
      
      bestgamma=-1;
    }
    gtpe->getOutput()->printfDebug("\n");
    gtpe->getOutput()->printfDebug("[feature_comparison]:matched at: ");
    if (params->features_ordered_comparison_move_num)
      gtpe->getOutput()->printfDebug("%d ",currentboard->getMovesMade()+1);
    gtpe->getOutput()->printfDebug("%d",matchedat);
    if (params->features_ordered_comparison_log_evidence)
    {
      float w = -1;
      if (move.isNormal())
        w = weights[move.getPosition()];
      else if (move.isPass())
        w = weights[currentboard->getPositionMax()];
      float p = 1e-9; // prevent log(0)
      if (w != -1)
        p = w / sumweights;
      float le = log(p);
      gtpe->getOutput()->printfDebug(" %.2f",le);
    }
    gtpe->getOutput()->printfDebug("\n");
    
    if (cfglastdist!=NULL)
      delete cfglastdist;
    if (cfgsecondlastdist!=NULL)
      delete cfgsecondlastdist;
    if (result!=NULL)
      delete[] result;
  }

  if (params->dt_ordered_comparison)
  {
    bool usedpos[currentboard->getPositionMax()];
    for (int i=0;i<currentboard->getPositionMax();i++)
      usedpos[i] = false;
    int posused = 0;
    float bestweight = -1;
    int bestpos = 0;
    Go::Color col = move.getColor();
    int matchedat = 0;

    if (graphs == NULL)
      graphs = new DecisionTree::GraphCollection(DecisionTree::getCollectionTypes(&decisiontrees),currentboard);

    float weights[currentboard->getPositionMax()];
    for (int p=0;p<currentboard->getPositionMax();p++)
    {
      Go::Move m = Go::Move(col,p);
      if (currentboard->validMove(m) || m==move)
        weights[p] = DecisionTree::getCollectionWeight(&decisiontrees,graphs,m);
      else
        weights[p] = -1;
    }
    
    gtpe->getOutput()->printfDebug("[dt_comparison]:# comparison (%d,%s)\n",(currentboard->getMovesMade()+1),Go::Position::pos2string(move.getPosition(),boardsize).c_str());
    
    gtpe->getOutput()->printfDebug("[dt_comparison]:");
    while (true)
    {
      for (int p=0;p<currentboard->getPositionMax();p++)
      {
        if (!usedpos[p])
        {
          Go::Move m = Go::Move(col,p);
          if (currentboard->validMove(m) || m==move)
          {
            float weight = weights[p];
            if (weight > bestweight)
            {
              bestweight = weight;
              bestpos = p;
            }
          }
        }
      }
      
      if (bestweight!=-1)
      {
        Go::Move m = Go::Move(col,bestpos);
        posused++;
        usedpos[bestpos]=true;
        gtpe->getOutput()->printfDebug(" %s",Go::Position::pos2string(m.getPosition(),boardsize).c_str());
        if (m==move)
        {
          gtpe->getOutput()->printfDebug("*");
          matchedat = posused;
        }
      }
      else
        break;
      
      bestweight = -1;
    }
    gtpe->getOutput()->printfDebug("\n");
    gtpe->getOutput()->printfDebug("[dt_comparison]:matched at: %d\n",matchedat);
  }

  if (graphs != NULL)
    delete graphs;
  
  currentboard->makeMove(move);
  movehistory->push_back(move);
  moveexplanations->push_back("");
  Go::ZobristHash hash=currentboard->getZobristHash(zobristtable);
  if (move.isNormal() && hashtree->hasHash(hash))
    gtpe->getOutput()->printfDebug("WARNING! move is a superko violation\n");
  hashtree->addHash(hash);
  params->uct_slow_update_last=0;
  params->uct_slow_debug_last=0;
  territorymap->decay(params->territory_decayfactor);

  for (int i=0;i<currentboard->getPositionMax()*2;i++)
    area_correlation_map[i]->decay(params->territory_decayfactor);
  
  //was memory leak
  //blackOldMoves=new float[currentboard->getPositionMax()];
  //whiteOldMoves=new float[currentboard->getPositionMax()];
  for (int i=0;i<currentboard->getPositionMax();i++)
  {
    blackOldMoves[i]=0;
    whiteOldMoves[i]=0;
  }
  blackOldMean=0.5;
  whiteOldMean=0.5;
  blackOldMovesNum=0;
  whiteOldMovesNum=0;

  if (params->uct_keep_subtree)
    this->chooseSubTree(move);
  else
    this->clearMoveTree();
  #ifdef HAVE_MPI
    mpihashtable.clear();
  #endif

  isgamefinished=false;
  if (currentboard->getPassesPlayed()>=2 || move.isResign())
    this->gameFinished();
  if (did_CNN && params->CNN_data_playouts>0) {
    me->doNPlayouts(params->CNN_data_playouts);
    for (int x=0;x<me->boardsize;x++)
    {
      for (int y=0;y<me->boardsize;y++)
      {
        int pos=Go::Position::xy2pos(x,y,me->boardsize);
        float tmp=me->territorymap->getPositionOwner(pos);
        gtpe->getOutput()->printf(",%.2f",tmp);
      }
    }
    float ratio=me->movetree->getRobustChild()->getRatio();
    gtpe->getOutput()->printf(",%.3f,%.1f\n",ratio,komi);
  }
}

void Engine::setBoardSize(int s)
{
  if (s<BOARDSIZE_MIN || s>BOARDSIZE_MAX)
    return;
  
  #ifdef HAVE_MPI
    MPIRANK0_ONLY(
      unsigned int tmp=(unsigned int)s;
      this->mpiBroadcastCommand(MPICMD_SETBOARDSIZE,&tmp);
    );
  #endif
  
  boardsize=s;
  params->board_size=boardsize;
  this->clearBoard();
}

void Engine::setKomi(float k)
{
  #ifdef HAVE_MPI
    MPIRANK0_ONLY(
      unsigned int tmp=(unsigned int)k;
      this->mpiBroadcastCommand(MPICMD_SETKOMI,&tmp);
    );
  #endif
  komi=k;
}

void Engine::clearBoard()
{
  this->gameFinished();
  #ifdef HAVE_MPI
    MPIRANK0_ONLY(this->mpiBroadcastCommand(MPICMD_CLEARBOARD););
  #endif
  bool newsize=(zobristtable->getSize()!=boardsize);
  for (int i=0;i<currentboard->getPositionMax()*2;i++)
  {
    delete area_correlation_map[i];
  }
  delete[] area_correlation_map;
  delete currentboard;
  delete movehistory;
  delete moveexplanations;
  delete hashtree;
  delete territorymap;
  delete probabilitymap;
  delete correlationmap;
  delete respondboard;
  delete[] blackOldMoves;
  delete[] whiteOldMoves;
  
  if (newsize)
    delete zobristtable;
  currentboard = new Go::Board(boardsize);
  for (int i=0;i<historyboards_num;i++)
  {
    if (historyboards[i]!=NULL) delete historyboards[i];
    historyboards[i]=NULL;
  }
  movehistory = new std::list<Go::Move>();
  moveexplanations = new std::list<std::string>();
  hashtree=new Go::ZobristTree();
  territorymap=new Go::TerritoryMap(boardsize);
  area_correlation_map=new Go::TerritoryMap*[currentboard->getPositionMax()*2];
#warning "memory allocated for area_correlation_map"
  for (int i=0;i<currentboard->getPositionMax()*2;i++)
  {
    area_correlation_map[i]=new Go::TerritoryMap(boardsize);
  }

  probabilitymap=new Go::MoveProbabilityMap (boardsize);
  correlationmap=new Go::ObjectBoard<Go::CorrelationData>(boardsize);
  respondboard=new Go::RespondBoard(boardsize);
  blackOldMoves=new float[currentboard->getPositionMax()];
  whiteOldMoves=new float[currentboard->getPositionMax()];
  for (int i=0;i<currentboard->getPositionMax();i++)
  {
    blackOldMoves[i]=0;
    whiteOldMoves[i]=0;
  }
  blackOldMean=0.5;
  whiteOldMean=0.5;
  blackOldMovesNum=0;
  whiteOldMovesNum=0;

  if (newsize)
    zobristtable=new Go::ZobristTable(params,boardsize,ZOBRIST_HASH_SEED);
  if (!params->uct_symmetry_use)
    currentboard->turnSymmetryOff();
  this->clearMoveTree();
  params->surewin_expected=false;
  playout->resetLGRF();
  params->cleanup_in_progress=false;
  isgamefinished=false;
  komi_handicap=0;
  recalc_dynkomi=0;
//  if (deltagammas!=NULL) delete[]deltagammas;
  if (deltagammaslocal!=NULL) delete[]deltagammaslocal;
  deltawhiteoffset=boardsize*boardsize*(local_feature_num+hashto5num);
//  deltagammas = new std::atomic<float>[2*boardsize*boardsize*(local_feature_num+hashto5num)];
//  for (int i=0;i<2*boardsize*boardsize*(local_feature_num+hashto5num);i++) deltagammas[i]=1.0;
  deltagammaslocal = new float[2*boardsize*boardsize*(local_feature_num+hashto5num)];
  for (int i=0;i<2*boardsize*boardsize*(local_feature_num+hashto5num);i++) deltagammaslocal[i]=1.0;
}

void Engine::clearMoveTree(int a_pos)
{
  #ifdef HAVE_MPI
    MPIRANK0_ONLY(this->mpiBroadcastCommand(MPICMD_CLEARTREE););
  #endif
  
  if (movetree!=NULL)
    delete movetree;
  if (a_pos<0) {
    if (currentboard->getMovesMade()>0)
      movetree=new Tree(params,currentboard->getZobristHash(zobristtable),currentboard->getLastMove());
    else
      movetree=new Tree(params,0);
  }
  else {
    if (currentboard->getMovesMade()>0)
      movetree=new Tree(params,currentboard->getZobristHash(zobristtable),currentboard->getLastMove(),NULL,a_pos);
    else
      movetree=new Tree(params,0,Go::Move(Go::EMPTY,Go::Move::RESIGN),NULL,a_pos);
  }
  
  params->uct_slow_update_last=0;
  params->uct_slow_debug_last=0;
  params->tree_instances=0; // reset as lock free implementation could be slightly off
}

bool Engine::undo()
{
  if (currentboard->getMovesMade()<=0)
    return false;

  std::list<Go::Move> oldhistory = *movehistory;
  oldhistory.pop_back();
  this->clearBoard();

  for(std::list<Go::Move>::iterator iter=oldhistory.begin();iter!=oldhistory.end();++iter)
  {
    this->makeMove((*iter));
  }

  return true;
}

void Engine::chooseSubTree(Go::Move move)
{
  Tree *subtree=movetree->getChild(move);
  
  if (subtree==NULL)
  {
    //gtpe->getOutput()->printfDebug("no such subtree...\n");
    this->clearMoveTree();
    return;
  }
  
  if (!subtree->isPrimary())
  {
    //gtpe->getOutput()->printfDebug("doing transformation...\n");
    subtree->performSymmetryTransformParentPrimary();
    subtree=movetree->getChild(move);
    if (subtree==NULL || !subtree->isPrimary())
      gtpe->getOutput()->printfDebug("WARNING! symmetry transformation failed! (null:%d)\n",(subtree==NULL));
  }
  
  if (subtree==NULL) // only true if a symmetry transform failed
  {
    gtpe->getOutput()->printfDebug("WARNING! clearing tree...\n");
    this->clearMoveTree();
    return;
  }

// fprintf(stderr,"before devorceChild\n");
  movetree->divorceChild(subtree);

  //keep the childrens values
  std::list<Tree*>* childtmp=movetree->getChildren();
  float sum=0;
  int num=0;
  std::list<Tree*>::iterator iter=childtmp->begin();
  Go::Color col=((*iter)->getMove()).getColor();
  for (int i=0;i<currentboard->getPositionMax();i++)
  {
    if (col==Go::BLACK)
      blackOldMoves[i]=0;
    else
      whiteOldMoves[i]=0;
  }
  for(iter=childtmp->begin();iter!=childtmp->end();++iter) 
    {
      if (!(*iter)->isPruned() && ((*iter)->getMove()).isNormal())
      {
        num++;
        sum+=(*iter)->getRatio();
        if (col==Go::BLACK)
        {
          blackOldMoves[((*iter)->getMove()).getPosition()]=(*iter)->getRatio();
          //fprintf(stderr,"blackOldMoves %s %f (%f) ((%f))\n",((*iter)->getMove()).toString(boardsize).c_str(),(*iter)->getRatio(),(*iter)->getUrgency(),(*iter)->getRatio()-(*iter)->getUrgency());
         }
        else
        {
          whiteOldMoves[((*iter)->getMove()).getPosition()]=(*iter)->getRatio();
          //fprintf(stderr,"whiteOldMoves %s %f (%f) ((%f))\n",((*iter)->getMove()).toString(boardsize).c_str(),(*iter)->getRatio(),(*iter)->getUrgency(),(*iter)->getRatio()-(*iter)->getUrgency());
        }
      }
    }
  if (col==Go::BLACK && num>0)
  {
    blackOldMean=sum/num;
    blackOldMovesNum=movetree->getPlayouts();
    //fprintf(stderr,"blackOldMean %f\n",blackOldMean);
  }
  if (col==Go::WHITE && num>0)
  {
    whiteOldMean=sum/num;
    whiteOldMovesNum=movetree->getPlayouts();
    //fprintf(stderr,"whiteOldMean %f\n",whiteOldMean);
  }
  delete movetree;
  movetree=subtree;
  movetree->pruneSuperkoViolations();
}

bool Engine::writeSGF(std::string filename, Go::Board *board, Tree *tree)
{
  std::ofstream sgffile;
  sgffile.open(filename.c_str());
  sgffile<<"(;\nFF[4]SZ["<<boardsize<<"]KM["<<komi<<"]\n";
  if (board==NULL)
    board=currentboard;
  sgffile<<board->toSGFString()<<"\n";
  if (tree==NULL)
    tree=movetree;
  sgffile<<tree->toSGFString()<<"\n)";
  sgffile.close();
  
  return true;
}

bool Engine::writeSGF(std::string filename, Go::Board *board, std::list<Go::Move> playoutmoves, std::list<std::string> *movereasons)
{
  std::ofstream sgffile;
  sgffile.open(filename.c_str());
  sgffile<<"(;\nFF[4]SZ["<<boardsize<<"]KM["<<komi<<"]\n";
  if (board==NULL)
    board=currentboard;
  sgffile<<board->toSGFString()<<"\n";

  std::list<std::string>::iterator reasoniter;
  if (movereasons!=NULL)
    reasoniter=movereasons->begin();
  for(std::list<Go::Move>::iterator iter=playoutmoves.begin();iter!=playoutmoves.end();++iter)
  {
    //sgffile<<tree->toSGFString()<<"\n)";
    sgffile<<";"<<Go::colorToChar((*iter).getColor())<<"[";
    if (!(*iter).isPass()&&!(*iter).isResign())
    {
      sgffile<<(char)((*iter).getX(params->board_size)+'a');
      sgffile<<(char)(params->board_size-(*iter).getY(params->board_size)+'a'-1);
    }
    else if ((*iter).isPass())
      sgffile<<"pass";
    sgffile<<"]";
    if (movereasons!=NULL)
      sgffile<<"C["<<(*reasoniter)<<"]";
    sgffile<<"\n";
    if (movereasons!=NULL)
    {
      ++reasoniter;
      if (reasoniter==movereasons->end())
        break;
    }
  }
  sgffile <<")\n";
  sgffile.close();
  return true;
}

bool Engine::writeGameSGF(std::string filename)
{
  std::ofstream sgffile;
  sgffile.open(filename.c_str());
  sgffile<<"(;\nFF[4]SZ["<<boardsize<<"]KM["<<komi<<"]C["<<VERSION<<params->version_config_file<<"]\n";

  std::list<std::string>::iterator expiter = moveexplanations->begin();
  for(std::list<Go::Move>::iterator iter=movehistory->begin();iter!=movehistory->end();++iter)
  {
    sgffile<<";"<<Go::colorToChar((*iter).getColor())<<"[";
    if (!(*iter).isPass()&&!(*iter).isResign())
    {
      sgffile<<(char)((*iter).getX(params->board_size)+'a');
      sgffile<<(char)(params->board_size-(*iter).getY(params->board_size)+'a'-1);
    }
    else if ((*iter).isPass())
      sgffile<<"pass";
    sgffile<<"]C["<<(*expiter)<<"]\n";

    ++expiter;
    if (expiter==moveexplanations->end())
      break;
  }
  sgffile <<")\n";
  sgffile.close();
  return true;
}

void Engine::doNPlayouts(int n)
{
  //gtpe->getOutput()->printfDebug("dddd1\n");
  if (params->move_policy==Parameters::MP_UCT || params->move_policy==Parameters::MP_ONEPLY)
  {
    stopthinking=false;
    
    this->allowContinuedPlay();
    
    int oldplts=params->playouts_per_move;
    params->playouts_per_move=n;
    
    params->uct_initial_playouts=(int)movetree->getPlayouts();
    params->thread_job=Parameters::TJ_DONPLTS;
    threadpool->startAll();
    threadpool->waitAll();
    if (movetree->isTerminalResult())
      gtpe->getOutput()->printfDebug("SOLVED! found 100%% sure result after %d plts!\n",(int)movetree->getPlayouts()-params->uct_initial_playouts);
    
    params->playouts_per_move=oldplts;
    
    if (params->livegfx_on)
      gtpe->getOutput()->printfDebug("gogui-gfx: CLEAR\n");
  }
}

void Engine::doPlayout(Worker::Settings *settings, Go::IntBoard *firstlist, Go::IntBoard *secondlist, Go::IntBoard *earlyfirstlist, Go::IntBoard *earlysecondlist, float *score_stats)
{
  //bool givenfirstlist,givensecondlist;
  Go::Color col=currentboard->nextToMove();

  if (movetree->isLeaf())
  {
    this->allowContinuedPlay();
    movetree->expandLeaf(settings,0);
    movetree->pruneSuperkoViolations();
  }
  
  //givenfirstlist=(firstlist==NULL);
  //givensecondlist=(secondlist==NULL);
  
  Tree *playouttree = movetree->getUrgentChild(settings);
  if (playouttree==NULL)
  {
    if (params->debug_on)
      gtpe->getOutput()->printfDebug("WARNING! No playout target found.\n");
    return;
  }
  if (playouttree->isPruned()) {
    fprintf(stderr,"playouttree is pruned\n");
  }
  std::list<Go::Move> playoutmoves=playouttree->getMovesFromRoot();
  std::list<Go::Move> playoutmoves_only_tree;
  if (params->uct_area_correlation_statistics)
    playoutmoves_only_tree=playouttree->getMovesFromRoot();
  
  if (playoutmoves.size()==0)
  {
    if (params->debug_on)
      gtpe->getOutput()->printfDebug("WARNING! Bad playout target found.\n");
    return;
  }
  
  //if (!givenfirstlist)
  //  firstlist=new Go::BitBoard(boardsize);
  //if (!givensecondlist)
  //  secondlist=new Go::BitBoard(boardsize);
  
  Go::Board *playoutboard=currentboard->copy();
  //Go::Board *playoutboard=pool_board.construct(currentboard->getSize());
  //currentboard->copyOver(playoutboard);
  playoutboard->komi_grouptesting=this->komi;
  playoutboard->turnSymmetryOff();
  if (debug_solid_group>=0 && playoutboard->inGroup(debug_solid_group)) {
    playoutboard->hasSolidGroups=true;
    Go::Group *thegroup=playoutboard->getGroup(debug_solid_group);
    thegroup->setSolid ();
  }
  if (params->playout_features_enabled>0)
    playoutboard->setFeatures(features,params->playout_features_incremental,params->test_p8==0);
  if (params->csstyle_enabled) {
    if (params->playout_features_enabled)
      fprintf(stderr,"playout_features_enabled and csstyle_enabled can not be used together!!!!\n");
    playoutboard->updatePlayoutGammas(params, features);
  }
  if (params->rave_moves>0)
  {
    firstlist->clear();
    secondlist->clear();
    earlyfirstlist->clear();
    earlysecondlist->clear();
  }
  Go::Color playoutcol=playoutmoves.back().getColor();

  //for(std::list<Go::Move>::iterator iter=playoutmoves.begin();iter!=playoutmoves.end();++iter)
  //{
  //    fprintf(stderr,"%s ",(*iter).toString(boardsize).c_str());
  //}
  //fprintf(stderr,"-\n");
  float finalscore;
  float cnn_winrate=-1;
  if (playouttree->isPruned()) {
    fprintf(stderr,"playouttree is pruned1\n");
  }
  playout->doPlayout(settings,playoutboard,finalscore,cnn_winrate,playouttree,playoutmoves,col,(params->rave_moves>0?firstlist:NULL),(params->rave_moves>0?secondlist:NULL),(params->rave_moves>0?earlyfirstlist:NULL),(params->rave_moves>0?earlysecondlist:NULL));
  if (this->getTreeMemoryUsage()>(params->memory_usage_max*1024*1024) && !stopthinking)
  {
      gtpe->getOutput()->printfDebug("WARNING! Memory limit reached! Stopping search right now!\n");
      this->stopThinking();
  }
  if (cnn_winrate>-1) {
    //params->test_p105/10.0 is added in playout.cc
    float mwr=params->test_p105;
    if (playoutcol!=Go::BLACK)
      cnn_winrate=1.0-cnn_winrate;
    playouttree->addPartialResult((params->test_p106*mwr)*cnn_winrate,params->test_p106*mwr,false,1.0-params->test_p107);
    //fprintf(stderr,"playouttreecol %s playoutcol %s winrate %.3f playouts %f wins %f\n",(playouttree->getMove().getColor()==Go::BLACK)?"black":"white",(playoutcol==Go::BLACK)?"black":"white",cnn_winrate,playouttree->getPlayouts(),playouttree->getWins());

    //this can be uncommented, to avoid a usual playout every CNN
    //delete playoutboard;
    //return;
    //this does a playout after a cnn territory
    //playoutboard=currentboard->copy();
    //playoutboard->turnSymmetryOff();
    //if (params->rave_moves>0)
    //{
    //  firstlist->clear();
    //  secondlist->clear();
    //  earlyfirstlist->clear();
    //  earlysecondlist->clear();
    //}
    //cnn_winrate=-2;
    //playout->doPlayout(settings,playoutboard,finalscore,cnn_winrate,playouttree,playoutmoves,col,(params->rave_moves>0?firstlist:NULL),(params->rave_moves>0?secondlist:NULL),(params->rave_moves>0?earlyfirstlist:NULL),(params->rave_moves>0?earlysecondlist:NULL));
  }
  
  if (!params->rules_all_stones_alive && !params->cleanup_in_progress && playoutboard->getPassesPlayed()>=2 && (playoutboard->getMovesMade()-currentboard->getMovesMade())<=2)
  {
    finalscore=playoutboard->territoryScore(territorymap,params->territory_threshold)-params->engine->getHandiKomi();
  }

  if (playouttree->isPruned()) {
    fprintf(stderr,"playouttree is pruned2\n");
  }
  
  bool playoutwin=Go::Board::isWinForColor(playoutcol,finalscore);
  bool playoutjigo=(finalscore==0);
  if (playoutjigo) {
    if (!playouttree->isPruned())  //this is the case in superko violation 
      playouttree->addPartialResult(0.5,1,false);
  }
  else if (playoutwin)
    playouttree->addWin(finalscore);
  else
    playouttree->addLose(finalscore);
  
  playoutboard->updateTerritoryMap(territorymap);
  if (params->uct_area_correlation_statistics)
  {
    //do not count as tree move, if not at least 2 more moves in the tree
    if (!playoutmoves_only_tree.empty())
      playoutmoves_only_tree.pop_back();
    if (!playoutmoves_only_tree.empty())
      playoutmoves_only_tree.pop_back();
    
    for(std::list<Go::Move>::iterator iter=playoutmoves_only_tree.begin();iter!=playoutmoves_only_tree.end();++iter)
      {
        if (iter->getPosition()>=0)
          playoutboard->updateTerritoryMap(area_correlation_map[iter->getPosition()+(iter->getColor()==Go::BLACK?playoutboard->getPositionMax():0)]);
      }
  }
  
  //here with with firstlist and secondlist the correlationmap can be updated
  if (col==Go::BLACK)
    playoutboard->updateCorrelationMap(correlationmap,firstlist,secondlist);
  else
    playoutboard->updateCorrelationMap(correlationmap,secondlist,firstlist);

  if (!playoutjigo)
  {
    Go::Color wincol=(finalscore>0?Go::BLACK:Go::WHITE);
    playouttree->updateCriticality(playoutboard,wincol);
  }
  
  if (!playouttree->isTerminalResult())
  {
    if (params->uct_points_bonus!=0)
    {
      float scorediff=(playoutcol==Go::BLACK?1:-1)*finalscore;
      //float bonus=params->uct_points_bonus*scorediff;
      float bonus;
      if (scorediff>0)
        bonus=params->uct_points_bonus*log(scorediff+1);
      else
        bonus=-params->uct_points_bonus*log(-scorediff+1);
      playouttree->addPartialResult(bonus,0);
      //fprintf(stderr,"[points_bonus]: %+6.1f %+f\n",scorediff,bonus);
    }
    if (params->uct_length_bonus!=0)
    {
      int moves=playoutboard->getMovesMade();
      float bonus=(playoutwin?1:-1)*params->uct_length_bonus*log(moves);
      playouttree->addPartialResult(bonus,0);
      //fprintf(stderr,"[length_bonus]: %6d %+f\n",moves,bonus);
    }
  }
  
  if (params->debug_on)
  {
    if (finalscore==0)
      gtpe->getOutput()->printfDebug("[result]:jigo\n");
    else if (playoutwin && playoutcol==col)
      gtpe->getOutput()->printfDebug("[result]:win (fs:%+.1f)\n",finalscore);
    else
      gtpe->getOutput()->printfDebug("[result]:lose (fs:%+.1f)\n",finalscore);
  }
  
  if (params->rave_moves>0)
  {
    //this can not be ignored for the critarray structures
    //if (!playoutjigo) // ignore jigos for RAVE
    {
      bool blackwin=Go::Board::isWinForColor(Go::BLACK,finalscore);
      Go::Color wincol=(blackwin?Go::BLACK:Go::WHITE);
      
      if (col==Go::BLACK)
        playouttree->updateRAVE(wincol,firstlist,secondlist,false,playoutboard);
      else
        playouttree->updateRAVE(wincol,secondlist,firstlist,false,playoutboard);
      if (params->uct_earlyrave_unprune_factor>0)
      {
        if (col==Go::BLACK)
          playouttree->updateRAVE(wincol,earlyfirstlist,earlysecondlist,true,NULL);
        else
          playouttree->updateRAVE(wincol,earlysecondlist,earlyfirstlist,true,NULL);
      }
    }
  }
  
  if (params->uct_virtual_loss)
    playouttree->removeVirtualLoss();
  
  if (settings->thread->getID()==0)
  {
    params->uct_slow_update_last++;
    if (params->uct_slow_update_last>=params->uct_slow_update_interval)
    {
      params->uct_slow_update_last=0;
      
      this->doSlowUpdate();
    }
    if (params->uct_slow_debug_interval>0)
    {
      params->uct_slow_debug_last++;
      if (params->uct_slow_debug_last>=params->uct_slow_debug_interval)
      {
        params->uct_slow_debug_last=0;
        
        if (!movetree->isLeaf())
        {
          std::ostringstream ss;
          ss << std::fixed;
#ifdef HAVE_MPI
          if (mpiworldsize>1)
            ss << "(mpi "  << mpirank << ") ";   
#endif
          ss << "[dbg|" << std::setprecision(0)<<movetree->getPlayouts() << "]";
          Tree *robustmove=movetree->getRobustChild();
          ss << " (rm:" << Go::Position::pos2string(robustmove->getMove().getPosition(),boardsize);
          ss << " r:" << std::setprecision(2)<<robustmove->getRatio();
          if (robustmove->getCNNvalue()>-1)
            ss << " v:"<<std::setprecision(3)<<robustmove->getCNNvalue();
          ss << " r2:" << std::setprecision(2)<<robustmove->secondBestPlayoutRatio();
          ss << " u:" << std::setprecision(2)<<robustmove->getUrgency();
          ss << " uv:" << std::setprecision(3)<<robustmove->getUrgencyVariance();
          ss << " p:" << std::setprecision(2)<<robustmove->getPlayouts();
          ss << " tw:" << robustmove->isTerminalWin();
          ss << ")";
          Tree *bestratio=movetree->getBestRatioChild(10);
          if (bestratio!=NULL)
          {
            if (robustmove==bestratio)
              ss << " (same)";
            else
            {
              ss << " (br:" << Go::Position::pos2string(bestratio->getMove().getPosition(),boardsize);
              ss << " r:" << std::setprecision(2)<<bestratio->getRatio();
              if (bestratio->getCNNvalue()>-1)
                ss << " v:"<<std::setprecision(3)<<bestratio->getCNNvalue();
              ss << " u:" << std::setprecision(2)<<bestratio->getUrgency();
              ss << " p:" << std::setprecision(2)<<bestratio->getPlayouts();
              ss << " tw:" << bestratio->isTerminalWin();
              ss << ")";
            }
          }
          Tree *bestcrit=movetree->getBestUrgencyChild(10);
          if (bestcrit!=NULL)
          {
            if (robustmove==bestcrit)
              ss << " (same)";
            else
            {
              ss << " (bu:" << Go::Position::pos2string(bestcrit->getMove().getPosition(),boardsize);
              ss << " r:" << std::setprecision(2)<<bestcrit->getRatio();
              if (bestcrit->getCNNvalue()>-1)
                ss << " v:"<<std::setprecision(3)<<bestcrit->getCNNvalue();
              ss << " u:" << std::setprecision(2)<<bestcrit->getUrgency();
              ss << " p:" << std::setprecision(2)<<bestcrit->getPlayouts();
              ss << " tw:" << bestcrit->isTerminalWin();
              ss << ")";
            }
          }
          ss << "\n";
          gtpe->getOutput()->printfDebug(ss.str());
        }
      }
    }
  }

  delete playoutboard;
  //pool_board.destroy(playoutboard);
  
  //if (!givenfirstlist)
  //  delete firstlist;
  //if (!givensecondlist)
  //  delete secondlist;
  //gtpe->getOutput()->printfDebug("dddd33a\n");
  if (score_stats!=NULL) {
    //gtpe->getOutput()->printfDebug("dddd33b\n");
    int score=(int)(finalscore+params->engine->getScoreKomi()+(float)boardsize*boardsize/2);
    if (score<0) score=0;
    if (score>=boardsize*boardsize) score=boardsize*boardsize-1;
    score_stats[score]+=1;
  }
}

void Engine::displayPlayoutLiveGfx(int totalplayouts, bool livegfx)
{
  Go::Color col=currentboard->nextToMove();
  
  if (livegfx)
    gtpe->getOutput()->printfDebug("gogui-gfx:\n");
  if (totalplayouts!=-1)
    gtpe->getOutput()->printfDebug("TEXT [genmove]: thinking... playouts:%d\n",totalplayouts);
  
  if (livegfx)
    gtpe->getOutput()->printfDebug("INFLUENCE");
  else
    gtpe->getOutput()->printf("INFLUENCE");
  int maxplayouts=1; //prevent div by zero
  for(std::list<Tree*>::iterator iter=movetree->getChildren()->begin();iter!=movetree->getChildren()->end();++iter) 
  {
    if ((*iter)->getPlayouts()>maxplayouts)
      maxplayouts=(int)(*iter)->getPlayouts();
  }
  float colorfactor=(col==Go::BLACK?1:-1);
  for(std::list<Tree*>::iterator iter=movetree->getChildren()->begin();iter!=movetree->getChildren()->end();++iter) 
  {
    if (!(*iter)->getMove().isPass() && !(*iter)->getMove().isResign())
    {
      Gtp::Vertex vert={(*iter)->getMove().getX(boardsize),(*iter)->getMove().getY(boardsize)};
      float playoutpercentage=(float)(*iter)->getPlayouts()/maxplayouts;
      if (playoutpercentage>1)
        playoutpercentage=1;
      
      if (livegfx)
      {
        gtpe->getOutput()->printfDebug(" ");
        gtpe->getOutput()->printDebugVertex(vert);
        gtpe->getOutput()->printfDebug(" %.2f",playoutpercentage*colorfactor);
      }
      else
      {
        gtpe->getOutput()->printf(" ");
        gtpe->getOutput()->printVertex(vert);
        gtpe->getOutput()->printf(" %.2f",playoutpercentage*colorfactor);
      }
    }
  }
  if (livegfx)
    gtpe->getOutput()->printfDebug("\n");
  else
    gtpe->getOutput()->printf("\n");
  if (params->move_policy==Parameters::MP_UCT)
  {
    if (livegfx)
      gtpe->getOutput()->printfDebug("VAR");
    else
      gtpe->getOutput()->printf("VAR");
    Tree *besttree=movetree->getRobustChild(true);
    if (besttree!=NULL)
    {
      std::list<Go::Move> bestmoves=besttree->getMovesFromRoot();
      for(std::list<Go::Move>::iterator iter=bestmoves.begin();iter!=bestmoves.end();++iter) 
      {
        if (!(*iter).isPass() && !(*iter).isResign())
        {
          Gtp::Vertex vert={(*iter).getX(boardsize),(*iter).getY(boardsize)};
          if (livegfx)
          {
            gtpe->getOutput()->printfDebug(" %c ",((*iter).getColor()==Go::BLACK?'B':'W'));
            gtpe->getOutput()->printDebugVertex(vert);
          }
          else
          {
            gtpe->getOutput()->printf(" %c ",((*iter).getColor()==Go::BLACK?'B':'W'));
            gtpe->getOutput()->printVertex(vert);
          }
        }
        else if ((*iter).isPass())
        {
          if (livegfx)
            gtpe->getOutput()->printfDebug(" %c PASS",((*iter).getColor()==Go::BLACK?'B':'W'));
          else
            gtpe->getOutput()->printf(" %c PASS",((*iter).getColor()==Go::BLACK?'B':'W'));
        }
      }
    }
  }
  else
  {
    if (livegfx)
      gtpe->getOutput()->printfDebug("SQUARE");
    else
      gtpe->getOutput()->printf("SQUARE");
    for(std::list<Tree*>::iterator iter=movetree->getChildren()->begin();iter!=movetree->getChildren()->end();++iter) 
    {
      if (!(*iter)->getMove().isPass() && !(*iter)->getMove().isResign())
      {
        if ((*iter)->getPlayouts()==maxplayouts)
        {
          Gtp::Vertex vert={(*iter)->getMove().getX(boardsize),(*iter)->getMove().getY(boardsize)};
          if (livegfx)
          {
            gtpe->getOutput()->printfDebug(" ");
            gtpe->getOutput()->printDebugVertex(vert);
          }
          else
          {
            gtpe->getOutput()->printf(" ");
            gtpe->getOutput()->printVertex(vert);
          }
        }
      }
    }
  }
  if (livegfx)
    gtpe->getOutput()->printfDebug("\n\n");
}

void Engine::allowContinuedPlay()
{
  if (currentboard->getPassesPlayed()>=2)
  {
    currentboard->resetPassesPlayed();
    movetree->allowContinuedPlay();
    gtpe->getOutput()->printfDebug("WARNING! continuing play from a terminal position\n");
  }
}

void Engine::ponder()
{
  if (!(params->pondering_enabled) || (currentboard->getMovesMade()<=0) || (currentboard->getPassesPlayed()>=2) || (currentboard->getLastMove().isResign()) || (book->getMoves(boardsize,movehistory).size()>0))
    return;
  
  if (params->move_policy==Parameters::MP_UCT || params->move_policy==Parameters::MP_ONEPLY)
  {
    if (this->getTreeMemoryUsage()>(params->memory_usage_max*1024*1024))
      return;

   // fprintf(stderr,"pondering starting!\n");
    #ifdef HAVE_MPI
      MPIRANK0_ONLY(
        unsigned int tmp1=0;// not used (unsigned int)col;
        this->mpiBroadcastCommand(MPICMD_PONDER,&tmp1);
      );

    //must sync all are started, otherwize stopping before starting possible!!!!!
    
    #endif
    this->allowContinuedPlay();
    params->uct_slow_update_last=0;
    stopthinking=false;
    #ifdef HAVE_MPI
      params->mpi_last_update=MPI::Wtime();
    #endif
    
    params->uct_initial_playouts=(int)movetree->getPlayouts();
    params->thread_job=Parameters::TJ_PONDER;
    #ifdef HAVE_MPI
    //isWaitingForStop=false;
    #endif    
    threadpool->startAll();
    threadpool->waitAll();
    #ifdef HAVE_MPI
    //isWaitingForStop=true;
    mpiSyncWaitStop();
    #endif    
    if (movetree->isTerminalResult())
      gtpe->getOutput()->printfDebug("SOLVED! found 100%% sure result after %d plts!\n",(int)movetree->getPlayouts()-params->uct_initial_playouts);
    //gtpe->getOutput()->printfDebug("pondering done! after all threads %.0f\n",movetree->getPlayouts());
  }
}

void Engine::ponderThread(Worker::Settings *settings)
{
  //stoppondering=false;
  //stopthinking=false;
  if (!(params->pondering_enabled) || (currentboard->getMovesMade()<=0) || (currentboard->getPassesPlayed()>=2) || (currentboard->getLastMove().isResign()) || (book->getMoves(boardsize,movehistory).size()>0))
    return;
  
  if (params->move_policy==Parameters::MP_UCT || params->move_policy==Parameters::MP_ONEPLY)
  {
    //fprintf(stderr,"pondering thread starting! %d rank %d stoppondering %d stopthinking %d\n",settings->thread->getID(),mpirank,stoppondering,stopthinking);
    #ifdef HAVE_MPI
      bool mpi_inform_others=true;
      bool mpi_rank_other=(mpirank!=0);
      //int mpi_update_num=0;
    #else
//      bool mpi_rank_other=false;
    #endif
    this->allowContinuedPlay();
    params->uct_slow_update_last=0;
    stopthinking=false;
    
    Go::IntBoard *firstlist=new Go::IntBoard(boardsize);
    Go::IntBoard *secondlist=new Go::IntBoard(boardsize);
    Go::IntBoard *earlyfirstlist=new Go::IntBoard(boardsize);
    Go::IntBoard *earlysecondlist=new Go::IntBoard(boardsize);

    long playouts=0;
//    bool initial_sync=true;

    //there might be a race condition left if stoppondering and stopthinking is changed before mpiSyncUpdate ()
    if (settings->thread->getID()==0)
      stop_called=false; //only this thread is allowed to handle mpi calls
    while (!stoppondering && !stopthinking && (playouts=(long)movetree->getPlayouts())<(params->pondering_playouts_max))
    {
      //if (movetree->isTerminalResult())
      //{
      //  stopthinking=true;
      //  break;
      //}
      
      params->uct_slow_debug_last=0; // don't print out debug info when pondering
      this->doPlayout(settings,firstlist,secondlist,earlyfirstlist,earlysecondlist);
      playouts++;
      #ifdef HAVE_MPI
      if (settings->thread->getID()==0 && mpiworldsize>1 && (MPI::Wtime()>(params->mpi_last_update+params->mpi_update_period) || initial_sync))
      {
        initial_sync=false;
        //mpi_update_num++;
        //gtpe->getOutput()->printfDebug("update (%d) at %lf (rank: %d) start\n",mpi_update_num,MPI::Wtime(),mpirank);
        
        mpi_inform_others=this->mpiSyncUpdate();
        
        params->mpi_last_update=MPI::Wtime();
        
        if (!mpi_inform_others)
        {
          params->early_stop_occured=true;
          break;
        }
      }
      #endif
    }

      
    delete firstlist;
    delete secondlist;
    delete earlyfirstlist;
    delete earlysecondlist;

    //fprintf(stderr,"pondering done! %ld %.0f stopthinking %d stoppondering %d playouts %ld\n",playouts,movetree->getPlayouts(),stopthinking,stoppondering,playouts);
    #ifdef HAVE_MPI
    //gtpe->getOutput()->printfDebug("ponder on rank %d stopping... (inform: %d) threadid %d stoppondering %d\n",mpirank,mpi_inform_others,settings->thread->getID(),stoppondering);
    if (!stop_called && settings->thread->getID()==0 && mpiworldsize>1 && mpi_inform_others)
    {
      stoppondering=true;
      this->mpiSyncUpdate(true);
      //here must be waited till all are stoped!!
      //gtpe->getOutput()->printfDebug("mpiSyncWaitStop on rank %d stoped (inform: %d) threadid %d\n",mpirank,mpi_inform_others,settings->thread->getID());
      //this->mpiSyncWaitStop();
      
      //stoppondering=false;
    }
    //gtpe->getOutput()->printfDebug("ponder on rank %d stoped (inform: %d) threadid %d\n",mpirank,mpi_inform_others,settings->thread->getID());
    #endif
  }
}

void Engine::doSlowUpdate()
{
  Tree *besttree=movetree->getRobustChild();
  if (besttree!=NULL)
  {
    params->surewin_expected=(besttree->getRatio()>=params->surewin_threshold);
    
    if (params->surewin_expected && (params->surewin_pass_bonus>0 || params->surewin_touchdead_bonus>0 || params->surewin_oppoarea_penalty>0))
    {
      Tree *passtree=movetree->getChild(Go::Move(currentboard->nextToMove(),Go::Move::PASS));
      
      if (passtree->isPruned())
      {
        passtree->setPruned(false);
        if (params->surewin_pass_bonus>0)
          passtree->setProgressiveBiasBonus(params->surewin_pass_bonus);
        
        if (params->surewin_touchdead_bonus>0)
        {
          int size=boardsize;
          for(std::list<Tree*>::iterator iter=movetree->getChildren()->begin();iter!=movetree->getChildren()->end();++iter) 
          {
            int pos=(*iter)->getMove().getPosition();
            Go::Color col=(*iter)->getMove().getColor();
            Go::Color othercol=Go::otherColor(col);
            
            bool founddead=false;
            if (pos>=0)
            {
              foreach_adjacent(pos,p,{
              if (currentboard->getColor(p)==othercol && !currentboard->isAlive(territorymap,params->territory_threshold,p))
                founddead=true;
              });
            }
            
            if (founddead)
            {
              movetree->unprunefromchild(*iter); // (*iter)->setPruned(false);
              (*iter)->setProgressiveBiasBonus(params->surewin_touchdead_bonus);
            }
          }
        }
        
        if (params->surewin_oppoarea_penalty>0)
        {
          for(std::list<Tree*>::iterator iter=movetree->getChildren()->begin();iter!=movetree->getChildren()->end();++iter) 
          {
            int pos=(*iter)->getMove().getPosition();
            Go::Color col=(*iter)->getMove().getColor();
            
            bool oppoarea=false;
            if (col==Go::BLACK)
              oppoarea=(-territorymap->getPositionOwner(pos))>params->territory_threshold;
            else
              oppoarea=territorymap->getPositionOwner(pos)>params->territory_threshold;
            
            if (oppoarea)
              (*iter)->setProgressiveBiasBonus(-params->surewin_oppoarea_penalty);
          }
        }
      }
    }
    
    params->uct_last_r2=besttree->secondBestPlayoutRatio();
  }
}

void Engine::generateThread(Worker::Settings *settings)
{
  boost::posix_time::ptime time_start=this->timeNow();
  Go::Color col=currentboard->nextToMove();
  int livegfxupdate=0;
  float time_allocated;
  long totalplayouts;
  #ifdef HAVE_MPI
    bool mpi_inform_others=true;
    bool mpi_rank_other=(mpirank!=0);
    if (mpi_rank_other) stopthinking=false;
    //int mpi_update_num=0;
  #else
    bool mpi_rank_other=false;
  #endif
  
  if (!time->isNoTiming())
  {
    if (params->time_ignore)
      time_allocated=0;
    else
      time_allocated=time->getAllocatedTimeForNextTurn(col);
  }
  else
    time_allocated=0;
  
  Go::IntBoard *firstlist=new Go::IntBoard(boardsize);
  Go::IntBoard *secondlist=new Go::IntBoard(boardsize);
  Go::IntBoard *earlyfirstlist=new Go::IntBoard(boardsize);
  Go::IntBoard *earlysecondlist=new Go::IntBoard(boardsize);

  while ((totalplayouts=(long)(movetree->getPlayouts()-params->uct_initial_playouts))<(params->playouts_per_move_max))
  {
    if (totalplayouts>=(params->playouts_per_move) && time_allocated==0)
      break;
    else if (totalplayouts>=(params->playouts_per_move_min) && time_allocated>0 && this->timeSince(time_start)>time_allocated)
      break;
    else if (movetree->isTerminalResult())
    {
      params->early_stop_occured=true;
      break;
    }
    else if (stopthinking)
    {
      params->early_stop_occured=true;
      break;
    }
    else if (this->timeSince(time_start)>params->time_move_max)
    {
      params->early_stop_occured=true;
      break;
    }
    
    this->doPlayout(settings,firstlist,secondlist,earlyfirstlist,earlysecondlist);
    totalplayouts+=1;
    
    #ifdef HAVE_MPI
      if (settings->thread->getID()==0 && mpiworldsize>1 && MPI::Wtime()>(params->mpi_last_update+params->mpi_update_period))
      {
        //mpi_update_num++;
        //gtpe->getOutput()->printfDebug("update (%d) at %lf (rank: %d) start\n",mpi_update_num,MPI::Wtime(),mpirank);
        
        mpi_inform_others=this->mpiSyncUpdate();
        
        params->mpi_last_update=MPI::Wtime();
        
        if (!mpi_inform_others)
        {
          params->early_stop_occured=true;
          break;
        }
      }
    #endif
    
    if (settings->thread->getID()==0 && !mpi_rank_other && params->uct_stop_early && params->uct_slow_update_last==0 && totalplayouts>=(params->playouts_per_move_min))
    {
      Tree *besttree=movetree->getRobustChild();
      //double newtotalplayouts=totalplayouts;
      double newtotalplayouts=movetree->getPlayouts(); //fixed a long standing timing bug, which was introduced probably when reusing childtrees
      if (besttree!=NULL)
      {
        double currentpart=(besttree->getPlayouts()-besttree->secondBestPlayouts())/newtotalplayouts;
        double overallratio,overallratiotimed;
        double calcmax=0;
        if (time_allocated>0) // timed search
        {
          overallratio=((double)params->playouts_per_move_max+newtotalplayouts-totalplayouts)/newtotalplayouts;
          double timeratio=(double)(time_allocated+TIME_RESOLUTION)/this->timeSince(time_start);
          calcmax=totalplayouts*timeratio;
          overallratiotimed=(calcmax+newtotalplayouts-totalplayouts)/newtotalplayouts;
        }
        else
        {
          overallratio=((double)params->playouts_per_move_max+newtotalplayouts-totalplayouts)/newtotalplayouts;
          overallratiotimed=0;
        }
        
        if (((overallratio-1)<currentpart) || ((time_allocated>0) && ((overallratiotimed-1)<currentpart)))
        {
          gtpe->getOutput()->printfDebug("best move cannot change! (current %.3f playratio %.3f timedratio %.3f calcmax %.3f time used %.3f total %.0f newtotal %.0f)\n",
                                         currentpart, overallratio, overallratiotimed, calcmax, (double)this->timeSince(time_start), (double)totalplayouts, newtotalplayouts);
          stopthinking=true;
          params->early_stop_occured=true;
          break;
        }
      }
    }
    
    if (settings->thread->getID()==0 && params->livegfx_on)
    {
      if (livegfxupdate>=(params->livegfx_update_playouts-1))
      {
        livegfxupdate=0;
        
        this->displayPlayoutLiveGfx(totalplayouts);
        
        boost::timer delay;
        while (delay.elapsed()<params->livegfx_delay) {}
      }
      else
        livegfxupdate++;
    }
  }
  stopthinking=true;
  params->early_stop_occured=true;
          
  delete firstlist;
  delete secondlist;
  delete earlyfirstlist;
  delete earlysecondlist;
  
  #ifdef HAVE_MPI
  //gtpe->getOutput()->printfDebug("genmove on rank %d stopping... (inform: %d)\n",mpirank,mpi_inform_others);
  if (settings->thread->getID()==0 && mpiworldsize>1 && mpi_inform_others)
    this->mpiSyncUpdate(true);
  #endif
  
  //stopthinking=true;
}

void Engine::doNPlayoutsThread(Worker::Settings *settings)
{
  int livegfxupdate=0;
  Go::IntBoard *firstlist=new Go::IntBoard(boardsize);
  Go::IntBoard *secondlist=new Go::IntBoard(boardsize);
  Go::IntBoard *earlyfirstlist=new Go::IntBoard(boardsize);
  Go::IntBoard *earlysecondlist=new Go::IntBoard(boardsize);
  long totalplayouts;
  float *score_stats=NULL;
  //gtpe->getOutput()->printfDebug("dddd2\n");
  if (params->CNN_data_playouts>0) {
    score_stats=new float[boardsize*boardsize]();
    //gtpe->getOutput()->printfDebug("dddd3a\n");
  }
  while ((totalplayouts=(long)(movetree->getPlayouts()-params->uct_initial_playouts))<(params->playouts_per_move))
  {
    if (movetree->isTerminalResult())
    {
      stopthinking=true;
      break;
    }
    else if (stopthinking)
      break;
    
    //gtpe->getOutput()->printfDebug("dddd3ab\n");
    this->doPlayout(settings,firstlist,secondlist,earlyfirstlist,earlysecondlist,score_stats);
    //gtpe->getOutput()->printfDebug("dddd3b\n");
    totalplayouts+=1;
    
    if (settings->thread->getID()==0 && params->livegfx_on)
    {
      if (livegfxupdate>=(params->livegfx_update_playouts-1))
      {
        livegfxupdate=0;
        
        this->displayPlayoutLiveGfx(totalplayouts+1);
        
        boost::timer delay;
        while (delay.elapsed()<params->livegfx_delay) {}
      }
      else
        livegfxupdate++;
    }
  }

  //gtpe->getOutput()->printfDebug("dddd4\n");
  if (params->CNN_data_playouts>0) {
    //gtpe->getOutput()->printfDebug("dddd5\n");
    float sum_stats=0;
    for (int i=0;i<boardsize*boardsize;i++)
      sum_stats+=score_stats[i];
    for (int i=0;i<boardsize*boardsize;i++)
      gtpe->getOutput()->printfDebug(",%.3f",score_stats[i]/sum_stats);
    //gtpe->getOutput()->printfDebug("\n");
    delete[] score_stats;
  }
  delete firstlist;
  delete secondlist;
  delete earlyfirstlist;
  delete earlysecondlist;  
}

void Engine::doThreadWork(Worker::Settings *settings)
{
  switch (params->thread_job)
  {
    case Parameters::TJ_GENMOVE:
      this->generateThread(settings);
      break;
    case Parameters::TJ_PONDER:
      this->ponderThread(settings);
      break;
    case Parameters::TJ_DONPLTS:
      this->doNPlayoutsThread(settings);
      break;
  }
}

void Engine::updateTerritoryScoringInTree()
{
  if (!params->rules_all_stones_alive)
  {
    float scorenow;
    if (params->cleanup_in_progress)
      scorenow=currentboard->score()-params->engine->getHandiKomi();
    else
      scorenow=currentboard->territoryScore(territorymap,params->territory_threshold)-params->engine->getHandiKomi();
    
    Go::Color col=currentboard->nextToMove();
    Go::Color othercol=Go::otherColor(col);
    
    bool winforcol=Go::Board::isWinForColor(col,scorenow);
    bool jigonow=(scorenow==0);
    
    Tree *passtree=movetree->getChild(Go::Move(col,Go::Move::PASS));
    if (passtree!=NULL)
    {
      passtree->resetNode();
      if (jigonow)
        passtree->addPartialResult(0.5,1,false);
      else if (winforcol)
        passtree->addWin(scorenow);
      else
        passtree->addLose(scorenow);
      
      Tree *pass2tree=passtree->getChild(Go::Move(othercol,Go::Move::PASS));
      if (pass2tree!=NULL)
      {
        pass2tree->resetNode();
        if (jigonow)
          pass2tree->addPartialResult(0.5,1,false);
        else if (!winforcol)
          pass2tree->addWin(scorenow);
        else
          pass2tree->addLose(scorenow);
      }
    }
  }
}

std::string Engine::chat(bool pm,std::string name,std::string msg)
{
  if (msg=="stat")
  {
    if (moveexplanations->size()>0)
      return moveexplanations->back();
    else
      return "";
  }
  else
    return ("Unknown command '"+msg+"'. Try 'stat'.");
}

void Engine::gameFinished()
{
  if (params->csstyle_adaptiveplayouts) {
    float min=1,max=1;
    for (int i=0;i<2*boardsize*boardsize*(local_feature_num+hashto5num);i++) {
      if (deltagammaslocal[i]<min) min=deltagammaslocal[i];
      if (deltagammaslocal[i]>max) max=deltagammaslocal[i];
    }
    gtpe->getOutput()->printfDebug("game finished min %f max %f\n",min,max);
  }
  
  if (isgamefinished)
    return;
  isgamefinished=true;

  if (params->mm_learn_enabled) 
  {
    fprintf(stderr,"files gamma %s circ %s\n",learn_filename_features.c_str(),learn_filename_circ_patterns.c_str()); 
    getFeatures()->saveGammaFile (learn_filename_features);
    getFeatures()->saveCircValueFile (learn_filename_circ_patterns);
    gtpe->getOutput()->printfDebug("learned gammas and circ patterns saved with orderquality\n");
  }
  
  if (currentboard->getMovesMade()==0)
    return;

  bool autosave=true;
#ifdef HAVE_MPI
  if (mpirank!=0) autosave=false;
#endif      
  if (params->auto_save_sgf && autosave)
  {
    boost::posix_time::time_facet *facet = new boost::posix_time::time_facet("_%Y-%m-%d_%H:%M:%S");
    std::ostringstream ss;
    ss << params->auto_save_sgf_prefix;
    ss.imbue(std::locale(ss.getloc(),facet));
    ss << boost::posix_time::second_clock::local_time();
    ss << ".sgf";
    std::string filename=ss.str();
    if (this->writeGameSGF(filename))
      gtpe->getOutput()->printfDebug("Wrote game SGF to '%s'\n",filename.c_str());
  }
}

#ifdef HAVE_MPI
void Engine::mpiCommandHandler()
{
  Engine::MPICommand cmd;
  unsigned int tmp1,tmp2,tmp3;
  bool running=true;
  
  //gtpe->getOutput()->printfDebug("mpi rank %d reporting for duty!\n",mpirank);
  
  while (running)
  {
    MPI::COMM_WORLD.Bcast(&tmp1,1,MPI::UNSIGNED,0);
    cmd=(Engine::MPICommand)tmp1;
    //gtpe->getOutput()->printfDebug("recv cmd: %d (%d)\n",cmd,mpirank);
    switch (cmd)
    {
      case MPICMD_CLEARBOARD:
        this->clearBoard();
        break;
      case MPICMD_SETBOARDSIZE:
        this->mpiRecvBroadcastedArgs(&tmp1);
        this->setBoardSize((int)tmp1);
        break;
      case MPICMD_SETKOMI:
        this->mpiRecvBroadcastedArgs(&tmp1);
        this->setKomi((float)tmp1);
        break;
      case MPICMD_MAKEMOVE:
        {
          this->mpiRecvBroadcastedArgs(&tmp1,&tmp2);
          Go::Move move = Go::Move((Go::Color)tmp1,(int)tmp2);
          this->makeMove(move);
        }
        break;
      case MPICMD_SETPARAM:
        {
          std::string param=this->mpiRecvBroadcastedString();
          std::string val=this->mpiRecvBroadcastedString();
          params->setParameter(param,val);
          //gtpe->getOutput()->printfDebug("rank of %d set %s to %s\n",mpirank,param.c_str(),val.c_str());
        }
        break;
      case MPICMD_TIMESETTINGS:
        this->mpiRecvBroadcastedArgs(&tmp1,&tmp2,&tmp3);
        delete time;
        time=new Time(params,(int)tmp1,(int)tmp2,(int)tmp3);
        break;
      case MPICMD_TIMELEFT:
        this->mpiRecvBroadcastedArgs(&tmp1,&tmp2,&tmp3);
        time->updateTimeLeft((Go::Color)tmp1,(float)tmp2,(int)tmp3);
        break;
      case MPICMD_LOADPATTERNS:
        delete patterntable;
        patterntable=new Pattern::ThreeByThreeTable();
        patterntable->loadPatternFile(this->mpiRecvBroadcastedString());
        break;
      case MPICMD_CLEARPATTERNS:
        delete patterntable;
        patterntable=new Pattern::ThreeByThreeTable();
        break;
      case MPICMD_LOADFEATUREGAMMAS:
        delete features;
        features=new Features(params);
        features->loadGammaFile(this->mpiRecvBroadcastedString());
        break;
      case MPICMD_BOOKADD:
        {
          this->mpiRecvBroadcastedArgs(&tmp1);
          Go::Move move = Go::Move(currentboard->nextToMove(),(int)tmp1);
          book->add(boardsize,movehistory,move);
        }
        break;
      case MPICMD_BOOKREMOVE:
        {
          this->mpiRecvBroadcastedArgs(&tmp1);
          Go::Move move = Go::Move(currentboard->nextToMove(),(int)tmp1);
          book->remove(boardsize,movehistory,move);
        }
        break;
      case MPICMD_BOOKCLEAR:
        book->clear(boardsize);
        break;
      case MPICMD_BOOKLOAD:
        delete book;
        book=new Book(params);
        book->loadFile(this->mpiRecvBroadcastedString());
        break;
      case MPICMD_CLEARTREE:
        this->clearMoveTree();
        break;
      case MPICMD_GENMOVE:
        this->mpiRecvBroadcastedArgs(&tmp1);
        this->mpiGenMove((Go::Color)tmp1);
        break;
      case MPICMD_PONDER:
        this->mpiRecvBroadcastedArgs(&tmp1);
        this->mpiPonder((Go::Color)tmp1);
        break;
      case MPICMD_QUIT:
      default:
        running=false;
        break;
    };
  }
}

void Engine::mpiBroadcastCommand(Engine::MPICommand cmd, unsigned int *arg1, unsigned int *arg2, unsigned int *arg3)
{
  //gtpe->getOutput()->printfDebug("send cmd: %d (%d)\n",cmd,mpirank);
  MPI::COMM_WORLD.Bcast(&cmd,1,MPI::UNSIGNED,0);
  
  if (arg1!=NULL)
    MPI::COMM_WORLD.Bcast(arg1,1,MPI::UNSIGNED,0);
  if (arg2!=NULL)
    MPI::COMM_WORLD.Bcast(arg2,1,MPI::UNSIGNED,0);
  if (arg3!=NULL)
    MPI::COMM_WORLD.Bcast(arg3,1,MPI::UNSIGNED,0);
}

void Engine::mpiRecvBroadcastedArgs(unsigned int *arg1, unsigned int *arg2, unsigned int *arg3)
{
  if (arg1!=NULL)
    MPI::COMM_WORLD.Bcast(arg1,1,MPI::UNSIGNED,0);
  if (arg2!=NULL)
    MPI::COMM_WORLD.Bcast(arg2,1,MPI::UNSIGNED,0);
  if (arg3!=NULL)
    MPI::COMM_WORLD.Bcast(arg3,1,MPI::UNSIGNED,0);
}

void Engine::mpiBroadcastString(std::string input)
{
  char buffer[MPI_STRING_MAX];
  
  if (input.length()>=MPI_STRING_MAX)
  {
    gtpe->getOutput()->printfDebug("string too long (%d>=%d)\n",input.length(),MPI_STRING_MAX);
    return;
  }
  
  for (int i=0;i<(int)input.length();i++)
  {
    buffer[i]=input.at(i);
  }
  for (int i=input.length();i<MPI_STRING_MAX;i++)
  {
    buffer[i]=0;
  }
  
  MPI::COMM_WORLD.Bcast(buffer,MPI_STRING_MAX,MPI::CHAR,0);
}

std::string Engine::mpiRecvBroadcastedString()
{
  char buffer[MPI_STRING_MAX];
  MPI::COMM_WORLD.Bcast(buffer,MPI_STRING_MAX,MPI::CHAR,0);
  return std::string(buffer);
}

void Engine::mpiSendString(int destrank, std::string input)
{
  char buffer[MPI_STRING_MAX];
  
  if (input.length()>=MPI_STRING_MAX)
  {
    gtpe->getOutput()->printfDebug("string too long (%d>=%d)\n",input.length(),MPI_STRING_MAX);
    return;
  }
  
  for (int i=0;i<(int)input.length();i++)
  {
    buffer[i]=input.at(i);
  }
  for (int i=input.length();i<MPI_STRING_MAX;i++)
  {
    buffer[i]=0;
  }
  
  MPI::COMM_WORLD.Send(buffer,MPI_STRING_MAX,MPI::CHAR,destrank,0);
}

std::string Engine::mpiRecvString(int srcrank)
{
  char buffer[MPI_STRING_MAX];
  MPI::COMM_WORLD.Recv(buffer,MPI_STRING_MAX,MPI::CHAR,srcrank,0);
  return std::string(buffer);
}

void Engine::mpiGenMove(Go::Color col)
{
  //gtpe->getOutput()->printfDebug("genmove on rank %d starting...\n",mpirank);
  currentboard->setNextToMove(col);
  
  movetree->pruneSuperkoViolations();
  this->allowContinuedPlay();
  //this->updateTerritoryScoringInTree();
  params->uct_slow_update_last=0;
  // generate immediatly on dbg line, was 0
  params->uct_slow_debug_last=params->uct_slow_debug_interval;
  params->uct_last_r2=-1;
  
  int startplayouts=(int)movetree->getPlayouts();
  params->mpi_last_update=MPI::Wtime();
  
  params->uct_initial_playouts=startplayouts;
  params->thread_job=Parameters::TJ_GENMOVE;
  threadpool->startAll();
  threadpool->waitAll();
  
  //gtpe->getOutput()->printfDebug("genmove on rank %d done.\n",mpirank);
}

void Engine::mpiPonder(Go::Color col)
{
  //fprintf(stderr,"ponder on rank %d starting...\n",mpirank);
  //gtpe->getOutput()->printfDebug("ponder on rank %d starting...\n",mpirank);
 // currentboard->setNextToMove(col);
  
  movetree->pruneSuperkoViolations();
  this->allowContinuedPlay();
  //this->updateTerritoryScoringInTree();
  params->uct_slow_update_last=0;
  params->uct_slow_debug_last=0;
  params->uct_last_r2=-1;
  
  int startplayouts=(int)movetree->getPlayouts();
  params->mpi_last_update=MPI::Wtime();
  
  params->uct_initial_playouts=startplayouts;
  params->thread_job=Parameters::TJ_PONDER;
  #ifdef HAVE_MPI
  //  isWaitingForStop=false;
  #endif
  stoppondering=false;
  threadpool->startAll();
  threadpool->waitAll(); // Here it makes it impossible to interrupt, as it is not listening to mpi commands
  #ifdef HAVE_MPI
  //  isWaitingForStop=true;
    mpiSyncWaitStop();
  #endif    
    
  //fprintf(stderr,"ponder on rank %d done.\n",mpirank);
  //gtpe->getOutput()->printfDebug("ponder on rank %d done.\n",mpirank);
}

void Engine::mpiBuildDerivedTypes()
{
  //mpistruct_updatemsg tmp;
  int i=0,count=4;
  int blocklengths[count];
  MPI::Datatype types[count];
  MPI::Aint displacements[count];
  MPI::Aint extent,lowerbound;
  
  blocklengths[i]=8;
  types[i]=MPI::BYTE;
  displacements[i]=0;
  types[i].Get_extent(lowerbound,extent);
  i++;
  //if (mpirank==0)
  //  fprintf(stderr,"lowerbound: %d, extent: %d\n",lowerbound,extent);
  
  blocklengths[i]=8;
  types[i]=MPI::BYTE;
  displacements[i]=displacements[i-1]+extent*blocklengths[i-1];
  types[i].Get_extent(lowerbound,extent);
  i++;
  //if (mpirank==0)
  //  fprintf(stderr,"lowerbound: %d, extent: %d\n",lowerbound,extent);
  
  blocklengths[i]=1;
  types[i]=MPI::FLOAT;
  displacements[i]=displacements[i-1]+extent*blocklengths[i-1];
  types[i].Get_extent(lowerbound,extent);
  i++;
  //if (mpirank==0)
  //  fprintf(stderr,"lowerbound: %d, extent: %d\n",lowerbound,extent);
  
  blocklengths[i]=1;
  types[i]=MPI::FLOAT;
  displacements[i]=displacements[i-1]+extent*blocklengths[i-1];
  types[i].Get_extent(lowerbound,extent);
  i++;
  //if (mpirank==0)
  //  fprintf(stderr,"lowerbound: %d, extent: %d\n",lowerbound,extent);
  
  //TODO: verify that above works if struct elements aren't contiguous (word boundaries)
  
  mpitype_updatemsg=MPI::Datatype::Create_struct(count,blocklengths,displacements,types);
  mpitype_updatemsg.Commit();
}

void Engine::mpiFreeDerivedTypes()
{
  mpitype_updatemsg.Free();
}

void Engine::MpiHashTable::clear()
{
  for (int i=0;i<MPI_HASHTABLE_SIZE;i++)
  {
    table[i].clear();
  }
}

Engine::MpiHashTable::TableEntry *Engine::MpiHashTable::lookupEntry(Go::ZobristHash hash)
{
  unsigned int index=hash%MPI_HASHTABLE_SIZE;
  for(std::list<Engine::MpiHashTable::TableEntry>::iterator iter=table[index].begin();iter!=table[index].end();++iter)
  {
    if (hash==(*iter).hash)
      return &(*iter);
  }
  return NULL;
}

std::list<Tree*> *Engine::MpiHashTable::lookup(Go::ZobristHash hash)
{
  Engine::MpiHashTable::TableEntry *entry=this->lookupEntry(hash);
  if (entry!=NULL)
    return &(entry->nodes);
  else
    return NULL;
}

void Engine::MpiHashTable::add(Go::ZobristHash hash, Tree *node)
{
  Engine::MpiHashTable::TableEntry *entry=this->lookupEntry(hash);
  if (entry!=NULL)
  {
    for(std::list<Tree*>::iterator iter=entry->nodes.begin();iter!=entry->nodes.end();++iter)
    {
      if ((*iter)==node) // is already there?
        return;
    }
    entry->nodes.push_back(node);
  }
  else
  {
    unsigned int index=hash%MPI_HASHTABLE_SIZE;
    Engine::MpiHashTable::TableEntry entry;
    entry.hash=hash;
    entry.nodes.push_back(node);
    table[index].push_back(entry);
  }
}

void Engine::mpiSyncWaitStop()
{
 //gtpe->getOutput()->printfDebug("try syncWaitStop (rank: %d)\n",mpirank);
 MPI::COMM_WORLD.Barrier();
 //while (true)
 // {
 //   params->mpi_last_update=MPI::Wtime();
 //   int localcount=(isWaitingForStop?1:0);
 //   int maxcount;
 //   MPI::COMM_WORLD.Allreduce(&localcount,&maxcount,1,MPI::INT,MPI::MIN);
    //gtpe->getOutput()->printfDebug("syncWaitStop localcount %d maxcount %d rank %d\n",localcount,maxcount,mpirank);
 //   if (maxcount==1)
 //     break;
 //   boost::this_thread::sleep(boost::posix_time::seconds(params->mpi_update_period));    
 // }
  //gtpe->getOutput()->printfDebug("syncWaitStop (rank: %d)\n",mpirank);
}

bool Engine::mpiSyncUpdate(bool stop)
{
  int localcount=(stop?0:1);
  int maxcount;
  
  //gtpe->getOutput()->printfDebug("!!!!!sync (rank: %d) (stop:%d)!!!!!\n",mpirank,stop);
  
  //TODO: should consider replacing first 2 mpi cmds with 1
  MPI::COMM_WORLD.Allreduce(&localcount,&maxcount,1,MPI::INT,MPI::MIN);

  //inform others about stopthinking and stoppondering events
  int stopping[2]={stopthinking,stoppondering};
  int resstopping[2];
  MPI::COMM_WORLD.Allreduce(&stopping,&resstopping,2,MPI::INT,MPI::MAX);
  stopthinking=resstopping[0];
  stoppondering=resstopping[1];
  if (stopthinking || stoppondering) stop_called=true;

  if (maxcount==0)
  {
    //gtpe->getOutput()->printfDebug("sync (rank: %d) stopping\n",mpirank);
    return false;
  }
  //gtpe->getOutput()->printfDebug("sync (rank: %d) not stopping\n",mpirank);
  
  std::list<mpistruct_updatemsg> locallist;
  if (movetree->getPlayouts()>0)
  {
    float threshold=params->mpi_update_threshold*movetree->getPlayouts();
    this->mpiFillList(locallist,threshold,params->mpi_update_depth,movetree);
  }
  
  if (locallist.size()==0) // add 1 empty msg, else MPI_Allgather() will stall
  {
    mpistruct_updatemsg msg;
    msg.hash=0;
    msg.parenthash=0;
    msg.playouts=0;
    msg.wins=0;
    locallist.push_back(msg);
    //gtpe->getOutput()->printfDebug("sync (rank: %d) added empty msg\n",mpirank);
  }
  
  localcount=0;
  mpistruct_updatemsg localmsgs[locallist.size()];
  for(std::list<mpistruct_updatemsg>::iterator iter=locallist.begin();iter!=locallist.end();++iter)
  {
    localmsgs[localcount]=(*iter);
    localcount++;
  }
  
  MPI::COMM_WORLD.Allreduce(&localcount,&maxcount,1,MPI::INT,MPI::MAX);
  //gtpe->getOutput()->printfDebug("sync (rank: %d) (local:%d, max:%d)\n",mpirank,localcount,maxcount);
  
  mpistruct_updatemsg allmsgs[maxcount*mpiworldsize];
  for (int i=0;i<maxcount*mpiworldsize;i++)
  {
    allmsgs[i].hash=0; // needed as maxcount is not necessarily mincount
  }
  MPI::COMM_WORLD.Allgather(localmsgs,localcount,mpitype_updatemsg,allmsgs,maxcount,mpitype_updatemsg);
  //gtpe->getOutput()->printfDebug("sync (rank: %d) gathered\n",mpirank);
  
  for (int i=0;i<mpiworldsize;i++)
  {
    if (i==mpirank) //ignore own messages
      continue;
    
    for (int j=0;j<maxcount;j++)
    {
      mpistruct_updatemsg *msg=&allmsgs[i*maxcount+j];
      if (msg->hash==0)
        break;
      
      //fprintf(stderr,"add msg: 0x%016Lx 0x%016Lx %.1f %.1f\n",msg->hash,msg->parenthash,msg->playouts,msg->wins);
      
      std::list<Tree*> *nodes=mpihashtable.lookup(msg->hash);
      if (nodes!=NULL)
      {
        for(std::list<Tree*>::iterator iter=nodes->begin();iter!=nodes->end();++iter)
        {
          (*iter)->addMpiDiff(msg->playouts,msg->wins);
        }
      }
      else
      {
//        bool foundnode=false;
        std::list<Tree*> *parentnodes=mpihashtable.lookup(msg->parenthash);
        if (parentnodes!=NULL)
        {
          for(std::list<Tree*>::iterator iter=parentnodes->begin();iter!=parentnodes->end();++iter)
          {
            for(std::list<Tree*>::iterator iter2=(*iter)->getChildren()->begin();iter2!=(*iter)->getChildren()->end();++iter2)
            {
              if (msg->hash==(*iter2)->getHash())
              {
                (*iter2)->addMpiDiff(msg->playouts,msg->wins);
                mpihashtable.add(msg->hash,(*iter2));
//                foundnode=true;
                //fprintf(stderr,"added hash: 0x%016Lx\n",msg->hash);
              }
            }
          }
        }
        
        //if (!foundnode)
        //  fprintf(stderr,"node not found! (0x%016Lx)\n",msg->hash);
      }
    }
  }
  
  //gtpe->getOutput()->printfDebug("sync (rank: %d) done\n",mpirank);
  
  return true;
}

void Engine::mpiFillList(std::list<mpistruct_updatemsg> &list, float threshold, int depthleft, Tree *tree)
{
  if (depthleft<=0)
    return;
  
  //fprintf(stderr,"adding nodes (%d)\n",depthleft);
  
  Go::ZobristHash parenthash=tree->getHash();
  if (tree->isRoot())
    mpihashtable.add(parenthash,tree);
  
  for(std::list<Tree*>::iterator iter=tree->getChildren()->begin();iter!=tree->getChildren()->end();++iter)
  {
    if ((*iter)->getPlayouts()>=threshold && (*iter)->getHash()!=0)
    {
      mpistruct_updatemsg msg;
      msg.hash=(*iter)->getHash();
      msg.parenthash=parenthash;
      (*iter)->fetchMpiDiff(msg.playouts,msg.wins);
      list.push_back(msg);
      mpihashtable.add(msg.hash,(*iter));
      this->mpiFillList(list,threshold,depthleft-1,(*iter));
    }
  }
}

#endif

float Engine::getOldMoveValue(Go::Move m)
{
  if (m.isNormal ())
  {
    if (m.getColor ()==Go::BLACK)
    {
      if (blackOldMoves[m.getPosition ()]-blackOldMean>0)
        return (blackOldMoves[m.getPosition ()]-blackOldMean)*pow(whiteOldMovesNum,params->uct_oldmove_unprune_factor_c);
      else
        return 0; //do not allow negative results ??? If not using this, one must take care of the Pruned childs!!!!!!
    }
    else
    {
      if (whiteOldMoves[m.getPosition ()]-blackOldMean>0)
        return (whiteOldMoves[m.getPosition ()]-whiteOldMean)*pow(blackOldMovesNum,params->uct_oldmove_unprune_factor_c);
      else
        return 0; //do not allow negative results ??? If not using this, one must take care of the Pruned childs!!!!!!
    }
  }
  else
    return 0; //was a pass move
}

void Engine::gtpCPUtime(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  float cpu_time=(float)clock()/CLOCKS_PER_SEC;
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printf("%f\n",cpu_time);
  gtpe->getOutput()->endResponse();
}

void Engine::gtpVERSION(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Engine *me=(Engine*)instance;
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printf("%s(%s)\n",VERSION,me->params->version_config_file.c_str());
  gtpe->getOutput()->endResponse();
}

void Engine::doGradientDescend(float grad[], float alpha)
{
//  return;
  //float alpha=0.001;
  float alphalambda=params->csstyle_adaptiveplayouts_lambda*alpha;
  float einsMinusAlphaLambda=1.0-alphalambda;
  gradlock.lock();
  if (DEBUG_ADAPTIVE) {
    float min=1,max=1;
    for (int i=0;i<2*boardsize*boardsize*(local_feature_num+hashto5num);i++) {
      if (grad[i]<min) min=grad[i];
      if (grad[i]>max) max=grad[i];
    }
    fprintf(stderr, "vor grad local min %f max %f\n",min,max);
  }
  if (DEBUG_ADAPTIVE) {
    float min=1,max=1;
    for (int i=0;i<2*boardsize*boardsize*(local_feature_num+hashto5num);i++) {
      if (deltagammaslocal[i]<min) min=deltagammaslocal[i];
      if (deltagammaslocal[i]>max) max=deltagammaslocal[i];
    }
    fprintf(stderr, "vor gammas local min %f max %f\n",min,max);
        for (int i=0;i<hashto5num;i++) {
          fprintf(stderr," %6.4f",deltagammaslocal[(5+3*boardsize)*(local_feature_num+hashto5num)+local_feature_num+i]);
        } fprintf(stderr,"\n");
  }
  for (int i=0;i<2*boardsize*boardsize*(local_feature_num+hashto5num);i++) 
    deltagammaslocal[i]=pow(deltagammaslocal[i],einsMinusAlphaLambda)*grad[i];
    //deltagammaslocal[i]=(deltagammas[i]*einsMinusAlphaLambda+alphalambda)*grad[i];  //taylor around x^(1-delta)
  //deltagammaslocal=deltagammas->exchange(*deltagammaslocal);
  if (DEBUG_ADAPTIVE) {
    float min=1,max=1;
    for (int i=0;i<2*boardsize*boardsize*(local_feature_num+hashto5num);i++) {
      if (deltagammaslocal[i]<min) min=deltagammaslocal[i];
      if (deltagammaslocal[i]>max) max=deltagammaslocal[i];
    }
    fprintf(stderr, "nach gammas local min %f max %f\n",min,max);
        for (int i=0;i<hashto5num;i++) {
          fprintf(stderr," %6.4f",deltagammaslocal[(5+3*boardsize)*(local_feature_num+hashto5num)+local_feature_num+i]);
        } fprintf(stderr,"\n");
  }
  gradlock.unlock();
}
