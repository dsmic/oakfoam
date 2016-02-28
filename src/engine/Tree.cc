#define BOOST_DISABLE_ASSERTS

#include "Tree.h"

#include <cmath>
#include <sstream>
#include "Parameters.h"
#include "Engine.h"
#include "Pattern.h"
#include "Worker.h"
#include <cfloat>

#define WITH_P(A) (A>=1.0 || (A>0 && settings->rand->getRandomReal()<A))
int debug=0;
    
//power with sign
#define pmpow(A,B) (((A)>=0)?pow(A,B):(-pow(-(A),B)))

Tree::Tree(Parameters *prms, Go::ZobristHash h, Go::Move mov, Tree *p, int a_pos) : params(prms)
{
  params->tree_instances++;
  parent=p;
  if (parent!=NULL) 
    around_pos=parent->around_pos;
  else
    around_pos=a_pos;
  children=new std::list<Tree*>();
  beenexpanded=false;
  move=mov;
  playouts=0;
  wins=0;
  bestLCBwins=0;
  bestLCBplayouts=0;
  scoresum=0;
  scoresumsq=0;
  urgency_variance=0;
  raveplayouts=0;
  earlyraveplayouts=0;
  ravewins=0;
  earlyravewins=0;
  raveplayoutsother=0;
  ravewinsother=0;
  earlyraveplayoutsother=0;
  earlyravewinsother=0;
  
  symmetryprimary=NULL;
  hasTerminalWinrate=false;
  hasTerminalWin=false;
  terminaloverride=false;
  pruned=false;
  prunedchildren=0;
  gamma=0;
  gamma_weak=0;
  gamma_local_part=0;
  stones_around=0;
  childrentotalgamma=0;
  childrenlogtotalchildgamma=0;
  maxchildgamma=0;
  unprunenextchildat=0;
  lastunprune=0;
  unprunebase=0;
  unprunedchildren=0;
  hash=h;
  ownedblack=0;
  ownedwhite=0;
  ownedwinner=0;
  biasbonus=0;
  superkoprunedchildren=false;
  superkoviolation=false;
  superkochecked=false;
  superkochildrenviolations=0;
  decayedwins=0;
  decayedplayouts=0;
  unprunednum=0;

  ownselfblack=0;
  ownselfwhite=0;
  ownotherblack=0;
  ownotherwhite=0;
  ownnobody=0;
  ownblack=0;
  ownwhite=0;
  ownercount=0;
  blackx=0; blacky=0; blackxy=0; blackx2=0;
  whitex=0; whitey=0; whitexy=0; whitex2=0;
  regcountb=0;
  regcountw=0;
  #ifdef HAVE_MPI
    this->resetMpiDiff();
  #endif
  
  if (parent!=NULL)
  {
    Go::Move pmove=parent->getMove();
    if (!pmove.isPass() && !pmove.isResign())
    {
      if (pmove.getColor()==move.getColor())
        fprintf(stderr,"WARNING! unexpected parent color\n");
    }
  }
  movecirc=NULL;
  eq_moves=NULL;
  cnn_territory_done=0;
  cnn_territory_wr=-1;
  CNNresults=NULL;
  CNNresults_weak=NULL;
}

Tree::~Tree()
{
  for(std::list<Tree*>::iterator iter=children->begin();iter!=children->end();++iter) 
  {
    delete (*iter);
  }
  children->clear();
  delete children;
  params->tree_instances--;
  if (movecirc!=NULL) params->engine->removeMoveCirc(movecirc,this);
  if (movecirc!=NULL) delete movecirc;
  if (CNNresults!=NULL)
    delete[] CNNresults;
  if (CNNresults_weak!=NULL)
    delete[] CNNresults_weak;
}

void Tree::addChild(Tree *node)
{
  if (move.getColor()==node->move.getColor() && (children->size()==0 || children->back()->move.getColor()!=node->move.getColor()))
    fprintf(stderr,"WARNING! Expecting alternating colors in tree\n");
  
//order changed, was not thread save ?!
  node->parent=this;
  children->push_back(node);
}

float Tree::getScoreMean() const
{
  if (playouts>0)
  {
    if (move.getColor()==Go::BLACK)
      return scoresum/playouts;
    else
      return -scoresum/playouts;
  }
  else
    return 0;
}

float Tree::getScoreSD() const
{
  if (playouts>0 && (scoresumsq/playouts-pow(scoresum/playouts,2))>0)
    return sqrt((scoresumsq/playouts-pow(scoresum/playouts,2))/playouts);
  else
    return 0;
}

float Tree::getRatio() const
{
  //return getRatio_intern();

  //was trying this, but it had some problems. Must be rethinked if passes should be handled in more detail!
  float ratio_intern=this->getRatio_intern();
  if (!move.isPass())
    return ratio_intern;
  else {
    return ratio_intern/2.0; //pass penaltiy
    if (ratio_intern > 1-params->resign_ratio_threshold)
      return ratio_intern;
    else 
      return ratio_intern/2.0; // params->resign_ratio_threshold-0.01; //this does not force to do another move allways, but instead resignes some times?!
  }
}

float Tree::getRatio_intern() const
{
  if (this->isTerminalResult())
    return (hasTerminalWin?1:0);
  else if (playouts>0)
  {
    if (params->uct_decay_alpha!=1 || params->uct_decay_k!=0)
      return decayedwins/decayedplayouts;
    else
    {
      //backup operator only allowed, if not (params->uct_decay_alpha!=1 || params->uct_decay_k!=0)
      if (params->test_p75>0 && bestLCBwins>0 && wins>0) //otherwize no sqrt
      {
        wins_playouts use=LCB_UrgentNode_winplayouts({bestLCBwins,bestLCBplayouts},{wins,playouts},params->test_p76);
        return use.wins/use.playouts;
        //if (bestLCBplayouts>0 && wins/playouts>bestLCBwins/bestLCBplayouts && ((wins+params->test_p75*sqrt(wins))/playouts > (bestLCBwins+params->test_p75*sqrt(bestLCBwins))/bestLCBplayouts))
        //  return bestLCBwins/bestLCBplayouts;
        //if (bestLCBplayouts>0 && wins/playouts<bestLCBwins/bestLCBplayouts && ((wins-params->test_p75*sqrt(wins))/playouts < (bestLCBwins-params->test_p75*sqrt(bestLCBwins))/bestLCBplayouts))
        //  return bestLCBwins/bestLCBplayouts;
      }
      return (float)wins/playouts;
    }
  }
  else
    return 0;
}

float Tree::getRAVERatio() const
{
  //passes and resigns do not have a correct rave value
  if (raveplayouts>0 && getMove().isNormal())
    return (float)ravewins/(raveplayouts+params->test_p91);
  else
    return 0.01;
}

float Tree::getEARLYRAVERatio() const
{
  //passes and resigns do not have a correct rave value
  if (earlyraveplayouts>0 && getMove().isNormal())
    return (float)earlyravewins/earlyraveplayouts;
  else
    return 0;
}

float Tree::getRAVERatioOther() const
{
  //passes and resigns do not have a correct rave value
  if (raveplayoutsother>0 && getMove().isNormal())
    return (float)ravewinsother/(raveplayoutsother+params->test_p91);
  else
    return 0;
}

float Tree::getRAVERatioForPool() const
{
  if (raveplayouts>0)
    return (float)(ravewins-params->rave_init_wins)/(raveplayouts-params->rave_init_wins+1);
  else
    return 0;
}

float Tree::getRAVERatioOtherForPool() const
{
  if (raveplayoutsother>0)
    return (float)(ravewinsother-params->rave_init_wins)/(raveplayoutsother-params->rave_init_wins+1);
  else
    return 0;
}

void Tree::addPriorValue(float v)
{
  wins+=v*params->cnn_preset_playouts;
  playouts+=params->cnn_preset_playouts;
  if (!this->isRoot ())
    parent->addPriorValue (1.0-v);
}
void Tree::addWin(int fscore, Tree *source)
{
  boost::mutex::scoped_lock *lock=(params->uct_lock_free?NULL:new boost::mutex::scoped_lock(updatemutex));
  /*if (this->isTerminalResult())
  {
    delete lock;
    return;
  }*/
  
  wins++;
  playouts++;
  scoresum+=fscore;
  scoresumsq+=fscore*fscore;
  if (params->uct_decay_alpha!=1 || params->uct_decay_k!=0)
    this->addDecayResult(1);
  
  if (lock!=NULL)
    delete lock;
  this->passPlayoutUp(fscore,true,source);
  this->checkForUnPruning();
  if (this->isPruned() && parent!=NULL) {
    fprintf(stderr,"updateing pruned child win %s\n",move.toString(params->board_size).c_str());
    parent->unprunefromchild (this); //this->setPruned (false);
  }
}

void Tree::addLose(int fscore, Tree *source)
{
  boost::mutex::scoped_lock *lock=(params->uct_lock_free?NULL:new boost::mutex::scoped_lock(updatemutex));
  /*if (this->isTerminalResult())
  {
    delete lock;
    return;
  }*/

  playouts++;
  scoresum+=fscore;
  scoresumsq+=fscore*fscore;
  if (params->uct_decay_alpha!=1 || params->uct_decay_k!=0)
    this->addDecayResult(0);
  
  if (lock!=NULL)
    delete lock;
  this->passPlayoutUp(fscore,false,source);
  this->checkForUnPruning();
  if (this->isPruned() && parent!=NULL) {
    fprintf(stderr,"updateing pruned child loss %s\n",move.toString(params->board_size).c_str());
    parent->unprunefromchild (this); //this->setPruned (false);
  }
}

void Tree::addDecayResult(float result)
{
  if (params->uct_decay_alpha!=1)
  {
    decayedwins*=params->uct_decay_alpha;
    decayedplayouts*=params->uct_decay_alpha;
  }
  if (params->uct_decay_k!=0 && playouts>0)
  {
    float decayfactor=pow((playouts+params->uct_decay_m-1)/(playouts+params->uct_decay_m),params->uct_decay_k);
    //fprintf(stderr,"decayfactor: %.6f\n",decayfactor);
    decayedwins*=decayfactor;
    decayedplayouts*=decayfactor;
  }
  decayedwins+=result;
  decayedplayouts++;
}

void Tree::addVirtualLoss()
{
  boost::mutex::scoped_lock *lock=(params->uct_lock_free?NULL:new boost::mutex::scoped_lock(updatemutex));
  playouts++;
  if (lock!=NULL)
    delete lock;
}

void Tree::removeVirtualLoss()
{
  boost::mutex::scoped_lock *lock=(params->uct_lock_free?NULL:new boost::mutex::scoped_lock(updatemutex));
  playouts--;
  if (lock!=NULL)
    delete lock;
  if (!this->isRoot() && !parent->isRoot())
    parent->removeVirtualLoss();
}

void Tree::addPriorWins(int n)
{
  wins+=n;
  playouts+=n;
  if (params->uct_decay_alpha!=1 || params->uct_decay_k!=0)
  {
    for (int i=0;i<n;i++)
    {
      this->addDecayResult(1);
    }
  }
}

void Tree::addPriorLoses(int n)
{
  playouts+=n;
  if (params->uct_decay_alpha!=1 || params->uct_decay_k!=0)
  {
    for (int i=0;i<n;i++)
    {
      this->addDecayResult(0);
    }
  }
}

void Tree::addRAVEWin(bool early,float weight)
{
  //boost::mutex::scoped_lock lock(updatemutex);
  if (early)
  {
    earlyravewins++;
    earlyraveplayouts++;
  }
  else
  {
    ravewins+=weight;
    raveplayouts+=weight;
  }
}

void Tree::addRAVELose(bool early,float weight)
{
  //boost::mutex::scoped_lock lock(updatemutex);
  if (early)
  {
    earlyraveplayouts++;
  }
  else
  {
    raveplayouts+=weight;
  }
}

void Tree::addRAVEWinOther(bool early,float weight)
{
  //boost::mutex::scoped_lock lock(updatemutex);
  if (early) 
  {
    earlyravewinsother++;
    earlyraveplayoutsother++;
  }
  else
  {
    ravewinsother+=weight;
    raveplayoutsother+=weight;
  }
}

void Tree::addRAVELoseOther(bool early,float weight)
{
  //boost::mutex::scoped_lock lock(updatemutex);
  if (early)
    earlyraveplayoutsother++;
  else
    raveplayoutsother+=weight;
}

void Tree::addRAVEWins(int n,bool early)
{
  if (early)
  {
//  earlyravewins+=n;
//  earlyraveplayouts+=n;

//  earlyravewinsother+=n;
//  earlyraveplayoutsother+=n;
  }
  else
  {
    ravewins+=n;
    raveplayouts+=n;

    //only used for initialization, therefore both can be done in this function
    ravewinsother+=n;
    raveplayoutsother+=n;
  }
}  


void Tree::addRAVELoses(int n,bool early)
{
  if (early)
  {
    earlyraveplayouts+=n;
  }
  else
  {
    raveplayouts+=n;
  }
}

void Tree::addPartialResult(float win, float playout, bool invertwin, float decayfactor)
{
  boost::mutex::scoped_lock *lock=(params->uct_lock_free?NULL:new boost::mutex::scoped_lock(updatemutex));
  //fprintf(stderr,"adding partial result: %f %f %d\n",win,playout,invertwin);
  if (!this->isTerminalResult())
  {
    wins+=win;
    playouts+=playout;
  }
  if (lock!=NULL)
    delete lock;
  if (!this->isRoot())
  {
    if (invertwin)
      parent->addPartialResult(-win,playout,invertwin);
    else
      parent->addPartialResult(decayfactor*(playout-win),decayfactor*playout,invertwin,decayfactor);
  }
  this->checkForUnPruning();
  if (this->isPruned() && parent!=NULL) {
    fprintf(stderr,"updateing pruned child Partial %s\n",move.toString(params->board_size).c_str());
    parent->unprunefromchild (this); //this->setPruned (false);
  }
}

Tree *Tree::getChild(Go::Move move) const
{
  for(std::list<Tree*>::iterator iter=children->begin();iter!=children->end();++iter) 
  {
    if ((*iter)->getMove()==move && !(*iter)->isSuperkoViolation()) 
      return (*iter);
  }
  return NULL;
}

bool Tree::allChildrenTerminalLoses()
{
  if (this->isLeaf()) return false; //no children !!
  
  //fprintf(stderr,"check if all children of %s are losses: ",move.toString(params->board_size).c_str());
  for(std::list<Tree*>::iterator iter=children->begin();iter!=children->end();++iter) 
  {
    if ((*iter)->isPrimary() && !(*iter)->isTerminalLose())
    {
      //fprintf(stderr,"false\n");
      return false;
    }
  }
  //fprintf(stderr,"true (%d)\n",children->size());
  return true;
}

