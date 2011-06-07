#include "Tree.h"

#include <cmath>
#include <sstream>
#include "Parameters.h"
#include "Engine.h"

Tree::Tree(Parameters *prms, Go::ZobristHash h, Go::Move mov, Tree *p)
{
  parent=p;
  children=new std::list<Tree*>();
  params=prms;
  move=mov;
  playouts=0;
  wins=0;
  raveplayouts=0;
  ravewins=0;
  priorplayouts=0;
  priorwins=0;
  symmetryprimary=NULL;
  hasTerminalWinrate=false;
  terminaloverride=false;
  pruned=false;
  prunedchildren=0;
  gamma=0;
  childrentotalgamma=0;
  maxchildgamma=0;
  unprunenextchildat=0;
  lastunprune=0;
  unprunebase=0;
  hash=h;
  
  if (parent!=NULL)
  {
    Go::Move pmove=parent->getMove();
    if (!pmove.isPass() && !pmove.isResign())
    {
      if (pmove.getColor()==move.getColor())
        fprintf(stderr,"WARNING! unexpected parent color\n");
    }
  }
}

Tree::~Tree()
{
  for(std::list<Tree*>::iterator iter=children->begin();iter!=children->end();++iter) 
  {
    delete (*iter);
  }
  children->clear();
  delete children;
}

void Tree::addChild(Tree *node)
{
  if (move.getColor()==node->move.getColor() && (children->size()==0 || children->back()->move.getColor()!=node->move.getColor()))
    fprintf(stderr,"WARNING! Expecting alternating colors in tree\n");
  
  children->push_back(node);
  node->parent=this;
}

float Tree::getRatio()
{
  if (playouts>0)
    return (float)wins/playouts;
  else if (hasTerminalWinrate)
    return wins;
  else
    return 0;
}

float Tree::getRAVERatio()
{
  if (raveplayouts>0)
    return (float)ravewins/raveplayouts;
  else
    return 0;
}

float Tree::getPriorRatio()
{
  if (priorplayouts>0)
    return (float)priorwins/priorplayouts;
  else
    return 0;
}

float Tree::getBasePriorRatio()
{
  if (playouts>0 || priorplayouts>0)
    return (float)(wins+priorwins)/(playouts+priorplayouts);
  else
    return 0;
}

void Tree::addWin(Tree *source)
{
  wins++;
  playouts++;
  this->passPlayoutUp(true,source);
  this->checkForUnPruning();
}

void Tree::addLose(Tree *source)
{
  playouts++;
  this->passPlayoutUp(false,source);
  this->checkForUnPruning();
}

void Tree::addPriorWins(int n)
{
  priorwins+=n;
  priorplayouts+=n;
}

void Tree::addPriorLoses(int n)
{
  priorplayouts+=n;
}

void Tree::addRAVEWin()
{
  ravewins++;
  raveplayouts++;
}

void Tree::addRAVELose()
{
  raveplayouts++;
}

void Tree::addRAVEWins(int n)
{
  ravewins+=n;
  raveplayouts+=n;
}

void Tree::addRAVELoses(int n)
{
  raveplayouts+=n;
}

void Tree::addPartialResult(float win, float playout, bool invertwin)
{
  //fprintf(stderr,"adding partial result: %f %f %d\n",win,playout,invertwin);
  wins+=win;
  playouts+=playout;
  if (!this->isRoot())
  {
    if (invertwin)
      parent->addPartialResult(-win,playout,invertwin);
    else
      parent->addPartialResult(1-win,playout,invertwin);
  }
  this->checkForUnPruning();
}

Tree *Tree::getChild(Go::Move move)
{
  for(std::list<Tree*>::iterator iter=children->begin();iter!=children->end();++iter) 
  {
    if ((*iter)->getMove()==move) 
      return (*iter);
  }
  return NULL;
}

void Tree::passPlayoutUp(bool win, Tree *source)
{
  bool passterminal=(this->isTerminal() && !this->isTerminalResult()) || (source!=NULL && source->isTerminalResult());
  
  if (passterminal && win)
  {
    if (prunedchildren==0)
    {
      wins=1;
      playouts=-1;
      hasTerminalWinrate=true;
    }
    else
      this->unPruneNow();
  }
  
  if (parent!=NULL)
  {
    //assume alternating colours going up
    if (win)
      parent->addLose(this);
    else
      parent->addWin(this);
  }
  
  if (passterminal && (!win || prunedchildren==0))
  {
    if (win)
      wins=1;
    else
      wins=0;
    playouts=-1;
    hasTerminalWinrate=true;
    //fprintf(stderr,"New Terminal Result %d!\n",win);
    
    if (!win && !this->isRoot() && parent->prunedchildren>0)
      parent->unPruneNow();
  }
}

