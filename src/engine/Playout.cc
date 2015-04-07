#include "Playout.h"

#include "Parameters.h"
#include "Random.h"
#include "Engine.h"
#include "Pattern.h"
#include "Random.h"
#include "Worker.h"

#define LGRFCOUNT1 1
#define LGRFCOUNT2 1
#define LGPF_EMPTY 0xFFFF
#define LGPF_NUM 10

#define WITH_P(A) (A>=1.0 || (A>0 && rand->getRandomReal()<A))

long int debugint;

Playout::Playout(Parameters *prms) : params(prms)
{
  gtpe=params->engine->getGtpEngine();
  
  lgrf1=NULL;
  lgrf1n=NULL;
  lgrf1o=NULL;
  lgrf2=NULL;
  lbm=NULL;
  badpassanswer=NULL;
  lgpf=NULL;
  lgpf_b=NULL;
  lgrf1hash=NULL;
  lgrf1hash2=NULL;
  lgrf2hash=NULL;
  lgrf1count=NULL;
  lgrf2count=NULL;
  this->resetLGRF();
}

Playout::~Playout()
{
  if (lgrf1!=NULL)
    delete[] lgrf1;
  if (lgrf1n!=NULL)
    delete[] lgrf1n;
  if (lgrf1o!=NULL)
    delete[] lgrf1o;
  if (lgrf2!=NULL)
    delete[] lgrf2;
  if (lbm!=NULL)
    delete[] lbm;
  if (badpassanswer!=NULL)
    delete[] badpassanswer;
  if (lgpf!=NULL)
    delete[] lgpf;
  if (lgpf_b!=NULL)
    delete[] lgpf_b;
  if (lgrf1hash!=NULL)
    delete[] lgrf1hash;
  if (lgrf1hash2!=NULL)
    delete[] lgrf1hash2;
  if (lgrf2hash!=NULL)
    delete[] lgrf2hash;
  if (lgrf1count!=NULL)
    delete[] lgrf1count;
  if (lgrf2count!=NULL)
    delete[] lgrf2count;
}