bool Tree::hasOneUnprunedChildNotTerminalLoss()
{
  for(std::list<Tree*>::iterator iter=children->begin();iter!=children->end();++iter) 
  {
    if (!(*iter)->isPruned() && !(*iter)->isTerminalLose())
      return true;
  }
  return false;
}

void Tree::passPlayoutUp(int fscore, bool win, Tree *source)
{
  if (params->uct_terminal_handling)
  {
    bool passterminal=(!this->isTerminalResult()) && (this->isTerminal() || (source!=NULL && source->isTerminalResult()));
    
    if (passterminal)
    {
      boost::mutex::scoped_lock *lock=(params->uct_lock_free?NULL:new boost::mutex::scoped_lock(updatemutex));
      if (!this->isTerminalResult())
      {
        if (source!=NULL)
          win=!source->isTerminalWin();
        if (!win || this->allChildrenTerminalLoses())
        {
          //fprintf(stderr,"Terminal info: (%d -> %d)(%d,%d)(%p,%d,%d,%s)\n",hasTerminalWin,win,hasTerminalWinrate,this->isTerminalResult(),source,(source!=NULL?source->isTerminalResult():0),(source!=NULL?source->hasTerminalWin:0),(source!=NULL?source->getMove().toString(params->board_size).c_str():""));
          hasTerminalWin=win;
          if (params->test_p83==0) //if set, no terminal winrate is used, as two passes leed to wrong winrates 
            hasTerminalWinrate=true;
          if (params->debug_on)
            params->engine->getGtpEngine()->getOutput()->printfDebug("New Terminal Result %d! (%s)\n",win,move.toString(params->board_size).c_str());
          if (!win && !this->isRoot() && parent->hasPrunedChildren())
          {
            parent->unPruneNow();
            while (parent->hasPrunedChildren() && !parent->hasOneUnprunedChildNotTerminalLoss())
              parent->unPruneNow(); //unprune a non-terminal loss
          }
        }
      }
      if (lock!=NULL)
        delete lock;
    }
  }
  else if (this->isTerminal() && !this->isTerminalResult())
  {
    hasTerminalWin=win;
    if (params->test_p83==0) //if set, no terminal winrate is used, as two passes leed to wrong winrates 
      hasTerminalWinrate=true;
  }
  
  if (!this->isRoot())
  {
    //assume alternating colours going up
    if (win)
      parent->addLose(fscore,this);
    else
      parent->addWin(fscore,this);
  }
}

float Tree::getVal(bool skiprave) const
{
  if (this->isTerminalResult())
    return (hasTerminalWin?1:0);
  else if (this->isTerminal())
    return 1;
  if (params->rave_moves==0 || raveplayouts==0 || skiprave)
    return this->getRatio();
  else if (playouts==0)
    return this->getRAVERatio();
  else
  {
    //float alpha=(float)(ravemoves-playouts)/params->rave_moves;

    float alpha=(float)raveplayouts/(raveplayouts + playouts + (float)(raveplayouts*playouts)/params->rave_moves);
    alpha=pow(alpha,params->test_p60);
    //float alpha=exp(-pow((float)playouts/params->rave_moves,params->test_p1));
    //float alpha=params->test_p2/(exp(pow((float)playouts/params->rave_moves,params->test_p1))+params->test_p2);
    //if (alpha<0.5)
    //  fprintf(stderr,"%5.0f %5.0f %5.2f\n",raveplayouts,playouts,alpha);
    //float alpha=exp(-pow((float)playouts/params->rave_moves,params->test_p1));
    //float alpha=1.0/pow(playouts*params->test_p6+params->test_p7,params->test_p8);
    float rave_here=(1-params->test_p26)*this->getRAVERatio()+params->test_p26*this->getRAVERatioOther();
    if (raveplayoutsother==ravewinsother)
      rave_here=this->getRAVERatio();  //in this case the other move is probably not allowed (only rave_init_wins)
    if (raveplayoutsother==0)
      rave_here=this->getRAVERatio();
    if (alpha<=0)
      return this->getRatio();
    else if (alpha>=1)
      return rave_here;
    else
      //return this->getEARLYRAVERatio()*alpha + this->getRatio()*(1-alpha);
      return rave_here*alpha + this->getRatio()*(1-alpha);
  }
}

float Tree::KL_xLogx_y(float x, float y) const
{
  if (x>0 && y==0)
    return FLT_MAX/10.0;
  if (x==0)
    return 0;
  return x*log(x/y);
}

float Tree::KL_d(float p, float q) const
{
  return KL_xLogx_y(p,q)+KL_xLogx_y(1.0-p,1.0-q);
}

float Tree::KL_max_q(float S, float N, float t) const
{
  static float c=0;
  if (S<0) S=0;
  if (S>N) S=N;
  if (N==0) return 1.0;  //this should not happen?!
  float q_min=S/N;
  float q_max=1.0;
  for (int i=0;i<15;i++)
  {
    float q=(q_min+q_max)/2.0;
    if (N*KL_d(S/N,q)<=log(t)+ c*log(log(t)))
      q_min=q;
    else
      q_max=q;
  }
  return q_min;
}

float Tree::getUrgency(bool skiprave, Tree * robustchild) const
{
  float uctbias;
  //handling was not correct I think
  if (this->isTerminalResult()) //this->isTerminal())
  {
    if (this->isTerminalWin())// || !this->isTerminalResult())
      return TREE_TERMINAL_URGENCY;
    else
      return -TREE_TERMINAL_URGENCY;
  }
  
  if (playouts==0 && raveplayouts==0)
    return params->ucb_init;

  if (parent==NULL || params->ucb_c==0)
    uctbias=0;
  else
  {
    if (params->test_p77>0) {
      if (parent->getPlayouts()>1 && playouts>0)
        uctbias=params->test_p77*sqrt(pow((float)parent->getPlayouts()-1,params->test_p78)/(playouts));
      else if (parent->getPlayouts()>1)
        uctbias=params->test_p77*sqrt(pow((float)parent->getPlayouts()-1,params->test_p78)/(1));
      else
        uctbias=params->test_p77/2;
    }
    else {
      if (parent->getPlayouts()>1 && playouts>0)
        uctbias=params->ucb_c*sqrt(((params->test_p120>0)?pow((float)parent->getPlayouts(),params->test_p120):log((float)parent->getPlayouts()))/(playouts));
      else if (parent->getPlayouts()>1)
        uctbias=params->ucb_c*sqrt(((params->test_p120>0)?pow((float)parent->getPlayouts(),params->test_p120):log((float)parent->getPlayouts()))/(1));
      else
        uctbias=params->ucb_c/2;
    }
  }

  if (params->bernoulli_a>0)
    uctbias+=params->bernoulli_a*exp(-params->bernoulli_b*playouts);

  float val=this->getVal(skiprave)+uctbias;
  if (params->test_p92>0) {  //ucb_c in this is test_p92, later there might be a good turning over to old style?!
    float save_playouts=playouts; if (save_playouts<=0) save_playouts=1;
    float save_raveplayouts=raveplayouts; if (save_raveplayouts<=0) save_raveplayouts=1;
    float save_parent_playouts=(float)parent->getPlayouts(); if (save_parent_playouts<=0) save_parent_playouts=1;

    float aya_UCB=wins/save_playouts+params->test_p92*sqrt(log(save_parent_playouts)/save_playouts);
    float aya_RAVE=ravewins/save_raveplayouts + params->test_p94*sqrt(log(save_parent_playouts*175)/(save_parent_playouts*0.48));
    float aya_beta=save_raveplayouts/(save_raveplayouts+save_playouts*(1.0/params->test_p95 + (1.0/params->test_p93)*save_raveplayouts));

    val=aya_beta*aya_RAVE + (1-aya_beta)*aya_UCB; //G*log(1+gamma) is handeled below
  }
  if (params->KL_ucb_enabled)
  {
    if (parent!=NULL && parent->getPlayouts()>0 && playouts>0) //otherwize only old val used! Not correct but safe?!
      val=KL_max_q(this->getVal(skiprave)*playouts,playouts,parent->getPlayouts());
  }
  if (params->weight_score>0)
    val+=params->weight_score*this->getScoreMean();

  if (params->uct_progressive_bias_enabled && params->test_p38==0) //p38 turns this off, as it is replaced by pachi style bias
  {
    if (params->test_p44==0 || (parent!=NULL && parent->getPlayouts()==0))
      val+=this->getProgressiveBias();
    else
    {
      float tmp=0;
      float playouts_use=(parent!=NULL)?parent->getPlayouts():0;
      if (params->test_p65>0 && robustchild!=NULL)
      {
        playouts_use=robustchild->getPlayouts();
      }
      if (params->test_p57>0)
      {
        tmp=log(this->getTreeResultsUnpruneFactor());
        tmp=params->test_p57*((tmp-exp(params->test_p56)>0)?tmp-exp(params->test_p56):0);
      }
      if (params->test_p62 >0)
      {
        tmp+=params->test_p62*exp(-params->test_p63*pow( this->getOwnership() - params->test_p64 ,2));
      }
      if (params->test_p90>0) {
          val+=(log(1000.0*(this->getProgressiveBias()+1.0))+params->test_p49*this->getCriticality ()
            +tmp)*(params->test_p44*(pow(params->test_p90/(params->test_p90+playouts_use),params->test_p51)));
        }
        else {
          //the factor 1000.0 is as a additive constant to log gamma. Should be a parameter, as it is not covered by other parameters in this formula
          val+=(log(1000.0*(this->getProgressiveBias()+1.0))+params->test_p49*this->getCriticality ()
            +tmp)*(params->test_p44/(pow(playouts_use,params->test_p51)+1.0));
        }
      if (params->test_p98>0 && this->getProgressiveBias()>0) {
        val+=params->test_p98*(sqrt(log(playouts_use)/(playouts+1)))*log(gamma+1.0);
      }
      //if (1000.0*(this->getProgressiveBias()+1.0) < 0) //wanted to test for nan, but did not work out
      if (std::isnan(val)) //wanted to test for nan, this test does not work with -ffast-math enabled!!!!
          fprintf(stderr,"this should not happen, urgency nan\n");
      if (params->debug_on)
          fprintf(stderr,"CalcUrg val: %f criticality %f playouts_use %f PW: %f %f %f %f\n",val,this->getCriticality(),playouts_use,this->getProgressiveBias(),1000.0*this->getProgressiveBias()+1.0,log(1000.0*this->getProgressiveBias()+1.0),params->test_p44/(pow(playouts_use,params->test_p51)+1.0));
      //val+=(log(this->getProgressiveBias()+0.1)-log(0.1)+params->test_p49*this->getCriticality ())*(params->test_p44/(parent->getPlayouts()+1.0));
    }
  }

  if (params->test_p38>0) //progressive bias as pre wins and games, comes closer to pachi style
  {
    //float pre_games=1;
    //float pre_wins=1;
    float sign=-1;
    if (gamma>1.0)
    {
      sign=1;
      //pre_wins=params->test_p38*pow(log(gamma),params->uct_progressive_bias_exponent);
      //pre_games=pre_wins;
    }
    else if (gamma>0.0)
    {
      //pre_wins=0;
      //pre_games=params->test_p38*(-pow(log(gamma),params->uct_progressive_bias_exponent));
    }
    val=//(params->test_p37/2.0+pre_wins+playouts*this->getVal(skiprave))/(params->test_p37+pre_games+playouts)

      this->getVal(skiprave)+10.0+
      sign*pow(fabs(log(gamma))*params->test_p38,params->uct_progressive_bias_exponent)/pow(playouts+1,params->test_p37)+
      exp(-params->test_p39*playouts)+ //first moves after unprunning need to be played first, otherwize the ucb takes long?!
          +uctbias + params->weight_score*this->getScoreMean() + biasbonus/pow(playouts+1,params->test_p37);
  }
  
  
  //risc penalty
  if (params->test_p3>0)
    val-=params->test_p3*this->getScoreSD()/params->board_size/params->board_size;
  
  if (params->uct_criticality_urgency_factor>0 && ((params->uct_criticality_siblings||params->test_p25>0)?parent->playouts:playouts)>(params->uct_criticality_min_playouts))
  {
    if (params->uct_criticality_urgency_decay>0.0)
      val+=params->uct_criticality_urgency_factor*this->getCriticality()*exp(-(float)playouts*params->uct_criticality_urgency_decay);// /pow((playouts+1),params->uct_criticality_urgency_decay); //added log 01.1.2013
    else
      val+=params->uct_criticality_urgency_factor*this->getCriticality();
  }

  if (params->test_p79>0)
    val+=params->test_p79*getUrgencyVariance();
  
  return val+1.0;  //avoid negative urgency!!!
}

std::list<Go::Move> Tree::getMovesFromRoot() const
{
  if (this->isRoot())
    return std::list<Go::Move>();
  else
  {
    std::list<Go::Move> list=parent->getMovesFromRoot();
    list.push_back(this->getMove());
    return list;
  }
}

bool Tree::isTerminal() const
{
  if (terminaloverride)
    return false;
  else if (hasTerminalWinrate)
    return true;
  else if (!this->isRoot())
  {
    // only tree with two passes does not guarantee, that it is terminal!
    // at least the random playout should be finished, otherwize no scoring possible!
        
    if (this->getMove().isPass() && parent->getMove().isPass())
      return true;
    else
      return false;
  }
  else
    return false;
}

void Tree::divorceChild(Tree *child)
{
  children->remove(child);
  child->parent=NULL;
}

float Tree::variance(int wins, int playouts)
{
  return wins-(float)(wins*wins)/playouts;
}