float Tree::getVal()
{
  if (this->isTerminal())
  {
    if (playouts==-1)
      return wins;
    else
      return 1;
  }
  if (params->rave_moves==0 || raveplayouts==0)
    return this->getBasePriorRatio();
  else if (playouts==0)
    return this->getRAVERatio();
  else
  {
    //float alpha=(float)(ravemoves-playouts)/params->rave_moves;
    
    float alpha=(float)raveplayouts/(raveplayouts + playouts + (float)(raveplayouts*playouts)/params->rave_moves);
    
    if (alpha<=0)
      return this->getBasePriorRatio();
    else if (alpha>=1)
      return this->getRAVERatio();
    else
      //return this->getRAVERatio()*alpha + this->getRatio()*(1-alpha);
      return this->getRAVERatio()*alpha + this->getBasePriorRatio()*(1-alpha);
  }
}

float Tree::getUrgency()
{
  float uctbias;
  if (this->isTerminalWin())
    return TREE_TERMINAL_URGENCY;
  else if (this->isTerminalLose())
    return -TREE_TERMINAL_URGENCY;
  
  if (playouts==0 && raveplayouts==0 && priorplayouts==0)
    return params->ucb_init;
  
  if (this->isTerminal())
  {
    if (playouts==0 || this->getVal()>0)
      return TREE_TERMINAL_URGENCY;
    else
      return -TREE_TERMINAL_URGENCY;
  }
  
  float plts=playouts+priorplayouts;
  
  if (parent==NULL || params->ucb_c==0)
    uctbias=0;
  else
  {
    if (parent->getPlayouts()>1 && plts>0)
      uctbias=params->ucb_c*sqrt(log((float)parent->getPlayouts())/(plts));
    else if (parent->getPlayouts()>1)
      uctbias=params->ucb_c*sqrt(log((float)parent->getPlayouts())/(1));
    else
      uctbias=params->ucb_c/2;
  }
  
  if (params->uct_progressive_bias_enabled)
    return this->getVal()+uctbias+this->getProgressiveBias();
  else
    return this->getVal()+uctbias;
}

std::list<Go::Move> Tree::getMovesFromRoot()
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

bool Tree::isTerminal()
{
  if (terminaloverride)
    return false;
  else if (hasTerminalWinrate)
    return true;
  else if (!this->isRoot())
  {
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

std::string Tree::toSGFString()
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
    if (this->isTerminalWin())
      ss<<"Terminal Win\n";
    else if (this->isTerminalLose())
      ss<<"Terminal Lose\n";
    else
    {
      ss<<"Wins/Playouts: "<<wins<<"/"<<playouts<<"("<<this->getRatio()<<")\n";
      ss<<"RAVE Wins/Playouts: "<<ravewins<<"/"<<raveplayouts<<"("<<this->getRAVERatio()<<")\n";
    }
    if (!this->isLeaf())
      ss<<"Pruned: "<<prunedchildren<<"/"<<children->size()<<"("<<(children->size()-prunedchildren)<<")\n";
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
    ss<<"Children: "<<children->size()<<"\n";
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
      if (usedchild[i]==false && ((*iter)->getPlayouts()>0 || (*iter)->isTerminal()))
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
}

void Tree::unPruneNextChild()
{
  if (prunedchildren>0)
  {
    Tree *bestchild=NULL;
    float bestfactor=0;
    int unpruned=0;
    
    for(std::list<Tree*>::iterator iter=children->begin();iter!=children->end();++iter) 
    {
      if ((*iter)->isPrimary())
      {
        if ((*iter)->isPruned())
        {
          if ((*iter)->gamma>bestfactor || bestchild==NULL)
          {
            bestfactor=(*iter)->gamma;
            bestchild=(*iter);
          }
        }
        else
          unpruned++;
      }
    }
    
    if (bestchild!=NULL)
    {
      //fprintf(stderr,"[unpruning]: (%d) %s\n",unprunenextchildat,bestchild->getMove().toString(params->board_size).c_str());
      bestchild->setPruned(false);
      unprunebase=params->uct_progressive_widening_a*pow(params->uct_progressive_widening_b,unpruned);
      lastunprune=this->unPruneMetric();
      this->updateUnPruneAt();
      prunedchildren--;
    }
  }
}

float Tree::unPruneMetric()
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
  unprunenextchildat=lastunprune+unprunebase*scale; //t(n+1)=t(n)+(a*b^n)*(1-c*p)
}

