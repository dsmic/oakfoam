#include "Playout.h"

#include "Parameters.h"
#include "Random.h"
#include "Engine.h"
#include "Pattern.h"
#include "Random.h"

Playout::Playout(Parameters *prms) : params(prms)
{
  gtpe=params->engine->getGtpEngine();
  
  lgrf1=NULL;
  lgrf2=NULL;
  this->resetLGRF();
}

Playout::~Playout()
{
  if (lgrf1!=NULL)
    delete[] lgrf1;
  if (lgrf2!=NULL)
    delete[] lgrf2;
}

void Playout::doPlayout(Go::Board *board, float &finalscore, Tree *playouttree, std::list<Go::Move> &playoutmoves, Go::Color colfirst, Go::BitBoard *firstlist, Go::BitBoard *secondlist)
{
  if (board->getPassesPlayed()>=2)
    return;
  
  board->turnSymmetryOff();
  
  if (params->debug_on)
    gtpe->getOutput()->printfDebug("[playout]:");
  for(std::list<Go::Move>::iterator iter=playoutmoves.begin();iter!=playoutmoves.end();++iter)
  {
    board->makeMove((*iter));
    if (params->debug_on)
      gtpe->getOutput()->printfDebug(" %s",(*iter).toString(board->getSize()).c_str());
    if (((*iter).getColor()==colfirst?firstlist:secondlist)!=NULL && !(*iter).isPass() && !(*iter).isResign())
      ((*iter).getColor()==colfirst?firstlist:secondlist)->set((*iter).getPosition());
    if (board->getPassesPlayed()>=2 || (*iter).isResign())
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
      return;
    }
  }
  
  Go::Color coltomove=board->nextToMove();
  Go::Move move=Go::Move(coltomove,Go::Move::PASS);
  int movesalready=board->getMovesMade();
  int *posarray = new int[board->getPositionMax()];
  bool mercywin=false;
  board->resetCaptures(); // for mercy rule
  while (board->getPassesPlayed()<2)
  {
    bool resign;
    this->getPlayoutMove(board,coltomove,move,posarray);
    board->makeMove(move);
    playoutmoves.push_back(move);
    if ((coltomove==colfirst?firstlist:secondlist)!=NULL && !move.isPass() && !move.isResign())
      (coltomove==colfirst?firstlist:secondlist)->set(move.getPosition());
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
  
  if (!mercywin)
    finalscore=board->score()-params->engine->getKomi();
  //Go::Color playoutcol=playoutmoves.back().getColor();
  //bool playoutwin=Go::Board::isWinForColor(playoutcol,finalscore);
  bool playoutjigo=(finalscore==0);
  
  //fprintf(stderr,"plt: %d %d %.1f\n%s\n",mercywin,board->getMovesMade(),finalscore,board->toString().c_str());
  
  if (params->playout_lgrf1_enabled)
  {
    if (!playoutjigo) // ignore jigos
    {
      bool blackwin=Go::Board::isWinForColor(Go::BLACK,finalscore);
      Go::Color wincol=(blackwin?Go::BLACK:Go::WHITE);
      
      Go::Move move1=Go::Move(Go::EMPTY,Go::Move::PASS);
      for(std::list<Go::Move>::iterator iter=playoutmoves.begin();iter!=playoutmoves.end();++iter)
      {
        if (!(*iter).isPass() && !move1.isPass())
        {
          Go::Color c=(*iter).getColor();
          int mp=(*iter).getPosition();
          int p1=move1.getPosition();
          if (c==wincol)
          {
            if (params->debug_on)
              fprintf(stderr,"adding LGRF1: %s %s\n",move1.toString(board->getSize()).c_str(),(*iter).toString(board->getSize()).c_str());
            this->setLGRF1(c,p1,mp);
          }
          else
          {
            if (params->debug_on && this->hasLGRF1(c,p1))
              fprintf(stderr,"forgetting LGRF1: %s %s\n",move1.toString(board->getSize()).c_str(),(*iter).toString(board->getSize()).c_str());
            this->clearLGRF1(c,p1);
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
      for(std::list<Go::Move>::iterator iter=playoutmoves.begin();iter!=playoutmoves.end();++iter)
      {
        if (!(*iter).isPass() && !move1.isPass() && !move2.isPass())
        {
          Go::Color c=(*iter).getColor();
          int mp=(*iter).getPosition();
          int p1=move1.getPosition();
          int p2=move2.getPosition();
          if (c==wincol)
          {
            if (params->debug_on)
              fprintf(stderr,"adding LGRF2: %s %s %s\n",move1.toString(board->getSize()).c_str(),move2.toString(board->getSize()).c_str(),(*iter).toString(board->getSize()).c_str());
            this->setLGRF2(c,p1,p2,mp);
          }
          else
          {
            if (params->debug_on && this->hasLGRF2(c,p1,p2))
              fprintf(stderr,"forgetting LGRF2: %s %s %s\n",move1.toString(board->getSize()).c_str(),move2.toString(board->getSize()).c_str(),(*iter).toString(board->getSize()).c_str());
            this->clearLGRF2(c,p1,p2);
          }
        }
        move1=move2;
        move2=(*iter);
      }
    }
  }
}

void Playout::getPlayoutMove(Go::Board *board, Go::Color col, Go::Move &move)
{
  int *posarray = new int[board->getPositionMax()];
  
  this->getPlayoutMove(board,col,move,posarray);
  
  delete[] posarray;
}

void Playout::getPlayoutMove(Go::Board *board, Go::Color col, Go::Move &move, int *posarray)
{
  Random *rand=params->engine->getRandom();
  Pattern::ThreeByThreeTable *patterntable=params->engine->getPatternTable();
  
  if (board->numOfValidMoves(col)==0)
  {
    move=Go::Move(col,Go::Move::PASS);
    return;
  }
  
  if (params->playout_random_chance>0)
  {
    float f=rand->getRandomReal();
    if (f<params->playout_random_chance)
      goto random;
  }
  
  if (params->playout_lgrf2_enabled)
  {
    if (board->getLastMove().isNormal() && board->getSecondLastMove().isNormal())
    {
      int pos1=board->getSecondLastMove().getPosition();
      int pos2=board->getLastMove().getPosition();
      if (this->hasLGRF2(col,pos1,pos2))
      {
        int np=this->getLGRF2(col,pos1,pos2);
        if (board->validMove(Go::Move(col,np)))
        {
          move=Go::Move(col,np);
          if (params->debug_on)
            gtpe->getOutput()->printfDebug("[playoutmove]: lgrf2\n");
          return;
        }
      }
    }
  }
  
  if (params->playout_lgrf1_enabled)
  {
    if (board->getLastMove().isNormal())
    {
      int pos1=board->getLastMove().getPosition();
      if (this->hasLGRF1(col,pos1))
      {
        int np=this->getLGRF1(col,pos1);
        if (board->validMove(Go::Move(col,np)))
        {
          move=Go::Move(col,np);
          if (params->debug_on)
            gtpe->getOutput()->printfDebug("[playoutmove]: lgrf1\n");
          return;
        }
      }
    }
  }
  
  if (params->playout_atari_enabled)
  {
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
      if (params->debug_on)
        gtpe->getOutput()->printfDebug("[playoutmove]: atari\n");
      return;
    }
  }
  
  if (params->playout_lastatari_enabled)
  {
    int *possiblemoves=posarray;
    int possiblemovescount=0;
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
            //fprintf(stderr,"la: %s %s %d %d %d\n",Go::Position::pos2string(p,size).c_str(),Go::Position::pos2string(liberty,size).c_str(),board->isCapture(Go::Move(col,liberty)),board->isExtension(Go::Move(col,liberty)),board->isSelfAtari(Go::Move(col,liberty)));
            if (board->validMove(Go::Move(col,liberty)) && iscaptureorconnect)
            {
              possiblemoves[possiblemovescount]=liberty;
              possiblemovescount++;
            }
          }
        }
      });
    }
    
    if (possiblemovescount>0)
    {
      int i=rand->getRandomInt(possiblemovescount);
      move=Go::Move(col,possiblemoves[i]);
      //gtpe->getOutput()->printfDebug("[playoutmove]: last atari %d %d %d\n",board->isCapture(move),board->isExtension(move),board->isSelfAtari(move));
      if (params->debug_on)
        gtpe->getOutput()->printfDebug("[playoutmove]: lastatari\n");
      return;
    }
  }
  
  if (params->playout_lastcapture_enabled)
  {
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
            adjacentgroups->sort();
            adjacentgroups->unique();
            for(Go::list_int::iterator iter=adjacentgroups->begin();iter!=adjacentgroups->end();++iter)
            {
              if (board->inGroup((*iter)))
              {
                Go::Group *othergroup=board->getGroup((*iter));
                if (othergroup->inAtari())
                {
                  int liberty=othergroup->getAtariPosition();
                  bool iscaptureorconnect=board->isCapture(Go::Move(col,liberty)) || board->isExtension(Go::Move(col,liberty));
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
      if (params->debug_on)
        gtpe->getOutput()->printfDebug("[playoutmove]: lastcapture\n");
      return;
    }
  }
  
  if (params->playout_nakade_enabled)
  {
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
      if (params->debug_on)
        gtpe->getOutput()->printfDebug("[playoutmove]: nakade\n");
      return;
    }
  }
  
  if (params->playout_fillboard_enabled)
  {
    for (int i=0;i<params->playout_fillboard_n;i++)
    {
      int p=rand->getRandomInt(board->getPositionMax());
      if (board->getColor(p)==Go::EMPTY && board->surroundingEmpty(p)==8 && board->validMove(Go::Move(col,p)))
      {
        move=Go::Move(col,p);
        if (params->debug_on)
          gtpe->getOutput()->printfDebug("[playoutmove]: fillboard\n");
        return;
      }
    }
  }
  
  if (params->playout_patterns_enabled)
  {
    int *patternmoves=posarray;
    int patternmovescount=0;
    
    if (board->getLastMove().isNormal())
    {
      int pos=board->getLastMove().getPosition();
      int size=board->getSize();
      
      foreach_adjdiag(pos,p,{
        if (board->validMove(Go::Move(col,p)) && !board->weakEye(col,p) && !board->isSelfAtari(Go::Move(col,p)))
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
        if (board->validMove(Go::Move(col,p)) && !board->weakEye(col,p))
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
      if (params->debug_on)
        gtpe->getOutput()->printfDebug("[playoutmove]: pattern\n");
      return;
    }
  }
  
  if (params->playout_anycapture_enabled)
  {
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
      if (params->debug_on)
        gtpe->getOutput()->printfDebug("[playoutmove]: anycapture\n");
      return;
    }
  }
  
  if (params->playout_features_enabled)
  {
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
    if (params->debug_on)
        gtpe->getOutput()->printfDebug("[playoutmove]: features\n");
    //delete gammas;
    return;
  }
  
  random: // for playout_random_chance
  
  for (int i=0;i<10;i++)
  {
    int p=rand->getRandomInt(board->getPositionMax());
    if (board->validMove(Go::Move(col,p)) && !board->weakEye(col,p))
    {
      move=Go::Move(col,p);
      if (params->debug_on)
        gtpe->getOutput()->printfDebug("[playoutmove]: random quick-pick\n");
      return;
    }
  }
  
  Go::BitBoard *validmoves=board->getValidMoves(col);
  /*int *possiblemoves=posarray;
  int possiblemovescount=0;
  
  for (int p=0;p<board->getPositionMax();p++)
  {
    if (validmoves->get(p) && !board->weakEye(col,p))
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
    if (validmoves->get(rp) && !board->weakEye(col,rp))
    {
      move=Go::Move(col,rp);
      if (params->debug_on)
        gtpe->getOutput()->printfDebug("[playoutmove]: random\n");
      return;
    }
  }
  move=Go::Move(col,Go::Move::PASS);
}

void Playout::resetLGRF()
{
  if (lgrf1!=NULL)
    delete[] lgrf1;
  if (lgrf2!=NULL)
    delete[] lgrf2;
  
  lgrfpositionmax=params->engine->getCurrentBoard()->getPositionMax();
  
  lgrf1=new int[2*lgrfpositionmax];
  lgrf2=new int[2*lgrfpositionmax*lgrfpositionmax];
  
  for (int c=0;c<2;c++)
  {
    Go::Color col=(c==0?Go::BLACK:Go::WHITE);
    for (int p1=0;p1<lgrfpositionmax;p1++)
    {
      this->clearLGRF1(col,p1);
      for (int p2=0;p2<lgrfpositionmax;p2++)
      {
        this->clearLGRF2(col,p1,p2);
      }
    }
  }
}

int Playout::getLGRF1(Go::Color col, int pos1) const
{
  int c=(col==Go::BLACK?0:1);
  return lgrf1[c*lgrfpositionmax+pos1];
}

int Playout::getLGRF2(Go::Color col, int pos1, int pos2) const
{
  int c=(col==Go::BLACK?0:1);
  return lgrf2[(c*lgrfpositionmax+pos1)*lgrfpositionmax+pos2];
}

void Playout::setLGRF1(Go::Color col, int pos1, int val)
{
  int c=(col==Go::BLACK?0:1);
  lgrf1[c*lgrfpositionmax+pos1]=val;
}

void Playout::setLGRF2(Go::Color col, int pos1, int pos2, int val)
{
  int c=(col==Go::BLACK?0:1);
  lgrf2[(c*lgrfpositionmax+pos1)*lgrfpositionmax+pos2]=val;
}

bool Playout::hasLGRF1(Go::Color col, int pos1) const
{
  return (this->getLGRF1(col,pos1)!=-1);
}

bool Playout::hasLGRF2(Go::Color col, int pos1, int pos2) const
{
  return (this->getLGRF2(col,pos1,pos2)!=-1);
}

void Playout::clearLGRF1(Go::Color col, int pos1)
{
  this->setLGRF1(col,pos1,-1);
}

void Playout::clearLGRF2(Go::Color col, int pos1, int pos2)
{
  this->setLGRF2(col,pos1,pos2,-1);
}