std::string Tree::toSGFString() const
{
  std::ostringstream ss;
  if (!this->isRoot())
  {
    ss<<"(;"<<Go::colorToChar(move.getColor())<<"[";
    if (!move.isPass()&&!move.isResign())
    {
      ss<<(char)(move.getX(params->board_size)+'a');
      ss<<(char)(params->board_size-move.getY(params->board_size)+'a'-1);
    }
    else if (move.isPass())
      ss<<"pass";
    ss<<"]C[";
    if (move.isPass())
      ss<<"PASS !!!!!!\n";
    if (this->isTerminalWin())
      ss<<"Terminal Win ("<<wins<<","<<hasTerminalWin<<")\n";
    else if (this->isTerminalLose())
      ss<<"Terminal Lose ("<<wins<<","<<hasTerminalWin<<")\n";
    else
    {
      ss<<"Wins/Playouts: "<<wins<<"/"<<playouts<<"("<<((playouts>0)?wins/playouts:0)<<")\n";
      if (params->uct_decay_alpha!=1 || params->uct_decay_k!=0)
        ss<<"Decayed Playouts: "<<decayedplayouts<<"\n";
      if (params->test_p75>0)
        ss<<"backup wins/playouts: "<<bestLCBwins<<"/"<<bestLCBplayouts<<"("<<((bestLCBplayouts>0)?bestLCBwins/bestLCBplayouts:0)<<") gR:"<<this->getRatio()<<"\n";
      ss<<"RAVE Wins/Playouts: "<<ravewins<<"/"<<raveplayouts<<"("<<this->getRAVERatio()<<")\n";
      ss<<"Score Mean: "<<scoresum<<"/"<<playouts<<"("<<this->getScoreMean()<<")\n";
      ss<<"Score SD: "<<this->getScoreSD()<<"\n";
    }
    ss<<"Urgency: "<<this->getUrgency()<<"\n";
    ss<<"UrgencyVar: "<<this->getUrgencyVariance()<<"\n";
    if (!this->isLeaf())
    {
        //ss<<"Pruned: "<<prunedchildren<<"/"<<children->size()<<"("<<(children->size()-prunedchildren)<<")\n";
      UrgentNode urgenttmp;
      std::list <UrgentNode> urgentlist;
      for(std::list<Tree*>::iterator iter=children->begin();iter!=children->end();++iter) 
      {
        //ss<<(*iter)->getMove().toString(params->board_size)<<" UPF"<<(*iter)->getUnPruneFactor()<<" gamma"<<(*iter)->getFeatureGamma()<<" rave"<<(*iter)->getRAVERatio()<<"other "<<(*iter)->getRAVERatioOther()<<" crit"<<(*iter)->getCriticality()<<"\n";
        ss<<(*iter)->getMove().toString(params->board_size)<<((*iter)->isPruned()?" ":"*")<<" UPF:"<<(*iter)->getUnPruneFactor()<<" rave"<<(*iter)->getRAVERatio()<<"("<<(*iter)->getRAVEWins()<<"/"<<(*iter)->getRAVEPlayouts()<<") "<<
          params->uct_rave_unprune_factor*((((*iter)->getRAVERatio()-(1.0-this->getRatio()))*(*iter)->getRAVEPlayouts()/((*iter)->getRAVEPlayouts()+params->test_p24))+1)<<" playouts" << (*iter)->getPlayouts()<<" urgency "<<(*iter)->getUrgency()<<"\n";
        if (!(*iter)->isPruned() && params->test_p75>0)
        {
          urgenttmp.urgency=0;
          urgenttmp.node=(*iter);
          urgenttmp.wins=(*iter)->getWins();
          urgenttmp.playouts=(*iter)->getPlayouts();
          urgenttmp.bestLCBplayouts=(*iter)->getBestLCBPlayouts();
          urgenttmp.bestLCBwins=(*iter)->getBestLCBWins();
          urgenttmp.bestLCBconst=params->test_p75;
          urgenttmp.k_confidence=params->test_p76;
          urgentlist.push_back(urgenttmp);
        }
      
      }
      if (params->test_p75>0 && urgentlist.size()>0)
        {
          int count_used=0;
          int count_node=0;
          urgentlist.sort(Tree::compare_UrgentNodes_LCB);
          float bestLCB=-100; //should be changed to -1.0 or so, but not now!! Should not matter to much, also bestLCB_tmp ändern!!!
    
          double bestLCBwins_tmp=0;
          double bestLCBplayouts_tmp=0;

          double bestLCBwins_t=0;
          double bestLCBplayouts_t=0;

          float bestLCB_tmp;
          for (std::list<UrgentNode>::iterator iter=urgentlist.begin();iter!=urgentlist.end();++iter)
          {
            float LCB=LCB_UrgentNode ((*iter));
            wins_playouts use=LCB_UrgentNode_winplayouts({(*iter).bestLCBwins,(*iter).bestLCBplayouts},{(*iter).wins,(*iter).playouts},params->test_p76);
            //if (!LCB_UrgentNode_useWins(*iter)) //(*iter).bestLCBplayouts>0 && (*iter).bestLCBplayouts>0)
            //{
            //  bestLCBwins_tmp+=(*iter).bestLCBwins;
            //  bestLCBplayouts_tmp+=(*iter).bestLCBplayouts;
            //}
            //else
            //{
            //  bestLCBwins_tmp+=(*iter).wins;
            //  bestLCBplayouts_tmp+=(*iter).playouts;
            //}
            bestLCBwins_tmp+=use.wins;
            bestLCBplayouts_tmp+=use.playouts;
            bestLCB_tmp=-10;
            if (bestLCBplayouts_tmp>0 && bestLCBwins>0) bestLCB_tmp=(bestLCBwins_tmp-params->test_p75*sqrt(bestLCBplayouts_tmp))/bestLCBplayouts_tmp;
            //else break;
            bool mark=false;
            if (!(bestLCB_tmp<bestLCB))
            {
              ss<<"   used: ("<<use.wins<<"/"<<use.playouts<<")"<<use.wins/(use.playouts+0.0000000001)<<"\n";
              bestLCB=bestLCB_tmp;
              bestLCBwins_t=bestLCBwins_tmp;
              bestLCBplayouts_t=bestLCBplayouts_tmp;
              count_used++;
              mark=true;
            }
            count_node++;
            ss<<count_node<<((mark)?"*":".")<<(*iter).node->getMove().toString(params->board_size).c_str()<< 
              "w:"<<(*iter).wins<<"/"<<(*iter).playouts<<"("<<(*iter).wins/(*iter).playouts<<") "<<
              "wbLBC:"<<(*iter).bestLCBwins<<"/"<<(*iter).bestLCBplayouts<<"("<<(*iter).bestLCBwins/(*iter).bestLCBplayouts<<") "<<
              "LBC:"<<LCB<<"\n   recalc bLCB:"<<bestLCB_tmp<<" "<<bestLCBwins_t<<"/"<<bestLCBplayouts_t<<"("<<bestLCBwins_t/bestLCBplayouts_t<<")\n";
          }
          ss<<"recalc bLCB "<<bestLCBwins_t<<"/"<<bestLCBplayouts_t<<"("<<bestLCBwins_t/bestLCBplayouts_t<<") used:"<<count_used<<" self:"<<1.0-bestLCBwins_t/bestLCBplayouts_t;
        }
    }
    else if (this->isTerminal())
      ss<<"Terminal Node\n";
    else
      ss<<"Leaf Node\n";
    ss<<"]";
  }
  else
  {
    ss<<"C[";
    if (!move.isResign())
      ss<<"Last Move: "<<move.toString(params->board_size)<<"\n";
    if (move.isPass())
      ss<<"PASS !!!!!!\n";
    ss<<"Children: "<<children->size()<<"\n";
    if (this->isTerminalWin())
      ss<<"Terminal Win\n";
    else if (this->isTerminalLose())
      ss<<"Terminal Lose\n";
    if (this->isSuperkoViolation())
      ss<<"Superko Violation!\n";
    ss<<"Wins/Playouts: "<<wins<<"/"<<playouts<<"("<<wins/playouts<<")\n";
    if (params->uct_decay_alpha!=1 || params->uct_decay_k!=0)
      ss<<"Decayed Playouts: "<<decayedplayouts<<"\n";
    if (params->test_p75>0)
      ss<<"backup wins/playouts: "<<bestLCBwins<<"/"<<bestLCBplayouts<<"("<<((bestLCBplayouts>0)?bestLCBwins/bestLCBplayouts:0)<<") gR:"<<this->getRatio()<<"\n";
    ss<<"Score Mean: "<<scoresum<<"/"<<playouts<<"("<<this->getScoreMean()<<")\n";
    ss<<"Score SD: "<<"("<<this->getScoreSD()<<")\n";
    if (params->uct_decay_alpha!=1 || params->uct_decay_k!=0)
        ss<<"Decayed Playouts: "<<decayedplayouts<<"\n";
    if (!this->isLeaf())
      {
        //ss<<"Pruned: "<<prunedchildren<<"/"<<children->size()<<"("<<(children->size()-prunedchildren)<<")\n";
      UrgentNode urgenttmp;
      std::list <UrgentNode> urgentlist;
      for(std::list<Tree*>::iterator iter=children->begin();iter!=children->end();++iter) 
      {
        //ss<<(*iter)->getMove().toString(params->board_size)<<" UPF"<<(*iter)->getUnPruneFactor()<<" gamma"<<(*iter)->getFeatureGamma()<<" rave"<<(*iter)->getRAVERatio()<<"other "<<(*iter)->getRAVERatioOther()<<" crit"<<(*iter)->getCriticality()<<"\n";
        ss<<(*iter)->getMove().toString(params->board_size)<<((*iter)->isPruned()?" ":"*")<<" UPF:"<<(*iter)->getUnPruneFactor()<<" rave"<<(*iter)->getRAVERatio()<<"("<<(*iter)->getRAVEWins()<<"/"<<(*iter)->getRAVEPlayouts()<<") "<<
          params->uct_rave_unprune_factor*((((*iter)->getRAVERatio()-(1.0-this->getRatio()))*(*iter)->getRAVEPlayouts()/((*iter)->getRAVEPlayouts()+params->test_p24))+1)<<" playouts" << (*iter)->getPlayouts()<<" urgency "<<(*iter)->getUrgency()<<"\n";
        if (!(*iter)->isPruned() && params->test_p75>0)
        {
          urgenttmp.urgency=0;
          urgenttmp.node=(*iter);
          urgenttmp.wins=(*iter)->getWins();
          urgenttmp.playouts=(*iter)->getPlayouts();
          urgenttmp.bestLCBplayouts=(*iter)->getBestLCBPlayouts();
          urgenttmp.bestLCBwins=(*iter)->getBestLCBWins();
          urgenttmp.bestLCBconst=params->test_p75;
          urgenttmp.k_confidence=params->test_p76;
          urgentlist.push_back(urgenttmp);
        }
      
      }
      if (params->test_p75>0 && urgentlist.size()>0)
        {
          int count_used=0;
          int count_node=0;
          urgentlist.sort(Tree::compare_UrgentNodes_LCB);
          float bestLCB=-100;
    
          double bestLCBwins_tmp=0;
          double bestLCBplayouts_tmp=0;

          double bestLCBwins_t=0;
          double bestLCBplayouts_t=0;

          float bestLCB_tmp;
          for (std::list<UrgentNode>::iterator iter=urgentlist.begin();iter!=urgentlist.end();++iter)
          {
            float LCB=LCB_UrgentNode ((*iter));
            wins_playouts use=LCB_UrgentNode_winplayouts({(*iter).bestLCBwins,(*iter).bestLCBplayouts},{(*iter).wins,(*iter).playouts},params->test_p76);
            //if (!LCB_UrgentNode_useWins(*iter)) //(*iter).bestLCBplayouts>0 && (*iter).bestLCBplayouts>0)
            //{
            //  bestLCBwins_tmp+=(*iter).bestLCBwins;
            //  bestLCBplayouts_tmp+=(*iter).bestLCBplayouts;
            //}
            //else
            //{
            //  bestLCBwins_tmp+=(*iter).wins;
            //  bestLCBplayouts_tmp+=(*iter).playouts;
            //}
            bestLCBwins_tmp+=use.wins;
            bestLCBplayouts_tmp+=use.playouts;
            bestLCB_tmp=-10;
            if (bestLCBplayouts_tmp>0) bestLCB_tmp=(bestLCBwins_tmp-params->test_p75*sqrt(bestLCBplayouts_tmp))/bestLCBplayouts_tmp;
            //else break;
            bool mark=false;
            if (!(bestLCB_tmp<bestLCB))
            {
              bestLCB=bestLCB_tmp;
              bestLCBwins_t=bestLCBwins_tmp;
              bestLCBplayouts_t=bestLCBplayouts_tmp;
              count_used++;
              mark=true;
            }
            count_node++;
            ss<<count_node<<((mark)?"*":".")<<(*iter).node->getMove().toString(params->board_size).c_str()<< 
              "w:"<<(*iter).wins<<"/"<<(*iter).playouts<<"("<<(*iter).wins/(*iter).playouts<<") "<<
              "wbLBC:"<<(*iter).bestLCBwins<<"/"<<(*iter).bestLCBplayouts<<"("<<(*iter).bestLCBwins/(*iter).bestLCBplayouts<<") "<<
              "LBC:"<<LCB<<
              "\n";
          }
          ss<<"recalc bLCB "<<bestLCBwins_t<<"/"<<bestLCBplayouts_t<<"("<<bestLCBwins_t/bestLCBplayouts_t<<") used:"<<count_used<<" self:"<<1.0-bestLCBwins_t/bestLCBplayouts_t;
        }
      }
      else if (this->isTerminal())
        ss<<"Terminal Node\n";
      else
      ss<<"Leaf Node\n";
    ss<<"]";
  }
  
  Tree *bestchild=NULL;
  int besti=0;
  bool usedchild[children->size()];
  for (unsigned int i=0;i<children->size();i++)
    usedchild[i]=false;
  int childrendone=0;
  
  while (true)
  {
    int i=0;
    for(std::list<Tree*>::iterator iter=children->begin();iter!=children->end();++iter) 
    {
      if (usedchild[i]==false && ((*iter)->getPlayouts()>0 || (*iter)->isTerminal() || (params->outputsgf_maxchildren==0 && (*iter)->isPrimary())) && !(*iter)->isSuperkoViolation())
      {
        if (bestchild==NULL || (*iter)->getPlayouts()>bestchild->getPlayouts() || (*iter)->isTerminalWin())
        {
          bestchild=(*iter);
          besti=i;
          if ((*iter)->isTerminalWin())
            break;
        }
      }
      i++;
    }
    if (bestchild==NULL)
      break;
    
    ss<<bestchild->toSGFString();
    usedchild[besti]=true;
    bestchild=NULL;
    besti=0;
    childrendone++;
    if (params->outputsgf_maxchildren!=0 && childrendone>=params->outputsgf_maxchildren)
      break;
  }
  if (!this->isRoot())
    ss<<")\n";
  return ss.str();
}

void Tree::performSymmetryTransform(Go::Board::SymmetryTransform trans)
{
  if (!move.isPass() && !move.isResign())
    move.setPosition(Go::Board::doSymmetryTransformStaticReverse(trans,params->board_size,move.getPosition()));
  
  for(std::list<Tree*>::iterator iter=children->begin();iter!=children->end();++iter) 
  {
    (*iter)->performSymmetryTransform(trans);
  }
}