void Playout::doPlayout(Worker::Settings *settings, Go::Board *board, float &finalscore, float &cnn_winrate, Tree *playouttree, std::list<Go::Move> &playoutmoves, Go::Color colfirst, Go::IntBoard *firstlist, Go::IntBoard *secondlist, Go::IntBoard *earlyfirstlist, Go::IntBoard *earlysecondlist, std::list<std::string> *movereasons)
{
  std::list<unsigned int> movehashes3x3;
  std::list<unsigned long> movehashes5x5;

  params->engine->ProbabilityClean();

  // can not score board if not played out with playout rules
  //if (board->getPassesPlayed()>=2)
  //{
  //  finalscore=board->score(params)-params->engine->getScoreKomi();
  //  return;
  //}
  int treemovescount=0;
  board->turnSymmetryOff();
  
  if (params->debug_on)
    gtpe->getOutput()->printfDebug("[playout]:");
  for(std::list<Go::Move>::iterator iter=playoutmoves.begin();iter!=playoutmoves.end();++iter)
  {
    if (params->playout_lgpf_enabled || params->playout_lgrf1_safe_enabled || params->playout_lgrf2_safe_enabled)
    {
      movehashes3x3.push_back(Pattern::ThreeByThree::makeHash(board,(*iter).getPosition()));
      if (params->playout_lgpf_enabled)
        movehashes5x5.push_back(Pattern::FiveByFiveBorder::makeHash(board,(*iter).getPosition()));
    }
    board->makeMove((*iter),gtpe);
    treemovescount++;
    if (movereasons!=NULL)
      movereasons->push_back("given");
    if (params->debug_on)
      gtpe->getOutput()->printfDebug(" %s",(*iter).toString(board->getSize()).c_str());
    if (((*iter).getColor()==colfirst?firstlist:secondlist)!=NULL && !(*iter).isPass() && !(*iter).isResign())
    {
      if (params->test_p2==0 || ((*iter).getColor()!=colfirst?firstlist:secondlist)->get((*iter).getPosition())==false)
      {
        ((*iter).getColor()==colfirst?firstlist:secondlist)->set((*iter).getPosition());
        if (((*iter).getColor()==colfirst?earlyfirstlist:earlysecondlist)!=NULL)
          ((*iter).getColor()==colfirst?earlyfirstlist:earlysecondlist)->set((*iter).getPosition());
      }
    }
    // can not score board if not played out with playout rules
    // no idea when isResign should happen here?!
    if ( //board->getPassesPlayed()>=2 || 
        (*iter).isResign())
    {
      if (params->debug_on)
        gtpe->getOutput()->printfDebug("\n");
      finalscore=board->score(params)-(params->engine->getScoreKomi());
      return;
    }
  }
  if (params->debug_on)
    gtpe->getOutput()->printfDebug("\n");
  
  if (params->rules_positional_superko_enabled && params->rules_superko_at_playout && playouttree!=NULL && !playouttree->isSuperkoChecked())
  {
    Go::ZobristHash hash=board->getZobristHash(params->engine->getZobristTable());
    playouttree->setHash(hash);
    playouttree->doSuperkoCheck();
    if (playouttree->isSuperkoViolation())
    {
      finalscore=0;
      //fprintf(stderr,"superko\n");
      return;
    }
  }

  // setup poolRAVE and its variants
  std::vector<int> pool;
  std::vector<int> poolother;
  std::vector<int> poolcrit;
  Go::Color poolcol=Go::EMPTY;
  critstruct *critarray=NULL;
  float *b_ravearray=NULL;
  float *w_ravearray=NULL;
  if (params->playout_criticality_random_n>0 || params->test_p15>0 || params->test_p47>0 || params->test_p68>0)
  {
    Tree *pooltree=playouttree;
    critarray=new critstruct[board->getPositionMax()];
    for (int i=0;i<board->getPositionMax ();i++)
        critarray[i]={0,0,0,0,0};
    b_ravearray=new float[board->getPositionMax()];
    w_ravearray=new float[board->getPositionMax()];
    if (playouttree!=NULL)
    {
      while (!pooltree->isRoot() && (pooltree->getRAVEPlayouts()<params->playout_poolrave_min_playouts || pooltree->isLeaf()))
        pooltree=pooltree->getParent();
      if (pooltree->getRAVEPlayouts()<params->playout_poolrave_min_playouts)
        pooltree=NULL;
    }
    else 
      fprintf(stderr,"playouttree==NULL\n");
    //if (pooltree==NULL)
    //  fprintf(stderr,"pooltree==NULL\n");
    if (pooltree!=NULL)
    {
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
                    (*iter)->getMove().toString(board->getSize()).c_str(),(*iter)->getMove().getPosition(),
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
  }
  if (params->playout_poolrave_enabled || params->playout_poolrave_criticality)
  {
    Tree *pooltree=playouttree;
    if (playouttree!=NULL)
    {
      while (!pooltree->isRoot() && (pooltree->getRAVEPlayouts()<params->playout_poolrave_min_playouts || pooltree->isLeaf()))
        pooltree=pooltree->getParent();
      if (pooltree->getRAVEPlayouts()<params->playout_poolrave_min_playouts)
        pooltree=NULL;
    }
    // color of tree nodes isn't taken into account

    if (pooltree!=NULL)
    {
      int k;
      if ((int)pooltree->getChildren()->size()<params->playout_poolrave_k)
        k=pooltree->getChildren()->size();
      else
        k=params->playout_poolrave_k;

      // below method is simple but inefficient if log(n)<k
      int totalused=0;
      Go::BitBoard *used=new Go::BitBoard(board->getSize());
      Go::BitBoard *usedother=new Go::BitBoard(board->getSize());
      Go::BitBoard *usedcrit=new Go::BitBoard(board->getSize());
      //fprintf(stderr,"playout\n");
      while (totalused<k)
      {
        float bestval=-1; // accept any values
        int bestpos=-1;
        float bestvalother=-1;
        int bestposother=-1;
        float bestvalcrit=-1;
        int bestposcrit=-1;
        for(std::list<Tree*>::iterator iter=pooltree->getChildren()->begin();iter!=pooltree->getChildren()->end();++iter) 
        {
          if (!(*iter)->getMove().isPass() && !used->get((*iter)->getMove().getPosition()) && (*iter)->getRAVERatioForPool()>bestval)
          {
            bestpos=(*iter)->getMove().getPosition();
            bestval=(*iter)->getRAVERatioForPool();
            poolcol=(*iter)->getMove().getColor();
          }
          if (!(*iter)->getMove().isPass() && !usedother->get((*iter)->getMove().getPosition()) && (*iter)->getRAVERatioOtherForPool()>bestvalother)
          {
            bestposother=(*iter)->getMove().getPosition();
            bestvalother=(*iter)->getRAVERatioOtherForPool();
          }
          if (!(*iter)->getMove().isPass() && !usedcrit->get((*iter)->getMove().getPosition()) && (*iter)->getCriticality()>bestvalcrit)
          {
            bestposcrit=(*iter)->getMove().getPosition();
            bestvalcrit=(*iter)->getCriticality();
          }
        }
        if (bestpos!=-1)
        {
          pool.push_back(bestpos);
          used->set(bestpos);
        }
        if (bestposother!=-1)
        {
          poolother.push_back(bestposother);
          usedother->set(bestposother);
        }
        if (bestposcrit!=-1)
        {
          poolcrit.push_back(bestposcrit);
          usedcrit->set(bestposcrit);
        }
        totalused++;
      }
      delete used;
      delete usedother;
      delete usedcrit;
    }
  }

  if (params->playout_features_enabled>0)
    board->updateFeatureGammas(true);

  float *white_gammas=NULL;
  float *black_gammas=NULL;
  if (0) // not working yet:( ?? params->playout_features_enabled>0)
  {
    //create an array of gamma values for black and for white
    white_gammas=new float[board->getPositionMax()];
    black_gammas=new float[board->getPositionMax()];
    for (int i=0;i<board->getPositionMax();i++)
    {
      white_gammas[i]=0;
      black_gammas[i]=0;
    }
    Tree * lastmove=playouttree;
    for (int i=0;i<2;i++)
    {
      if (lastmove==NULL || lastmove->isRoot())
        break;
      lastmove=lastmove->getParent();
      fprintf(stderr,"lastmove %d %s\n",i,lastmove->getMove().toString(19).c_str());
      std::list<Tree*> *childrenTmp=lastmove->getChildren();
      for(std::list<Tree*>::iterator iter=childrenTmp->begin();iter!=childrenTmp->end();++iter) 
      { 
        Go::Move move_tmp=(*iter)->getMove();
        if (move_tmp.isNormal())
        {
          Go::Color c=move_tmp.getColor();
          switch (c)
          {
            case Go::BLACK:
              //fprintf(stderr,"BLACK gamma %d %f\n",move_tmp.getPosition(),(*iter)->getFeatureGamma());
              black_gammas[move_tmp.getPosition()]=(*iter)->getFeatureGamma();
              break;
            case Go::WHITE:
              //fprintf(stderr,"WHITE gamma %d %f\n",move_tmp.getPosition(),(*iter)->getFeatureGamma());
              white_gammas[move_tmp.getPosition()]=(*iter)->getFeatureGamma();
              break;
            default:
              break;
          }
        }
      }
      
    }
  }
  
  Go::IntBoard *treeboardBlack=NULL;
  Go::IntBoard *treeboardWhite=NULL;
  int used_playouts=0;
  if (params->test_p87) {
    treeboardBlack=new Go::IntBoard(board->getSize()); treeboardBlack->clear();
    treeboardWhite =new Go::IntBoard(board->getSize()); treeboardWhite->clear();
    Tree *movetree=playouttree;
    while (movetree!=NULL && !movetree->isRoot())
      movetree=movetree->getParent();
    movetree->fillTreeBoard(treeboardBlack,treeboardWhite);
    used_playouts=movetree->getPlayouts();
  }
  
  Go::Color coltomove=board->nextToMove();
  Go::Move move=Go::Move(coltomove,Go::Move::PASS);
  int movesalready=board->getMovesMade();
  int *posarray = new int[board->getPositionMax()];
  bool mercywin=false;
  std::string reason;
  board->resetCaptures(); // for mercy rule
  int playoutmovescount=0;
  int bpasses=0;
  int wpasses=0;
  int kodelay=(board->isCurrentSimpleKo()?3:0); // if a ko occurs near the end of a playout, carry on for a bit

  float trylocal_p=1.0;
  bool earlymoves=true;

  
  //board->resetPassesPlayed(); // if the tree plays two passes it is not guaranteed, that score can count the result!!!
  //but it is the chinese counting, therefore the win is correct, if it is not played to the end within this counting

  //cnn_winrate==-2 disables cnn, for other calls than genmove
  //lock can be optimized not to lock, if tree wr is used !!!!!!!!!!!!!!!!!!!!!!!!!
  if (params->test_p105>0 && cnn_winrate>-2 && playouttree->cnn_territory_done<params->test_p105 && params->engine->CNNmutex.try_lock()) {
    if (playouttree->cnn_territory_wr>-1)
      cnn_winrate=playouttree->cnn_territory_wr;
    else {
      cnn_winrate=params->engine->getCNNwr (board,coltomove);
      playouttree->cnn_territory_wr=cnn_winrate;
    }
    if (coltomove!=Go::BLACK)
      cnn_winrate=1.0-cnn_winrate;
    playouttree->cnn_territory_done+=params->test_p105*params->test_p106;
    params->engine->CNNmutex.unlock();
  }
  //do both, CNN and playout if commented
  //else 
  {
  while (board->getPassesPlayed()<2 || kodelay>0)
  {
    bool resign;
    bool nonlocalmove=false;
    if (movereasons!=NULL)
    {
      this->getPlayoutMove(settings,board,coltomove,move,posarray,critarray,(coltomove==Go::BLACK)?b_ravearray:w_ravearray,(coltomove==Go::BLACK?bpasses:wpasses),(coltomove==poolcol?&pool:&poolcrit),&poolcrit,&reason,&trylocal_p,black_gammas,white_gammas,&earlymoves,(coltomove==colfirst?firstlist:secondlist),playoutmovescount+1,&nonlocalmove,treeboardBlack,treeboardWhite,used_playouts);
      //fprintf(stderr,"move test %s\n",move.toString (9).c_str());
      if (params->playout_useless_move)
        this->checkUselessMove(settings,board,coltomove,move,posarray,&reason);
    }
    else
    {
      this->getPlayoutMove(settings,board,coltomove,move,posarray,critarray,(coltomove==Go::BLACK)?b_ravearray:w_ravearray,(coltomove==Go::BLACK?bpasses:wpasses),(coltomove==poolcol?&pool:&poolcrit),&poolcrit,NULL,&trylocal_p,black_gammas,white_gammas,&earlymoves,(coltomove==colfirst?firstlist:secondlist),playoutmovescount+1,&nonlocalmove,treeboardBlack,treeboardWhite,used_playouts);
      //fprintf(stderr,"move test %s\n",move.toString (9).c_str());
      if (params->playout_useless_move)
        this->checkUselessMove(settings,board,coltomove,move,posarray);
    }
    
    if ((params->playout_lgpf_enabled || params->playout_lgrf1_safe_enabled || params->playout_lgrf2_safe_enabled) && move.isNormal())
    {
      movehashes3x3.push_back(Pattern::ThreeByThree::makeHash(board,move.getPosition()));
      if (params->playout_lgpf_enabled)
        movehashes5x5.push_back(Pattern::FiveByFiveBorder::makeHash(board,move.getPosition()));
    }

    if (params->debug_on)
      fprintf(stderr,"playout makeMove %s playoutmovescount %d nonlocal %d\n",move.toString (board->getSize()).c_str(),playoutmovescount,nonlocalmove); 
    board->makeMove(move);
    playoutmoves.push_back(move);
    playoutmovescount++;
    if (params->test_p4>0)
      params->engine->ProbabilityMoveAs(move.getPosition(),playoutmovescount);
    if (move.isPass())
      (coltomove==Go::BLACK)? bpasses++ : wpasses++;
    if (movereasons!=NULL)
      movereasons->push_back(reason);
    int p=move.getPosition();
    if ((coltomove==colfirst?firstlist:secondlist)!=NULL && !move.isPass() && !move.isResign())
    {
      if ((params->test_p2==0 || ((coltomove==colfirst)?firstlist:secondlist)->get(p)==0))
      {
        if (params->debug_on) 
          fprintf(stderr,"set called %s test_p2 %f get(p) %d\n",Go::getColorName(coltomove),params->test_p2,((coltomove==colfirst)?firstlist:secondlist)->get(p));
        //if (p==228 || p==231) fprintf(stderr,"add a move %s %d\n",Go::Move(coltomove,p).toString(19).c_str(),nonlocalmove);
        ((coltomove==colfirst)?firstlist:secondlist)->set(p,playoutmovescount,nonlocalmove);
        if (params->debug_on) 
          fprintf(stderr,"after set %s test_p2 %f get(p) %d\n",Go::getColorName(coltomove),params->test_p2,((coltomove==colfirst)?firstlist:secondlist)->get(p));
        if (((coltomove==colfirst)?earlyfirstlist:earlysecondlist)!=NULL && params->rave_moves_use>0 && playoutmovescount < (board->getSize()*board->getSize()-treemovescount)*params->rave_moves_use)
          ((coltomove==colfirst)?earlyfirstlist:earlysecondlist)->set(p);
      }
    }
    resign=move.isResign();
    coltomove=Go::otherColor(coltomove);
    if (resign)
      break;
    if (params->playout_mercy_rule_enabled)
    {
      if (coltomove==Go::BLACK && (board->getStoneCapturesOf(Go::BLACK)-board->getStoneCapturesOf(Go::WHITE))>(board->getSize()*board->getSize()*params->playout_mercy_rule_factor))
      {
        mercywin=true;
        finalscore=-0.5;
        break;
      }
      else if (coltomove==Go::WHITE && (board->getStoneCapturesOf(Go::WHITE)-board->getStoneCapturesOf(Go::BLACK))>(board->getSize()*board->getSize()*params->playout_mercy_rule_factor))
      {
        mercywin=true;
        finalscore=+0.5;
        break;
      }
    }
    if (board->getMovesMade()>(board->getSize()*board->getSize()*PLAYOUT_MAX_MOVE_FACTOR+movesalready))
      break;

    if (board->isCurrentSimpleKo())
      kodelay=3;
    else if (kodelay>0)
      kodelay--;
  }
  }
  delete[] posarray;
  if (critarray)
    delete[] critarray;
  if (b_ravearray)
    delete[] b_ravearray;
  if (w_ravearray)
    delete[] w_ravearray;
  if (!mercywin)
    finalscore=board->score(params)-params->engine->getScoreKomi();
  //Go::Color playoutcol=playoutmoves.back().getColor();
  //bool playoutwin=Go::Board::isWinForColor(playoutcol,finalscore);
  bool playoutjigo=(finalscore==0);
  
  if (params->playout_lgrf1_enabled || params->playout_avoid_lbmf_p || params->playout_avoid_lbmf_p2)
  {
    if (!playoutjigo) // ignore jigos
    {
      bool blackwin=Go::Board::isWinForColor(Go::BLACK,finalscore);
      Go::Color wincol=(blackwin?Go::BLACK:Go::WHITE);
      
      std::list<unsigned int>::iterator movehashiter=movehashes3x3.begin();
      Go::Move move1=Go::Move(Go::EMPTY,Go::Move::PASS); // the last move
      unsigned int poshashlast=0;
      for(std::list<Go::Move>::iterator iter=playoutmoves.begin();iter!=playoutmoves.end();++iter)
      {
        if (!(*iter).isPass() && !move1.isPass())
        {
          Go::Color c=(*iter).getColor();
          int mp=(*iter).getPosition();
          int p1=move1.getPosition();
          unsigned int poshash=*movehashiter;
          bool iswin;
          if (params->playout_lgrf_local)
            iswin=(c==board->getScoredOwner(mp) && (c==wincol));
          else
            iswin=(c==wincol);
          // Below couldn't be disabled with parameter
          // if ((*iter).is_useforlgrf ())
          // {
            if (iswin)
            {
              if (params->debug_on)
                fprintf(stderr,"adding LGRF1: %s %s\n",move1.toString(board->getSize()).c_str(),(*iter).toString(board->getSize()).c_str());
              if (params->playout_lgrf1_safe_enabled)
                this->setLGRF1(c,p1,mp,poshash,poshashlast);
              else
                this->setLGRF1(c,p1,mp);
              this->clearLGRF1n(c,p1,mp);
            }
            else
            {
              if (params->debug_on && this->hasLGRF1(c,p1))
                fprintf(stderr,"forgetting LGRF1: %s %s\n",move1.toString(board->getSize()).c_str(),(*iter).toString(board->getSize()).c_str());
              this->clearLGRF1(c,p1);
              this->setLGRF1n(c,p1,mp);
            }
          // }
          poshashlast=poshash;
        }
        ++movehashiter;
        if (!(*iter).isPass() && move1.isPass())
        {
          Go::Color c=(*iter).getColor();
          int mp=(*iter).getPosition();
          //int p1=move1.getPosition();
          bool iswin;
          if (params->playout_lgrf_local)
            iswin=(c==board->getScoredOwner(mp) && (c==wincol));
          else
            iswin=(c==wincol);
          if (iswin)
          {
            //fprintf(stderr,"forgetting LGRF1n_PASS: %s %s\n",move1.toString(board->getSize()).c_str(),(*iter).toString(board->getSize()).c_str());
            this->clearBadPassAnswer(c,mp);
          }
          else
          {
            //fprintf(stderr,"setting LGRF1n_PASS: %s %s\n",move1.toString(board->getSize()).c_str(),(*iter).toString(board->getSize()).c_str());
            this->setBadPassAnswer(c,mp);
          }
        }
        move1=(*iter);
      }
    }
  }
  
  if (params->playout_lgrf2_enabled)
  {
    if (!playoutjigo) // ignore jigos
    {
      bool blackwin=Go::Board::isWinForColor(Go::BLACK,finalscore);
      Go::Color wincol=(blackwin?Go::BLACK:Go::WHITE);
      
      Go::Move move1=Go::Move(Go::EMPTY,Go::Move::PASS);
      Go::Move move2=Go::Move(Go::EMPTY,Go::Move::PASS);
      std::list<unsigned int>::iterator movehashiter=movehashes3x3.begin();
      for(std::list<Go::Move>::iterator iter=playoutmoves.begin();iter!=playoutmoves.end();++iter)
      {
        if (!(*iter).isPass() && !move1.isPass() && !move2.isPass())
        {
          Go::Color c=(*iter).getColor();
          int mp=(*iter).getPosition();
          int p1=move1.getPosition();
          int p2=move2.getPosition();
          unsigned int poshash=*movehashiter;
          bool iswin;
          if (params->playout_lgrf_local)
            iswin=(c==board->getScoredOwner (mp) && (c==wincol));
          else
            iswin=(c==wincol);
          // Below couldn't be disabled with parameter
          // if ((*iter).is_useforlgrf())
          // {
            if (iswin)
            {
              if (params->debug_on)
                fprintf(stderr,"adding LGRF2: %s %s %s\n",move1.toString(board->getSize()).c_str(),move2.toString(board->getSize()).c_str(),(*iter).toString(board->getSize()).c_str());
              if (params->playout_lgrf2_safe_enabled)
                this->setLGRF2(c,p1,p2,mp,poshash);
              else
                this->setLGRF2(c,p1,p2,mp);
              this->setLGRF1o(c,p1,mp);
            }
            else
            {
              if (params->debug_on && this->hasLGRF2(c,p1,p2))
                fprintf(stderr,"forgetting LGRF2: %s %s %s\n",move1.toString(board->getSize()).c_str(),move2.toString(board->getSize()).c_str(),(*iter).toString(board->getSize()).c_str());
              this->clearLGRF2(c,p1,p2);
              this->clearLGRF1o(c,p1);
            }
          // }
        }
        ++movehashiter;
        move1=move2;
        move2=(*iter);
      }
    }
  }

  if (params->playout_lgpf_enabled)
  {
    if (!playoutjigo) // ignore jigos
    {
      bool blackwin=Go::Board::isWinForColor(Go::BLACK,finalscore);
      Go::Color wincol=(blackwin?Go::BLACK:Go::WHITE);
      
      std::list<unsigned int>::iterator movehashiter=movehashes3x3.begin();
      std::list<unsigned long>::iterator movehashiter2=movehashes5x5.begin();
      for(std::list<Go::Move>::iterator iter=playoutmoves.begin();iter!=playoutmoves.end();++iter)
      {
        if (!(*iter).isPass())
        {
          Go::Color c=(*iter).getColor();
          int mp=(*iter).getPosition();
          unsigned int p1=*movehashiter;
          unsigned long p1_b=*movehashiter2;
          bool iswin;
          if (params->playout_lgrf_local)
            iswin=(c==board->getScoredOwner(mp) && (c==wincol));
          else
            iswin=(c==wincol);
          if (iswin)
          {
            if (params->debug_on)
              fprintf(stderr,"adding LGRP: %s %x\n",(*iter).toString(board->getSize()).c_str(),p1);
            this->setLGPF(settings,c,mp,p1,p1_b);
          }
          else
          {
            this->clearLGPF(c,mp,p1,p1_b);
            if (params->debug_on && this->hasLGPF(c,mp,p1,p1_b))
              fprintf(stderr,"forgetting LGRP: %s %x\n",(*iter).toString(board->getSize()).c_str(),p1);
          }
        }
        ++movehashiter;
        ++movehashiter2;
      }
    }
  }

  if (white_gammas!=NULL)
      delete white_gammas;
  if (black_gammas!=0)
      delete black_gammas;
  if (treeboardBlack!=NULL)
    delete treeboardBlack;
  if (treeboardWhite!=NULL)
    delete treeboardWhite;
}

void Playout::getPlayoutMove(Worker::Settings *settings, Go::Board *board, Go::Color col, Go::Move &move, critstruct critarray[], float ravearray[], int passes, std::vector<int> *pool, std::vector<int> *poolCR, std::string *reason, float *trylocal_p, float *black_gammas, float *white_gammas, bool *earlymoves, Go::IntBoard *firstlist,int playoutmovescount, bool *nonlocalmove,Go::IntBoard *treeboardBlack,Go::IntBoard *treeboardWhite, int used_playouts)
{
  int *posarray = new int[board->getPositionMax()];
  
  this->getPlayoutMove(settings,board,col,move,posarray,critarray,ravearray,passes,pool,poolCR,reason,trylocal_p,black_gammas,white_gammas,earlymoves, firstlist, playoutmovescount, nonlocalmove,treeboardBlack,treeboardWhite, used_playouts);
  
  delete[] posarray;
}

void Playout::checkUselessMove(Worker::Settings *settings, Go::Board *board, Go::Color col, Go::Move &move, std::string *reason)
{
  //not needed for the actual replacements
  int *posarray = NULL; //new int[board->getPositionMax()];
  
  this->checkUselessMove(settings,board,col,move,posarray,reason);
  
  //delete[] posarray;
}


void Playout::checkUselessMove(Worker::Settings *settings, Go::Board *board, Go::Color col, Go::Move &move, int *posarray, std::string *reason)
{
  Go::Move replacemove;
  this->checkEyeMove(settings,board,col,move,posarray,replacemove);
  // Below couldn't be disabled with parameter
  // But the default is checkUselessMove () disabled anyway!
  if (replacemove.isPass())
    this->checkAntiEyeMove(settings,board,col,move,posarray,replacemove);
  if (replacemove.isPass())
    this->checkEmptyTriangleMove(settings,board,col,move,posarray,replacemove);
  if (reason!=NULL && replacemove.isNormal())
    (*reason).append(" UselessEye");
  if (!replacemove.isPass())
  {
    if (params->debug_on)
        gtpe->getOutput()->printfDebug("[before replace]: %s \n",move.toString(board->getSize()).c_str());
    move=replacemove;
  }
}

void Playout::getPlayoutMove(Worker::Settings *settings, Go::Board *board, Go::Color col, Go::Move &move, int *posarray, critstruct critarray[], float ravearray[], int passes, std::vector<int> *pool, std::vector<int> *poolcrit, std::string *reason, float *trylocal_p, float *black_gammas, float *white_gammas, bool *earlymoves, Go::IntBoard *firstlist,int playoutmovescount, bool *nonlocalmove,Go::IntBoard *treeboardBlack,Go::IntBoard *treeboardWhite, int used_playouts)
{
  /* trylocal_p can be used to influence parameters from move to move in a playout. It starts with 1.0
   * 
   */
  
  /*
   The playout_order==4 and safe_lgrfs try to do the following logic
   1. Learned moves, which (with a high probability) do not break local response chains (safe lgrf)
   2. Local response moves
   3. Learned moves which possible break local response chains as lgrf moves sometimes do
   4. Non local learned moves as lgpf might be
   5. Non local moves as random ...
   */
  //fprintf(stderr,"playoutmove\n");
  Random *const rand=settings->rand;
  int ncirc=0;

  move=Go::Move(col,Go::Move::PASS);
  if (board->numOfValidMoves(col)==0)
    return;
  
  if (params->playout_random_chance>0)
  {
    float f=rand->getRandomReal();
    if (f<params->playout_random_chance)
      goto random;
  }

  if (params->playout_order==1 && params->playout_lastcapture_enabled)
  {
    this->getLastCaptureMove(settings,board,col,move,posarray);
    if (!move.isPass())
    {
      if (params->debug_on)
        gtpe->getOutput()->printfDebug("[playoutmove]: %s lastcapture\n",move.toString(board->getSize()).c_str());
      if (reason!=NULL)
        *reason="lastcapture";
      return;
    }
  }

  // LGRF is back here because a parameter couldn't do it!
  if (params->playout_lgrf2_enabled && params->playout_lgrf2_safe_enabled)
  {
    this->getLGRF2Move(settings, board,col,move);
    if (!move.isPass())
    {
      int p=move.getPosition();
      int pos1=board->getSecondLastMove().getPosition();
      int pos2=board->getLastMove().getPosition();
      unsigned int hash3x3=Pattern::ThreeByThree::makeHash(board,p);
      if (hash3x3==this->getLGRF2hash(col,pos1,pos2))
      {
        if (params->debug_on)
          gtpe->getOutput()->printfDebug("[playoutmove]: %s lgrf2\n",move.toString(board->getSize()).c_str());
        if (reason!=NULL)
          *reason="save lgrf2";
        params->engine->statisticsPlus(Engine::LGRF2);
      move.set_useforlgrf (true);
        return;
      }
      move=Go::Move(col,Go::Move::PASS);
    }
  }
  
  if (params->playout_lgrf2_enabled && !params->playout_lgrf2_safe_enabled)
  {
    this->getLGRF2Move(settings, board,col,move);
    if (!move.isPass())
    {
      if (params->debug_on)
        gtpe->getOutput()->printfDebug("[playoutmove]: %s lgrf2\n",move.toString(board->getSize()).c_str());
      if (reason!=NULL)
        *reason="lgrf2";
      params->engine->statisticsPlus(Engine::LGRF2);
      move.set_useforlgrf (true);
      return;
    }
  }

  if (params->playout_lgrf1_enabled && params->playout_lgrf1_safe_enabled)
  {
    //safe LGRF1 move
    this->getLGRF1Move(settings, board,col,move);
    if (!move.isPass())
    {
      int p=move.getPosition();
      unsigned int hash3x3=Pattern::ThreeByThree::makeHash(board,p);
      int pos1=board->getLastMove().getPosition();
      unsigned int hash3x3_2=Pattern::ThreeByThree::makeHash(board,pos1);
      if (hash3x3!=0 && this->getLGRF1hash(col,pos1)==hash3x3 && this->getLGRF1hash2(col,pos1)==hash3x3_2) //,Pattern::FiveByFiveBorder::makeHash(board,p)))
      {
        if (params->debug_on)
          gtpe->getOutput()->printfDebug("[playoutmove]: %s safe lgrf1\n",move.toString(board->getSize()).c_str());
        if (reason!=NULL)
          *reason="safe lgrf1";
        move.set_useforlgrf (true);
        return;
      }
      move=Go::Move(col,Go::Move::PASS);
    }
  }

  if (params->playout_lgrf1_enabled  && !params->playout_lgrf1_safe_enabled)
  {
    this->getLGRF1Move(settings, board,col,move);
    if (!move.isPass())
    {
      if (params->debug_on)
        gtpe->getOutput()->printfDebug("[playoutmove]: %s lgrf1\n",move.toString(board->getSize()).c_str());
      if (reason!=NULL)
        *reason="lgrf1";
      move.set_useforlgrf (true);
      return;
    }
  }
  
  if (params->playout_atari_enabled)
  {
    this->getAtariMove(settings,board,col,move,posarray);
    if (!move.isPass())
    {
      if (params->debug_on)
        gtpe->getOutput()->printfDebug("[playoutmove]: %s atari\n",move.toString(board->getSize()).c_str());
      if (reason!=NULL)
        *reason="atari";
      return;
    }
  }

//  if (trylocal_p!=NULL && !WITH_P(*trylocal_p))
//    goto notlocal;
  
  if (params->playout_lastatari_p>0.0 && (WITH_P(params->test_p34)||*earlymoves)) //p is used in the getLastAtariMove function
  {
    this->getLastAtariMove(settings,board,col,move,posarray,params->test_p19);
    if (!move.isPass())
    {
      if (params->debug_on)
        gtpe->getOutput()->printfDebug("[playoutmove]: %s lastatari\n",move.toString(board->getSize()).c_str());
      if (reason!=NULL)
        *reason="lastatari";
      params->engine->statisticsPlus(Engine::LASTATARI);
      return;
    }
  }
  
  if ((params->playout_order==0 || params->playout_order>3) && params->playout_lastcapture_enabled && (WITH_P(params->test_p33)||*earlymoves))
  {
    this->getLastCaptureMove(settings,board,col,move,posarray);
    if (!move.isPass())
    {
      if (params->debug_on)
        gtpe->getOutput()->printfDebug("[playoutmove]: %s lastcapture\n",move.toString(board->getSize()).c_str());
      if (reason!=NULL)
        *reason="lastcapture";
      params->engine->statisticsPlus(Engine::LASTCAPTURE);
      return;
    }
  }

  if (params->playout_last2libatari_enabled && (WITH_P(params->test_p34)||*earlymoves))
  {
    this->getLast2LibAtariMove(settings,board,col,move,posarray);
    if (!move.isPass())
    {
      if (params->debug_on)
        gtpe->getOutput()->printfDebug("[playoutmove]: %s last2libatari\n",move.toString(board->getSize()).c_str());
      if (reason!=NULL)
        *reason="last2libatari";
      params->engine->statisticsPlus(Engine::LAST2LIBATARI);
      return;
    }
  }

  if (params->playout_order==2 && params->playout_lastcapture_enabled)
  {
    this->getLastCaptureMove(settings,board,col,move,posarray);
    if (!move.isPass())
    {
      if (params->debug_on)
        gtpe->getOutput()->printfDebug("[playoutmove]: %s lastcapture\n",move.toString(board->getSize()).c_str());
      if (reason!=NULL)
        *reason="lastcapture";
      return;
    }
  }

  if (params->playout_nakade_enabled && WITH_P(params->test_p35))
  {
    this->getNakadeMove(settings,board,col,move,posarray);
    //fprintf(stderr,"move nakade %s\n",move.toString (9).c_str());
    if (!move.isPass())
    {
      if (params->debug_on)
        gtpe->getOutput()->printfDebug("[playoutmove]: %s nakade\n",move.toString(board->getSize()).c_str());
      if (reason!=NULL)
        *reason="nakade";
      params->engine->statisticsPlus(Engine::NAKED);
      return;
    }
  }

  if (move.isPass()&&earlymoves!=NULL)
    *earlymoves=false;

  if (params->playout_nearby_enabled)
  {
    this->getNearbyMove(settings,board,col,move);
    if (!move.isPass())
    {
      if (params->debug_on)
        gtpe->getOutput()->printfDebug("[playoutmove]: %s nearby\n",move.toString(board->getSize()).c_str());
      if (reason!=NULL)
        *reason="nearby";
      return;
    }
  }

  if (params->playout_order!=4 && params->playout_fillboard_enabled)
  {
    std::string tmpreason="fillboard";
    this->getFillBoardMove(settings,board,col,move,posarray,passes,&tmpreason);
    if (!move.isPass())
    {
      if (params->debug_on)
        gtpe->getOutput()->printfDebug("[playoutmove]: %s fillboard\n",move.toString(board->getSize()).c_str());
      if (reason!=NULL)
        *reason=tmpreason;
      return;
    }
  }

  if (params->features_output_for_playout)
  {
    move=Go::Move(col,Go::Move::PASS);
    return ;  //used for training of playout patterns 3x3. If returned PASS the pattern move is triggered!
  }


  if (nonlocalmove!=NULL) *nonlocalmove=true;
  
  //fprintf(stderr,"should be nonlocal\n");
  
  if (WITH_P(params->playout_patterns_p))
  {
    this->getPatternMove(settings,board,col,move,posarray,passes,critarray);
    if (board->validMove(move) && !this->isBadMove(settings,board,col,move.getPosition(),params->playout_avoid_lbrf1_p,params->playout_avoid_lbmf_p, params->playout_avoid_bpr_p, passes, NULL, 0, critarray))
    {
      if (!move.isPass())
      {
        if (params->debug_on)
          gtpe->getOutput()->printfDebug("[playoutmove]: %s pattern\n",move.toString(board->getSize()).c_str());
        if (reason!=NULL)
        	*reason="pattern";
        params->engine->statisticsPlus(Engine::PATTERN);
        return;
      }
    }
    else
      move=Go::Move(col,Go::Move::PASS);
  }

//  notlocal:
//  if (trylocal_p!=NULL) *trylocal_p*=params->test_p1;
  
  if (WITH_P(params->playout_anycapture_p))
  {
    this->getAnyCaptureMove(settings,board,col,move,posarray);
    if (!move.isPass())
    {
      if (params->debug_on)
        gtpe->getOutput()->printfDebug("[playoutmove]: %s anycapture\n",move.toString(board->getSize()).c_str());
      if (reason!=NULL)
        *reason="anycapture";
      params->engine->statisticsPlus(Engine::ANYCAPTURE);
      return;
    }
  }
  
  while (ncirc<params->playout_circpattern_n)
  {
    ncirc++;
    int p=rand->getRandomInt(board->getPositionMax());
    if (board->validMove(col,p) && !this->isBadMove(settings,board,col,p,params->playout_avoid_lbrf1_p,params->playout_avoid_lbmf_p, params->playout_avoid_bpr_p, passes, NULL, 0, critarray))
    {
      Pattern::Circular pattcirc=Pattern::Circular(params->engine->getCircDict(),board,p,params->engine->getCircSize());
      if (col==Go::WHITE)
        pattcirc.invert();
      pattcirc.convertToSmallestEquivalent(params->engine->getCircDict());
      if (params->engine->isCircPattern(pattcirc.toString(params->engine->getCircDict())))
      {
        move=Go::Move(col,p);
        if (params->debug_on)
          gtpe->getOutput()->printfDebug("[playoutmove]: %s circpattern quick-pick %s\n",move.toString(board->getSize()).c_str(),pattcirc.toString(params->engine->getCircDict()).c_str());
        if (reason!=NULL)
          *reason="circpattern quick-pick";
        params->engine->statisticsPlus(Engine::CIRCPATTERN_QUICK);
        return;
      }
    }
  }

  if (params->playout_poolrave_enabled)
  {
    this->getPoolRAVEMove(settings,board,col,move,pool);
    if (!move.isPass())
    {
      if (params->debug_on)
        gtpe->getOutput()->printfDebug("[playoutmove]: %s poolrave\n",move.toString(board->getSize()).c_str());
      if (reason!=NULL)
      {
        *reason="poolrave";
        for (unsigned int i=0;i<pool->size();i++)
          *reason+=" "+Go::Position::pos2string(pool->at(i),board->getSize());
      }
      return;
    }
  }
  
  if (params->playout_poolrave_criticality)
  {
    this->getPoolRAVEMove(settings,board,col,move,poolcrit);
    if (!move.isPass())
    {
      if (params->debug_on)
        gtpe->getOutput()->printfDebug("[playoutmove]: %s poolcriticality\n",move.toString(board->getSize()).c_str());
      if (reason!=NULL)
      {
        *reason="poolcriticality";
        for (unsigned int i=0;i<poolcrit->size();i++)
          *reason+=" "+Go::Position::pos2string(poolcrit->at(i),board->getSize());
      }
      return;
    }
  }
  
  if (params->playout_lgrf1o_enabled)
  {
    this->getLGRF1oMove(settings,board,col,move);
    if (!move.isPass())
    {
      if (params->debug_on)
        gtpe->getOutput()->printfDebug("[playoutmove]: %s lgrf1o\n",move.toString(board->getSize()).c_str());
      if (reason!=NULL)
        *reason="lgrf1o";
      return;
    }
  }
  
  if (params->playout_lgpf_enabled)
  {
    this->getLGPFMove(settings,board,col,move,posarray);
    if (!move.isPass())
    {
      if (params->debug_on)
        gtpe->getOutput()->printfDebug("[playoutmove]: %s lgpf\n",move.toString(board->getSize()).c_str());
      if (reason!=NULL)
        *reason="lgpf";
      return;
    }
  }
  
 if (params->playout_order==4 && params->playout_fillboard_enabled)
  {
    if (params->playout_fillboard_bestcirc_enabled)
    {
      std::string tmpreason="fillboard bestpattern";
      this->getFillBoardMoveBestPattern(settings,board,col,move,posarray,passes,&tmpreason);
      if (!move.isPass())
      {
        if (params->debug_on)
          gtpe->getOutput()->printfDebug("[playoutmove]: %s fillboard bestpattern\n",move.toString(board->getSize()).c_str());
        if (reason!=NULL)
          *reason=tmpreason;
        return;
      }
    }
    else
    {
      std::string tmpreason="fillboard";
      this->getFillBoardMove(settings,board,col,move,posarray,passes,&tmpreason);
      if (!move.isPass())
      {
        if (params->debug_on)
          gtpe->getOutput()->printfDebug("[playoutmove]: %s fillboard\n",move.toString(board->getSize()).c_str());
        if (reason!=NULL)
          *reason=tmpreason;
        return;
      }
    }
  }

  if (WITH_P(params->playout_features_enabled))
  {
    if (params->test_p8>0)
    {
      bool doapproachmoves=(rand->getRandomReal()<params->playout_random_approach_p);
      int p=-1;
      float prob=0;
      for (int i=0;i<params->test_p8;i++)
      {
        int p_tmp=rand->getRandomInt(board->getPositionMax());
        if (board->validMove(col,p_tmp))
        {
          float prob_tmp=0;//=board->getFeatureGamma(p_tmp);
          /*
          //should be faster, but does not work yet:(
           switch (col)
          {
            case Go::BLACK:
              prob_tmp=black_gammas[p_tmp];
              break;
            case Go::WHITE:
              prob_tmp=white_gammas[p_tmp];
              break;
            default:
              break;
          }
          */
          prob_tmp=board->getFeatureGamma(p_tmp);
          if (prob_tmp>prob)
          {
            prob=prob_tmp;
            p=p_tmp;
          }
        }
      }
      if (p>=0)
      {
        if (doapproachmoves)
          this->replaceWithApproachMove(settings,board,col,p);
        if (board->validMove(col,p) && !this->isBadMove(settings,board,col,p,params->playout_avoid_lbrf1_p,params->playout_avoid_lbmf_p, params->playout_avoid_bpr_p, passes, NULL, 0, critarray))
        {
          move=Go::Move(col,p);
          move.set_useforlgrf (true);
          if (params->debug_on)
            gtpe->getOutput()->printfDebug("[playoutmove]: %s random reweighted quick-pick\n",move.toString(board->getSize()).c_str());
          if (reason!=NULL)
            *reason="random reweighted quick-pick";
          params->engine->statisticsPlus(Engine::RANDOM_REWEIGHTED_QUICK);
          return;
        }
      }
    }
    else
    {
      this->getFeatureMove(settings,board,col,move);
      if (params->debug_on)
        gtpe->getOutput()->printfDebug("[playoutmove]: %s features\n",move.toString(board->getSize()).c_str());
      if (reason!=NULL)
        *reason="features";
      return;
    }
  }
  
  random: // for playout_random_chance

  bool doapproachmoves=(rand->getRandomReal()<params->playout_random_approach_p);

  if (params->playout_criticality_random_n>0)
  {
    int patternmove=-1;
    float bestvalue=-1.0;
    
    for (int i=0;i<params->playout_criticality_random_n;i++)
    {
      int p=rand->getRandomInt(board->getPositionMax());
      if (doapproachmoves)
        this->replaceWithApproachMove(settings,board,col,p);
      if (board->validMove(col,p) && !this->isBadMove(settings,board,col,p,params->playout_avoid_lbrf1_p,params->playout_avoid_lbmf_p, params->playout_avoid_bpr_p, passes, NULL, 0, critarray))
      {
        float v=critarray[p].crit;
        
        if (v>bestvalue)
        {
          patternmove=p;
          bestvalue=v;
        }
      }
    }
    if (patternmove>=0)
    {
      move=Go::Move(col,patternmove);
      move.set_useforlgrf (true);
      if (params->debug_on)
        gtpe->getOutput()->printfDebug("[playoutmove]: %s random quick-pick with criticakity\n",move.toString(board->getSize()).c_str());
      if (reason!=NULL)
        *reason="random quick-pick with criticakity";
      //params->engine->statisticsPlus(Engine::RANDOM_QUICK_TERRITORY);
      return;
    }
  }
  

if (params->playout_random_weight_territory_n>0)
{
  int patternmove=-1;
  float bestvalue=-10000.0;
   
  int count_legal_moves=0;
  int size=board->getSize ();
  int direction[9]={0,P_N,P_S,P_W,P_E,P_NW,P_NE,P_SW,P_SE};
  int p_nodir=0;
  for (int i=0;i<params->playout_random_weight_territory_n*(params->test_p32+1);i++)
  {
    //try 5 around each random point
    int p;
    if (params->test_p97>0) 
    {
      if ((i % (int)params->test_p97)==0) p_nodir=rand->getRandomInt(board->getPositionMax()+P_NW-P_SE)+P_SE;
      fprintf(stderr,"debug %d\n",p_nodir);
      p=p_nodir+direction[rand->getRandomInt(9)];
    }
    else
      p=rand->getRandomInt(board->getPositionMax());
    if (doapproachmoves)
      this->replaceWithApproachMove(settings,board,col,p);
    if (board->validMove(col,p) && !this->isBadMove(settings,board,col,p,params->playout_avoid_lbrf1_p,params->playout_avoid_lbmf_p, params->playout_avoid_bpr_p, passes, NULL, 0, critarray))
    {
      count_legal_moves++;
      // only circular pattern
      float v=params->engine->getTerritoryMap()->getPositionOwner(p);
        
      if (col==Go::WHITE)
        v=-v;
      v+=params->playout_random_weight_territory_f1;  //shift the center
      v=params->playout_random_weight_territory_f0*v + 1.0 + params->test_p22*exp(-params->playout_random_weight_territory_f*v*v);
      if (params->test_p18>0)
        v+=params->test_p18*params->engine->getProbabilityMoveAt(p);
      if (params->test_p15>0 && critarray!=NULL)
        v+=params->test_p15*critarray[p].crit;
      if (params->test_p47>0 && ravearray!=NULL)
        v+=params->test_p47*ravearray[p];
      if (params->test_p81>0 && critarray!=NULL) {
        float slope=-(col==Go::BLACK?critarray[p].slopeblack:critarray[p].slopewhite);
        if (slope>0) v+=params->test_p81*slope;
      }
        
      if (params->test_p11>0 && (board->getDistanceToBorder(p)==1 || board->getDistanceToBorder(p)==2))
        v+=params->test_p11;
      if (params->test_p21>0 && board->surroundingEmpty(p)==8)
        v+=params->test_p21;
      if (params->test_p31>0 && board->isAtari(Go::Move(col,p)))
        v+=params->test_p31;
      if (params->test_p87>0) {
        Go::IntBoard *treeboard=((col==Go::BLACK)?treeboardBlack:treeboardWhite);
      //  v+=params->test_p87*(treeboard->get(p));
        if (treeboard->get(p)>params->test_p88*used_playouts)
          v+=params->test_p87*log(1.0+treeboard->get(p)-params->test_p88*used_playouts);
      }
      if (params->test_p29>0)
      {
        unsigned int pattern=Pattern::ThreeByThree::makeHash(board,p);
        if (col==Go::WHITE)
          pattern=Pattern::ThreeByThree::invert(pattern);
        pattern=Pattern::ThreeByThree::smallestEquivalent(pattern);
        int MaxSecondLast=board->getMaxDistance(p,board->getSecondLastMove().getPosition());
        int MaxLast=board->getMaxDistance(p,board->getLastMove().getPosition());
        v+=params->test_p29*(log(params->engine->getFeatures()->getFeatureGammaPlayoutPattern(pattern,MaxLast,MaxSecondLast)+exp(-1.0))+1.0);
      }
      if (params->test_p48>0)
      {
        v+=params->test_p48*(log(params->engine->getFeatures()->getFeatureGammaPlayoutCircPattern(board,Go::Move(col,p))+exp(-1.0))+1.0);
      }
      if (v>bestvalue)
      {
        patternmove=p;
        bestvalue=v;
      }
      if (params->debug_on)
        gtpe->getOutput()->printfDebug(" all_weight %s %f (%f)\n",Go::Move(col,p).toString(board->getSize()).c_str(),v,bestvalue);
      
    }
    if (count_legal_moves >= params->playout_random_weight_territory_n)
      break;
  }
  if (patternmove>=0)
  {
    move=Go::Move(col,patternmove);
    move.set_useforlgrf (true);
    if (params->debug_on)
      gtpe->getOutput()->printfDebug("[playoutmove]: %s random quick-pick with territory\n",move.toString(board->getSize()).c_str());
    if (reason!=NULL)
      *reason="random quick-pick with territory";
    params->engine->statisticsPlus(Engine::RANDOM_QUICK_TERRITORY);
    return;
  }
}


//reweight random moves with probability measured
if (params->test_p4>0)
{
  int p=-1;
  float prob=0;
  int count=0;
  for (int i=0;i<params->test_p4*(params->test_p36+1);i++)
  {
    int p_tmp=rand->getRandomInt(board->getPositionMax());
    if (doapproachmoves)
      this->replaceWithApproachMove(settings,board,col,p_tmp);
    if (board->validMove(col,p_tmp)&& !this->isBadMove(settings,board,col,p_tmp,params->playout_avoid_lbrf1_p,params->playout_avoid_lbmf_p, params->playout_avoid_bpr_p, passes, NULL, 0, critarray))
    {
      float prob_tmp=params->engine->getProbabilityMoveAt(p_tmp);
      if (prob_tmp>prob)
      {
        prob=prob_tmp;
        p=p_tmp;
      }
      count++;
      if (count>=params->test_p4)
        break;
    }
  }
  if (p>=0)
  {
    //if (board->validMove(col,p) && !this->isBadMove(settings,board,col,p,params->playout_avoid_lbrf1_p,params->playout_avoid_lbmf_p, params->playout_avoid_bpr_p, passes, NULL, 0, critarray))
    {
      move=Go::Move(col,p);
      move.set_useforlgrf (true);
      if (params->debug_on)
        gtpe->getOutput()->printfDebug("[playoutmove]: %s random reweighted quick-pick\n",move.toString(board->getSize()).c_str());
      if (reason!=NULL)
        *reason="random reweighted quick-pick";
      params->engine->statisticsPlus(Engine::RANDOM_REWEIGHTED_QUICK);
      return;
    }
  }
}  

if (params->playout_randomquick_bestcirc_n>0)
  {
    int patternmove=-1;
    float bestvalue=-1.0;
    
    for (int i=0;i<params->playout_randomquick_bestcirc_n;i++)
    {
      int p=rand->getRandomInt(board->getPositionMax());
      if (doapproachmoves)
        this->replaceWithApproachMove(settings,board,col,p);
      if (board->validMove(col,p) && !this->isBadMove(settings,board,col,p,params->playout_avoid_lbrf1_p,params->playout_avoid_lbmf_p, params->playout_avoid_bpr_p, passes, NULL, 0, critarray))
      {
        // only circular pattern
        Pattern::Circular pattcirc=Pattern::Circular(params->engine->getCircDict(),board,p,params->engine->getCircSize());
        if (col==Go::WHITE)
          pattcirc.invert();
        pattcirc.convertToSmallestEquivalent(params->engine->getCircDict());
        float v=params->engine->valueCircPattern(pattcirc.toString(params->engine->getCircDict()));
        
        /*
        //full gamma
        Go::ObjectBoard<int> *cfglastdist=NULL;
        Go::ObjectBoard<int> *cfgsecondlastdist=NULL;
        //this is tooo slow
        //params->engine->getFeatures()->computeCFGDist(board,&cfglastdist,&cfgsecondlastdist);
        float v=params->engine->getFeatures()->getMoveGamma(board,cfglastdist,cfgsecondlastdist,Go::Move(col,p));
        //fprintf(stderr,"v %f move %s\n",v,Go::Position::pos2string(p,19).c_str());
        */
        if (v>bestvalue)
        {
          patternmove=p;
          bestvalue=v;
        }
        /*if (cfglastdist!=NULL)
          delete cfglastdist;
        if (cfgsecondlastdist!=NULL)
          delete cfgsecondlastdist;
        */
      }
    }
    if (patternmove>=0)
    {
      move=Go::Move(col,patternmove);
      move.set_useforlgrf (true);
      if (params->debug_on)
        gtpe->getOutput()->printfDebug("[playoutmove]: %s random quick-pick circ\n",move.toString(board->getSize()).c_str());
      if (reason!=NULL)
        *reason="random quick-pick circ";
      params->engine->statisticsPlus(Engine::RANDOM_QUICK_CIRC);
      return;
    }
  }
  
  if (params->test_p4==0)  //improved ramdomness, if test_p4 used this should have the same effect
  {
    for (int i=0;i<10;i++)
    {
      int p=rand->getRandomInt(board->getPositionMax());
      if (doapproachmoves)
        this->replaceWithApproachMove(settings,board,col,p);
      if (board->validMove(col,p) && !this->isBadMove(settings,board,col,p,params->playout_avoid_lbrf1_p,params->playout_avoid_lbmf_p, params->playout_avoid_bpr_p, passes, NULL, 0, critarray))
      {
        move=Go::Move(col,p);
        move.set_useforlgrf (true);
        if (params->debug_on)
          gtpe->getOutput()->printfDebug("[playoutmove]: %s random quick-pick\n",move.toString(board->getSize()).c_str());
        if (reason!=NULL)
          *reason="random quick-pick";
        params->engine->statisticsPlus(Engine::RANDOM_QUICK);
        return;
      }
    }
  }
  
  Go::BitBoard *validmoves=board->getValidMoves(col);
  /*int *possiblemoves=posarray;
  int possiblemovescount=0;
  
  for (int p=0;p<board->getPositionMax();p++)
  {
    if (validmoves->get(p) && !this->isBadMove(board,col,p))
    {
      possiblemoves[possiblemovescount]=p;
      possiblemovescount++;
    }
  }
  
  if (possiblemovescount==0)
    move=Go::Move(col,Go::Move::PASS);
  else if (possiblemovescount==1)
    move=Go::Move(col,possiblemoves[0]);
  else
  {
    int r=rand->getRandomInt(possiblemovescount);
    move=Go::Move(col,possiblemoves[r]);
  }*/

  //const int directions[6]={-4,-2,-1,1,2,4}; //no even board sizes, therefore powers of 2 should be safe
  //int d=directions[rand->getRandomInt(6)];
  //int d=rand->getRandomInt(2)*2-1;

  int rp_tmp=rand->getRandomInt(board->getPositionMax());
  int NextPrime=board->getNextPrimePositionMax();
  int d=rand->getRandomInt(NextPrime);
  //fprintf(stderr,"d= %d rp= %d np= %d\n",d,d,NextPrime);
  for (int p=0;p<NextPrime;p++)
  {
    //int rp=(r+p*d);
    rp_tmp+=d;
    // only positive d here: if (rp<0) rp+=NextPrime;
    if (rp_tmp>=NextPrime) rp_tmp-=NextPrime;
    int rp=rp_tmp; // approach moves can change rp !!!! random wrong then!!!
    if (rp<board->getPositionMax() && doapproachmoves)
      this->replaceWithApproachMove(settings,board,col,rp);
    if (rp<board->getPositionMax() && validmoves->get(rp) && !this->isBadMove(settings,board,col,rp,params->playout_avoid_lbrf1_p,params->playout_avoid_lbmf_p, params->playout_avoid_bpr_p, passes, NULL, 0, critarray))
    {
      move=Go::Move(col,rp);
      move.set_useforlgrf (true);
      if (params->debug_on)
        gtpe->getOutput()->printfDebug("[playoutmove]: %s random\n",move.toString(board->getSize()).c_str());
      if (reason!=NULL)
        *reason="random";
      params->engine->statisticsPlus(Engine::RANDOM);
      return;
    }
  }

  if (params->playout_fill_weak_eyes)
  {
    //filling weak eyes, if not selfatari and no other move was possible
    for (int p=0;p<NextPrime;p++)
    {
      //int rp=(r+p*d);
      rp_tmp+=d;
      // only positive d here: if (rp<0) rp+=NextPrime;
      if (rp_tmp>=NextPrime) rp_tmp-=NextPrime;
      int rp=rp_tmp; // approach moves can change rp !!!! random wrong then!!! (not here at the moment)
    
      if (rp<board->getPositionMax() && validmoves->get(rp) && !this->isEyeFillMove(board,col,rp))
      {
        move=Go::Move(col,rp);
        if (params->debug_on)
          gtpe->getOutput()->printfDebug("[playoutmove]: %s fill weak eye\n",move.toString(board->getSize()).c_str());
        if (reason!=NULL)
          *reason="fill weak eye";
        params->engine->statisticsPlus(Engine::FILL_WEAK_EYE);
        return;
      }
    }
  }

  params->engine->statisticsPlus(Engine::PASS);
  move=Go::Move(col,Go::Move::PASS);
  if (params->debug_on)
    gtpe->getOutput()->printfDebug("[playoutmove]: %s pass\n",move.toString(board->getSize()).c_str());
  if (reason!=NULL)
    *reason="pass";
}

bool Playout::isBadMove(Worker::Settings *settings, Go::Board *board, Go::Color col, int pos, float lbr_p, float lbm_p, float lbpr_p, int passes, Go::IntBoard *firstlist, int playoutmovescount, critstruct critarray[])
{
  if (pos<0) return false;
  Random *const rand=settings->rand;

  // below lines are a bit ridiculous and should rather be rewritten in a more readable manner
  bool isBad= (board->weakEye(col,pos,params->test_p5!=0) || (params->playout_avoid_selfatari && board->isSelfAtariOfSize(Go::Move(col,pos),params->playout_avoid_selfatari_size,params->playout_avoid_selfatari_complex))
      || (lbr_p > 0.0 && rand->getRandomReal() < lbr_p && (board->getLastMove().isNormal() && this->hasLGRF1n(col,board->getLastMove().getPosition(),pos)))
      || (lbpr_p > 0.0 && (passes>1 || rand->getRandomReal() < lbpr_p) && (board->getLastMove().isPass() && this->isBadPassAnswer(col,pos)))
      || (lbm_p > 0.0 && (rand->getRandomReal() < lbm_p) && this->hasLBM (col,pos)));

  
   if (params->debug_on && critarray)
  {
    if (pos==22||pos==11)
      fprintf(stderr,"%s %f %d ",Go::Move(col,pos).toString(board->getSize()).c_str(),(col==Go::BLACK)?critarray[pos].ownselfblack:critarray[pos].ownselfwhite,isBad);
  }


  //maybe one must take into account, the getOwnRatio(), simelar to criticality!!!
  
  if (critarray!=NULL && ((!isBad && params->test_p68>0
      && ((col==Go::BLACK)?critarray[pos].ownselfblack:critarray[pos].ownselfwhite)>0 
      && ((col==Go::BLACK)?critarray[pos].ownselfblack:critarray[pos].ownselfwhite)<params->test_p69
     // && ((col==Go::BLACK)?critarray[pos].ownratio:(1-critarray[pos].ownratio))>params->test_p69
      && rand->getRandomReal() > params->test_p68)
      || ((col==Go::BLACK)?critarray[pos].isbadblack:critarray[pos].isbadwhite)))
  {
    isBad=true;
    ((col==Go::BLACK)?critarray[pos].isbadblack:critarray[pos].isbadwhite)=true;
  }
   if (params->debug_on && critarray && (pos==22||pos==11))
  {
  //  fprintf(stderr,"after %d\n",isBad);
  //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  // only debugging !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  //  isBad=true;
  //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!  
  }

  
  if (isBad && params->debug_on)
    gtpe->getOutput()->printfDebug("[badmove triggered]: %s pass weakey %d selfatari %d\n",Go::Move(col,pos).toString(board->getSize()).c_str(),board->weakEye(col,pos,params->test_p5!=0),board->isSelfAtariOfSize(Go::Move(col,pos),params->playout_avoid_selfatari_size,params->playout_avoid_selfatari_complex));

  if (isBad && firstlist!=NULL && firstlist->get(pos)==0)
    firstlist->set(pos,-playoutmovescount);
  
  return isBad;
}

bool Playout::isEyeFillMove(Go::Board *board, Go::Color col, int pos)
{
  if (params->debug_on)
    gtpe->getOutput()->printfDebug("[ifEyeFillMove triggered]: %s pass strongeye %d selfatari %d twoGroupEye %d\n",
                                   Go::Move(col,pos).toString(board->getSize()).c_str(),board->strongEye(col,pos),board->isSelfAtariOfSize(Go::Move(col,pos),2) , board->twoGroupEye(col,pos) );
    
  return (board->strongEye(col,pos) || board->isSelfAtariOfSize(Go::Move(col,pos),2) || board->twoGroupEye(col,pos));
}

void Playout::getPoolRAVEMove(Worker::Settings *settings, Go::Board *board, Go::Color col, Go::Move &move, std::vector<int> *pool)
{
  if (pool==NULL || pool->size()<=0)
    return;

  Random *const rand=settings->rand;
  if (rand->getRandomReal()>params->playout_poolrave_p)
    return;

  // try a few times in case there are many invalid moves in the pool
  for (int i=0;i<10;i++)
  {
    int selectedpos=pool->at(rand->getRandomInt(pool->size()));
    if (selectedpos!=-1 && board->validMove(col,selectedpos))
    {
      if (!this->isBadMove(settings,board,col,selectedpos))
        move=Go::Move(col,selectedpos);
      return;
    }
  }
}

void Playout::getLGRF2Move(Worker::Settings *settings, Go::Board *board, Go::Color col, Go::Move &move)
{
  if (board->getLastMove().isNormal() && board->getSecondLastMove().isNormal())
  {
    int pos1=board->getSecondLastMove().getPosition();
    int pos2=board->getLastMove().getPosition();
    if (this->hasLGRF2(col,pos1,pos2))
    {
      int np=this->getLGRF2(col,pos1,pos2);
      if (board->validMove(col,np) && !this->isBadMove(settings,board,col,np))
        move=Go::Move(col,np);
    }
  }
}

void Playout::getLGRF1Move(Worker::Settings *settings, Go::Board *board, Go::Color col, Go::Move &move)
{
  if (board->getLastMove().isNormal())
  {
    int pos1=board->getLastMove().getPosition();
    if (this->hasLGRF1(col,pos1))
    {
      int np=this->getLGRF1(col,pos1);
      if (board->validMove(col,np) && !this->isBadMove(settings,board,col,np))
        move=Go::Move(col,np);
    }
  }
}

void Playout::getLGPFMove(Worker::Settings *settings, Go::Board *board, Go::Color col, Go::Move &move, int *posarray)
{
  Random *const rand=settings->rand;

  int *patternmoves=posarray;
  int patternmovescount=0;

  for (int p=0;p<board->getPositionMax();p++)
  {
     if (board->validMove(col,p) && !this->isBadMove(settings,board,col,p))
      {
        if (hasLGPF(col,p,Pattern::ThreeByThree::makeHash(board,p),Pattern::FiveByFiveBorder::makeHash(board,p)))
        {
          patternmoves[patternmovescount]=p;
          patternmovescount++;
        }
      }
  }
  if (patternmovescount>0)
  {
    int i=rand->getRandomInt(patternmovescount);
    move=Go::Move(col,patternmoves[i]);
    //fprintf(stderr,"used lgpf move at %-6s of %d lgpf moves\n",move.toString(board->getSize()).c_str(),patternmovescount);
  }
}

void Playout::getLGRF1oMove(Worker::Settings *settings,Go::Board *board, Go::Color col, Go::Move &move)
{
  if (board->getLastMove().isNormal() && board->getSecondLastMove().isNormal())
  {
    int pos1=board->getSecondLastMove().getPosition();
    int pos2=board->getLastMove().getPosition();
    if (this->hasLGRF1o(col,pos1))
    {
      int np=this->getLGRF1o(col,pos1);
      if (!this->hasLGRF1n(col,pos2,np) && board->validMove(col,np) && !this->isBadMove(settings,board,col,np))
        move=Go::Move(col,np);
    }
  }
}

void Playout::getFeatureMove(Worker::Settings *settings, Go::Board *board, Go::Color col, Go::Move &move)
{
  Random *const rand=settings->rand;

  //Go::ObjectBoard<float> *gammas=new Go::ObjectBoard<float>(boardsize);
  //float totalgamma=features->getBoardGammas(board,col,gammas);
  float totalgamma=board->getFeatureTotalGamma();
  float randomgamma=totalgamma*rand->getRandomReal();
  bool foundmove=false;
  
  for (int p=0;p<board->getPositionMax();p++)
  {
    //Go::Move m=Go::Move(col,p);
    if (board->validMove(col,p))
    {
      //float gamma=gammas->get(p);
      float gamma=board->getFeatureGamma(p);
      if (randomgamma<gamma)
      {
        move=Go::Move(col,p);
        foundmove=true;
        break;
      }
      else
        randomgamma-=gamma;
    }
  }
  
  if (!foundmove)
    move=Go::Move(col,Go::Move::PASS);

  //delete gammas;
}

void Playout::getAnyCaptureMove(Worker::Settings *settings, Go::Board *board, Go::Color col, Go::Move &move, int *posarray)
{
  Random *const rand=settings->rand;

  int *possiblemoves=posarray;
  int possiblemovescount=0;
  
  std::ourset<Go::Group*> *groups=board->getGroups();
  for(std::ourset<Go::Group*>::iterator iter=groups->begin();iter!=groups->end();++iter) 
  {
    if ((*iter)->getColor()!=col && (*iter)->inAtari())
    {
      int liberty=(*iter)->getAtariPosition();
      if (board->validMove(col,liberty))
      {
        possiblemoves[possiblemovescount]=liberty;
        possiblemovescount++;
      }
    }
  }
  
  if (possiblemovescount>0)
  {
    int i=rand->getRandomInt(possiblemovescount);
    move=Go::Move(col,possiblemoves[i]);
  }
}

void Playout::getPatternMove(Worker::Settings *settings, Go::Board *board, Go::Color col, Go::Move &move, int *posarray, int passes, critstruct critarray[])
{
  Random *const rand=settings->rand;
  Pattern::ThreeByThreeTable *const patterntable=params->engine->getPatternTable();
//  Pattern::ThreeByThreeGammas *const patterngammas=params->engine->getFeatures()->getPatternGammas();
  float patternmovesgamma[17]; //slower if not used, but cleaner and faster if playout_patterns_gammas_p used
  if (params->playout_patterns_gammas_p>0)
  {
    //patternmovesgamma=new float[17];
    for (int i=0;i<17;i++)
      patternmovesgamma[i]=0;
  }
    
  int *patternmoves=posarray;
  int patternmovescount=0;
  
  if (board->getLastMove().isNormal())
  {
    int pos=board->getLastMove().getPosition();
    int size=board->getSize();
    foreach_adjdiag(pos,p,{
      if (board->validMove(col,p) && !this->isBadMove(settings,board,col,p,params->playout_avoid_lbrf1_p2,params->playout_avoid_lbmf_p2, params->playout_avoid_bpr_p2, passes, NULL, 0, critarray))
      {
        unsigned int pattern=Pattern::ThreeByThree::makeHash(board,p);
        if (col==Go::WHITE)
          pattern=Pattern::ThreeByThree::invert(pattern);
        
        if (params->playout_patterns_gammas_p==0.0)
        {
          if (patterntable->isPattern(pattern))
          {
            patternmoves[patternmovescount]=p;
            patternmovescount++;
          }
        }
        else
        {
          int MaxSecondLast=board->getMaxDistance(p,board->getSecondLastMove().getPosition());
          pattern=Pattern::ThreeByThree::smallestEquivalent(pattern);
          //if (patterngammas->hasGamma(pattern) && params->test_p6*rand->getRandomReal()+params->playout_patterns_gammas_p < patterngammas->getGamma(pattern))
          /*//This was not significantly weaker than the move complicated solution impemented at the moment
          if (params->test_p6*rand->getRandomReal()+params->playout_patterns_gammas_p < params->engine->getFeatures()->getFeatureGammaPlayoutPattern(pattern,1,MaxSecondLast))
          {
            //fprintf(stderr,"patterngamma %f\n",patterngammas->getGamma(pattern));
            if (params->debug_on)
              fprintf(stderr,"patterngamma %i %i %s %f\n",1,MaxSecondLast,Go::Move(col,p).toString(board->getSize()).c_str(),params->engine->getFeatures()->getFeatureGammaPlayoutPattern(pattern,1,MaxSecondLast));
            patternmoves[patternmovescount]=p;
            patternmovescount++;
          }
          */
            if (params->debug_on)
              fprintf(stderr,"patterngamma %i %i %s %f\n",1,MaxSecondLast,Go::Move(col,p).toString(board->getSize()).c_str(),params->engine->getFeatures()->getFeatureGammaPlayoutPattern(pattern,1,MaxSecondLast));
            patternmoves[patternmovescount]=p;
            patternmovesgamma[patternmovescount]=params->engine->getFeatures()->getFeatureGammaPlayoutPattern(pattern,1,MaxSecondLast);
            patternmovescount++;
        }
      }
    });
  }
  int pcnow=patternmovescount;
  
  if (board->getSecondLastMove().isNormal() && (WITH_P(params->test_p14) || //(params->playout_patterns_gammas_p>0) || 
                                                (WITH_P(params->test_p27) && board->getMaxDistance(board->getSecondLastMove().getPosition(),board->getLastMove().getPosition())==1)))
  {
    int pos=board->getSecondLastMove().getPosition();
    int size=board->getSize();
    
    foreach_adjdiag(pos,p,{
      bool found=false;
      for (int j=0;j<pcnow;j++)
        if (p==patternmoves[j]) {found=true; break;} //remove double entries
      if (!found && board->validMove(col,p) && !this->isBadMove(settings,board,col,p,params->playout_avoid_lbrf1_p2,params->playout_avoid_lbmf_p2, params->playout_avoid_bpr_p2, passes, NULL, 0, critarray))
      {
        unsigned int pattern=Pattern::ThreeByThree::makeHash(board,p);
        if (col==Go::WHITE)
          pattern=Pattern::ThreeByThree::invert(pattern);
        
        if (params->playout_patterns_gammas_p==0.0)
        {
          if (patterntable->isPattern(pattern))
          {
            patternmoves[patternmovescount]=p;
            patternmovescount++;
          }
          }
        else
        {
          int MaxLast=board->getMaxDistance(p,board->getLastMove().getPosition());
          pattern=Pattern::ThreeByThree::smallestEquivalent(pattern);
          //if (patterngammas->hasGamma(pattern) && params->test_p6*rand->getRandomReal()+params->playout_patterns_gammas_p < patterngammas->getGamma(pattern))
          /* //This was not significantly weaker than the move complicated solution impemented at the moment
          if (params->test_p6*rand->getRandomReal()+params->playout_patterns_gammas_p < params->engine->getFeatures()->getFeatureGammaPlayoutPattern(pattern,MaxLast,1))
          {
            //fprintf(stderr,"patterngamma %f\n",patterngammas->getGamma(pattern));
            if (params->debug_on)
              fprintf(stderr,"patterngamma %i %i %s %f\n",MaxLast,1,Go::Move(col,p).toString(board->getSize()).c_str(),params->engine->getFeatures()->getFeatureGammaPlayoutPattern(pattern,MaxLast,1));
            patternmoves[patternmovescount]=p;
            patternmovescount++;
          }
          */
            if (params->debug_on)
              fprintf(stderr,"patterngamma %i %i %s %f\n",MaxLast,1,Go::Move(col,p).toString(board->getSize()).c_str(),params->engine->getFeatures()->getFeatureGammaPlayoutPattern(pattern,MaxLast,1));
            patternmoves[patternmovescount]=p;
            patternmovesgamma[patternmovescount]=params->test_p111*params->engine->getFeatures()->getFeatureGammaPlayoutPattern(pattern,MaxLast,1);
            patternmovescount++;
          
        }
      }
    });
  }
  
  if (patternmovescount>0 && params->playout_patterns_gammas_p==0)
  {
    int i=rand->getRandomInt(patternmovescount);
    move=Go::Move(col,patternmoves[i]);
  }
  if (params->playout_patterns_gammas_p>0)
  {
    patternmoves[patternmovescount]=-1; //PASS move
    patternmovesgamma[patternmovescount]=params->playout_patterns_gammas_p;
    patternmovescount++;
    if (patternmovescount>17)
    {
      patternmovescount=17;
      fprintf(stderr,"Should not happen!!!\n");
    }
    float gammasum=0;
// very slow, if needed one should pre scale the gammas in the file!!!!!! test_p28 was 0.7
//#define scalegamma(A) (pow((A)+1.0,params->test_p28)-1.0)
//#define scalegamma(A) (exp(log((A)+1.0)*params->test_p28)-1.0)
#define scalegamma(A) (A)    
    for (int i=0;i<patternmovescount;i++)
    {
      //for (int j=0;j<i;j++)
      //  if (patternmoves[i]==patternmoves[j]) patternmovesgamma[i]=0; //remove double entries
      gammasum+=scalegamma(patternmovesgamma[i]);
    }
    float gammatest=rand->getRandomReal()*gammasum;
    gammasum=0;
    int ii;
    for (ii=0;ii<patternmovescount;ii++)
      {
        gammasum+=scalegamma(patternmovesgamma[ii]);
        if (gammasum>gammatest)
          break;
      }
    debugint=ii;
    if (ii>=patternmovescount-1)
      move=Go::Move(col,Go::Move::PASS);
    else
      move=Go::Move(col,patternmoves[ii]);
  //delete patternmovesgamma;
  }
}

void Playout::getFillBoardMove(Worker::Settings *settings, Go::Board *board, Go::Color col, Go::Move &move, int *posarray, int passes, std::string *reason)
{
  Random *const rand=settings->rand;
  
  for (int i=0;i<params->playout_fillboard_n;i++)
  {
    int p=rand->getRandomInt(board->getPositionMax());
    if (board->getColor(p)==Go::EMPTY && (params->test_p85==0?board->surroundingEmpty(p)==8:board->surroundingEmptyPlus(p)==12) && board->validMove(col,p))
    {
      move=Go::Move(col,p);
      params->engine->statisticsPlus(Engine::FILL_BOARD);
      return;
    }
  }
}

void Playout::getFillBoardMoveBestPattern(Worker::Settings *settings, Go::Board *board, Go::Color col, Go::Move &move, int *posarray, int passes, std::string *reason)
{
  Random *const rand=settings->rand;
  int patternmove=-1;
  float bestvalue=-1.0;
  for (int i=0;i<params->playout_fillboard_n;i++)
  {
    int p=rand->getRandomInt(board->getPositionMax());
    if (board->getColor(p)==Go::EMPTY && board->surroundingEmpty(p)==8 && board->validMove(col,p))
    {
      Pattern::Circular pattcirc=Pattern::Circular(params->engine->getCircDict(),board,p,params->engine->getCircSize());
      if (col==Go::WHITE)
        pattcirc.invert();
      pattcirc.convertToSmallestEquivalent(params->engine->getCircDict());
      float v=params->engine->valueCircPattern(pattcirc.toString(params->engine->getCircDict()));
      if (v>bestvalue)
      {
        patternmove=p;
        bestvalue=v;
      }
    }
  }
  if (patternmove>=0)
    move=Go::Move(col,patternmove);
  //now we have the best circpattern fillboard moves
}


static int mx[24]={0,-1,0,1,-2,-1,0,1,2,-3,-2,-1,1,2,3,-2,-1, 0, 1, 2,-1, 0, 1, 0};
static int my[24]={3, 2,2,2, 1, 1,1,1,1, 0, 0, 0,0,0,0,-1,-1,-1,-1,-1,-2,-2,-2,-3};

void Playout::getNearbyMove(Worker::Settings *settings, Go::Board *board, Go::Color col, Go::Move &move)
{
  Random *const rand=settings->rand;

  if (board->getLastMove().isNormal())
  {
    int pos=board->getLastMove().getPosition();
    int size=board->getSize();
    //for (int i=0;i<3;i++)
    {
      int m=rand->getRandomInt(24);
      int x=Go::Position::pos2x(pos,size)+mx[m];
      int y=Go::Position::pos2y(pos,size)+my[m];
      if (x>=0 && x<size && y>=0 && y<size)
      {
        int p=Go::Position::xy2pos(x,y,size);
        if (board->getColor(p)==Go::EMPTY && board->validMove(col,p) && !this->isBadMove(settings,board,col,p))
        {
          move=Go::Move(col,p);
          return;
        }
      }
    }
  }
}

void Playout::getNakadeMove(Worker::Settings *settings, Go::Board *board, Go::Color col, Go::Move &move, int *posarray)
{
  Random *const rand=settings->rand;

  int *possiblemoves=posarray;
  int possiblemovescount=0;
  
  if (board->getLastMove().isNormal())
  {
    int size=board->getSize();
    foreach_adjacent(board->getLastMove().getPosition(),p,{
      int twogroupotherpos=board->getOtherOfEmptyTwoGroup(p);
      if (twogroupotherpos>=0) {
        possiblemoves[possiblemovescount]=p;
        possiblemovescount++;
        possiblemoves[possiblemovescount]=twogroupotherpos;
        possiblemovescount++;
      }
      int centerpos=board->getThreeEmptyGroupCenterFrom(p);
      //fprintf(stderr,"playout 3 in %s\n",Go::Position::pos2string(centerpos,size).c_str());
      if (centerpos!=-1 && board->validMove(col,centerpos))
      {
        possiblemoves[possiblemovescount]=centerpos;
        possiblemovescount++;
      }
    });
    if (params->playout_nakade4_enabled)
    {
      foreach_adjacent(board->getLastMove().getPosition(),p,{
        int centerpos=board->getFourEmptyGroupCenterFrom(p);
        //fprintf(stderr,"playout 4 in %s\n",Go::Position::pos2string(centerpos,size).c_str());
        if (centerpos!=-1 && board->validMove(col,centerpos))
        {
          possiblemoves[possiblemovescount]=centerpos;
          possiblemovescount++;
        }
      });
    }
    if (params->playout_nakade_bent4_enabled)
    {
      foreach_adjacent(board->getLastMove().getPosition(),p,{
        int centerpos=board->getBent4EmptyGroupCenterFrom(p);
        //fprintf(stderr,"playout b4 in %s\n",Go::Position::pos2string(centerpos,size).c_str());
        if (centerpos!=-1 && board->validMove(col,centerpos))
        {
          possiblemoves[possiblemovescount]=centerpos;
          possiblemovescount++;
        }
      });
    }
    if (params->playout_nakade5_enabled)
    {
      foreach_adjacent(board->getLastMove().getPosition(),p,{
        int centerpos=board->getFiveEmptyGroupCenterFrom(p);
        //fprintf(stderr,"playout 5 in %s\n",Go::Position::pos2string(centerpos,size).c_str());
        if (centerpos!=-1 && board->validMove(col,centerpos))
        {
          possiblemoves[possiblemovescount]=centerpos;
          possiblemovescount++;
        }
      });
    }
  }
  
  if (possiblemovescount>0)
  {
    int i=rand->getRandomInt(possiblemovescount);
    move=Go::Move(col,possiblemoves[i]);
  }
}

//testing if performance is better with this
//probably not, disabled again
int Playout::getOtherOneOfTwoLiberties(Go::Group *g, int pos) 
{
  if (g->numOfPseudoLiberties()>8 || g->inAtari())
    return -1;
  
  int ps=g->numOfPseudoLiberties();
  int lps=g->getLibpossum();
  int lpsq=g->getLibpossumsq();
  Go::Board *board=g->getBoard();
  int size=board->getSize();
  
  foreach_adjacent(pos,p,{
    if (board->inGroup(p) && board->getGroup(p)==g)
    {
      ps--;
      lps-=pos;
      lpsq-=pos*pos;
    }
  });
  
  if (ps>0 && (ps*lpsq)==(lps*lps))
    return lps/ps;
  else
    return -1;
}

void Playout::getLast2LibAtariMove(Worker::Settings *settings, Go::Board *board, Go::Color col, Go::Move &move, int *posarray)
{
  Random *const rand=settings->rand;

  int *possiblemoves=posarray;
  int *possiblegroupsize=posarray+board->getSize()/2;  //not more than half of the board used here
  int possiblemovescount_base=0;
  int possiblemovescount=0;
  int size=board->getSize();
  float bestlevel=1;
  Go::Group *lastgroup=NULL;
  
  if (board->getLastMove().isNormal())
  {
    foreach_adjdiag(board->getLastMove().getPosition(),p,{
      if (board->getColor(p)==Go::EMPTY)
      {
        foreach_adjacent(p,q,{
          if (board->inGroup(q))
          {
            Go::Group *group=board->getGroup(q);
            if (params->playout_last2libatari_allow_different_groups && lastgroup!=group)
            {
              possiblemovescount_base=possiblemovescount;
              bestlevel=1;
            }
            lastgroup=group;
            if (group!=NULL)
            {
              int s=group->getOtherOneOfTwoLiberties(p);
              if (s>0)
              {
                if (board->validMove(col,p))
                {
                  float lvl=this->getTwoLibertyMoveLevel(board,Go::Move(col,p),group); // *(params->test_p10+ rand->getRandomReal())
                  if (params->debug_on)
                    gtpe->getOutput()->printfDebug("1 atlevel %s %f\n",Go::Move(col,p).toString (size).c_str(),lvl);
                  if (lvl>0 && lvl>=bestlevel)
                  {
                    if (lvl>bestlevel)
                    {
                      bestlevel=lvl;
                      possiblemovescount=possiblemovescount_base;
                    }
                    possiblemoves[possiblemovescount]=p;
                    possiblegroupsize[possiblemovescount]=group->numOfStones();
                    possiblemovescount++;
                  }
                }
                if (board->validMove(col,s))
                {
                  float lvl=this->getTwoLibertyMoveLevel(board,Go::Move(col,s),group); // *(params->test_p10+ rand->getRandomReal())
                  if (params->debug_on)
                    gtpe->getOutput()->printfDebug("2 atlevel %s %f\n",Go::Move(col,s).toString (size).c_str(),lvl);
                  if (lvl>0 && lvl>=bestlevel)
                  {
                    if (lvl>bestlevel)
                    {
                      bestlevel=lvl;
                      possiblemovescount=possiblemovescount_base;
                    }
                    possiblemoves[possiblemovescount]=s;
                    possiblegroupsize[possiblemovescount]=group->numOfStones();
                    possiblemovescount++;
                  }
                }
              }
            }
          }
        });
      }
    });
  }

  if (possiblemovescount>0 && (bestlevel>1 || WITH_P(params->test_p13)) && (bestlevel>2 || params->test_p71==0) && (bestlevel>7 || WITH_P(params->test_p109)))
  {
    int i=rand->getRandomInt(possiblemovescount);
    if (!this->isBadMove(settings,board,col,possiblemoves[i]))
    {
      if (!board->isSelfAtari(Go::Move(col,possiblemoves[i])))
        move=Go::Move(col,possiblemoves[i]);
       //printf("\a");
       if (params->debug_on)
      {
          gtpe->getOutput()->printfDebug("2libok %s %f\n",move.toString(size).c_str(),bestlevel);
          if (bestlevel<3)
            fprintf(stderr,"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
      }
      //board->isSelfAtariOfSize(Go::Move(col,possiblemoves[i]),params->playout_avoid_selfatari_size);
    }
  }
}

void Playout::getLastCaptureMove(Worker::Settings *settings, Go::Board *board, Go::Color col, Go::Move &move, int *posarray)
{
  Random *const rand=settings->rand;

  int *possiblemoves=posarray;
  int possiblemovescount=0;
  int size=board->getSize();
  
  if (board->getLastMove().isNormal())
  {
    foreach_adjacent(board->getLastMove().getPosition(),p,{
      if (board->inGroup(p))
      {
        Go::Group *group=board->getGroup(p);
        if (group!=NULL && group->inAtari())
        {
          Go::list_int *adjacentgroups=group->getAdjacentGroups();
          if (adjacentgroups->size()>(unsigned int)board->getPositionMax())
          {
              adjacentgroups->sort();
              adjacentgroups->unique();
              //std::sort(adjacentgroups->begin(),adjacentgroups->end());
              //std::unique(adjacentgroups->begin(),adjacentgroups->end());
          }
          for(Go::list_int::iterator iter=adjacentgroups->begin();iter!=adjacentgroups->end();)
          {
            if (board->inGroup((*iter)))
            {
              Go::Group *othergroup=board->getGroup((*iter));
              if (othergroup->inAtari())
              {
                int liberty=othergroup->getAtariPosition();
                bool iscaptureorconnect=board->isCapture(Go::Move(col,liberty)) || board->isExtension(Go::Move(col,liberty)); // Why is the check for capture here?
                if (board->validMove(col,liberty) && iscaptureorconnect)
                {
                  if (possiblemovescount<board->getPositionMax())
                  {
                    possiblemoves[possiblemovescount]=liberty;
                    possiblemovescount++;
                  }
                }
              }
            iter++;
            }
            else
            {
              //Go::list_int::iterator tmp=iter;
              //--iter;
              iter=adjacentgroups->erase(iter);
            }
          }
        }
      }
    });
  }
  if (params->test_p70>0 && board->getSecondLastMove().isNormal())
  {
    foreach_adjacent(board->getSecondLastMove().getPosition(),p,{
      if (board->inGroup(p))
      {
        Go::Group *group=board->getGroup(p);
        if (group!=NULL && group->inAtari())
        {
          Go::list_int *adjacentgroups=group->getAdjacentGroups();
          if (adjacentgroups->size()>(unsigned int)board->getPositionMax())
          {
              adjacentgroups->sort();
              adjacentgroups->unique();
              //std::sort(adjacentgroups->begin(),adjacentgroups->end());
              //std::unique(adjacentgroups->begin(),adjacentgroups->end());
          }
          for(Go::list_int::iterator iter=adjacentgroups->begin();iter!=adjacentgroups->end();)
          {
            if (board->inGroup((*iter)))
            {
              Go::Group *othergroup=board->getGroup((*iter));
              if (othergroup->inAtari())
              {
                int liberty=othergroup->getAtariPosition();
                bool iscaptureorconnect=board->isCapture(Go::Move(col,liberty)) || board->isExtension(Go::Move(col,liberty)); // Why is the check for capture here?
                if (board->validMove(col,liberty) && iscaptureorconnect)
                {
                  if (possiblemovescount<board->getPositionMax())
                  {
                    possiblemoves[possiblemovescount]=liberty;
                    possiblemovescount++;
                  }
                }
              }
            iter++;
            }
            else
            {
              //Go::list_int::iterator tmp=iter;
              //--iter;
              iter=adjacentgroups->erase(iter);
            }
          }
        }
      }
    });
  }
  
  if (possiblemovescount>0)
  {
    int i=rand->getRandomInt(possiblemovescount);
    move=Go::Move(col,possiblemoves[i]);
  }
}

void Playout::getLastAtariMove(Worker::Settings *settings, Go::Board *board, Go::Color col, Go::Move &move, int *posarray,float pp)
{
  Random *const rand=settings->rand;

  int *possiblemoves=posarray;
  int possiblemovescount=0;
  int size=board->getSize();
  Go::Group *lastgroup=NULL;
  bool doubleatari=false;

  if (board->getLastMove().isNormal())
  {
    // try connect to an outside group
    foreach_adjacent(board->getLastMove().getPosition(),p,{
      if (board->inGroup(p) && !doubleatari)
      {
        //if (board->getLastMove().getPosition()==Go::Position::string2pos(std::string("A16"),19))
        //  fprintf(stderr,"last atari at A16\n");
        Go::Group *group=board->getGroup(p);
        if (group!=NULL && group->inAtari())
        {
          if (params->playout_lastatari_leavedouble)
          {
            if (lastgroup==NULL)
              lastgroup=group;
            else if (lastgroup!=group && lastgroup->getAtariPosition()!=group->getAtariPosition())
            {
              doubleatari=true;
              if (params->debug_on)
                gtpe->getOutput()->printfDebug("leavedouble triggered\n");
            }
          }
          if (!doubleatari)
          {
            bool atarigroupfound=false;
            Go::list_int *adjacentgroups=group->getAdjacentGroups();
            if (params->playout_lastatari_captureattached_p>0.0)
            {
              if (adjacentgroups->size()>(unsigned int)board->getPositionMax())
              {
                adjacentgroups->sort();
                adjacentgroups->unique();
                //std::sort(adjacentgroups->begin(),adjacentgroups->end());
                //std::unique(adjacentgroups->begin(),adjacentgroups->end());
              }
              for(Go::list_int::iterator iter=adjacentgroups->begin();iter!=adjacentgroups->end();)
              {
                if (board->inGroup((*iter)))
                {
                  Go::Group *othergroup=board->getGroup((*iter));
                  if (othergroup->inAtari())
                  {
                    int liberty=othergroup->getAtariPosition();
                    bool iscaptureorconnect=board->isCapture(Go::Move(col,liberty)) || board->isExtension(Go::Move(col,liberty)); // Why is the check for capture here?
                    //fprintf(stderr,"debug %f %d\n",pow(params->playout_lastatari_captureattached_p,1/group->numOfStones ()),group->numOfStones ());
                    // was a bug in there && WITH_P(pow(params->playout_lastatari_captureattached_p,1/group->numOfStones ()))
                    // as 1/group-> was integer and only 0 or 1. Thefore replaced, but should be checked once again
                    if (board->validMove(col,liberty) && iscaptureorconnect && (group->numOfStones()>1 || WITH_P(params->playout_lastatari_captureattached_p)))
                    {
                      if (possiblemovescount<board->getPositionMax())
                      {
                        possiblemoves[possiblemovescount]=liberty;
                        possiblemovescount++;
                        atarigroupfound=true;
                      }
                    }
                  }
                iter++;
                }
                else
                {
                  //Go::list_int::iterator tmp=iter;
                  //--iter;
                  iter=adjacentgroups->erase(iter);
                }
              }
            }
            int liberty=group->getAtariPosition();
            bool isconnect=board->isExtension(Go::Move(col,liberty));
            bool iscaptureorconnect=board->isCapture(Go::Move(col,liberty)) || isconnect; // Why is the check for capture here?
            //fprintf(stderr,"la: %s %s %d %d %d\n",Go::Position::pos2string(p,size).c_str(),Go::Position::pos2string(liberty,size).c_str(),board->isCapture(Go::Move(col,liberty)),board->isExtension(Go::Move(col,liberty)),board->isSelfAtari(Go::Move(col,liberty)));
            if (!atarigroupfound && board->validMove(col,liberty) && iscaptureorconnect && (group->numOfStones()>1 || WITH_P(params->playout_lastatari_p)))
            {
              if (possiblemovescount<board->getPositionMax())
              {
                possiblemoves[possiblemovescount]=liberty;
                possiblemovescount++;
              }
            }
          }
        }
      }
    });
    // try capture the stone that was just placed
    if (!doubleatari)
    {
      Go::Group *group=board->getGroup(board->getLastMove().getPosition());
      if (group!=NULL && group->inAtari())
      {
        int liberty=group->getAtariPosition();
        if ((WITH_P(pp) || group->numOfStones()>1) && board->validMove(col,liberty))
        {
          possiblemoves[possiblemovescount]=liberty;
          possiblemovescount++;
        }
      }
    }
  }

  if (possiblemovescount>0 && !doubleatari)
  {
    int i=rand->getRandomInt(possiblemovescount);
    move=Go::Move(col,possiblemoves[i]);
    //gtpe->getOutput()->printfDebug("[playoutmove]: last atari %d %d %d\n",board->isCapture(move),board->isExtension(move),board->isSelfAtari(move));
    //if (board->getLastMove().getPosition()==Go::Position::string2pos(std::string("A16"),19))
    //  fprintf(stderr," answer %s %d %d\n", move.toString(19).c_str(),i,possiblemovescount);        
  }
}

void Playout::checkEyeMove(Worker::Settings *settings, Go::Board *board, Go::Color col, Go::Move &move, int *posarray,Go::Move &replacemove)
{
  Go::Move tmp_move=Go::Move(col,Go::Move::PASS);
  replacemove=Go::Move(col,Go::Move::PASS);
  if (move.isPass())
    return;
  int size=board->getSize();
  Go::Group *group=NULL;

  int count_empty=0;
  foreach_adjacent(move.getPosition(),p,{
    if (board->getColor (p)==Go::otherColor(col))
      return;  //has the other color
    if (board->getColor(p)==Go::EMPTY)
    {
      count_empty++;
      if (count_empty>1) return; //more than 1 empty surounded
      tmp_move=Go::Move(col,p);
    }
    if (group)
    {
      if (board->inGroup(p) && group!=board->getGroup(p))
        return;
    }
    else
    {
      if (board->inGroup(p))
        group=board->getGroup(p);
    }        
  });
  
  if (!tmp_move.isPass())
  {
    foreach_adjacent(tmp_move.getPosition(),p,{
      if (board->inGroup(p) && group==board->getGroup(p))
      {
        replacemove=tmp_move;
        return; //the replaceMove is continous with the surounding string
      }
    });
  }
}

void Playout::checkAntiEyeMove(Worker::Settings *settings, Go::Board *board, Go::Color col, Go::Move &move, int *posarray,Go::Move &replacemove)
{
  
  if (move.isPass())
    return;
  if (board->isCapture(move)||board->isAtari(move))
    return; //do not replace in case of capture and atari moves!!
  Go::Move tmp_move=Go::Move(col,Go::Move::PASS);
  replacemove=Go::Move(col,Go::Move::PASS);
  int size=board->getSize();

  int count_empty=0;
  foreach_adjacent(move.getPosition(),p,{
    if (board->getColor (p)==col)
      return;  //has the own color
    if (board->getColor(p)==Go::EMPTY)
    {
      count_empty++;
      if (count_empty>1) return; //more than 1 empty surounded
      tmp_move=Go::Move(col,p);
    }        
  });
  if (!tmp_move.isPass ())
    replacemove=tmp_move;
}

void Playout::checkEmptyTriangleMove(Worker::Settings *settings, Go::Board *board, Go::Color col, Go::Move &move, int *posarray,Go::Move &replacemove)
{
  // also usefull for empty XXX
  //                        XXX
  // plays R instead of N with color B
  // R 
  //  NB
  //  BB
  replacemove=Go::Move(col,Go::Move::PASS);
  if (move.isPass())
    return;
  int size=board->getSize();
  Go::Group *group=NULL;

  int count_empty=0;
  int new_dist=0;
  int pos=move.getPosition();
  foreach_adjacent(pos,p,{
    // this might be good for both colors, so remove this check ?!
    if (board->getColor (p)==Go::otherColor(col))
      return;  //has the other color
    
    if (board->getColor(p)==Go::EMPTY)
    {
      count_empty++;
      if (count_empty>2) return; //more than 2 empty surounded
      new_dist+=p-pos; //tmp_move=Go::Move(col,p);
    }
    if (group)
    {
      if (board->inGroup(p) && group!=board->getGroup(p))
        return;
    }
    else
    {
      if (board->inGroup(p))
        group=board->getGroup(p);
    }        
  });
  
  int new_pos=pos+new_dist;
  if (board->getColor(new_pos)==Go::EMPTY) {
    replacemove=Go::Move(col,new_pos);
  }
}


void Playout::getAtariMove(Worker::Settings *settings, Go::Board *board, Go::Color col, Go::Move &move, int *posarray)
{
  Random *const rand=settings->rand;
  
  int *atarimoves=posarray;
  int atarimovescount=0;
  int size=board->getSize();
  
  if (board->getLastMove().isNormal())
  {
    foreach_onandadj(board->getLastMove().getPosition(),p,{
      if (board->inGroup(p))
      {
        Go::Group *group=board->getGroup(p);
        if (group!=NULL && group->inAtari())
        {
          int liberty=group->getAtariPosition();
          bool iscaptureorconnect=board->isCapture(Go::Move(col,liberty)) || board->isExtension(Go::Move(col,liberty));
          //fprintf(stderr,"a: %s %s %d",Go::Position::pos2string(p,size).c_str(),Go::Position::pos2string(liberty,size).c_str(),iscaptureorconnect);
          if (board->validMove(col,liberty) && iscaptureorconnect)
          {
            atarimoves[atarimovescount]=liberty;
            atarimovescount++;
          }
        }
      }
    });
  }
  if (board->getSecondLastMove().isNormal())
  {
    foreach_onandadj(board->getSecondLastMove().getPosition(),p,{
      if (board->inGroup(p))
      {
        Go::Group *group=board->getGroup(p);
        if (group!=NULL && group->inAtari())
        {
          int liberty=group->getAtariPosition();
          bool iscaptureorconnect=board->isCapture(Go::Move(col,liberty)) || board->isExtension(Go::Move(col,liberty));
          if (board->validMove(col,liberty) && iscaptureorconnect)
          {
            atarimoves[atarimovescount]=liberty;
            atarimovescount++;
          }
        }
      }
    });
  }
  
  if (atarimovescount>0)
  {
    int i=rand->getRandomInt(atarimovescount);
    move=Go::Move(col,atarimoves[i]);
  }
}

void Playout::resetLGRF()
{
  if (lgrf1!=NULL)
    delete[] lgrf1;
  if (lgrf1n!=NULL)
    delete[] lgrf1n;
  if (lgrf1o!=NULL)
    delete[] lgrf1o;
  if (lgrf2!=NULL)
    delete[] lgrf2;
  if (lbm!=NULL)
    delete[] lbm;
  if (badpassanswer!=NULL)
    delete[] badpassanswer;
  if (lgpf!=NULL)
    delete[] lgpf;
  if (lgpf_b!=NULL)
    delete[] lgpf_b;
  if (lgrf1hash!=NULL)
    delete[] lgrf1hash;
  if (lgrf1hash2!=NULL)
    delete[] lgrf1hash2;
  if (lgrf2hash!=NULL)
    delete[] lgrf2hash;
  if (lgrf1count!=NULL)
    delete[] lgrf1count;
  if (lgrf2count!=NULL)
    delete[] lgrf2count;
  
  lgrfpositionmax=params->engine->getCurrentBoard()->getPositionMax();
  
  lgrf1=new int[2*lgrfpositionmax];
  lgrf1n=new unsigned char[2*lgrfpositionmax*(lgrfpositionmax/8+1)]; //check dim later!!
  lgrf1o=new int[2*lgrfpositionmax];
  lgrf2=new int[2*lgrfpositionmax*lgrfpositionmax];
  lbm=new bool[2*lgrfpositionmax];
  badpassanswer=new bool[2*lgrfpositionmax];
  lgpf=new unsigned int[LGPF_NUM*2*lgrfpositionmax];
  lgpf_b=new unsigned long int[LGPF_NUM*2*lgrfpositionmax];
  lgrf1hash=new unsigned int[2*lgrfpositionmax];
  lgrf1hash2=new unsigned int[2*lgrfpositionmax];
  lgrf2hash=new unsigned int[2*lgrfpositionmax*lgrfpositionmax];
  #if (LGRFCOUNT1!=1)
    lgrf1count=new int[2*lgrfpositionmax];
  #endif
  #if (LGRFCOUNT2!=1)
    lgrf2count=new int[2*lgrfpositionmax*lgrfpositionmax];
  #endif
  
  for (int c=0;c<2;c++)
  {
    Go::Color col=(c==0?Go::BLACK:Go::WHITE);
    for (int p1=0;p1<lgrfpositionmax;p1++)
    {
      #if (LGRFCOUNT1!=1)
        lgrf1count[c*lgrfpositionmax+p1]=0;
      #endif
      this->clearLGRF1(col,p1);
      //this->clearLGRF1n(col,p1);
      this->clearLGRF1o(col,p1);
      this->clearBadPassAnswer(col,p1);
      this->clearLGPF(col,p1);
      for (int p2=0;p2<lgrfpositionmax;p2++)
      {
        #if (LGRFCOUNT2!=1)
          lgrf2count[(c*lgrfpositionmax+p1)*lgrfpositionmax+p2]=0;
        #endif
        this->clearLGRF2(col,p1,p2);
        this->clearLGRF1n(col,p1,p2);
      }
    }
  }
}

int Playout::getLGRF1(Go::Color col, int pos1) const
{
  int c=(col==Go::BLACK?0:1);
  #if (LGRFCOUNT1!=1)
    if (lgrf1count[c*lgrfpositionmax+pos1]>=LGRFCOUNT1)
      return lgrf1[c*lgrfpositionmax+pos1];
    else
      return -1;
  #endif
  return lgrf1[c*lgrfpositionmax+pos1];
}

unsigned int Playout::getLGRF1hash(Go::Color col, int pos1) const
{
  int c=(col==Go::BLACK?0:1);
  return lgrf1hash[c*lgrfpositionmax+pos1];
}

unsigned int Playout::getLGRF1hash2(Go::Color col, int pos1) const
{
  int c=(col==Go::BLACK?0:1);
  return lgrf1hash2[c*lgrfpositionmax+pos1];
}

//int Playout::getLGRF1n_l(Go::Color col, int pos1) const
//{
//  int c=(col==Go::BLACK?0:1);
//  return lgrf1n[c*lgrfpositionmax+pos1];
//}

int Playout::getLGRF1o(Go::Color col, int pos1) const
{
  int c=(col==Go::BLACK?0:1);
  return lgrf1o[c*lgrfpositionmax+pos1];
}

int Playout::getLGRF2(Go::Color col, int pos1, int pos2) const
{
  int c=(col==Go::BLACK?0:1);
  #if (LGRFCOUNT2!=1)
    if (lgrf2count[c*lgrfpositionmax+pos1]>=LGRFCOUNT2)
      return lgrf2[(c*lgrfpositionmax+pos1)*lgrfpositionmax+pos2];
    else
      return -1;
  #endif
  return lgrf2[(c*lgrfpositionmax+pos1)*lgrfpositionmax+pos2];
}

unsigned int Playout::getLGRF2hash(Go::Color col, int pos1, int pos2) const
{
  int c=(col==Go::BLACK?0:1);
  return lgrf2hash[(c*lgrfpositionmax+pos1)*lgrfpositionmax+pos2];
}

void Playout::setLGRF1(Go::Color col, int pos1, int val)
{
  int c=(col==Go::BLACK?0:1);
  #if (LGRFCOUNT1!=1)
    if (lgrf1[c*lgrfpositionmax+pos1]==val)
    {
      lgrf1count[c*lgrfpositionmax+pos1]++;
      if (lgrf1count[c*lgrfpositionmax+pos1]>LGRFCOUNT1)
        lgrf1count[c*lgrfpositionmax+pos1]=LGRFCOUNT1;
    }
  #endif
  lgrf1[c*lgrfpositionmax+pos1]=val;
}

void Playout::setLGRF1(Go::Color col, int pos1, int val, unsigned int hash, unsigned int hash2)
{
  int c=(col==Go::BLACK?0:1);
  #if (LGRFCOUNT1!=1)
    if (lgrf1[c*lgrfpositionmax+pos1]==val && lgrf1hash[c*lgrfpositionmax+pos1]==hash && lgrf1hash2[c*lgrfpositionmax+pos1]=hash2)
    {
      lgrf1count[c*lgrfpositionmax+pos1]++;
      if (lgrf1count[c*lgrfpositionmax+pos1]>LGRFCOUNT1)
        lgrf1count[c*lgrfpositionmax+pos1]=LGRFCOUNT1;
    }
  #endif
  lgrf1[c*lgrfpositionmax+pos1]=val;
  lgrf1hash[c*lgrfpositionmax+pos1]=hash;
  lgrf1hash2[c*lgrfpositionmax+pos1]=hash2;
  }

void Playout::setBadPassAnswer(Go::Color col, int pos1)
{
  int c=(col==Go::BLACK?0:1);
  badpassanswer[c*lgrfpositionmax+pos1]=true;
}

bool Playout::isBadPassAnswer(Go::Color col, int pos1)
{
  int c=(col==Go::BLACK?0:1);
  //fprintf(stderr,"bad pass answer %s %d\n", Go::Move(col,pos1).toString(9).c_str(),badpassanswer[c*lgrfpositionmax+pos1]);
  return badpassanswer[c*lgrfpositionmax+pos1];
}

void Playout::clearBadPassAnswer(Go::Color col, int pos1)
{
  int c=(col==Go::BLACK?0:1);
  badpassanswer[c*lgrfpositionmax+pos1]=false;
}

void Playout::setLGRF1o(Go::Color col, int pos1, int val)
{
  int c=(col==Go::BLACK?0:1);
  lgrf1o[c*lgrfpositionmax+pos1]=val;
}

void Playout::setLGRF2(Go::Color col, int pos1, int pos2, int val)
{
  int c=(col==Go::BLACK?0:1);
  lgrf2[(c*lgrfpositionmax+pos1)*lgrfpositionmax+pos2]=val;
  #if (LGRFCOUNT2!=1)
    lgrf2count[(c*lgrfpositionmax+pos1)*lgrfpositionmax+pos2]++;
    if (lgrf2count[(c*lgrfpositionmax+pos1)*lgrfpositionmax+pos2]>LGRFCOUNT2)
      lgrf2count[(c*lgrfpositionmax+pos1)*lgrfpositionmax+pos2]=LGRFCOUNT2;
  #endif
}

void Playout::setLGRF2(Go::Color col, int pos1, int pos2, int val, unsigned int hash)
{
  int c=(col==Go::BLACK?0:1);
  lgrf2[(c*lgrfpositionmax+pos1)*lgrfpositionmax+pos2]=val;
  lgrf2hash[(c*lgrfpositionmax+pos1)*lgrfpositionmax+pos2]=hash;
  #if (LGRFCOUNT2!=1)
    lgrf2count[(c*lgrfpositionmax+pos1)*lgrfpositionmax+pos2]++;
    if (lgrf2count[(c*lgrfpositionmax+pos1)*lgrfpositionmax+pos2]>LGRFCOUNT2)
      lgrf2count[(c*lgrfpositionmax+pos1)*lgrfpositionmax+pos2]=LGRFCOUNT2;
  #endif
}

bool Playout::hasLGRF1(Go::Color col, int pos1) const
{
  return (this->getLGRF1(col,pos1)!=-1);
}

void Playout::setLGRF1n(Go::Color col, int pos1, int val)
{
  if (pos1<0 || val<0) return;
  int c=(col==Go::BLACK?0:1);
  lgrf1n[(c*lgrfpositionmax+pos1)*(lgrfpositionmax/8)+val/8]|=(1<<(val%8));
  lbm[c*lgrfpositionmax+val]=true;
}

void Playout::clearLGRF1n(Go::Color col, int pos1, int val)
{
  if (val<0 || pos1<0) return;
  //only delete pos later!!!!!!!
  //this->setLGRF1n(col,pos1,-1);
  int c=(col==Go::BLACK?0:1);
  lgrf1n[(c*lgrfpositionmax+pos1)*(lgrfpositionmax/8)+val/8]&=~(1<<(val%8));
  lbm[c*lgrfpositionmax+val]=false;
}

bool Playout::hasLGRF1n(Go::Color col, int pos1, int val) const
{
  if (val<0 || pos1<0) return false;
  int c=(col==Go::BLACK?0:1);
  return (lgrf1n[(c*lgrfpositionmax+pos1)*(lgrfpositionmax/8)+val/8]&&(1<<(val%8)));
}

bool Playout::hasLBM(Go::Color col, int val) const
{
  if (val<0 || val<0) return false;
  int c=(col==Go::BLACK?0:1);
  return lbm[c*lgrfpositionmax+val];
}

bool Playout::hasLGRF1o(Go::Color col, int pos1) const
{
  return (this->getLGRF1o(col,pos1)!=-1);
}

bool Playout::hasLGRF2(Go::Color col, int pos1, int pos2) const
{
  return (this->getLGRF2(col,pos1,pos2)!=-1);
}

void Playout::clearLGRF1(Go::Color col, int pos1)
{
  #if (LGRFCOUNT1!=1)
    int c=(col==Go::BLACK?0:1);
    lgrf1count[c*lgrfpositionmax+pos1]=0;
    if (lgrf1count[c*lgrfpositionmax+pos1]==0) 
      this->setLGRF1(col,pos1,-1);
  #else
    this->setLGRF1(col,pos1,-1);
  #endif
}

void Playout::clearLGRF1o(Go::Color col, int pos1)
{
  this->setLGRF1o(col,pos1,-1);
}

void Playout::clearLGRF2(Go::Color col, int pos1, int pos2)
{
  #if (LGRFCOUNT2!=1)
    int c=(col==Go::BLACK?0:1);
    lgrf2count[(c*lgrfpositionmax+pos1)*lgrfpositionmax+pos2]=0;
    if (lgrf2count[(c*lgrfpositionmax+pos1)*lgrfpositionmax+pos2]==0) 
      this->setLGRF2(col,pos1,pos2,-1);
  #else
    this->setLGRF2(col,pos1,pos2,-1);
  #endif
}

void Playout::setLGPF(Worker::Settings *settings, Go::Color col, int pos1, unsigned int val)
{
  Random *const rand=settings->rand;

  //if (val==0) return;
  if (hasLGPF(col,pos1,val)) return;
  int c=(col==Go::BLACK?0:1);
  for (int i=0;i<LGPF_NUM;i++)
  {
    if (lgpf[(2*i+c)*lgrfpositionmax+pos1]==LGPF_EMPTY)
    {
      lgpf[(2*i+c)*lgrfpositionmax+pos1]=val;
      return;
    }
  }
  //fprintlgpf
  //fprintf(stderr,"full at %-6s\n",Go::Move::Move(col,pos1).toString(19).c_str());
  int i=(int)rand->getRandomInt(LGPF_NUM);
  lgpf[(2*i+c)*lgrfpositionmax+pos1]=val;
}

void Playout::setLGPF(Worker::Settings *settings, Go::Color col, int pos1, unsigned int val, unsigned long int val_b)
{
  Random *const rand=settings->rand;

  if (val==0 && val_b==0) return;
  if (hasLGPF(col,pos1,val,val_b)) return;
  int c=(col==Go::BLACK?0:1);
  for (int i=0;i<LGPF_NUM;i++)
  {
    if (lgpf[(2*i+c)*lgrfpositionmax+pos1]==LGPF_EMPTY)
    {
      lgpf[(2*i+c)*lgrfpositionmax+pos1]=val;
      lgpf_b[(2*i+c)*lgrfpositionmax+pos1]=val_b;
      return;
    }
  }
  //fprintlgpf
  //fprintf(stderr,"full at %-6s\n",Go::Move::Move(col,pos1).toString(19).c_str());
  int i=(int)rand->getRandomInt(LGPF_NUM);
  lgpf[(2*i+c)*lgrfpositionmax+pos1]=val;
  lgpf_b[(2*i+c)*lgrfpositionmax+pos1]=val_b;
}

bool Playout::hasLGPF(Go::Color col, int pos1,unsigned int hash) const
{
  //if (hash==0) return true; //if the sourounding is empty, we do not recognize
  int c=(col==Go::BLACK?0:1);
  for (int i=0;i<LGPF_NUM;i++)
  {
    if (lgpf[(2*i+c)*lgrfpositionmax+pos1]==hash)
      return true;
  }
  return false;
}

bool Playout::hasLGPF(Go::Color col, int pos1,unsigned int hash, unsigned long int hash_b) const
{
  if (hash==0 && hash_b==0) return false; //if the sourounding is empty, we do not recognize
  int c=(col==Go::BLACK?0:1);
  for (int i=0;i<LGPF_NUM;i++)
  {
    if (lgpf[(2*i+c)*lgrfpositionmax+pos1]==hash && lgpf_b[(2*i+c)*lgrfpositionmax+pos1]==hash_b)
      return true;
  }
  return false;
}

void Playout::clearLGPF(Go::Color col, int pos1)
{
  //clears all
  int c=(col==Go::BLACK?0:1);
  for (int i=0;i<LGPF_NUM;i++)
  {
    lgpf[(2*i+c)*lgrfpositionmax+pos1]=LGPF_EMPTY;
  }
}

void Playout::clearLGPF(Go::Color col, int pos1, unsigned int hash)
{
  //should only clear the hash one
  if (hash==0) return;
  int c=(col==Go::BLACK?0:1);
  for (int i=0;i<LGPF_NUM;i++)
  {
    if (lgpf[(2*i+c)*lgrfpositionmax+pos1]==hash)
    {
      //fprintlgpf
      //fprintf(stderr,"deleted (fixed boardsize 19 here) %s\n",Go::Move::Move(col,pos1).toString(19).c_str());
      lgpf[(2*i+c)*lgrfpositionmax+pos1]=LGPF_EMPTY;
      return;
    }
  }
 }

void Playout::clearLGPF(Go::Color col, int pos1, unsigned int hash, unsigned long int hash_b)
{
  //should only clear the hash one
  if (hash==0) return;
  int c=(col==Go::BLACK?0:1);
  for (int i=0;i<LGPF_NUM;i++)
  {
    if (lgpf[(2*i+c)*lgrfpositionmax+pos1]==hash && lgpf_b[(2*i+c)*lgrfpositionmax+pos1]==hash_b)
    {
      //fprintlgpf
      //fprintf(stderr,"deleted (fixed boardsize 19 here) %s\n",Go::Move::Move(col,pos1).toString(19).c_str());
      lgpf[(2*i+c)*lgrfpositionmax+pos1]=LGPF_EMPTY;
      return;
    }
  }
 }

float Playout::getTwoLibertyMoveLevel(Go::Board *board, Go::Move move, Go::Group *group)
{
  if (params->playout_last2libatari_complex)
  {
    if (!board->isSelfAtari(move) && (group->numOfStones()>1||board->isAtari(move))) //a single ladder block at the boards was used as 2 lib string, this should not harm, as single stones should not be extended by this rule
    {
      if ((board->isAtari(move) && params->test_p110==0) || (group->getColor()==move.getColor() && group->getAtariPosition()==move.getPosition() && params->test_p110!=0))
      {
        Go::Color col=move.getColor();
        Go::Color othercol=Go::otherColor(col);
        int size=board->getSize();
        bool stopsconnection=false;
        bool one_stone_atari_avoid=false;
        if (params->debug_on)
          gtpe->getOutput()->printfDebug("move %s\n",move.toString(19).c_str());
        foreach_adjacent(move.getPosition(),p,{
          if (board->getColor(p)==othercol)
          {
            Go::Group *othergroup=board->getGroup(p);
            if (params->debug_on)
              gtpe->getOutput()->printfDebug("othergroup %p group %p  othergroup!=group %d inAtari %d number of stones %d pseudoliberties %d\n",othergroup,group,othergroup!=group,othergroup->inAtari(),othergroup->numOfStones(),board->getPseudoLiberties(p));
            if (params->debug_on)
              gtpe->getOutput()->printfDebug("group %s othergroup %s\n",
                    Go::Move(group->getColor(),group->getPosition()).toString(19).c_str(),
                    Go::Move(othergroup->getColor(),othergroup->getPosition()).toString(19).c_str());
            //stops connection between groups of the same color
            if (othergroup!=group && !othergroup->inAtari() && group->getColor()==othergroup->getColor())
              stopsconnection=true;
          }
          else //extending a single own stone avoids atari later in the ladder
          {
            if (board->getColor(p)==col)
            {
              Go::Group *othergroup=board->getGroup(p);
              if (params->debug_on && othergroup!=NULL)
                gtpe->getOutput()->printfDebug("own color:: othergroup %p group %p  othergroup!=group %d inAtari %d number of stones %d pseudoliberties %d\n",othergroup,group,othergroup!=group,othergroup->inAtari(),othergroup->numOfStones(),board->getPseudoLiberties(p));
              if (othergroup!=NULL && othergroup->numOfStones()==1 && board->getPseudoLiberties(p)==2)
                one_stone_atari_avoid=true;
            }
          }
        });
        
        if (stopsconnection)
        {
          if (params->debug_on)
            gtpe->getOutput()->printfDebug("lib2 value 11 + touching empty %d + touching color %d\n",board->touchingEmpty(move.getPosition()),board->touchingColor(move.getPosition(),col));
          return board->touchingEmpty(move.getPosition())+11+params->test_p9*group->numOfStones() + params->test_p108*board->touchingColor(move.getPosition(),col);
        }
        else
        {
          if (params->debug_on)
            gtpe->getOutput()->printfDebug("lib2 value 7 + touching empty %d avoid_atari %d + touching color %d\n",board->touchingEmpty(move.getPosition()),one_stone_atari_avoid,board->touchingColor(move.getPosition(),col));
          return board->touchingEmpty(move.getPosition())+7+params->test_p9*group->numOfStones()+((one_stone_atari_avoid)?params->test_p12:0)+params->test_p108*board->touchingColor(move.getPosition(),col);        }
      }
      else if (group->numOfStones()>1)
      {
        bool doesconnection=false;
        int size=board->getSize();
        if (params->test_p71>0 && board->touchingEmpty(move.getPosition())<2)
        {
          foreach_adjacent(move.getPosition(),p,{
            if (board->getColor(p)==move.getColor())
            {
              Go::Group *othergroup=board->getGroup(p);
              if (params->debug_on)
                gtpe->getOutput()->printfDebug("othergroup %p group %p  othergroup!=group %d inAtari %d number of stones %d pseudoliberties %d\n",othergroup,group,othergroup!=group,othergroup->inAtari(),othergroup->numOfStones(),board->getPseudoLiberties(p));
              if (params->debug_on)
                gtpe->getOutput()->printfDebug("group %s othergroup %s\n",
                      Go::Move(group->getColor(),group->getPosition()).toString(19).c_str(),
                      Go::Move(othergroup->getColor(),othergroup->getPosition()).toString(19).c_str());
              //stops connection between groups of the same color
              if (othergroup!=group && !othergroup->inAtari() && group->getColor()==othergroup->getColor())
                doesconnection=true;
            }
          });
        }
        //in case of connects touching empties might be a disadvantage, as with more empties it can be connected later too! Than test_p86 -1.1 might change this behaviour 
        return (params->test_p86+1.0)*board->touchingEmpty(move.getPosition())+1+((doesconnection)?2:0)+params->test_p9*group->numOfStones();
      }
      else
        return 0;
    }
    else if (move.getColor()!=group->getColor())
    {
      Go::Color col=move.getColor();
      Go::Color othercol=Go::otherColor(col);
      int size=board->getSize();
      
      foreach_adjacent(move.getPosition(),p,{
        if (board->getColor(p)==othercol)
        {
          Go::Group *othergroup=board->getGroup(p);
          if (othergroup!=group && !othergroup->inAtari())
            return 0;
        }
        else if (board->getColor(p)==col)
          return 0;
      });
      
      return 1+params->test_p9*group->numOfStones();
    }
    else
      return 0;
  }
  else
    return 1;
}

void Playout::replaceWithApproachMove(Worker::Settings *settings, Go::Board *board, Go::Color col, int &pos)
{
  if (board->validMove(col,pos) && board->isSelfAtari(Go::Move(col,pos)))
  {
    int size=board->getSize();
    int approaches[4];
    int approachescount=0;

    foreach_adjacent(pos,p,{
      if (board->getColor(p)==col)
      {
        Go::Group *group=board->getGroup(p);
        int lib=group->getOtherOneOfTwoLiberties(pos);
        if (lib!=-1)
        {
          approaches[approachescount]=lib;
          approachescount++;
        }
      }
    });

    if (approachescount>0)
    {
      if (approachescount==1)
        pos=approaches[0];
      else
      {
        Random *const rand=settings->rand;
        int i=rand->getRandomInt(approachescount);
        pos=approaches[i];
      }
    }
  }
}

