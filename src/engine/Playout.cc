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

Playout::Playout(Parameters *prms) : params(prms)
{
  gtpe=params->engine->getGtpEngine();
  
  lgrf1=NULL;
  lgrf1n=NULL;
  lgrf1o=NULL;
  lgrf2=NULL;
  lbm=NULL;
  bad_pass_answer=NULL;
  lgpf=NULL;
  lgpf_b=NULL;
  lgrf1hash=NULL;
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
  if (lgpf!=NULL)
    delete[] lgpf;
  if (lgpf_b!=NULL)
    delete[] lgpf_b;
  if (lgrf1hash!=NULL)
    delete[] lgrf1hash;
  if (lgrf2hash!=NULL)
    delete[] lgrf2hash;
  if (lgrf1count!=NULL)
    delete[] lgrf1count;
  if (lgrf2count!=NULL)
    delete[] lgrf2count;
}

void Playout::doPlayout(Worker::Settings *settings, Go::Board *board, float &finalscore, Tree *playouttree, std::list<Go::Move> &playoutmoves, Go::Color colfirst, Go::BitBoard *firstlist, Go::BitBoard *secondlist, std::list<std::string> *movereasons)
{
  std::list<unsigned int> movehashes;
  std::list<unsigned long int> movehashes_b;
  if (board->getPassesPlayed()>2)
  {
    finalscore=board->score()-params->engine->getKomi();
    return;
  }
  
  board->turnSymmetryOff();
  
  if (params->debug_on)
    gtpe->getOutput()->printfDebug("[playout]:");
  for(std::list<Go::Move>::iterator iter=playoutmoves.begin();iter!=playoutmoves.end();++iter)
  {
    if (params->playout_lgpf_enabled||params->playout_lgrf1_safe_enabled||params->playout_lgrf2_safe_enabled)
    {
      movehashes.push_back(Pattern::ThreeByThree::makeHash(board,(*iter).getPosition()));
      if (params->playout_lgpf_enabled)
        movehashes_b.push_back(Pattern::FiveByFiveBorder::makeHash(board,(*iter).getPosition()));
    }
    board->makeMove((*iter));
    if (movereasons!=NULL)
      movereasons->push_back("given");
    if (params->debug_on)
      gtpe->getOutput()->printfDebug(" %s",(*iter).toString(board->getSize()).c_str());
    if (((*iter).getColor()==colfirst?firstlist:secondlist)!=NULL && !(*iter).isPass() && !(*iter).isResign())
      ((*iter).getColor()==colfirst?firstlist:secondlist)->set((*iter).getPosition());
    if (board->getPassesPlayed()>2 || (*iter).isResign())
    {
      if (params->debug_on)
        gtpe->getOutput()->printfDebug("\n");
      finalscore=board->score()-(params->engine->getKomi());
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
  std::vector<int> pool;
  std::vector<int> poolOC;
  std::vector<int> poolCR;
  Go::Color poolColor=Go::EMPTY;
  if (params->playout_poolrave_enabled || params->playout_poolrave_criticality)
  {
    Tree *pooltree=playouttree;
    while (playouttree!=NULL && !pooltree->isRoot() && pooltree->getRAVEPlayouts()<params->playout_poolrave_min_playouts)
      pooltree=pooltree->getParent();
    if (!pooltree->isRoot() && pooltree->getRAVEPlayouts()<params->playout_poolrave_min_playouts)
      pooltree=NULL;
    // color of tree nodes isn't taken into account
    if (pooltree!=NULL)
    {
      int k;
      if ((int)pooltree->getChildren()->size()<params->playout_poolrave_k)
        k=pooltree->getChildren()->size();
      else
        k=params->playout_poolrave_k;
      // below method is simple but probably inefficient
      int totalused=0;
      Go::BitBoard *used=new Go::BitBoard(board->getSize());
      Go::BitBoard *usedOC=new Go::BitBoard(board->getSize());
      Go::BitBoard *usedCR=new Go::BitBoard(board->getSize());
      //fprintf(stderr,"playout\n");
      while (totalused<k)
      {
        float bestval=0.1;
        int bestpos=-1;
        float bestvalOC=0.1;
        int bestposOC=-1;
        float bestvalCR=0.0001;
        int bestposCR=-1;
        for(std::list<Tree*>::iterator iter=pooltree->getChildren()->begin();iter!=pooltree->getChildren()->end();++iter) 
        {
          if (!(*iter)->getMove().isPass() && !used->get((*iter)->getMove().getPosition()) && (*iter)->getRAVERatioForPool()>bestval)
          {
            bestpos=(*iter)->getMove().getPosition();
            bestval=(*iter)->getRAVERatioForPool();
            poolColor=(*iter)->getMove().getColor();
          }
          if (!(*iter)->getMove().isPass() && !usedOC->get((*iter)->getMove().getPosition()) && (*iter)->getRAVERatioOtherForPool()>bestvalOC)
          {
            bestposOC=(*iter)->getMove().getPosition();
            bestvalOC=(*iter)->getRAVERatioOtherForPool();
          }
          if (!(*iter)->getMove().isPass() && !usedCR->get((*iter)->getMove().getPosition()) && (*iter)->getCriticality()>bestvalCR)
          {
            bestposCR=(*iter)->getMove().getPosition();
            bestvalCR=(*iter)->getCriticality();
          }
        }
        if (bestpos!=-1)
        {
          pool.push_back(bestpos);
          used->set(bestpos);
        }
        if (bestposOC!=-1)
        {
          poolOC.push_back(bestposOC);
          usedOC->set(bestposOC);
          //fprintf(stderr,"poolrave %3s %.4f\n",Go::Position::pos2string(bestposCR,19).c_str(),bestvalCR);
        }
        if (bestposCR!=-1)
        {
          poolCR.push_back(bestposCR);
          usedCR->set(bestposCR);
        }
        totalused++;
      }
      delete used;
      delete usedOC;
      delete usedCR;
    }
  }

  Go::Color coltomove=board->nextToMove();
  Go::Move move=Go::Move(coltomove,Go::Move::PASS);
  int movesalready=board->getMovesMade();
  int *posarray = new int[board->getPositionMax()];
  bool mercywin=false;
  std::string reason;
  board->resetCaptures(); // for mercy rule
  int count_playout_moves=0;

  bpasses=0;
  wpasses=0;
  //Go::BitBoard *bvalid=board->getValidMoves(Go::BLACK)->copy();
  //Go::BitBoard *wvalid=board->getValidMoves(Go::WHITE)->copy();
  //Go::Board *tmpboard=board->copy();
  while (board->getPassesPlayed()<3)
  {
    bool resign;
    if (movereasons!=NULL)
    {
      if (coltomove==poolColor)
        this->getPlayoutMove(settings,board,coltomove,move,posarray,&pool,&poolCR,&reason);
      else
        this->getPlayoutMove(settings,board,coltomove,move,posarray,&poolOC,&poolCR,&reason);
      if (params->playout_useless_move)
        this->checkUselessMove(settings,board,coltomove,move,posarray,&reason);
    }
    else
    {
      if (coltomove==poolColor)
        this->getPlayoutMove(settings,board,coltomove,move,posarray,&pool,&poolCR,&reason);
      else
        this->getPlayoutMove(settings,board,coltomove,move,posarray,&poolOC,&poolCR,&reason);
      if (params->playout_useless_move)
        this->checkUselessMove(settings,board,coltomove,move,posarray,NULL);
    }
    count_playout_moves++;
    
    if ((params->playout_lgpf_enabled || params->playout_lgrf1_safe_enabled || params->playout_lgrf2_safe_enabled) && move.isNormal())
    {
      movehashes.push_back(Pattern::ThreeByThree::makeHash(board,move.getPosition()));
      if (params->playout_lgpf_enabled)
        movehashes_b.push_back(Pattern::FiveByFiveBorder::makeHash(board,move.getPosition()));
    }
    
    if (move.isPass())
    {
        if (move.getColor()==Go::BLACK)
            bpasses++;
        else
            wpasses++;
    }
    board->makeMove(move);
    playoutmoves.push_back(move);
    if (movereasons!=NULL)
      movereasons->push_back(reason);
    int p=move.getPosition();
    if ((coltomove==colfirst?firstlist:secondlist)!=NULL && !move.isPass() && !move.isResign()
        //limit rave to the first moves in playout
        &&(params->rave_moves_use==0 || 
            count_playout_moves < board->getSize()*board->getSize()*params->rave_moves_use)
        //if this pos is already played by the other color it makes no sense to put it into rave now
        &&!(coltomove==colfirst?secondlist:firstlist)->get(p)
        //use only moves valid in after tree played
        //&&(coltomove==Go::BLACK?bvalid:wvalid)->get(p)
        //check if pattern the same as after tree played
        //Problem the probability of different patterns in the playouts is different
        //&&(Pattern::ThreeByThree::makeHash(board,p)==Pattern::ThreeByThree::makeHash(tmpboard,p))
        )
      (coltomove==colfirst?firstlist:secondlist)->set(p);
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
  }
  delete[] posarray;
  //delete wvalid;
  //delete bvalid;
  //delete tmpboard;
    
  if (!mercywin)
    finalscore=board->score()-params->engine->getKomi();
  //Go::Color playoutcol=playoutmoves.back().getColor();
  //bool playoutwin=Go::Board::isWinForColor(playoutcol,finalscore);
  bool playoutjigo=(finalscore==0);
  
  if (params->playout_lgrf1_enabled)
  {
    if (!playoutjigo) // ignore jigos
    {
      bool blackwin=Go::Board::isWinForColor(Go::BLACK,finalscore);
      Go::Color wincol=(blackwin?Go::BLACK:Go::WHITE);
      
      std::list<unsigned int>::iterator movehashiter=movehashes.begin();
      Go::Move move1=Go::Move(Go::EMPTY,Go::Move::PASS);
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
            iswin=(c==board->getScoredOwner (mp) && (c==wincol));
          else
            iswin=(c==wincol);
          if (iswin)
          {
            if (params->debug_on)
              fprintf(stderr,"adding LGRF1: %s %s\n",move1.toString(board->getSize()).c_str(),(*iter).toString(board->getSize()).c_str());
            if (!params->playout_lgrf1_safe_enabled)
              this->setLGRF1(c,p1,mp);
            else
              this->setLGRF1(c,p1,mp,poshash);
            this->clearLGRF1n(c,p1,mp);
          }
          else
          {
            if (params->debug_on && this->hasLGRF1(c,p1))
              fprintf(stderr,"forgetting LGRF1: %s %s\n",move1.toString(board->getSize()).c_str(),(*iter).toString(board->getSize()).c_str());
            this->clearLGRF1(c,p1);
            this->setLGRF1n(c,p1,mp);
          }
        }
        ++movehashiter;
        if (!(*iter).isPass() && move1.isPass())
        {
          Go::Color c=(*iter).getColor();
          int mp=(*iter).getPosition();
          //int p1=move1.getPosition();
          bool iswin;
          if (params->playout_lgrf_local)
            iswin=(c==board->getScoredOwner (mp) && (c==wincol));
          else
            iswin=(c==wincol);
          if (iswin)
          {
            //fprintf(stderr,"forgetting LGRF1n_PASS: %s %s\n",move1.toString(board->getSize()).c_str(),(*iter).toString(board->getSize()).c_str());
            this->clear_bad_pass_answer(c,mp);
          }
          else
          {
            //fprintf(stderr,"setting LGRF1n_PASS: %s %s\n",move1.toString(board->getSize()).c_str(),(*iter).toString(board->getSize()).c_str());
            this->set_bad_pass_answer(c,mp);
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
      std::list<unsigned int>::iterator movehashiter=movehashes.begin();
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
          if (iswin)
          {
            if (params->debug_on)
              fprintf(stderr,"adding LGRF2: %s %s %s\n",move1.toString(board->getSize()).c_str(),move2.toString(board->getSize()).c_str(),(*iter).toString(board->getSize()).c_str());
            if (!params->playout_lgrf2_safe_enabled)
              this->setLGRF2(c,p1,p2,mp);
            else
              this->setLGRF2(c,p1,p2,mp,poshash);
            this->setLGRF1o(c,p1,mp);
          }
          else
          {
            if (params->debug_on && this->hasLGRF2(c,p1,p2))
              fprintf(stderr,"forgetting LGRF2: %s %s %s\n",move1.toString(board->getSize()).c_str(),move2.toString(board->getSize()).c_str(),(*iter).toString(board->getSize()).c_str());
            this->clearLGRF2(c,p1,p2);
            this->clearLGRF1o(c,p1);
          }
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
      
      std::list<unsigned int>::iterator movehashiter=movehashes.begin();
      std::list<unsigned long int>::iterator movehashiter_b=movehashes_b.begin();
      for(std::list<Go::Move>::iterator iter=playoutmoves.begin();iter!=playoutmoves.end();++iter)
      {
        if (!(*iter).isPass())
        {
          Go::Color c=(*iter).getColor();
          int mp=(*iter).getPosition();
          unsigned int p1=*movehashiter;
          unsigned int p1_b=*movehashiter_b;
          bool iswin;
          if (params->playout_lgrf_local)
            iswin=(c==board->getScoredOwner (mp) && (c==wincol));
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
        ++movehashiter_b;
      }
    }
  }
}

void Playout::getPlayoutMove(Worker::Settings *settings, Go::Board *board, Go::Color col, Go::Move &move, std::vector<int> *pool, std::vector<int> *poolCR, std::string *reason)
{
  int *posarray = new int[board->getPositionMax()];
  
  this->getPlayoutMove(settings,board,col,move,posarray,pool,poolCR,reason);
  
  delete[] posarray;
}

void Playout::checkUselessMove(Worker::Settings *settings, Go::Board *board, Go::Color col, Go::Move &move, std::string *reason)
{
  int *posarray = new int[board->getPositionMax()];
  
  this->checkUselessMove(settings,board,col,move,posarray,reason);
  
  delete[] posarray;
}


void Playout::checkUselessMove(Worker::Settings *settings, Go::Board *board, Go::Color col, Go::Move &move, int *posarray, std::string *reason)
{
  Go::Move replacemove;
  this->checkEyeMove(settings,board,col,move,posarray,replacemove);
  if (reason!=NULL && replacemove.isNormal())
    (*reason).append(" UselessEye");
  if (!replacemove.isPass())
    move=replacemove;
}

void Playout::getPlayoutMove(Worker::Settings *settings, Go::Board *board, Go::Color col, Go::Move &move, int *posarray, std::vector<int> *pool, std::vector<int> *poolCR,std::string *reason)
{
  /**
   The playout_order==4 and safe_lgrfs try to do the following logic
   1. Learned moves, which (with a high probability) do not break local response chains (safe lgrf)
   2. Local response moves
   3. Learned moves which possible break local response chains as lgrf moves sometimes do
   4. Non local learned moves as lgpf might be
   5. Non local moves as random ...
   */
  Random *const rand=settings->rand;
  
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
        return;
      }
      move=Go::Move(col,Go::Move::PASS);
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
      if (hash3x3!=0 && this->getLGRF1hash(col,pos1)==hash3x3) //,Pattern::FiveByFiveBorder::makeHash(board,p)))
      {
        if (params->debug_on)
          gtpe->getOutput()->printfDebug("[playoutmove]: %s safe lgrf1\n",move.toString(board->getSize()).c_str());
        if (reason!=NULL)
          *reason="safe lgrf1";
        return;
      }
      move=Go::Move(col,Go::Move::PASS);
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

  if (WITH_P(params->playout_lastatari_p))
  {
    this->getLastAtariMove(settings,board,col,move,posarray);
      if (!move.isPass())
      {
        if (params->debug_on)
          gtpe->getOutput()->printfDebug("[playoutmove]: %s lastatari\n",move.toString(board->getSize()).c_str());
        if (reason!=NULL)
          *reason="lastatari";
        return;
    }
  }
  
  if ((params->playout_order==0 || params->playout_order>3) && params->playout_lastcapture_enabled)
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

  if (params->playout_last2libatari_enabled)
  {
    this->getLast2LibAtariMove(settings,board,col,move,posarray);
    if (!move.isPass())
    {
      if (params->debug_on)
        gtpe->getOutput()->printfDebug("[playoutmove]: %s last2libatari\n",move.toString(board->getSize()).c_str());
      if (reason!=NULL)
        *reason="last2libatari";
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

  if (params->playout_nakade_enabled)
  {
    this->getNakadeMove(settings,board,col,move,posarray);
    if (!move.isPass())
    {
      if (params->debug_on)
        gtpe->getOutput()->printfDebug("[playoutmove]: %s nakade\n",move.toString(board->getSize()).c_str());
      if (reason!=NULL)
        *reason="nakade";
      return;
    }
  }

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
    this->getFillBoardMove(settings,board,col,move);
    if (!move.isPass())
    {
      if (params->debug_on)
        gtpe->getOutput()->printfDebug("[playoutmove]: %s fillboard\n",move.toString(board->getSize()).c_str());
      if (reason!=NULL)
        *reason="fillboard";
      return;
    }
  }
  
  if (WITH_P(params->playout_patterns_p))
  {
    this->getPatternMove(settings,board,col,move,posarray);
    if (!move.isPass())
    {
      if (params->debug_on)
        gtpe->getOutput()->printfDebug("[playoutmove]: %s pattern\n",move.toString(board->getSize()).c_str());
      if (reason!=NULL)
      	*reason="pattern";
      return;
    }
  }
  
  if (params->playout_lgrf2_enabled)
  {
    this->getLGRF2Move(settings, board,col,move);
    if (!move.isPass())
    {
      if (params->debug_on)
        gtpe->getOutput()->printfDebug("[playoutmove]: %s lgrf2\n",move.toString(board->getSize()).c_str());
      if (reason!=NULL)
        *reason="lgrf2";
      return;
    }
  }

  if (WITH_P(params->playout_anycapture_p))
  {
    this->getAnyCaptureMove(settings,board,col,move,posarray);
    if (!move.isPass())
    {
      if (params->debug_on)
        gtpe->getOutput()->printfDebug("[playoutmove]: %s anycapture\n",move.toString(board->getSize()).c_str());
      if (reason!=NULL)
        *reason="anycapture";
      return;
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
    this->getPoolRAVEMove(settings,board,col,move,poolCR);
    if (!move.isPass())
    {
      if (params->debug_on)
        gtpe->getOutput()->printfDebug("[playoutmove]: %s poolcriticality\n",move.toString(board->getSize()).c_str());
      if (reason!=NULL)
      {
        *reason="poolcriticality";
        for (unsigned int i=0;i<poolCR->size();i++)
          *reason+=" "+Go::Position::pos2string(poolCR->at(i),board->getSize());
      }
      return;
    }
  }
  
  if (params->playout_lgrf1_enabled)
  {
    this->getLGRF1Move(settings, board,col,move);
    if (!move.isPass())
    {
      if (params->debug_on)
        gtpe->getOutput()->printfDebug("[playoutmove]: %s lgrf1\n",move.toString(board->getSize()).c_str());
      if (reason!=NULL)
        *reason="lgrf1";
      return;
    }
  }
  
  if (params->playout_lgrf1o_enabled)
  {
    this->getLGRF1oMove(settings, board,col,move);
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
    this->getLGPFMove(settings, board,col,move,posarray);
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
    this->getFillBoardMove(settings,board,col,move);
    if (!move.isPass())
    {
      if (params->debug_on)
        gtpe->getOutput()->printfDebug("[playoutmove]: %s fillboard\n",move.toString(board->getSize()).c_str());
      if (reason!=NULL)
        *reason="fillboard";
      return;
    }
  }
  
  if (params->playout_features_enabled)
  {
    this->getFeatureMove(settings,board,col,move);
    if (params->debug_on)
      gtpe->getOutput()->printfDebug("[playoutmove]: %s features\n",move.toString(board->getSize()).c_str());
    if (reason!=NULL)
      *reason="features";
    return;
  }

  random: // for playout_random_chance
  
  for (int i=0;i<10;i++)
  {
    int p=rand->getRandomInt(board->getPositionMax());
    if (board->validMove(Go::Move(col,p)) && !this->isBadMove(settings, board,col,p,params->playout_avoid_lbrf1_p,params->playout_avoid_lbmf_p))
    {
      move=Go::Move(col,p);
      if (params->debug_on)
        gtpe->getOutput()->printfDebug("[playoutmove]: %s random quick-pick\n",move.toString(board->getSize()).c_str());
      if (reason!=NULL)
        *reason="random quick-pick";
      return;
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
  
  int r=rand->getRandomInt(board->getPositionMax());
  int d=rand->getRandomInt(1)*2-1;
  for (int p=0;p<board->getPositionMax();p++)
  {
    int rp=(r+p*d);
    if (rp<0) rp+=board->getPositionMax();
    if (rp>=board->getPositionMax()) rp-=board->getPositionMax();
    if (validmoves->get(rp) && !this->isBadMove(settings, board,col,rp,params->playout_avoid_lbrf1_p,params->playout_avoid_lbmf_p))
    {
      move=Go::Move(col,rp);
      if (params->debug_on)
        gtpe->getOutput()->printfDebug("[playoutmove]: %s random\n",move.toString(board->getSize()).c_str());
      if (reason!=NULL)
        *reason="random";
      return;
    }
  }

  //filling weak eyes, if not selfatari and no other move was possible
  for (int p=0;p<board->getPositionMax();p++)
  {
    int rp=(r+p*d);
    if (rp<0) rp+=board->getPositionMax();
    if (rp>=board->getPositionMax()) rp-=board->getPositionMax();
    if (validmoves->get(rp) && !this->isEyeFillMove(board,col,rp))
    {
      move=Go::Move(col,rp);
      if (params->debug_on)
        gtpe->getOutput()->printfDebug("[playoutmove]: %s fill weak eye\n",move.toString(board->getSize()).c_str());
      if (reason!=NULL)
        *reason="fill weak eye";
      return;
    }
  }

  move=Go::Move(col,Go::Move::PASS);
  if (params->debug_on)
    gtpe->getOutput()->printfDebug("[playoutmove]: %s pass\n",move.toString(board->getSize()).c_str());
  if (reason!=NULL)
    *reason="pass";
}

bool Playout::isBadMove(Worker::Settings *settings, Go::Board *board, Go::Color col, int pos, float p, float p2)
{
  if (pos<0) return false;
  Random *const rand=settings->rand;

  return (board->weakEye(col,pos) 
      || (params->playout_avoid_selfatari && board->isSelfAtariOfSize(Go::Move(col,pos),params->playout_avoid_selfatari_size))
      || (p > 0.0 && rand->getRandomReal() < p  && 
          (board->getLastMove().isNormal() && this->hasLGRF1n(col,board->getLastMove().getPosition(),pos)))
      || (p > 0.0 && (rand->getRandomReal() < p || getPasses(col)>1) &&
          (board->getLastMove().isPass() && this->is_bad_pass_answer(col,pos)))
      || (p2 > 0.0 && (rand->getRandomReal() < p2) && this->hasLBM (col,pos))
          );
}

int Playout::getPasses(Go::Color col)
{
    if (col==Go::BLACK)
        return wpasses;
    else
        return bpasses;
}


bool Playout::isEyeFillMove(Go::Board *board, Go::Color col, int pos)
{
  //Random *const rand=settings->rand;
//  fprintf(stderr,"[isEyeFillingMove]: %s strongeye %d selfatari%d\n",Go::Move(col,pos).toString(board->getSize()).c_str(),
//        board->strongEye(col,pos),board->isSelfAtariOfSize(Go::Move(col,pos),2) );
  return (board->strongEye(col,pos) 
      || board->isSelfAtariOfSize(Go::Move(col,pos),2) 
      || board->twoGroupEye(col,pos)
      );
}


void Playout::getPoolRAVEMove(Worker::Settings *settings, Go::Board *board, Go::Color col, Go::Move &move, std::vector<int> *pool)
{
  if (pool==NULL || pool->size()<=0)
    return;

  Random *const rand=settings->rand;
  if (rand->getRandomReal()>params->playout_poolrave_p)
    return;

  for (int i=0;i<10;i++) {
    //make sure the playout_poolrave_p is at least approximatly ok:)
    int selectedpos=pool->at(rand->getRandomInt(pool->size()));
    if (selectedpos!=-1 && board->validMove(Go::Move(col,selectedpos)))
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
      if (board->validMove(Go::Move(col,np)) && !this->isBadMove(settings,board,col,np))
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
      if (board->validMove(Go::Move(col,np)) && !this->isBadMove(settings,board,col,np))
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
     if (board->validMove(Go::Move(col,p)) && !this->isBadMove(settings,board,col,p))
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
    //fprintlgpf
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
      if (!this->hasLGRF1n(col,pos2,np) && board->validMove(Go::Move(col,np)) && !this->isBadMove(settings,board,col,np))
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
    Go::Move m=Go::Move(col,p);
    if (board->validMove(m))
    {
      //float gamma=gammas->get(p);
      float gamma=board->getFeatureGamma(p);
      if (randomgamma<gamma)
      {
        move=m;
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
  
  std::list<Go::Group*,Go::allocator_groupptr> *groups=board->getGroups();
  for(std::list<Go::Group*,Go::allocator_groupptr>::iterator iter=groups->begin();iter!=groups->end();++iter) 
  {
    if ((*iter)->getColor()!=col && (*iter)->inAtari())
    {
      int liberty=(*iter)->getAtariPosition();
      if (board->validMove(Go::Move(col,liberty)))
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

void Playout::getPatternMove(Worker::Settings *settings, Go::Board *board, Go::Color col, Go::Move &move, int *posarray)
{
  Random *const rand=settings->rand;
  Pattern::ThreeByThreeTable *const patterntable=params->engine->getPatternTable();

  int *patternmoves=posarray;
  int patternmovescount=0;
  
  if (board->getLastMove().isNormal())
  {
    int pos=board->getLastMove().getPosition();
    int size=board->getSize();
    
    foreach_adjdiag(pos,p,{
      if (board->validMove(Go::Move(col,p)) && !this->isBadMove(settings,board,col,p,params->playout_avoid_lbrf1_p2,params->playout_avoid_lbmf_p2))
      {
        unsigned int pattern=Pattern::ThreeByThree::makeHash(board,p);
        if (col==Go::WHITE)
          pattern=Pattern::ThreeByThree::invert(pattern);
        
        if (patterntable->isPattern(pattern))
        {
          patternmoves[patternmovescount]=p;
          patternmovescount++;
        }
      }
    });
  }
  
  if (board->getSecondLastMove().isNormal())
  {
    int pos=board->getSecondLastMove().getPosition();
    int size=board->getSize();
    
    foreach_adjdiag(pos,p,{
      if (board->validMove(Go::Move(col,p)) && !this->isBadMove(settings,board,col,p,params->playout_avoid_lbrf1_p2,params->playout_avoid_lbmf_p2))
      {
        unsigned int pattern=Pattern::ThreeByThree::makeHash(board,p);
        if (col==Go::WHITE)
          pattern=Pattern::ThreeByThree::invert(pattern);
        
        if (patterntable->isPattern(pattern))
        {
          patternmoves[patternmovescount]=p;
          patternmovescount++;
        }
      }
    });
  }
  
  if (patternmovescount>0)
  {
    int i=rand->getRandomInt(patternmovescount);
    move=Go::Move(col,patternmoves[i]);
  }
}

void Playout::getFillBoardMove(Worker::Settings *settings, Go::Board *board, Go::Color col, Go::Move &move)
{
  Random *const rand=settings->rand;

  for (int i=0;i<params->playout_fillboard_n;i++)
  {
    int p=rand->getRandomInt(board->getPositionMax());
    if (board->getColor(p)==Go::EMPTY && board->surroundingEmpty(p)==8 && board->validMove(Go::Move(col,p)))
    {
      move=Go::Move(col,p);
      return;
    }
  }
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
        if (board->getColor(p)==Go::EMPTY 
            && board->validMove(Go::Move(col,p)) 
            && !this->isBadMove(settings,board,col,p))
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
      int centerpos=board->getThreeEmptyGroupCenterFrom(p);
      if (centerpos!=-1 && board->validMove(Go::Move(col,centerpos)))
      {
        possiblemoves[possiblemovescount]=centerpos;
        possiblemovescount++;
      }
    });
  }
  
  if (possiblemovescount>0)
  {
    int i=rand->getRandomInt(possiblemovescount);
    move=Go::Move(col,possiblemoves[i]);
  }
}
 
void Playout::getLast2LibAtariMove(Worker::Settings *settings, Go::Board *board, Go::Color col, Go::Move &move, int *posarray)
{
  Random *const rand=settings->rand;

  int *possiblemoves=posarray;
  int *possiblegroupsize=posarray+board->getSize()/2;  //not more than half of the board used here
  int possiblemovescount=0;
  int size=board->getSize();
  int bestlevel=1;
  
  if (board->getLastMove().isNormal())
  {
    foreach_adjdiag(board->getLastMove().getPosition(),p,{
      if (board->getColor(p)==Go::EMPTY)
      {
        foreach_adjacent(p,q,{
          if (board->inGroup(q))
          {
            Go::Group *group=board->getGroup(q);
            if (group!=NULL)
            {
              int s=group->getOtherOneOfTwoLiberties(p);
              if (s>0)
              {
                if (board->validMove(Go::Move(col,p)))
                {
                  int lvl=this->getTwoLibertyMoveLevel(board,Go::Move(col,p),group); //*(params->test_p10+ rand->getRandomReal())
                  //fprintf(stderr,"atlevel %s %d\n",Go::Move(col,p).toString (9).c_str(),lvl);
                  if (lvl>0 && lvl>=bestlevel)
                  {
                    if (lvl>bestlevel)
                    {
                      bestlevel=lvl;
                      possiblemovescount=0;
                    }
                    possiblemoves[possiblemovescount]=p;
                    possiblegroupsize[possiblemovescount]=group->numOfStones();
                    possiblemovescount++;
                  }
                }
                if (board->validMove(Go::Move(col,s)))
                {
                  int lvl=this->getTwoLibertyMoveLevel(board,Go::Move(col,s),group); //*(params->test_p10+ rand->getRandomReal())
                  //fprintf(stderr,"atlevel %s %d\n",Go::Move(col,s).toString (9).c_str(),lvl);
                  if (lvl>0 && lvl>=bestlevel)
                  {
                    if (lvl>bestlevel)
                    {
                      bestlevel=lvl;
                      possiblemovescount=0;
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

  if (possiblemovescount>0)
  {
    int i=rand->getRandomInt(possiblemovescount);
    if (!this->isBadMove(settings, board,col,possiblemoves[i]))
    {
      if (!board->isSelfAtari(Go::Move(col,possiblemoves[i])))
        move=Go::Move(col,possiblemoves[i]);
      
//      fprintf(stderr,"2libok %s\n",move.toString(size).c_str());
//      board->isSelfAtariOfSize(Go::Move(col,possiblemoves[i]),params->playout_avoid_selfatari_size);
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
          }
          for(Go::list_int::iterator iter=adjacentgroups->begin();iter!=adjacentgroups->end();++iter)
          {
            if (board->inGroup((*iter)))
            {
              Go::Group *othergroup=board->getGroup((*iter));
              if (othergroup->inAtari())
              {
                int liberty=othergroup->getAtariPosition();
                bool iscaptureorconnect=board->isCapture(Go::Move(col,liberty)) || board->isExtension(Go::Move(col,liberty)); // Why is the check for capture here?
                if (board->validMove(Go::Move(col,liberty)) && iscaptureorconnect)
                {
                  if (possiblemovescount<board->getPositionMax())
                  {
                    possiblemoves[possiblemovescount]=liberty;
                    possiblemovescount++;
                  }
                }
              }
            }
            else
            {
              Go::list_int::iterator tmp=iter;
              --iter;
              adjacentgroups->erase(tmp);
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

void Playout::getLastAtariMove(Worker::Settings *settings, Go::Board *board, Go::Color col, Go::Move &move, int *posarray)
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
      if (!doubleatari && board->inGroup(p))
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
            else if (lastgroup!=group)
              doubleatari=true;
          }
          if (!doubleatari)
          {
            bool atarigroupfound=false;
            Go::list_int *adjacentgroups=group->getAdjacentGroups();
            if (params->playout_lastatari_captureattached>0.0)
            {
              if (adjacentgroups->size()>(unsigned int)board->getPositionMax())
              {
                adjacentgroups->sort();
                adjacentgroups->unique();
              }
              for(Go::list_int::iterator iter=adjacentgroups->begin();iter!=adjacentgroups->end();++iter)
              {
                if (board->inGroup((*iter)))
                {
                  Go::Group *othergroup=board->getGroup((*iter));
                  if (othergroup->inAtari())
                  {
                    int liberty=othergroup->getAtariPosition();
                    bool iscaptureorconnect=board->isCapture(Go::Move(col,liberty)) || board->isExtension(Go::Move(col,liberty)); // Why is the check for capture here?
                    if (board->validMove(Go::Move(col,liberty)) && iscaptureorconnect && WITH_P(pow(params->playout_lastatari_captureattached,1/group->numOfStones ())))
                    {
                      if (possiblemovescount<board->getPositionMax())
                      {
                        possiblemoves[possiblemovescount]=liberty;
                        possiblemovescount++;
                        atarigroupfound=true;
                      }
                    }
                  }
                }
                else
                {
                  Go::list_int::iterator tmp=iter;
                  --iter;
                  adjacentgroups->erase(tmp);
                }
              }
            }
            int liberty=group->getAtariPosition();
            bool iscaptureorconnect=board->isCapture(Go::Move(col,liberty)) || board->isExtension(Go::Move(col,liberty)); // Why is the check for capture here?
            //fprintf(stderr,"la: %s %s %d %d %d\n",Go::Position::pos2string(p,size).c_str(),Go::Position::pos2string(liberty,size).c_str(),board->isCapture(Go::Move(col,liberty)),board->isExtension(Go::Move(col,liberty)),board->isSelfAtari(Go::Move(col,liberty)));
            if (!atarigroupfound && board->validMove(Go::Move(col,liberty)) && iscaptureorconnect && WITH_P(pow(params->playout_lastatari_p,1/group->numOfStones ())))
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
        if (board->validMove(Go::Move(col,liberty)))
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
          if (board->validMove(Go::Move(col,liberty)) && iscaptureorconnect)
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
          if (board->validMove(Go::Move(col,liberty)) && iscaptureorconnect)
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
  if (bad_pass_answer!=NULL)
    delete[] bad_pass_answer;
  if (lgpf!=NULL)
    delete[] lgpf;
  
  lgrfpositionmax=params->engine->getCurrentBoard()->getPositionMax();
  
  lgrf1=new int[2*lgrfpositionmax];
  lgrf1n=new unsigned char[2*lgrfpositionmax*(lgrfpositionmax/8+1)]; //check dim later!!
  lgrf1o=new int[2*lgrfpositionmax];
  lgrf2=new int[2*lgrfpositionmax*lgrfpositionmax];
  lbm=new bool[2*lgrfpositionmax];
  bad_pass_answer=new bool[2*lgrfpositionmax];
  lgpf=new unsigned int[LGPF_NUM*2*lgrfpositionmax];
  lgpf_b=new unsigned long int[LGPF_NUM*2*lgrfpositionmax];
  lgrf1hash=new unsigned int[2*lgrfpositionmax];
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
      this->clear_bad_pass_answer(col,p1);
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
  return lgrf1[c*lgrfpositionmax+pos1];
}

unsigned int Playout::getLGRF1hash(Go::Color col, int pos1) const
{
  int c=(col==Go::BLACK?0:1);
  return lgrf1hash[c*lgrfpositionmax+pos1];
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
  lgrf1[c*lgrfpositionmax+pos1]=val;
  #if (LGRFCOUNT1!=1)
    lgrf1count[c*lgrfpositionmax+pos1]=lgrfcount1;
  #endif
}

void Playout::setLGRF1(Go::Color col, int pos1, int val, unsigned int hash)
{
  int c=(col==Go::BLACK?0:1);
  lgrf1[c*lgrfpositionmax+pos1]=val;
  lgrf1hash[c*lgrfpositionmax+pos1]=hash;
  #if (LGRFCOUNT1!=1)
    lgrf1count[c*lgrfpositionmax+pos1]=lgrfcount1;
  #endif
}

void Playout::set_bad_pass_answer(Go::Color col, int pos1)
{
  int c=(col==Go::BLACK?0:1);
  bad_pass_answer[c*lgrfpositionmax+pos1]=true;
}

bool Playout::is_bad_pass_answer(Go::Color col, int pos1)
{
  int c=(col==Go::BLACK?0:1);
  //fprintf(stderr,"bad pass answer %s %d\n",
  //                  Go::Move(col,pos1).toString(9).c_str(),bad_pass_answer[c*lgrfpositionmax+pos1]);
  return bad_pass_answer[c*lgrfpositionmax+pos1];
}

void Playout::clear_bad_pass_answer(Go::Color col, int pos1)
{
  int c=(col==Go::BLACK?0:1);
  bad_pass_answer[c*lgrfpositionmax+pos1]=false;
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
    lgrf2count[(c*lgrfpositionmax+pos1)*lgrfpositionmax+pos2]=lgrfcount2;
  #endif
}

void Playout::setLGRF2(Go::Color col, int pos1, int pos2, int val, unsigned int hash)
{
  int c=(col==Go::BLACK?0:1);
  lgrf2[(c*lgrfpositionmax+pos1)*lgrfpositionmax+pos2]=val;
  lgrf2hash[(c*lgrfpositionmax+pos1)*lgrfpositionmax+pos2]=hash;
  #if (LGRFCOUNT2!=1)
    lgrf2count[(c*lgrfpositionmax+pos1)*lgrfpositionmax+pos2]=lgrfcount2;
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
//void Playout::clearLGRF1n(Go::Color col, int pos1)
//{
//  this->setLGRF1n(col,pos1,-1);
//}
void Playout::clearLGRF1n(Go::Color col, int pos1,int val)
{
  if (val<0 || pos1<0) return;
  //only delete pos later!!!!!!!
  //this->setLGRF1n(col,pos1,-1);
  int c=(col==Go::BLACK?0:1);
  lgrf1n[(c*lgrfpositionmax+pos1)*(lgrfpositionmax/8)+val/8]&=~(1<<(val%8));
  lbm[c*lgrfpositionmax+val]=false;
}

bool Playout::hasLGRF1n(Go::Color col, int pos1,int val) const
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
    if  (lgrf1count[c*lgrfpositionmax+pos1]>0) lgrf1count[c*lgrfpositionmax+pos1]--;
    if  (lgrf1count[c*lgrfpositionmax+pos1]==0) 
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
    if  (lgrf2count[(c*lgrfpositionmax+pos1)*lgrfpositionmax+pos2]>0) 
        lgrf2count[(c*lgrfpositionmax+pos1)*lgrfpositionmax+pos2]--;
    if  (lgrf2count[(c*lgrfpositionmax+pos1)*lgrfpositionmax+pos2]==0) 
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
    if (lgpf[(2*i+c)*lgrfpositionmax+pos1]==LGPF_EMPTY)
    {
      lgpf[(2*i+c)*lgrfpositionmax+pos1]=val;
      return;
    }
  //fprintlgpf
  //fprintf(stderr,"full at %-6s\n",Go::Move::Move(col,pos1).toString(19).c_str());
  int i=(int)rand->getRandomReal()%LGPF_NUM;
  lgpf[(2*i+c)*lgrfpositionmax+pos1]=val;
}

void Playout::setLGPF(Worker::Settings *settings, Go::Color col, int pos1, unsigned int val, unsigned long int val_b)
{
  Random *const rand=settings->rand;

  if (val==0 && val_b==0) return;
  if (hasLGPF(col,pos1,val,val_b)) return;
  int c=(col==Go::BLACK?0:1);
  for (int i=0;i<LGPF_NUM;i++)
    if (lgpf[(2*i+c)*lgrfpositionmax+pos1]==LGPF_EMPTY)
    {
      lgpf[(2*i+c)*lgrfpositionmax+pos1]=val;
      lgpf_b[(2*i+c)*lgrfpositionmax+pos1]=val_b;
      return;
    }
  //fprintlgpf
  //fprintf(stderr,"full at %-6s\n",Go::Move::Move(col,pos1).toString(19).c_str());
  int i=(int)rand->getRandomReal()%LGPF_NUM;
  lgpf[(2*i+c)*lgrfpositionmax+pos1]=val;
  lgpf_b[(2*i+c)*lgrfpositionmax+pos1]=val_b;
}

bool Playout::hasLGPF(Go::Color col, int pos1,unsigned int hash) const
{
  //if (hash==0) return true; //if the sourounding is empty, we do not recognize
  int c=(col==Go::BLACK?0:1);
  for (int i=0;i<LGPF_NUM;i++)
    if (lgpf[(2*i+c)*lgrfpositionmax+pos1]==hash) return true;
  return false;
}

bool Playout::hasLGPF(Go::Color col, int pos1,unsigned int hash, unsigned long int hash_b) const
{
  if (hash==0 && hash_b==0) return false; //if the sourounding is empty, we do not recognize
  int c=(col==Go::BLACK?0:1);
  for (int i=0;i<LGPF_NUM;i++)
    if (lgpf[(2*i+c)*lgrfpositionmax+pos1]==hash
        &&
        lgpf_b[(2*i+c)*lgrfpositionmax+pos1]==hash_b
        ) return true;
  return false;
}

void Playout::clearLGPF(Go::Color col, int pos1)
{
  //clears all
  int c=(col==Go::BLACK?0:1);
  for (int i=0;i<LGPF_NUM;i++)
    lgpf[(2*i+c)*lgrfpositionmax+pos1]=LGPF_EMPTY;  
}

void Playout::clearLGPF(Go::Color col, int pos1, unsigned int hash)
{
  //should only clear the hash one
  if (hash==0) return;
  int c=(col==Go::BLACK?0:1);
  for (int i=0;i<LGPF_NUM;i++)
    if (lgpf[(2*i+c)*lgrfpositionmax+pos1]==hash)
    {
      //fprintlgpf
      //fprintf(stderr,"deleted (fixed boardsize 19 here) %s\n",Go::Move::Move(col,pos1).toString(19).c_str());
      lgpf[(2*i+c)*lgrfpositionmax+pos1]=LGPF_EMPTY;
      return;
    }
 }

void Playout::clearLGPF(Go::Color col, int pos1, unsigned int hash, unsigned long int hash_b)
{
  //should only clear the hash one
  if (hash==0) return;
  int c=(col==Go::BLACK?0:1);
  for (int i=0;i<LGPF_NUM;i++)
    if (lgpf[(2*i+c)*lgrfpositionmax+pos1]==hash
        &&
        lgpf_b[(2*i+c)*lgrfpositionmax+pos1]==hash_b
        )
    {
      //fprintlgpf
      //fprintf(stderr,"deleted (fixed boardsize 19 here) %s\n",Go::Move::Move(col,pos1).toString(19).c_str());
      lgpf[(2*i+c)*lgrfpositionmax+pos1]=LGPF_EMPTY;
      return;
    }
 }

int Playout::getTwoLibertyMoveLevel(Go::Board *board, Go::Move move, Go::Group *group)
{
  if (params->playout_last2libatari_complex)
  {
    if (!board->isSelfAtari(move))
    {
      if (board->isAtari(move))
      {
        Go::Color col=move.getColor();
        Go::Color othercol=Go::otherColor(col);
        int size=board->getSize();
        bool stopsconnection=false;
        
        foreach_adjacent(move.getPosition(),p,{
          if (board->getColor(p)==othercol)
          {
            Go::Group *othergroup=board->getGroup(p);
            if (othergroup!=group && !othergroup->inAtari())
              stopsconnection=true;
          }
        });
        
        if (stopsconnection)
          return board->touchingEmpty(move.getPosition())+9;
        else
          return board->touchingEmpty(move.getPosition())+5;
      }
      else if (group->numOfStones()>1)
        return board->touchingEmpty(move.getPosition())+1;
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
      
      return 1;
    }
    else
      return 0;
  }
  else
    return 1;
}