void Tree::performSymmetryTransformParentPrimary()
{
  if (!this->isRoot() && !this->isPrimary())
  {
    Go::Board::SymmetryTransform trans=Go::Board::getSymmetryTransformBetweenPositions(params->board_size,move.getPosition(),symmetryprimary->getMove().getPosition());
    //fprintf(stderr,"transform: ix:%d iy:%d sxy:%d\n",trans.invertX,trans.invertY,trans.swapXY);
    parent->performSymmetryTransform(trans);
  }
}

void Tree::pruneChildren()
{
  prunedchildren=0;
  for(std::list<Tree*>::iterator iter=children->begin();iter!=children->end();++iter) 
  {
    if ((*iter)->isPrimary())
    {
      (*iter)->setPruned(true);
      prunedchildren++;
    }
  }
  unprunedchildren=0;
}

Tree *Tree::getWorstChild()
{
  //fprintf(stderr,"+");
  Tree *worstchild=NULL;
  int unprunedn=0;
  for(std::list<Tree*>::iterator iter=children->begin();iter!=children->end();++iter) 
  {
    //fprintf(stderr,".");
    if ((*iter)->isPrimary() && !(*iter)->isPruned())
    {
      if (worstchild==NULL || (*iter)->getPlayouts()<worstchild->getPlayouts())
        worstchild=(*iter);
      unprunedn++;
    }
  }
  if (unprunedn<2)
    return NULL;
  return worstchild;
}

void Tree::unPruneNextChildNew()
{
  if (params->uct_reprune_factor>0.0)
  {
    Tree* worstChild=NULL;
    worstChild=getWorstChild();
    if (worstChild)
    {
      if (this->isRoot())
      {
      //  Gtp::Engine *gtpe=params->engine->getGtpEngine();
      //  gtpe->getOutput()->printfDebug("wc:%s %5.3f %5.3f (%5.0f) %5.3f\n",worstChild->getMove().toString(params->board_size).c_str(),worstChild->getUnPruneFactor(),worstChild->getRAVERatio(),worstChild->getRAVEPlayouts(),worstChild->getFeatureGamma());
      }
      worstChild->setPruned(true);
      unprunedchildren--;
      unPruneNextChild();
    }
  }
  unPruneNextChild();
}

void Tree::unPruneNextChild()
{
//  Gtp::Engine *gtpe=params->engine->getGtpEngine();
  if (this->hasPrunedChildren())
  {
    Tree *bestchild=NULL;
    float bestfactor=-1;
    unsigned int unpruned=0;

    //moves of the same color two levels higher in the tree
    Tree *higher=this->getParent();
    if (higher) higher=higher->getParent();
    //if (higher) higher=higher->getParent();
    float *moveValues=NULL;
    int num=0;
    float sum=0;
    float mean=0;
    if (higher && params->uct_oldmove_unprune_factor_b>0.0)
    {
      //allocated here, as it would be a lot of memory if kept on every tree node!!
      moveValues=new float[params->engine->getCurrentBoard()->getPositionMax()];
      for (int i=0;i<params->engine->getCurrentBoard()->getPositionMax();i++)
        moveValues[i]=0;
      std::list<Tree*> *childrenTmp=higher->getChildren();
      for(std::list<Tree*>::iterator iter=childrenTmp->begin();iter!=childrenTmp->end();++iter) 
      { 
        if (!(*iter)->isPruned() && ((*iter)->getMove()).isNormal())
        {
          num++;
          sum+=(*iter)->getRatio();
          moveValues[((*iter)->getMove()).getPosition()]=(*iter)->getRatio();
        }
      }
      if (num>0)
        mean=sum/num;
    }
    //float unprune_select_p=(float)rand()/RAND_MAX;
    float local_prob=0;
    if (params->test_p23 > 0) local_prob=(float)rand()/RAND_MAX;
    if (params->test_p89 >0) {
    int countit=0;
    Tree *tocount=this->getParent();
      while (tocount!=NULL && tocount->getParent()!=NULL) {
        countit++;
        tocount=tocount->getParent();
      }
      if (countit>0) {
        local_prob=params->test_p89 * countit;
        if (local_prob>1) local_prob=1;
      }
    }
    for(std::list<Tree*>::iterator iter=children->begin();iter!=children->end();++iter) 
    {
      if ((*iter)->isPrimary() && !(*iter)->isSuperkoViolation())
      {
        if ((*iter)->isPruned())
        {
          if ((*iter)->getUnPruneFactor(moveValues,mean,num,local_prob)>bestfactor || bestchild==NULL)
          {
            bestfactor=(*iter)->getUnPruneFactor(moveValues,mean,num,local_prob);
            bestchild=(*iter);
          }
          //  fprintf(stderr,"%s %5.3f %5.3f (%5.0f) %5.3f",(*iter)->getMove().toString(params->board_size).c_str(),(*iter)->getUnPruneFactor(),(*iter)->getRAVERatio(),(*iter)->getRAVEPlayouts(),(*iter)->getFeatureGamma());
          // Following is commented out because is cannot be disabled
          /*if (unprune_select_p<params->test_p9 || this->getNumUnprunedChildren()<3)
          {
            if ((*iter)->getUnPruneFactor()>bestfactor || bestchild==NULL)
            {
              bestfactor=(*iter)->getUnPruneFactor(moveValues,mean,num);
              bestchild=(*iter);
            }
          }
          else
          {
            if (unprune_select_p<(1.0-params->test_p9)*params->test_p10+params->test_p9)
            {
              if ((*iter)->getRAVERatio()>bestfactor || bestchild==NULL)
              {
                bestfactor=(*iter)->getRAVERatio();
                bestchild=(*iter);
              }
            }
            else
            {
              if ((*iter)->getCriticality()>bestfactor || bestchild==NULL)
              {
                bestfactor=(*iter)->getCriticality();
                bestchild=(*iter);
              }
            }
          }*/
        }
        else
        {
          unpruned++;
          //if (this->isRoot())
          //  fprintf(stderr,"%s %5.3f %5.3f (%5.0f) %5.3f ",(*iter)->getMove().toString(params->board_size).c_str(),(*iter)->getUnPruneFactor(),(*iter)->getRAVERatio(),(*iter)->getRAVEPlayouts(),(*iter)->getFeatureGamma());
        }
      }
    }
    //if (this->isRoot())
    //  fprintf(stderr,"unpruned %d\n",unpruned);
    
    if (bestchild!=NULL)
    {
      //if (unpruned>2)
      //  fprintf(stderr,"\n[unpruning]: (%d) %s bestfactor %f Rave %f EarlyRave %f Criticality %f -- %f\n\n",unpruned,bestchild->getMove().toString(params->board_size).c_str(),bestfactor,bestchild->getRAVERatio (),bestchild->getEARLYRAVERatio (),bestchild->getCriticality(), bestchild->getUnPruneFactor ());
      bestchild->setPruned(false);
      //if (this->isRoot())
      //      params->engine->getGtpEngine()->getOutput()->printfDebug("nc:%s %5.3f %5.3f (%5.2f) %5.3f (ravepart: %5.3f)\n",bestchild->getMove().toString(params->board_size).c_str(),bestchild->getUnPruneFactor(),bestchild->getRAVERatio(),bestchild->getRAVEPlayouts(),bestchild->getFeatureGamma(),params->uct_rave_unprune_factor*(((bestchild->getRAVERatio()-(1.0-this->getRatio()))*bestchild->getRAVEPlayouts()/(bestchild->getRAVEPlayouts()+params->test_p24))+1));
      if ((unpruned+superkochildrenviolations)!=unprunedchildren)
        fprintf(stderr,"WARNING! unpruned running total doesn't match (%u:%u)\n",unpruned,unprunedchildren);
      bestchild->setUnprunedNum(unpruned+1);
      unprunedchildren++;
      float correct_b=childrenlogtotalchildgamma;//(childrenlogtotalchildgamma-params->test_p67>0)?childrenlogtotalchildgamma-params->test_p67 : 0;
      unprunebase=params->uct_progressive_widening_a*pow(params->uct_progressive_widening_b-params->test_p66*correct_b,unpruned);
      lastunprune=this->unPruneMetric();
      this->updateUnPruneAt();
      prunedchildren--;
    }
    if (moveValues) delete moveValues;
  }
}

float Tree::unPruneMetric() const
{
  if (params->uct_progressive_widening_count_wins)
    return wins;
  else
    return playouts;
}

void Tree::updateUnPruneAt()
{
  float scale;
  if (playouts>0)
    scale=(1-params->uct_progressive_widening_c*this->getRatio());
  else
    scale=1;
  float bestfactor=-1;
  //float worstfactor=1e20;
  float sumunpruned=0;
  int   numunpruned=0;
  float local_prob=0;
  if (params->test_p23 > 0) local_prob=(float)rand()/RAND_MAX;
  if (params->test_p89 >0) {
    int countit=0;
    Tree *tocount=this->getParent();
    while (tocount!=NULL && tocount->getParent()!=NULL) {
      countit++;
      tocount=tocount->getParent();
    }
    if (countit>0) {
      local_prob=params->test_p89 * countit;
      if (local_prob>1) local_prob=1;
    }
  }
  if (params->test_p16>0 || params->test_p58>0)
  {
    for(std::list<Tree*>::iterator iter=children->begin();iter!=children->end();++iter) 
    {
      if ((*iter)->isPrimary() && !(*iter)->isSuperkoViolation())
      {
        if ((*iter)->isPruned())
        {
          if ((*iter)->getUnPruneFactor(NULL,0,0,local_prob)>bestfactor)
            bestfactor=(*iter)->getUnPruneFactor(NULL,0,0,local_prob);
        }
        else
        {
          sumunpruned+=(*iter)->getUnPruneFactor(NULL,0,0,local_prob);
          numunpruned++;
          //if ((*iter)->getUnPruneFactor()<worstfactor)
          //  worstfactor=(*iter)->getUnPruneFactor();
        }
      }
    }
  }
  float DistanceToWorst=1;
  //if (bestfactor>0 && worstfactor<1e20 && worstfactor>bestfactor)
  //{
  //  DistanceToWorst=worstfactor/bestfactor;
  //  DistanceToWorst=pow(DistanceToWorst,params->test_p16);
    //DistanceToWorst=(DistanceToWorst-1.0)*params->test_p16+1.0;
  //}
  if (bestfactor>0 && numunpruned>0)
  {
    DistanceToWorst=sumunpruned/numunpruned/bestfactor;
    if (DistanceToWorst<1.0)
      DistanceToWorst=1.0;
    else
      DistanceToWorst=(DistanceToWorst-1.0)*params->test_p16+1.0;
  }
  if (params->test_p58>0)
    unprunenextchildat=lastunprune+unprunebase*scale*(1-((bestfactor>1)?params->test_p58*(log(bestfactor)-log(1)):0)); //t(n+1)=t(n)+(a*b^n)*(1-c*p)
  else
    unprunenextchildat=lastunprune+unprunebase*scale*DistanceToWorst; //t(n+1)=t(n)+(a*b^n)*(1-c*p)
}

void Tree::checkForUnPruning()
{
  this->updateUnPruneAt();
  if (this->unPruneMetric()>=unprunenextchildat)
  {
    //boost::mutex::scoped_lock lock(unprunemutex);
    //unprunemutex.lock();
    if (!unprunemutex.try_lock())
      return;
    if (this->unPruneMetric()>=unprunenextchildat)
      this->unPruneNextChildNew();
    unprunemutex.unlock();
  }
}

void Tree::unPruneNow()
{
  float tmp=unprunenextchildat=this->unPruneMetric();
  //boost::mutex::scoped_lock lock(unprunemutex);
  unprunemutex.lock();
  if (tmp==unprunenextchildat)
    this->unPruneNextChildNew();
  unprunemutex.unlock();
}

float Tree::getTreeResultsUnpruneFactor() const
{
  if (parent==NULL) return 0;
  if (eq_moves==NULL) return 0;
  tree_result result={0,0,0,0};
  eq_moves->lock();
  for(std::set<Tree*>::iterator iter=eq_moves->begin();iter!=eq_moves->end();++iter) 
     {
       if ((*iter)==this) break;
       Tree* t_tmp=(*iter)->getParent();
       if (t_tmp==NULL) break;
       EqMoves *eq_moves2=t_tmp->get_eq_moves();
       if (eq_moves2==NULL) break;
       if (!eq_moves2->try_lock()) break;
       bool found=false;
       for(std::set<Tree*>::iterator iter2=eq_moves2->begin();iter2!=eq_moves2->end();++iter2)
       {
         if ((*iter2)==this) {found=true; break;}
       }
       eq_moves2->unlock();
       tree_result tmp=(*iter)->getTreeResult();
       if (found) 
       {
         result.playouts+=tmp.playouts; result.parent_playouts+=tmp.parent_playouts;
       }
     }
  eq_moves->unlock();
  return (result.playouts+1)/(result.parent_playouts+20);     
}