void Tree::checkForUnPruning()
{
  this->updateUnPruneAt();
  if (this->unPruneMetric()>=unprunenextchildat)
    this->unPruneNextChild();
}

void Tree::unPruneNow()
{
  unprunenextchildat=this->unPruneMetric();
  this->unPruneNextChild();
}

void Tree::allowContinuedPlay()
{
  terminaloverride=true;
  if (hasTerminalWinrate)
  {
    hasTerminalWinrate=false;
    playouts=1;
  }
}

Tree *Tree::getRobustChild(bool descend)
{
  float bestsims=0;
  Tree *besttree=NULL;
  
  for(std::list<Tree*>::iterator iter=children->begin();iter!=children->end();++iter) 
  {
    if ((*iter)->getPlayouts()>bestsims || (*iter)->isTerminalWin() || besttree==NULL)
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

Tree *Tree::getUrgentChild()
{
  float besturgency=0;
  Tree *besttree=NULL;
  
  for(std::list<Tree*>::iterator iter=children->begin();iter!=children->end();++iter) 
  {
    if ((*iter)->isPrimary() && !(*iter)->isPruned())
    {
      float urgency;
      
      if ((*iter)->getPlayouts()==0 && (*iter)->getRAVEPlayouts()==0 && (*iter)->getPriorPlayouts()==0)
        urgency=(*iter)->getUrgency()+(params->engine->getRandom()->getRandomReal()/1000);
      else
      {
        urgency=(*iter)->getUrgency();
        if (params->debug_on)
          fprintf(stderr,"[urg]:%s %.3f %.2f(%f) %.2f(%f) %.2f(%f)\n",(*iter)->getMove().toString(params->board_size).c_str(),urgency,(*iter)->getRatio(),(*iter)->getPlayouts(),(*iter)->getRAVERatio(),(*iter)->getRAVEPlayouts(),(*iter)->getPriorRatio(),(*iter)->getPriorPlayouts());
      }
          
      if (urgency>besturgency || besttree==NULL)
      {
        besttree=(*iter);
        besturgency=urgency;
      }
    }
  }
  
  if (params->debug_on)
  {
    if (besttree!=NULL)
      fprintf(stderr,"[best]:%s\n",besttree->getMove().toString(params->board_size).c_str());
    else
      fprintf(stderr,"WARNING! No urgent move found\n");
  }
  
  if (besttree==NULL)
    return NULL;
  
  if (params->move_policy==Parameters::MP_UCT && besttree->isLeaf() && !besttree->isTerminal())
  {
    if (besttree->getPlayouts()>params->uct_expand_after)
      besttree->expandLeaf();
  }
  
  if (besttree->isLeaf())
    return besttree;
  else
    return besttree->getUrgentChild();
}

void Tree::expandLeaf()
{
  if (!this->isLeaf())
    return;
  else if (this->isTerminal())
    fprintf(stderr,"WARNING! Trying to expand a terminal node!\n");
  
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
      return;
    }
  }
  //gtpe->getOutput()->printfDebug("\n"); //!!!
  
  Go::Color col=startboard->nextToMove();
  
  bool winnow=Go::Board::isWinForColor(col,startboard->score()-params->engine->getKomi());
  Tree *nmt=new Tree(params,startboard->getZobristHash(params->engine->getZobristTable()),Go::Move(col,Go::Move::PASS));
  if (winnow)
    nmt->addPriorWins(1);
  #if UCT_PASS_DETER>0
    if (startboard->getPassesPlayed()==0 && !(params->surewin_expected && col==params->engine->getCurrentBoard()->nextToMove()))
      nmt->addPriorLoses(UCT_PASS_DETER);
  #endif
  nmt->addRAVEWins(params->rave_init_wins);
  this->addChild(nmt);
  
  if (startboard->numOfValidMoves(col)>0)
  {
    Go::BitBoard *validmovesbitboard=startboard->getValidMoves(col);
    for (int p=0;p<startboard->getPositionMax();p++)
    {
      if (validmovesbitboard->get(p))
      {
        bool superkoviolation=false;
        if (params->rules_positional_superko_enabled && !params->rules_superko_top_ply)
        {
          Go::Board *thisboard=startboard->copy();
          thisboard->makeMove(Go::Move(col,p));
          Go::ZobristHash hash=thisboard->getZobristHash(params->engine->getZobristTable());
          delete thisboard;
          
          superkoviolation=this->isSuperkoViolationWith(hash);
          
          if (!superkoviolation)
            superkoviolation=params->engine->getZobristHashTree()->hasHash(hash);
        }
        
        //if (superkoviolation)
        //  fprintf(stderr,"superko violation avoided\n");
        
        if (!superkoviolation)
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
          nmt->addRAVEWins(params->rave_init_wins);
          
          this->addChild(nmt);
        }
      }
    }
    
    if (params->uct_atari_prior>0)
    {
      std::list<Go::Group*,Go::allocator_groupptr> *groups=startboard->getGroups();
      for(std::list<Go::Group*,Go::allocator_groupptr>::iterator iter=groups->begin();iter!=groups->end();++iter) 
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
    params->engine->getFeatures()->setupCFGDist(startboard);
    
    for(std::list<Tree*>::iterator iter=children->begin();iter!=children->end();++iter) 
    {
      if ((*iter)->isPrimary())
      {
        float gamma=params->engine->getFeatures()->getMoveGamma(startboard,(*iter)->getMove());
        (*iter)->setFeatureGamma(gamma);
      }
    }
    
    params->engine->getFeatures()->clearCFGDist();
    
    if (params->uct_progressive_widening_enabled)
    {
      this->pruneChildren();
      this->unPruneNow(); //unprune first child
    }
  }
  
  delete startboard;
}