float Tree::getUnPruneFactor(float *moveValues,float mean, int num, float prob_local) const
{
  if (params->cnn_weak_gamma>0) {
    //at the moment this disables all other handling!!! should only be used with pure CNN gammas....
    Tree *tmp=this->getParent();
    double parent_playouts=tmp->getPlayouts();
    while (!tmp->isRoot())
      tmp=tmp->getParent();
    double root_playouts=tmp->getPlayouts();
    if (parent_playouts/root_playouts < params->cnn_weak_gamma) 
      return gamma_weak;
    return gamma;
  }
  float local_part=1;
  if (params->test_p23 > 0 && prob_local > 0)
    local_part=(gamma_local_part-1.0)*(1.0-pow(prob_local,params->test_p23))+1.0;

  float factor=gamma/parent->getChildrenTotalFeatureGamma()/local_part;
  
  if (params->uct_rave_unprune_decay>0)
  {
    factor=log((1000.0*gamma/local_part*params->uct_rave_unprune_decay)/(parent->raveplayouts+params->uct_rave_unprune_decay)+1.0); 
    if (params->uct_rave_unprune_decay>=1000000) //to disable without changing to old factor...
      factor=log((1000.0*gamma/local_part)+1.0);
    //ELO tests
    //
    // factor=gamma/parent->getChildrenTotalFeatureGamma()*exp(-parent->raveplayouts*params->uct_rave_unprune_decay);
    // -70ELO
    //factor=gamma/parent->getChildrenTotalFeatureGamma()*exp(-parent->playouts*params->uct_rave_unprune_decay);
    // -100ELO
    //factor=gamma/parent->getChildrenTotalFeatureGamma()/(parent->raveplayouts*params->uct_rave_unprune_decay+1);
    // -50ELO
    //factor=log(gamma+1.0)/(parent->raveplayouts*params->uct_rave_unprune_decay+1);
    // -20 ELO
    //factor=sqrt(gamma)/(parent->raveplayouts*params->uct_rave_unprune_decay+1);
    // -20ELO 
    //factor=sqrt(gamma)/(parent->playouts*params->uct_rave_unprune_decay+1);
    // -40ELO
    //factor=sqrt(gamma)/(parent->playouts*params->uct_rave_unprune_decay+10);
    // -30ELO
    //factor=pow(gamma,0.1)/(parent->raveplayouts*params->uct_rave_unprune_decay+1);
    // -20ELO
    //factor=pow(gamma,0.22)/(parent->raveplayouts*params->uct_rave_unprune_decay+1);
    // -15elo
    //factor=pow(gamma,0.5)/pow(parent->raveplayouts*params->uct_rave_unprune_decay+1,0.5);
    // -20ELO
    //factor=pow(gamma,params->test_p6)/(parent->raveplayouts*params->uct_rave_unprune_decay+1);
    
    //factor=pow(gamma,params->test_p6)*exp(-pow(parent->raveplayouts*params->uct_rave_unprune_decay,params->test_p7));
    //factor=pow(gamma,params->test_p6)*(params->test_p5+exp(-parent->raveplayouts/params->uct_rave_unprune_decay));
    //factor=pow(gamma,params->test_p6);
    
  }
  //fprintf(stderr,"unprunefactore %f %f %f\n",gamma,parent->raveplayouts,factor);
  if (params->test_p41>0)
    factor=pow(factor,params->test_p41);
  if (params->uct_criticality_unprune_factor>0 && ((params->uct_criticality_siblings||params->test_p25>0)?parent->playouts:playouts)>(params->uct_criticality_min_playouts))
  {
    if (params->uct_criticality_unprune_multiply)
      factor*=(1+params->uct_criticality_unprune_factor*this->getCriticality());
    else
      factor+=params->uct_criticality_unprune_factor*this->getCriticality();
  }
  if (params->uct_prior_unprune_factor>0)
  {
    //fprintf(stderr,"prior wins? %f %f %f\n",wins,playouts,this->getRatio());
    factor*=wins*params->uct_prior_unprune_factor;
  }
  if (params->uct_rave_unprune_factor>0 && this->getRAVEPlayouts()>0)
  {
    if (params->uct_rave_unprune_multiply)
      factor*=(1+params->uct_rave_unprune_factor*this->getRAVERatio());
    else
      factor+=params->uct_rave_unprune_factor*(((this->getRAVERatio()-(1.0-parent->getRatio()))*this->getRAVEPlayouts()/(this->getRAVEPlayouts()+params->test_p24))+1);
      //factor+=params->uct_rave_unprune_factor*(pmpow(this->getRAVERatio()-(1.0-parent->getRatio()),params->test_p40)+1);
   //    factor+=params->uct_rave_unprune_factor*this->getRAVERatio();
  }

  if (params->uct_rave_other_unprune_factor>0 && this->getRAVEPlayoutsOther()>1)
  {
    if (params->uct_rave_unprune_multiply)
    {
      if (raveplayoutsother!=ravewinsother) //otherwize probably not allowed move and the ratio comes from init_rave_wins
        factor*=(1+params->uct_rave_other_unprune_factor*this->getRAVERatioOther());
      else
        factor*=(1+params->uct_rave_other_unprune_factor*this->getRAVERatio());        
    }
    else
      if (raveplayoutsother!=ravewinsother) //otherwize probably not allowed move and the ratio comes from init_rave_wins
        factor+=params->uct_rave_other_unprune_factor*this->getRAVERatioOther();
      else
        factor+=params->uct_rave_other_unprune_factor*this->getRAVERatio();
  }


  factor+=params->uct_criticality_rave_unprune_factor*this->getRAVERatio()*this->getCriticality();

  float terrOwn=(params->test_p61>0)?this->getOwnership() : params->engine->getTerritoryMap()->getPositionOwner(move.getPosition());
  
  if (move.getColor()==Go::WHITE)
    terrOwn=-terrOwn;

  //tested version was ok with parameters
  //optimized_settings +='param uct_area_owner_factor_a 2.8\n'
  //optimized_settings +='param uct_area_owner_factor_b 0.1\n'
  //optimized_settings +='param uct_area_owner_factor_c 1.3\n'

  float StoneDensity=1.0+(float)stones_around*params->test_p20;
  
  if (params->test_p20==0.0) StoneDensity=1;
  
  factor+=StoneDensity*params->uct_area_owner_factor_a*exp(-pow(params->uct_area_owner_factor_c*(terrOwn-params->uct_area_owner_factor_b),2));

  //factor+=(params->uct_area_owner_factor_a+params->uct_area_owner_factor_b*terrOwn+params->uct_area_owner_factor_c*terrOwn*terrOwn)*exp(-pow(params->test_p1*terrOwn,2));
  float terrCovar=params->engine->getCorrelation(move.getPosition());
  if (terrCovar <0) terrCovar=0;
  factor+=params->test_p20*params->test_p1*terrCovar;
  
  if (params->uct_earlyrave_unprune_factor>0 && this->getEARLYRAVEPlayouts ()>1)
  {
    if (params->uct_rave_unprune_multiply)
      factor*=1+params->uct_earlyrave_unprune_factor*this->getEARLYRAVERatio();
    else
      factor+=params->uct_earlyrave_unprune_factor*this->getEARLYRAVERatio();
  }
  if (params->uct_reprune_factor>0.0)
    factor*=1.0/(1.0+log(1+params->uct_reprune_factor*playouts));

  if (params->uct_oldmove_unprune_factor>0.0 || params->uct_oldmove_unprune_factor_b>0.0)
  {
    if (moveValues!=NULL)
    {
      //use values in the tree
      if (move.isNormal() && moveValues[move.getPosition ()]-mean>0)
        factor*=1+(moveValues[move.getPosition ()]-mean)*params->uct_oldmove_unprune_factor_b*pow(num,params->uct_oldmove_unprune_factor_c);
    }
    //else
    {
      factor*=1+params->engine->getOldMoveValue(move)*params->uct_oldmove_unprune_factor; //use the old values, which are not in the tree anymore 
    }
  }

  if (params->test_p42>0)
    factor+=params->test_p42*params->engine->getAreaCorrelation(move);

  if (params->test_p55>0)
  {
    float tmp=log(this->getTreeResultsUnpruneFactor());
    factor+=params->test_p55*((tmp-exp(params->test_p56)>0)?tmp-exp(params->test_p56):0);
  }
  return factor;
}

void Tree::allowContinuedPlay()
{
  terminaloverride=true;
  hasTerminalWinrate=false;
}

Tree *Tree::getRobustChild(bool descend) const
{
  float bestsims=0;
  Tree *besttree=NULL;
  
  for(std::list<Tree*>::iterator iter=children->begin();iter!=children->end();++iter) 
  {
    if (!(*iter)->isTerminalLose() && !(*iter)->isSuperkoViolation() && ((*iter)->getPlayouts()>bestsims || (*iter)->isTerminalWin() || besttree==NULL))
    {
      besttree=(*iter);
      bestsims=(*iter)->getPlayouts();
      if (besttree->isTerminalWin())
        break;
    }
  }
  
  if (besttree==NULL)
    return NULL;
  
  if (besttree->isLeaf() || !descend)
    return besttree;
  else
    return besttree->getRobustChild(descend);
}

bool Tree::LCB_UrgentNode_useWins (UrgentNode &u)
{
  if (u.bestLCBplayouts==0)
    return true;
  if (u.playouts==0)
    return false;
  
  float r1=5,r2=5;
  float rc1=u.bestLCBwins/u.bestLCBplayouts;
  float rc2=u.wins/u.playouts;
  
  if (u.bestLCBplayouts>0 && u.bestLCBwins>0)
    r1= (u.bestLCBwins+u.bestLCBconst*sqrt(u.bestLCBwins))/u.bestLCBplayouts;
  if (u.playouts>0 && u.wins>0)
    r2= (u.wins+u.bestLCBconst*sqrt(u.wins))/u.playouts;
  if (r1<r2 && rc1<rc2)
    return false; //1. reason to use LCB (LCB significantly lower than wins)
  r1=-5; r2=-5;
  if (u.bestLCBplayouts>0 && u.bestLCBwins>0)
    r1= (u.bestLCBwins-u.bestLCBconst*sqrt(u.bestLCBwins))/u.bestLCBplayouts;
  if (u.playouts>0 && u.wins>0)
    r2= (u.wins-u.bestLCBconst*sqrt(u.wins))/u.playouts;
  if (r1>r2 && rc1>rc2)
    return false; //2. reason to use LCB (LCB significantly higher than wins)
  return true;
}

wins_playouts Tree::LCB_UrgentNode_winplayouts(wins_playouts UCBwp, wins_playouts wp, float k_confidence)
{
  float cw=UCBwp.wins,cp=UCBwp.playouts,w=wp.wins,p=wp.playouts;
  if (cp==0 || p==0 || cp==p) return {w,p}; //no UCB values
  
  float k=((cp*w-p*cw))/(cp*sqrt(p)-p*sqrt(cp));
  if (k==0) return {w,p}; //same winrate, take the more playouts

  float alpha=1.0/(1.0+pow(k*k_confidence,2));
  
  //gives back a calculated avarage depending on k*sigma confidence bound
  return {alpha*w+(1.0-alpha)*cw,alpha*p+(1.0-alpha)*cp};
}


//attention changed to sqrt(playouts) was sqrt(wins)
//the playouts>0 tests should not be necessary!!!!
float Tree::LCB_UrgentNode (UrgentNode &u)
{
  wins_playouts wp=LCB_UrgentNode_winplayouts({u.bestLCBwins,u.bestLCBplayouts},{u.wins,u.playouts},u.k_confidence);

  if (wp.playouts==0) return -10; //happens in case of pass moves ..??
  return (wp.wins-u.bestLCBconst*sqrt(wp.playouts))/wp.playouts;

  //old code
  //if (LCB_UrgentNode_useWins(u))
  //  return (u.playouts>0)?((u.wins-u.bestLCBconst*sqrt(u.playouts))/u.playouts):-100;
  //else
  //  return (u.bestLCBplayouts>0)?((u.bestLCBwins-u.bestLCBconst*sqrt(u.bestLCBplayouts))/u.bestLCBplayouts):-100;
  //return -100;
}

bool Tree::compare_UrgentNodes(UrgentNode &u1,UrgentNode &u2)
{
  return u1.urgency>u2.urgency;  //should result in biggest first
}
  
bool Tree::compare_UrgentNodes_LCB(UrgentNode &u1,UrgentNode &u2)
{
  float u1LCB=-99999;
  float u2LCB=-99999;
  u1LCB=LCB_UrgentNode(u1);
  u2LCB=LCB_UrgentNode(u2);
  if (u1LCB>-10 || u2LCB>-10)
    return u1LCB>u2LCB;  //should result in biggest first
  return u1.playouts>u2.playouts;
}
  
Tree *Tree::getUrgentChild(Worker::Settings *settings)
{
  if (this->isLeaf()) return NULL; //expantion not ready
  float besturgency=0;
  Tree *besttree=NULL;
  UrgentNode urgenttmp;
  std::list <UrgentNode> urgentlist;
  
  bool skiprave=false;
  if (params->rave_skip>0 && (params->rave_skip)>(settings->rand->getRandomReal()))
    skiprave=true;
  
  if (!superkoprunedchildren && playouts>(params->rules_superko_prune_after))
    this->pruneSuperkoViolations();
  Tree * robustchild=NULL;
  if (params->test_p65>0) robustchild=getRobustChild();
  if (debug) fprintf(stderr,"getUrgentChild\n");
  float sum_urgency=0,sum_urgency2=0,var_count=0;
  for(std::list<Tree*>::iterator iter=children->begin();iter!=children->end();++iter) 
  {
    if ((*iter)->isPrimary() && (!(*iter)->isPruned() || WITH_P(params->test_p7)))
    {
      float urgency;
      
      if ((*iter)->getPlayouts()==0 && (*iter)->getRAVEPlayouts()==0)
        urgency=(*iter)->getUrgency(skiprave,robustchild)+(settings->rand->getRandomReal()/1000);
      else
      {
        urgency=(*iter)->getUrgency(skiprave,robustchild);
        if (params->random_f>0)
          urgency-=params->random_f*log(settings->rand->getRandomReal());
      }
      if (debug)
          fprintf(stderr,"urgency %s %f \n",(*iter)->getMove().toString(19).c_str(),urgency);
      sum_urgency+=urgency;
      sum_urgency2+=urgency*urgency;
      var_count+=1;
      if (params->test_p74>0||params->test_p75>0)
      {
        urgenttmp.urgency=urgency;
        urgenttmp.node=(*iter);
        urgenttmp.wins=(*iter)->getWins();
        urgenttmp.playouts=(*iter)->getPlayouts();
        urgenttmp.bestLCBplayouts=(*iter)->getBestLCBPlayouts();
        urgenttmp.bestLCBwins=(*iter)->getBestLCBWins();
        urgenttmp.bestLCBconst=params->test_p75;
        urgenttmp.k_confidence=params->test_p76;
        urgentlist.push_back(urgenttmp);
      }
      
      if (params->debug_on)
        fprintf(stderr,"[urg]:%s %.3f %.2f(%f) %.2f(%f)\n",(*iter)->getMove().toString(params->board_size).c_str(),urgency,(*iter)->getRatio(),(*iter)->getPlayouts(),(*iter)->getRAVERatio(),(*iter)->getRAVEPlayouts());
          
      if (urgency>besturgency || besttree==NULL)
      {
        besttree=(*iter);
        besturgency=urgency;
      }
    }
  }
  urgency_variance=(sum_urgency2-sum_urgency*sum_urgency/var_count)/var_count;
  if (params->test_p74>0 && urgentlist.size()>0)
  {
    urgentlist.sort(Tree::compare_UrgentNodes);
    //urgenttmp=urgentlist.front();
    if (debug) {
      for (std::list<UrgentNode>::iterator iter=urgentlist.begin();iter!=urgentlist.end();++iter)
      {
        fprintf(stderr,"urgenttmp.node %p %s %f \n",(*iter).node,(*iter).node->getMove().toString(9).c_str(),(*iter).urgency);
      }
    }
    float lastplayouts=-1; //nothing yet
    for (std::list<UrgentNode>::iterator iter=urgentlist.begin();iter!=urgentlist.end();++iter)
    {
      if (lastplayouts>=0 && lastplayouts < (*iter).playouts * params->test_p74)
        break; //if the factor is not bigger than test_p74, than you are allowed to take the node, otherwize take the next
      urgenttmp=(*iter);
      lastplayouts=urgenttmp.playouts;
    }
    //fprintf(stderr,"besttree %p %f urgenttmp.node %p %f urgenttmp %p\n",besttree,besturgency,urgenttmp.node,urgenttmp.urgency,urgenttmp);
    besttree=urgenttmp.node;
  }
  if (params->test_p75>0 && urgentlist.size()>0)
  {
    urgentlist.sort(Tree::compare_UrgentNodes_LCB);
    float bestLCB=-100;
    
    double bestLCBwins_tmp=0;
    double bestLCBplayouts_tmp=0;

    double bestLCBwins_t=0;
    double bestLCBplayouts_t=0;

    float bestLCB_tmp;
    for (std::list<UrgentNode>::iterator iter=urgentlist.begin();iter!=urgentlist.end();++iter)
    {
      wins_playouts use=LCB_UrgentNode_winplayouts({(*iter).bestLCBwins,(*iter).bestLCBplayouts},{(*iter).wins,(*iter).playouts},params->test_p76);
      //if (!LCB_UrgentNode_useWins(*iter)) //(*iter).bestLCBplayouts>0 && (*iter).bestLCBplayouts>0)
      //{
      //  bestLCBwins_tmp+=(*iter).bestLCBwins;
      //  bestLCBplayouts_tmp+=(*iter).bestLCBplayouts;
      //}
      //else
      //{
      //  bestLCBwins_tmp+=(*iter).wins;
      //  bestLCBplayouts_tmp+=(*iter).playouts;
      //}
      bestLCBwins_tmp+=use.wins;
      bestLCBplayouts_tmp+=use.playouts;
      bestLCB_tmp=-10;
      if (bestLCBplayouts_tmp>0 && bestLCBwins_tmp>0) bestLCB_tmp=(bestLCBwins_tmp-params->test_p75*sqrt(bestLCBplayouts_tmp))/bestLCBplayouts_tmp;
      //else break;
      //if (bestLCB_tmp<bestLCB)
      //  break;  //this way deeper moves which increase LCB are ignored
      if (!(bestLCB_tmp<bestLCB))
      {
        bestLCB=bestLCB_tmp;
        bestLCBwins_t=bestLCBwins_tmp;
        bestLCBplayouts_t=bestLCBplayouts_tmp;
      }
    }
    //bestLCB_tmp=0;
    //losses in this are compared to wins in childs
    //if (playouts>0) bestLCB_tmp=((playouts-wins)-params->test_p75*sqrt(playouts))/playouts;

    //should be handled elsewhere
    //if (bestLCB<bestLCB_tmp||bestLCBplayouts==0)
    //{
    //  bestLCBwins=wins;
    //  bestLCBplayouts=playouts;
    //}
    //else
    {
      //best answer wins converted to actual wins
      bestLCBwins=bestLCBplayouts_t-bestLCBwins_t;
      bestLCBplayouts=bestLCBplayouts_t;
    }
  }
  
  if (besttree!=NULL && besttree->isTerminalLose() && this->hasPrunedChildren())
  {
    //fprintf(stderr,"ERROR! Unpruning didn't occur! (%d,%d,%d)\n",children->size(),prunedchildren,superkochildrenviolations);
    while (this->hasPrunedChildren() && !this->hasOneUnprunedChildNotTerminalLoss())
      this->unPruneNow(); //unprune a non-terminal loss
  }
  
  if (debug || params->debug_on)
  {
    if (besttree!=NULL)
      fprintf(stderr,"[best]:%s (%d,%d)\n",besttree->getMove().toString(params->board_size).c_str(),(int)(children->size()-prunedchildren),superkochildrenviolations);
    else
      fprintf(stderr,"WARNING! No urgent move found\n");
  }
  
  if (besttree==NULL)
    return NULL;
  
  bool busyexpanding=false;
  if (params->move_policy==Parameters::MP_UCT && besttree->isLeaf() && !besttree->isTerminal())
  {
    if (besttree->getPlayouts()>params->uct_expand_after)
      busyexpanding=!besttree->expandLeaf(settings, besttree->getPlayouts());
  }
  
  if (params->uct_virtual_loss)
    besttree->addVirtualLoss();
  if (besttree->isPruned()) {
    fprintf(stderr,"besttree is pruned?!\n");
  }
  if (besttree->isLeaf() || busyexpanding)
    return besttree;
  else
    return besttree->getUrgentChild(settings);
}

void Tree::presetRave (float ravew,float ravep)
{
  params->engine->addpresetplayout (ravep);
  ravewins+=params->uct_preset_rave_f*ravew; 
  raveplayouts+=params->uct_preset_rave_f*ravep;
  //fprintf(stderr,"ravewins %f raveplayouts %f\n",ravewins,raveplayouts);
}
void Tree::presetRaveEarly (float ravew,float ravep)
{
  earlyravewins+=params->uct_preset_rave_f*ravew; 
  earlyraveplayouts+=params->uct_preset_rave_f*ravep;
  fprintf(stderr,"ravewins %f raveplayouts %f\n",ravewins,raveplayouts);
}

bool Tree::expandLeaf(Worker::Settings *settings, int expand_num)
{
  float *rwins=NULL;
  float *rplayouts=NULL;

  //earlyrave preset does not seem to work, bug or just bad?? commented out!!!
  //float *earlyrwins=NULL;
  //float *earlyrplayouts=NULL;
  if (!this->isLeaf())
    return true;
  else if (this->isTerminal())
    fprintf(stderr,"WARNING! Trying to expand a terminal node!\n");
  
  //boost::mutex::scoped_try_lock lock(expandmutex);
  //expandmutex.lock();
  if (params->test_p100>0) {
    //now we need an extended lock, as not two nodes may be expanded at the same time now!
    if (!params->engine->CNNmutex.try_lock())
      return false;
  } 
  else if (!expandmutex.try_lock())
    return false;

  //this is never executed? It is in race conditions I think.
  if (!this->isLeaf())
  {
    //fprintf(stderr,"Node was already expanded!\n");
    if (params->test_p100>0) 
      params->engine->CNNmutex.unlock();
    else
      expandmutex.unlock();
    return true;
  }
  
  std::list<Go::Move> startmoves=this->getMovesFromRoot();
  Go::Board *startboard=params->engine->getCurrentBoard()->copy();
  
  //gtpe->getOutput()->printfDebug("[moves]:"); //!!!
  for(std::list<Go::Move>::iterator iter=startmoves.begin();iter!=startmoves.end();++iter)
  {
    //gtpe->getOutput()->printfDebug(" %s",(*iter).toString(boardsize).c_str()); //!!!
    startboard->makeMove((*iter));
    if (startboard->getPassesPlayed()>=2 || (*iter).isResign())
    {
      delete startboard;
      fprintf(stderr,"WARNING! Trying to expand a terminal node? (passes:%d)\n",startboard->getPassesPlayed());
      if (params->test_p100>0) 
        params->engine->CNNmutex.unlock();
      else
        expandmutex.unlock();
      return true;
    }
  }
  //gtpe->getOutput()->printfDebug("\n"); //!!!
  
  Go::Color col=startboard->nextToMove();
  
  bool winnow=Go::Board::isWinForColor(col,startboard->score()-params->engine->getHandiKomi());
  Tree *nmt=new Tree(params,startboard->getZobristHash(params->engine->getZobristTable()),Go::Move(col,Go::Move::PASS));
  if (winnow)
    nmt->addPriorWins(1);
  #if UCT_PASS_DETER>0
    if (startboard->getPassesPlayed()==0 && !(params->surewin_expected && col==params->engine->getCurrentBoard()->nextToMove()))
      nmt->addPriorLoses(UCT_PASS_DETER);
  #endif
  nmt->addRAVEWins(params->rave_init_wins,false);
  nmt->addRAVEWins(params->rave_init_wins,true);
  this->addChild(nmt);

  // here we could add preknown rave values?!
  
  if (params->uct_preset_rave_f!=0.0 && this->getParent())
  {
    Tree *p=this->getParent();
    bool twohigher=false;
    if (p->getParent())
    {
      twohigher=true;
      p=p->getParent();
    }
    int posmax=startboard->getPositionMax();
    //fprintf(stderr,"posmax %d\n",posmax);
    rwins=new float[posmax];
    rplayouts=new float[posmax];
    //earlyrwins=new float[posmax];
    //earlyrplayouts=new float[posmax];
    for (int i=0;i<posmax;i++)
    {
      rplayouts[i]=-1;
      //earlyrplayouts[i]=-1;
    }
    std::list<Tree*> *childrenTmp=p->getChildren();
    for(std::list<Tree*>::iterator iter=childrenTmp->begin();iter!=childrenTmp->end();++iter) 
    {
      int pos=(*iter)->getMove().getPosition();
      if (pos>=0)
      {
        if (!twohigher)
        {
          rwins[pos]=(*iter)->getRAVEWinsOther();
          rplayouts[pos]=(*iter)->getRAVEPlayoutsOther();
        //earlyrwins[pos]=(*iter)->getRAVEWinsOtherEarly();
        //earlyrplayouts[pos]=(*iter)->getRAVEPlayoutsOtherEarly();
        }
        else
        {
          rwins[pos]=(*iter)->getRAVEWins();
          rplayouts[pos]=(*iter)->getRAVEPlayouts();
        //earlyrwins[pos]=(*iter)->getRAVEWinsEarly();
        //earlyrplayouts[pos]=(*iter)->getRAVEPlayoutsEarly();
        }
      }
    }
  }
  
  if (startboard->numOfValidMoves(col)>0)
  {
    Go::BitBoard *validmovesbitboard=startboard->getValidMoves(col);
    for (int p=0;p<startboard->getPositionMax();p++)
    {
      if (validmovesbitboard->get(p))
      {
        bool violation=false;
        Go::ZobristHash hash=0;
        if (params->rules_positional_superko_enabled && !params->rules_superko_top_ply && !params->rules_superko_at_playout)
        {
          Go::Board *thisboard=startboard->copy();
          thisboard->makeMove(Go::Move(col,p));
          hash=thisboard->getZobristHash(params->engine->getZobristTable());
          delete thisboard;
          
          violation=this->isSuperkoViolationWith(hash);
          
          if (!violation)
            violation=params->engine->getZobristHashTree()->hasHash(hash);
        }
        
        //if (violation)
        //  fprintf(stderr,"superko violation avoided\n");
        
        if (!violation)
        {
          Tree *nmt=new Tree(params,hash,Go::Move(col,p));
          
          if (params->uct_pattern_prior>0 && !startboard->weakEye(col,p))
          {
            unsigned int pattern=Pattern::ThreeByThree::makeHash(startboard,p);
            if (col==Go::WHITE)
              pattern=Pattern::ThreeByThree::invert(pattern);
            if (params->engine->getPatternTable()->isPattern(pattern))
              nmt->addPriorWins(params->uct_pattern_prior);
          }
          nmt->addRAVEWins(params->rave_init_wins,false);
          nmt->addRAVEWins(params->rave_init_wins,true);

          if (rplayouts && rplayouts[nmt->getMove().getPosition()]>0)
          {
            //fprintf(stderr,"%d %f %f\n",nmt->getMove().getPosition(),rwins[nmt->getMove().getPosition()],rplayouts[nmt->getMove().getPosition()]);
            nmt->presetRave(rwins[nmt->getMove().getPosition()],rplayouts[nmt->getMove().getPosition()]);
          }
          //if (earlyrplayouts && earlyrplayouts[nmt->getMove().getPosition()]>0)
          //{
            //fprintf(stderr,"%d %f %f\n",nmt->getMove().getPosition(),rwins[nmt->getMove().getPosition()],rplayouts[nmt->getMove().getPosition()]);
          //  nmt->presetRaveEarly(earlyrwins[nmt->getMove().getPosition()],earlyrplayouts[nmt->getMove().getPosition()]);
          //}


          //This might be not thread save, as the childs are not ready yet!!! they get prepared later!!
          this->addChild(nmt);
        }
      }
    }



    if (rwins) delete rwins;
    if (rplayouts) delete rplayouts;
    //if (earlyrwins) delete earlyrwins;
    //if (earlyrplayouts) delete earlyrplayouts;
    
    if (params->uct_playoutmove_prior>0)
    {
      //try unpruning a playout move?!
      Go::Move tmpmove;
      params->engine->getOnePlayoutMove(startboard, startboard->nextToMove(),&tmpmove);       
      fprintf(stderr,"test %s\n",tmpmove.toString (params->board_size).c_str());
      Tree *mt=this->getChild(tmpmove);
      if (mt!=NULL)
        mt->addPriorWins(params->uct_playoutmove_prior);
    }

    if (params->uct_atari_prior>0)
    {
      std::ourset<Go::Group*> *groups=startboard->getGroups();
      for(std::ourset<Go::Group*>::iterator iter=groups->begin();iter!=groups->end();++iter) 
      {
        if ((*iter)->inAtari())
        {
          int liberty=(*iter)->getAtariPosition();
          bool iscaptureorconnect=startboard->isCapture(Go::Move(col,liberty)) || startboard->isExtension(Go::Move(col,liberty));
          if (startboard->validMove(Go::Move(col,liberty)) && iscaptureorconnect)
          {
            Tree *mt=this->getChild(Go::Move(col,liberty));
            if (mt!=NULL)
              mt->addPriorWins(params->uct_atari_prior);
          }
        }
      }
    }
  }
  
  if (params->uct_symmetry_use)
  {
    Go::Board::Symmetry sym=startboard->getSymmetry();
    if (sym!=Go::Board::NONE)
    {
      for(std::list<Tree*>::iterator iter=children->begin();iter!=children->end();++iter) 
      {
        int pos=(*iter)->getMove().getPosition();
        Go::Board::SymmetryTransform trans=startboard->getSymmetryTransformToPrimary(sym,pos);
        int primarypos=startboard->doSymmetryTransform(trans,pos);
        if (pos!=primarypos)
        {
          Tree *pmt=this->getChild(Go::Move(col,primarypos));
          if (pmt!=NULL)
          {
            if (!pmt->isPrimary())
              fprintf(stderr,"WARNING! bad primary\n");
            (*iter)->setPrimary(pmt);
          }
          else
            fprintf(stderr,"WARNING! missing primary\n");
        }
      }
    }
  }
  
  if (params->uct_progressive_widening_enabled || params->uct_progressive_bias_enabled)
  {
    if (params->uct_progressive_widening_enabled)
    {
      this->pruneChildren();  //in multithreading otherwize not unpruned moves are played
    }
    Go::ObjectBoard<int> *cfglastdist=NULL;
    Go::ObjectBoard<int> *cfgsecondlastdist=NULL;
    Go::ObjectBoard<int> *cfgaroundposdist=NULL;
    params->engine->getFeatures()->computeCFGDist(startboard,&cfglastdist,&cfgsecondlastdist);
    params->engine->getFeatures()->computeCFGDist(startboard,around_pos,&cfgaroundposdist);
    //float *CNNresults=NULL;
    int size=startboard->getSize();
    if (params->test_p100>0 && CNNresults==NULL)
    {
      CNNresults=new float[size*size];
      params->engine->addExpandStats(expand_num);
      float value=-1;
      params->engine->getCNN(startboard,col,CNNresults,0,&value);
      if (params->cnn_preset_playouts>0 && value>0) {
        this->addPriorValue (1.0-value);
      }
      if (params->cnn_weak_gamma>0) {
        CNNresults_weak=new float[size*size];
        params->engine->getCNN(startboard,col,CNNresults_weak,1);
      }
      if (params->test_p116>0) {
        float *CNNresults_other=new float[size*size];
        params->engine->getCNN(startboard,Go::otherColor(col),CNNresults_other);
        for (int i=0;i<size*size;i++) {
          int pp=Go::Position::cnn2pos(i,size);
          float value=-(int(CNNresults[i]*10000)+float(i)/size/size);
          float value_other=-(int(CNNresults_other[i]*10000)+float(i)/size/size);
          if (Go::otherColor(col)==Go::BLACK) {
            cnn_b.insert({pp,value_other});
            cnn_w.insert({pp,value});
          }
          else {
            cnn_w.insert({pp,value_other});
            cnn_b.insert({pp,value});
          }
        }
        delete[] CNNresults_other;
        //for (boost::bimap <int,float>::right_const_iterator it = cnn_b.right.begin();it!=cnn_b.right.end();++it) {
        // fprintf(stderr,"%f %d\n",it->first,it->second);
        //}
      }
    } 
    DecisionTree::GraphCollection *graphs = NULL;
    if (params->features_dt_use)
      graphs = new DecisionTree::GraphCollection(DecisionTree::getCollectionTypes(params->engine->getDecisionTrees()),startboard);

    //int now_unpruned=this->getUnprunedNum();
    //fprintf(stderr,"debugging %d\n",now_unpruned);
    for(std::list<Tree*>::iterator iter=children->begin();iter!=children->end();++iter) 
    {
      if ((*iter)->isPrimary())
      {
        float gamma_local_part=0;
        Pattern::Circular *pattcirc_for_move =NULL;
        if (params->test_p55>0 || params->test_p57>0)
        {
          Pattern::CircularDictionary *circdict= params->engine->getFeatures()->getCircDict();
          //does this expensive construction twice, should be optimized later
          pattcirc_for_move = new Pattern::Circular(circdict,startboard,(*iter)->getMove().getPosition(),PATTERN_CIRC_MAXSIZE);
        }
        float gammal=params->engine->getFeatures()->getMoveGamma(startboard,cfglastdist,cfgsecondlastdist,graphs,(*iter)->getMove(),true,true,&gamma_local_part,pattcirc_for_move,cfgaroundposdist);
        float gammal2=0;
        if (params->test_p100>0) {
          if ( (*iter)->getMove().isNormal()) {
            int p=(*iter)->getMove().getPosition();
            int x=Go::Position::pos2x(p,size);
            int y=Go::Position::pos2y(p,size);
            //fprintf(stderr,"%d %d %f\n",x,y,CNNresults[size*x+y]);
            gammal=((gammal-1.0)*params->test_p102+1.0)*(CNNresults[size*x+y]*params->test_p100+params->test_p101);
            if (params->cnn_weak_gamma>0) gammal2=CNNresults_weak[size*x+y];
          }
          else {
            gammal=params->CNN_pass_probability;
            if (params->cnn_weak_gamma>0) gammal2=params->CNN_pass_probability;
          }            
        }
        (*iter)->setFeatureGamma(gammal);
        (*iter)->setFeatureGammaWeak(gammal2);
        (*iter)->setFeatureGammaLocalPart(gamma_local_part);
        if (around_pos>=0)
          (*iter)->around_pos=around_pos;
        if (params->test_p55>0 || params->test_p57>0)
        {
          //may be not necessary, as we are at the identical move!!
          //pattcirc_for_move.convertToSmallestEquivalent(circdict);
          MoveCirc *movecirc_tmp = new MoveCirc(*pattcirc_for_move,(*iter)->getMove());
          EqMoves *eq_moves_tmp=params->engine->addMoveCirc(movecirc_tmp,(*iter));
          (*iter)->setMoveCirc(movecirc_tmp,eq_moves_tmp);
          if (pattcirc_for_move) delete pattcirc_for_move;
        }

        
        Pattern::Circular pattcirc = Pattern::Circular(params->engine->getFeatures()->getCircDict(),startboard,(*iter)->getMove().getPosition(),5);
        int NumNonOffboard=pattcirc.countNonOffboard(params->engine->getFeatures()->getCircDict());
        int NumStones=pattcirc.countStones(params->engine->getFeatures()->getCircDict());
        (*iter)->setStonesAround((float)NumStones/NumNonOffboard);
        //if ((*iter)->getMove().toString(params->board_size).compare("B:E1")==0)
        //  fprintf(stderr,"move %s %f\n",(*iter)->getMove().toString(params->board_size).c_str(),gamma);
      }
    }
    
    if (cfglastdist!=NULL)
      delete cfglastdist;
    if (cfgsecondlastdist!=NULL)
      delete cfgsecondlastdist;
    if (cfgaroundposdist!=NULL)
      delete cfgaroundposdist;
    //if (CNNresults!=NULL)
    //  delete[] CNNresults;
    if (graphs!=NULL)
      delete graphs;
    
    if (params->uct_progressive_widening_enabled)
    {
      this->pruneChildren();
      for (int i=0;i<params->uct_progressive_widening_init;i++)
      {
        this->unPruneNow(); //unprune a child
      }
      while (this->hasPrunedChildren() && !this->hasOneUnprunedChildNotTerminalLoss())
        this->unPruneNow(); //unprune a non-terminal loss
    }
  }
  
  beenexpanded=true;
  delete startboard;
  if (params->test_p100>0) 
      params->engine->CNNmutex.unlock();
    else
      expandmutex.unlock();
  return true;
}