void Tree::updateRAVE(Go::Color wincol,Go::BitBoard *blacklist,Go::BitBoard *whitelist)
{
  if (params->rave_moves<=0)
    return;
  
  //fprintf(stderr,"[update rave] %s (%c)\n",move.toString(params->board_size).c_str(),Go::colorToChar(wincol));
  
  if (!this->isRoot())
  {
    parent->updateRAVE(wincol,blacklist,whitelist);
    
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
        
        bool movemade=(col==Go::BLACK?blacklist:whitelist)->get(pos);
        
        if (movemade)
        {
          if ((*iter)->isPrimary())
          {
            if (col==wincol)
              (*iter)->addRAVEWin();
            else
              (*iter)->addRAVELose();
          }
          else
          {
            Tree *primary=(*iter)->getPrimary();
            if (col==wincol)
              primary->addRAVEWin();
            else
              primary->addRAVELose();
          }
        }
      }
    }
  }
}

float Tree::getProgressiveBias()
{
  float bias=params->uct_progressive_bias_h*gamma/(playouts+1);
  if (params->uct_progressive_bias_scaled && !this->isRoot())
    bias/=parent->getChildrenTotalFeatureGamma();
  if (params->uct_progressive_bias_relative && !this->isRoot())
    bias/=parent->getMaxChildFeatureGamma();
  return bias;
}

bool Tree::isSuperkoViolationWith(Go::ZobristHash h)
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
  gamma=g;
  if (!this->isRoot())
  {
    parent->childrentotalgamma+=gamma;
    if (gamma>(parent->maxchildgamma))
      parent->maxchildgamma=gamma;
  }
}

void Tree::prunePossibleSuperkoViolations()
{
  if (params->rules_positional_superko_enabled && params->rules_superko_top_ply)
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
        delete thisboard;
        
        bool superkoviolation=this->isSuperkoViolationWith(hash);
        
        if (!superkoviolation)
          superkoviolation=params->engine->getZobristHashTree()->hasHash(hash);
        
        if (superkoviolation)
        {
          std::list<Tree*>::iterator tmpiter=iter;
          Tree *tmptree=(*iter);
          --iter;
          children->erase(tmpiter);
          delete tmptree;
        }
      }
    }
    
    delete startboard;
  }
}

float Tree::bestChildRatioDiff()
{
  Tree *bestchild=this->getRobustChild();
  
  if (bestchild==NULL)
    return -1;
  
  return (this->getRatio()+bestchild->getRatio()-1);
}

Tree *Tree::getSecondRobustChild(Tree *firstchild)
{
  if (firstchild==NULL)
    firstchild=this->getRobustChild();
  
  Tree *besttree=NULL;
  int bestsims=0;
  
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

float Tree::secondBestPlayoutRatio()
{
  if (this->isRoot())
    return -1;
  
  Tree *secondbest=parent->getSecondRobustChild(this);
  return (this->getPlayouts()/secondbest->getPlayouts());
}

Tree *Tree::getBestRatioChild(float playoutthreshold)
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