void Tree::updateRAVE(Go::Color wincol,Go::IntBoard *blacklist,Go::IntBoard *whitelist,bool early, Go::Board *scoredboard, int childpos)
{
  if (params->rave_moves<=0)
    return;
  
  //fprintf(stderr,"[update rave] %s (%c)\n",move.toString(params->board_size).c_str(),Go::colorToChar(wincol));
  
  if (!this->isRoot())
  {
    parent->updateRAVE(wincol,blacklist,whitelist,early, scoredboard,this->getMove().getPosition());

    if (this->getMove().isNormal())
    {
      int pos=this->getMove().getPosition();
      if (this->getMove().getColor()==Go::BLACK)
        blacklist->clear(pos);
      else
        whitelist->clear(pos);
    }
  }
  
  if (!this->isLeaf())
  {
    for(std::list<Tree*>::iterator iter=children->begin();iter!=children->end();++iter) 
    {
      if (!(*iter)->getMove().isPass() && !(*iter)->getMove().isResign())
      {
        Go::Color col=(*iter)->getMove().getColor();
        int pos=(*iter)->getMove().getPosition();
        int ravenum=(col==Go::BLACK?blacklist:whitelist)->get(pos);
        if (scoredboard!=NULL)
        {
          if (params->debug_on)
            fprintf(stderr, "%s ScoredOwner %s black %d white %d nonlocalblack %d nonlocalwhite %d\n",(*iter)->getMove().toString(scoredboard->getSize()).c_str(),Go::getColorName(scoredboard->getScoredOwner(pos)),blacklist->get(pos),whitelist->get(pos),blacklist->getb(pos),whitelist->getb(pos));
          if (scoredboard->getScoredOwner(pos)==Go::BLACK && blacklist->get(pos)!=0 && blacklist->getb(pos)) (*iter)->ownselfblack++;
          if (scoredboard->getScoredOwner(pos)==Go::WHITE && whitelist->get(pos)!=0 && whitelist->getb(pos)) (*iter)->ownselfwhite++;
          if (scoredboard->getScoredOwner(pos)==Go::WHITE && blacklist->get(pos)!=0 && blacklist->getb(pos)) (*iter)->ownotherblack++;
          if (scoredboard->getScoredOwner(pos)==Go::BLACK && whitelist->get(pos)!=0 && whitelist->getb(pos)) (*iter)->ownotherwhite++;
          if (scoredboard->getScoredOwner(pos)==Go::EMPTY                          ) (*iter)->ownnobody++;

          if (scoredboard->getScoredOwner(pos)==Go::BLACK) (*iter)->ownblack++;
          if (scoredboard->getScoredOwner(pos)==Go::WHITE) (*iter)->ownwhite++;
          (*iter)->ownercount++;
          if (blacklist->get(pos)!=0) {
            float rnum=(blacklist->get(pos)>0)?blacklist->get(pos):-blacklist->get(pos);
            if (rnum<params->test_p82) {
              (*iter)->regcountb++;
              (*iter)->blackx+=rnum;
              (*iter)->blackx2+=rnum*rnum;
              if (wincol==Go::BLACK) {
                (*iter)->blacky+=1; (*iter)->blackxy+=rnum;
              }
            }
          }
          if (whitelist->get(pos)!=0) {
            float rnum=(whitelist->get(pos)>0)?whitelist->get(pos):-whitelist->get(pos);
            if (rnum<params->test_p82) {
              (*iter)->regcountw++;
              (*iter)->whitex+=rnum; 
              (*iter)->whitex2+=rnum*rnum;
              if (wincol==Go::WHITE) {
                (*iter)->whitey+=1; (*iter)->whitexy+=rnum; 
              }
            }
          }
        }  
        if (ravenum>0)
        {
          float raveweight=1;
          //if (!early) raveweight=exp(-params->test_p30*ravenum);
          //if (!early) raveweight=pow(ravenum,-params->test_p30);
          if (!early) raveweight=1.0/((ravenum-1)*params->test_p30+1);
          
          if ((params->test_p96==0 || pos!=childpos)) //do not update playoutpath, if test_p92 enabled
          {
            if ((*iter)->isPrimary())
            {
              if (col==wincol)
                (*iter)->addRAVEWin(early,raveweight);
              else
                (*iter)->addRAVELose(early,raveweight);
            }
            else
            {
              Tree *primary=(*iter)->getPrimary();
              if (col==wincol)
                primary->addRAVEWin(early,raveweight);
              else
                primary->addRAVELose(early,raveweight);
            }
          }
        }
        if (ravenum<0)
        {  //marked as bad move during playout, therefore counted as rave loss
          float raveweight=1;
          //if (!early) raveweight=exp(-params->test_p30*ravenum);
          //if (!early) raveweight=pow(-ravenum,-params->test_p30); //this was not sufficently tested and slow
          if (!early) raveweight=1.0/(-(ravenum+1)*params->test_p30+1);
          
          if ((params->test_p96==0 || pos!=childpos)) //do not update playoutpath, if test_p92 enabled
          {
            if ((*iter)->isPrimary())
            {
              (*iter)->addRAVELose(early,raveweight);
            }
            else
            {
              Tree *primary=(*iter)->getPrimary();
              primary->addRAVELose(early,raveweight);
            }
          }
        }
        //check for the other color
        ravenum=(col==Go::WHITE?blacklist:whitelist)->get(pos);
        if (ravenum>0)
        {
          float raveweight=1;
          //if (!early) raveweight=exp(-params->test_p30*ravenum);
          //if (!early) raveweight=pow(ravenum,-params->test_p30);
          if (!early) raveweight=1.0/((ravenum-1)*params->test_p30+1);
            if ((*iter)->isPrimary())
            {
              if (Go::otherColor(col)==wincol)
                (*iter)->addRAVEWinOther(early,raveweight);
              else
                (*iter)->addRAVELoseOther(early,raveweight);
            }
            else
            {
              Tree *primary=(*iter)->getPrimary();
              if (Go::otherColor(col)==wincol)
                primary->addRAVEWinOther(early,raveweight);
              else
                primary->addRAVELoseOther(early,raveweight);
            }
          
        }
        if (ravenum<0)
        {  //marked as bad move during playout, therefore counted as rave loss
          float raveweight=1;
          //if (!early) raveweight=exp(-params->test_p30*ravenum);
          //if (!early) raveweight=pow(-ravenum,-params->test_p30);
          if (!early) raveweight=1.0/(-(ravenum-1)*params->test_p30+1);
          
            if ((*iter)->isPrimary())
            {
              (*iter)->addRAVELoseOther(early,raveweight);
            }
            else
            {
              Tree *primary=(*iter)->getPrimary();
              primary->addRAVELoseOther(early,raveweight);
            }
          
        }
        
      }
    }
  }
  else if (params->debug_on) fprintf(stderr,"%s\n",scoredboard->toString().c_str());
}

float Tree::getProgressiveBias() const
{
  if (params->uct_progressive_bias_log_add>0) {
    float bias=0;
    float gamma_use=gamma;
    if (params->cnn_weak_gamma>0) {
      //at the moment this disables all other handling!!! should only be used with pure CNN gammas....
      Tree *tmp=getParent();
      double parent_playouts=tmp->getPlayouts();
      while (!tmp->isRoot())
        tmp=tmp->getParent();
      double root_playouts=tmp->getPlayouts();
      if (parent_playouts/root_playouts < params->cnn_weak_gamma) 
        gamma_use=gamma_weak;
    }
  
    if (parent == NULL) return 0;
    
    float po=parent->getPlayouts();
    if (gamma_use>0) bias=log(gamma_use)+params->uct_progressive_bias_log_add;
    if (bias<0) bias=0;
    bias*=params->uct_progressive_bias_h;
    return (bias+biasbonus)/sqrt(po+1);
  }
    
  float bias=params->uct_progressive_bias_h*gamma;
  if (params->uct_progressive_bias_scaled>0.0 && !this->isRoot())
    bias/=parent->getChildrenTotalFeatureGamma();
  if (params->uct_progressive_bias_relative && !this->isRoot())
    bias/=parent->getMaxChildFeatureGamma();
  bias=pow(bias,params->uct_progressive_bias_exponent);
  //return (bias+biasbonus)*exp(-(float)playouts/params->uct_progressive_bias_moves); // /pow(playouts+1,params->uct_criticality_urgency_decay);
  if (params->test_p44>0) return (bias+biasbonus);
  return (bias+biasbonus)/pow(playouts+1,params->test_p45);
  return (bias+biasbonus)/(playouts+1);
}

bool Tree::isSuperkoViolationWith(Go::ZobristHash h) const
{
  //fprintf(stderr,"0x%016llx 0x%016llx\n",h,hash);
  if (hash==h)
    return true;
  else if (!this->isRoot())
    return parent->isSuperkoViolationWith(h);
  else
    return false;
}

void Tree::setFeatureGamma(float g)
{
  gamma=g+params->test_p59;
  if (!this->isRoot())
  {
    parent->childrentotalgamma+=gamma;
    parent->childrenlogtotalchildgamma+=(log(gamma+1.0)>params->test_p67)? 1 : 0;
    if (gamma>(parent->maxchildgamma))
      parent->maxchildgamma=gamma;
  }
}

void Tree::setStonesAround(float g)
{
  stones_around=g;
}

void Tree::pruneSuperkoViolations()
{
  if (!superkoprunedchildren && !this->isLeaf() && params->rules_positional_superko_enabled && params->rules_superko_top_ply && !params->rules_superko_at_playout)
  {
    std::list<Go::Move> startmoves=this->getMovesFromRoot();
    Go::Board *startboard=params->engine->getCurrentBoard()->copy();
    
    for(std::list<Go::Move>::iterator iter=startmoves.begin();iter!=startmoves.end();++iter)
    {
      startboard->makeMove((*iter));
    }
    
    for(std::list<Tree*>::iterator iter=children->begin();iter!=children->end();++iter) 
    {
      if ((*iter)->getMove().isNormal())
      {
        Go::Board *thisboard=startboard->copy();
        thisboard->makeMove((*iter)->getMove());
        Go::ZobristHash hash=thisboard->getZobristHash(params->engine->getZobristTable());
        (*iter)->setHash(hash);
        delete thisboard;
        (*iter)->doSuperkoCheck();
        
        /*bool violation=this->isSuperkoViolationWith(hash);
        
        if (!violation)
          violation=params->engine->getZobristHashTree()->hasHash(hash);
        
        if (violation)
        {
          std::list<Tree*>::iterator tmpiter=iter;
          Tree *tmptree=(*iter);
          --iter;
          children->erase(tmpiter);
          if (tmptree->isPruned())
            prunedchildren--;
          delete tmptree;
        }*/
      }
    }
    
    /*if (prunedchildren==children->size())
    {
      unprunenextchildat=0;
      lastunprune=0;
      unprunebase=0;
      this->unPruneNow(); //unprune at least one child
    }*/
    
    delete startboard;
    superkoprunedchildren=true;
  }
}

float Tree::bestChildRatioDiff() const
{
  Tree *bestchild=this->getRobustChild();
  
  if (bestchild==NULL)
    return -1;
  
  return (this->getRatio()+bestchild->getRatio()-1);
}

Tree *Tree::getSecondRobustChild(const Tree *firstchild) const
{
  if (firstchild==NULL)
    firstchild=this->getRobustChild();
  
  Tree *besttree=NULL;
  float bestsims=0;
  
  for(std::list<Tree*>::iterator iter=children->begin();iter!=children->end();++iter) 
  {
    if ((((*iter)->getPlayouts()>bestsims || (*iter)->isTerminalWin()) && (*iter)!=firstchild) || besttree==NULL)
    {
      besttree=(*iter);
      bestsims=(*iter)->getPlayouts();
      if (besttree->isTerminalWin())
        break;
    }
  }
  
  return besttree;
}

float Tree::secondBestPlayouts() const
{
  if (this->isRoot())
    return 0;
  
  Tree *secondbest=parent->getSecondRobustChild(this);
  if (secondbest==NULL)
    return 0;
  else
    return secondbest->getPlayouts();
}

float Tree::secondBestPlayoutRatio() const
{
  if (this->isRoot())
    return -1;
  
  Tree *secondbest=parent->getSecondRobustChild(this);
  if (secondbest==NULL)
    return -1;
  else
    return (this->getPlayouts()/secondbest->getPlayouts());
}

Tree *Tree::getBestRatioChild(float playoutthreshold) const
{
  Tree *besttree=NULL;
  float bestratio=0;
  
  for(std::list<Tree*>::iterator iter=children->begin();iter!=children->end();++iter) 
  {
    if (((*iter)->getRatio()>bestratio && (*iter)->getPlayouts()>playoutthreshold) || (*iter)->isTerminalWin())
    {
      besttree=(*iter);
      bestratio=(*iter)->getRatio();
      if (besttree->isTerminalWin())
        break;
    }
  }
  
  return besttree;
}

Tree *Tree::getBestUrgencyChild(float playoutthreshold) const
{
  Tree *besttree=NULL;
  float bestratio=0;
  
  for(std::list<Tree*>::iterator iter=children->begin();iter!=children->end();++iter) 
  {
    if (((*iter)->getUrgency()>bestratio && (*iter)->getPlayouts()>playoutthreshold) || (*iter)->isTerminalWin())
    {
      besttree=(*iter);
      bestratio=(*iter)->getUrgency();
      if (besttree->isTerminalWin())
        break;
    }
  }
  
  return besttree;
}

void Tree::updateCriticality(Go::Board *board, Go::Color wincol)
{
  if (params->uct_criticality_unprune_factor==0 && params->uct_criticality_urgency_factor==0 && params->playout_poolrave_criticality==0)
    return;
  //fprintf(stderr,"[crit_up]: %d %d\n",this->isRoot(),params->uct_criticality_siblings);
  
  if (params->uct_criticality_siblings||params->test_p25>0) //force slibings here, even if in criticality turned off:)
  {
    for(std::list<Tree*>::iterator iter=children->begin();iter!=children->end();++iter) 
    {
      if ((*iter)->getMove().isNormal())
      {
        int pos=(*iter)->getMove().getPosition();
        bool black=(board->getScoredOwner(pos)==Go::BLACK);
        bool white=(board->getScoredOwner(pos)==Go::WHITE);
        bool winner=(board->getScoredOwner(pos)==wincol);
        (*iter)->addCriticalityStats(winner,black,white);
      }
    }
  }
  
  if (this->isRoot())
    return;
  else
  {
    if (!(params->uct_criticality_siblings||params->test_p25>0) && move.isNormal())
    {
      int pos=move.getPosition();
      bool black=(board->getScoredOwner(pos)==Go::BLACK);
      bool white=(board->getScoredOwner(pos)==Go::WHITE);
      bool winner=(board->getScoredOwner(pos)==wincol);
      this->addCriticalityStats(winner,black,white);
    }
    
    parent->updateCriticality(board,wincol);
  }
}

void Tree::addCriticalityStats(bool winner, bool black, bool white)
{
  //fprintf(stderr,"[crit_add]: %d %d %d\n",winner,black,white);
  
  if (winner)
    ownedwinner++;
  if (black)
    ownedblack++;
  if (white)
    ownedwhite++;
}

float Tree::getSelfOwner(int size) const
{
  if (!move.isNormal() || (params->uct_criticality_siblings && this->isRoot()))
    return 0;
  fprintf(stderr, "%s %d ownercount %f ownselfblack %f ownotherblack %f ownselfwhite %f ownotherwhite %f ownnobody %f\n",move.toString(size).c_str(),move.getPosition(),ownercount,ownselfblack,ownotherblack,ownselfwhite,ownotherwhite,ownnobody);
  if (move.getColor()==Go::BLACK)
  {
    return ((float)ownselfblack+1)/(ownotherblack+1);
  }
  else
  {
    return ((float)ownselfwhite+1)/(ownotherwhite+1);
  }
}

float Tree::getOwnSelfBlack()
{
  return ((float)ownselfblack+1)/(ownotherblack+1);
}
float Tree::getOwnSelfWhite()
{
  return ((float)ownselfwhite+1)/(ownotherwhite+1);
}
float Tree::getOwnRatio(Go::Color col)
{
  //             ownblack              /          ownwhite
  if (ownercount-ownnobody<=0) return 0.5;
  if (col==Go::BLACK)
    return ((float)ownblack)/(ownercount);
  else
    return ((float)ownwhite)/(ownercount);
}

float Tree::getSlope(Go::Color col)
{
  if (col==Go::BLACK)
    return (blackx>0)?(regcountb*blackxy-blackx*blacky)/(regcountb*blackx2-blackx*blackx):0;
  else
    return (whitex>0)?(regcountw*whitexy-whitex*whitey)/(regcountw*whitex2-whitex*whitex):0;
}

float Tree::getCriticality() const
{
  // passes get more critical in the end of the game
  if (params->test_p73>0 && !this->isRoot() && move.isPass())
  {
    float crit=((float)params->board_size*params->board_size - getChildren()->size()) / (params->board_size*params->board_size) *params->test_p73;
    //fprintf(stderr,"[crit]: %.2f\n",crit);
    if (crit>params->test_p72)
      return params->test_p72;
    if (crit>=0)
      return crit;
    else
      return 0;
  } 
  if (!move.isNormal() || (params->uct_criticality_siblings && this->isRoot()))
    return 0;
  else
  {
    int plts=(int)(params->uct_criticality_siblings?parent->playouts:playouts);
    if (plts==0)
      return 0;
    float ratio=(params->uct_criticality_siblings?1-parent->getRatio():this->getRatio());
    float crit;
    if (move.getColor()==Go::BLACK)
      crit=(float)ownedwinner/plts-(ratio*ownedblack/plts+(1-ratio)*ownedwhite/plts);
    else
      crit=(float)ownedwinner/plts-(ratio*ownedwhite/plts+(1-ratio)*ownedblack/plts);
    
    //fprintf(stderr,"[crit]: %.2f\n",crit);
    if (crit>params->test_p72)
      return params->test_p72;
    if (crit>=0)
      return crit;
    else
      return 0;
  }
}

float Tree::getTerritoryOwner() const
{
  if (!move.isNormal() || (params->uct_criticality_siblings && this->isRoot()))
    return 0;
  else
  {
    int plts=(int)(params->uct_criticality_siblings?parent->playouts:playouts);
    if (plts==0)
      return 0;
    return (float)(ownedblack-ownedwhite)/plts;
  }
}

void Tree::doSuperkoCheck()
{
  //boost::mutex::scoped_lock lock(superkomutex);
  //superkomutex.lock();
  if (!superkomutex.try_lock())
    return;
  if (superkochecked)
  {
    superkomutex.unlock();
    return;
  }
  superkoviolation=false;
  if (move.isNormal())
  {
    if (!this->isRoot())
      superkoviolation=parent->isSuperkoViolationWith(hash);
    if (!superkoviolation)
      superkoviolation=params->engine->getZobristHashTree()->hasHash(hash);
    if (superkoviolation)
    {
      //fprintf(stderr,"superko violation pruned (%s)\n",move.toString(params->board_size).c_str());
      pruned=true;
      if (!this->isRoot())
      {
        parent->superkochildrenviolations++;
        parent->prunedchildren++;
        if (parent->prunedchildren==parent->children->size())
        {
          parent->unprunenextchildat=0;
          parent->lastunprune=0;
          parent->unprunebase=0;
          parent->unPruneNow(); //unprune at least one child
          while (parent->hasPrunedChildren() && !parent->hasOneUnprunedChildNotTerminalLoss())
            parent->unPruneNow(); //unprune a non-terminal loss
        }
      }
    }
  }
  superkochecked=true;
  superkomutex.unlock();
}

void Tree::resetNode()
{
  playouts=0;
  wins=0;
  scoresum=0;
  scoresumsq=0;
  urgency_variance=0;
  raveplayouts=0;
  earlyraveplayouts=0;
  ravewins=0;
  earlyravewins=0;
  raveplayoutsother=0;
  ravewinsother=0;
  earlyraveplayoutsother=0;
  earlyravewinsother=0;
  hasTerminalWinrate=false;
  hasTerminalWin=false;
  decayedwins=0;
  decayedplayouts=0;

  ownselfblack=0;
  ownselfwhite=0;
  ownotherblack=0;
  ownotherwhite=0;
  ownnobody=0;
  ownblack=0;
  ownwhite=0;
  ownercount=0;
  blackx=0; blacky=0; blackxy=0; blackx2=0;
  whitex=0; whitey=0; whitexy=0; whitex2=0;
  regcountw=0;
  regcountb=0;
  
  #ifdef HAVE_MPI
    this->resetMpiDiff();
  #endif
}

int Tree::countMoveCirc() 
{
 // if (movecirc==NULL)
 //   fprintf(stderr,"movecirc NULL\n");
  if (movecirc!=NULL && params->engine->getMoveCirc(movecirc)!=eq_moves)
    fprintf(stderr,"should not happen %p %p\n",params->engine->getMoveCirc(movecirc),eq_moves);
  return params->engine->countMoveCirc(movecirc);
}

void Tree::fillTreeBoard(Go::IntBoard *treeboardBlack,Go::IntBoard *treeboardWhite)
{
  if (treeboardBlack==NULL || treeboardWhite==NULL) return;
  Go::IntBoard *treeboard=(this->getMove().getColor()==Go::BLACK?treeboardBlack:treeboardWhite);
  //int debug=0,debug1=0;
  if (this->getMove().isNormal())
  {
    //debug1=treeboard->get(this->getMove().getPosition());
    treeboard->add(this->getMove().getPosition(),playouts);
    //debug=treeboard->get(this->getMove().getPosition());
    //fprintf(stderr,"deb %d %f %d %d\n",this->getMove().getPosition(),playouts,debug1,debug);
  }
  for(std::list<Tree*>::iterator iter=children->begin();iter!=children->end();++iter) 
  {
    if ((*iter)->isPrimary() && (!(*iter)->isPruned()))
      (*iter)->fillTreeBoard(treeboardBlack,treeboardWhite);
  }
}

#ifdef HAVE_MPI

void Tree::resetMpiDiff()
{
  mpi_lastplayouts=playouts;
  mpi_lastwins=wins;
}

void Tree::addMpiDiff(float plts, float wns)
{
  playouts+=plts;
  wins+=wns;
  this->resetMpiDiff();
  if (!this->isRoot() && parent->isRoot())
    parent->addMpiDiff(plts,plts-wns);
}

void Tree::fetchMpiDiff(float &plts, float &wns)
{
  plts=playouts-mpi_lastplayouts;
  wns=wins-mpi_lastwins;
  this->resetMpiDiff();
}

#endif

