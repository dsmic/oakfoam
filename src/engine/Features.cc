#include "Features.h"

#include <sstream>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <iomanip>
#include "Parameters.h"
#include "Pattern.h"

Features::Features(Parameters *prms) : params(prms)
{
  patterngammas=new Pattern::ThreeByThreeGammas();
  patternids=new Pattern::ThreeByThreeGammas();
  
  for (int i=0;i<PASS_LEVELS;i++)
    gammas_pass[i]=1.0;
  for (int i=0;i<CAPTURE_LEVELS;i++)
    gammas_capture[i]=1.0;
  for (int i=0;i<EXTENSION_LEVELS;i++)
    gammas_extension[i]=1.0;
  for (int i=0;i<SELFATARI_LEVELS;i++)
    gammas_selfatari[i]=1.0;
  for (int i=0;i<ATARI_LEVELS;i++)
    gammas_atari[i]=1.0;
  for (int i=0;i<BORDERDIST_LEVELS;i++)
    gammas_borderdist[i]=1.0;
  for (int i=0;i<LASTDIST_LEVELS;i++)
    gammas_lastdist[i]=1.0;
  for (int i=0;i<SECONDLASTDIST_LEVELS;i++)
    gammas_secondlastdist[i]=1.0;
  for (int i=0;i<CFGLASTDIST_LEVELS;i++)
    gammas_cfglastdist[i]=1.0;
  for (int i=0;i<CFGSECONDLASTDIST_LEVELS;i++)
    gammas_cfgsecondlastdist[i]=1.0;
}

Features::~Features()
{
  delete patterngammas;
  delete patternids;
}

unsigned int Features::matchFeatureClass(Features::FeatureClass featclass, Go::Board *board, Go::ObjectBoard<int> *cfglastdist, Go::ObjectBoard<int> *cfgsecondlastdist, Go::Move move, bool checkforvalidmove) const
{
  if ((featclass!=Features::PASS && move.isPass()) || move.isResign())
    return 0;
  
  if (checkforvalidmove && !board->validMove(move))
    return 0;
  
  switch (featclass)
  {
    case Features::PASS:
    {
      if (!move.isPass())
        return 0;
      else if (board->getPassesPlayed()==0)
        return 1;
      else
        return 2;
    }
    case Features::CAPTURE:
    {
      if (board->isCapture(move))
      {
        Go::Color col=move.getColor();
        int pos=move.getPosition();
        int size=board->getSize();

        foreach_adjacent(pos,p,{
          if (board->inGroup(p))
          {
            Go::Group *group=board->getGroup(p);
            if (group->getColor()!=col && group->inAtari()) // captured group
            {
              Go::list_int *adjacentgroups=group->getAdjacentGroups();
              for(Go::list_int::iterator iter=adjacentgroups->begin();iter!=adjacentgroups->end();++iter)
              {
                if (board->inGroup((*iter)) && board->getGroup((*iter))->inAtari())
                {
                  if (group->numOfStones()>=10)
                    return 3;
                  else
                    return 2;
                }
              }
            }
          }
        });

        return 1;
      }
      else
        return 0;
    }
    case Features::EXTENSION:
    {
      if (board->isExtension(move))
        return 1;
      else
        return 0;
    }
    case Features::SELFATARI:
    {
      if (board->isSelfAtari(move))
        return 1;
      else
        return 0;
    }
    case Features::ATARI:
    {
      if (board->isAtari(move))
      {
        if (board->isCurrentSimpleKo())
          return 2;
        else
          return 1;
      }
      else
        return 0;
    }
    case Features::BORDERDIST:
    {
      int dist=board->getDistanceToBorder(move.getPosition());
      
      if (dist<BORDERDIST_LEVELS)
        return (dist+1);
      else
        return 0;
    }
    case Features::LASTDIST:
    {
      if (board->getLastMove().isPass() || board->getLastMove().isResign())
        return 0;
      
      int dist=board->getCircularDistance(move.getPosition(),board->getLastMove().getPosition());
      
      int maxdist=LASTDIST_LEVELS;
      if (params->features_only_small)
        maxdist=3;
      
      if (dist<=maxdist)
        return dist;
      else
        return 0;
    }
    case Features::SECONDLASTDIST:
    {
      if (board->getSecondLastMove().isPass() || board->getSecondLastMove().isResign())
        return 0;
      
      int dist=board->getCircularDistance(move.getPosition(),board->getSecondLastMove().getPosition());
      
      int maxdist=SECONDLASTDIST_LEVELS;
      if (params->features_only_small)
        maxdist=3;
      
      if (dist<=maxdist)
        return dist;
      else
        return 0;
    }
    case Features::CFGLASTDIST:
    {
      if (board->getLastMove().isPass() || board->getLastMove().isResign())
        return 0;
      
      if (cfglastdist==NULL)
        return 0;
      
      int dist=cfglastdist->get(move.getPosition());
      if (dist==-1)
        return 0;
      
      if (dist<=CFGLASTDIST_LEVELS)
        return dist;
      else
        return 0;
    }
    case Features::CFGSECONDLASTDIST:
    {
      if (board->getLastMove().isPass() || board->getLastMove().isResign())
        return 0;
      
      if (cfgsecondlastdist==NULL)
        return 0;
      
      int dist=cfgsecondlastdist->get(move.getPosition());
      if (dist==-1)
        return 0;
      
      if (dist<=CFGSECONDLASTDIST_LEVELS)
        return dist;
      else
        return 0;
    }
    case Features::PATTERN3X3:
    {
      Go::Color col=move.getColor();
      int pos=move.getPosition();
      
      unsigned int hash=Pattern::ThreeByThree::makeHash(board,pos);
      if (col==Go::WHITE)
        hash=Pattern::ThreeByThree::invert(hash);
      hash=Pattern::ThreeByThree::smallestEquivalent(hash);
      
      return hash;
    }
    default:
      return 0;
  }
}

float Features::getFeatureGamma(Features::FeatureClass featclass, unsigned int level) const
{
  if (level==0 && featclass!=Features::PATTERN3X3)
    return 1.0;
  
  if (featclass==Features::PATTERN3X3)
  {
    if (patterngammas->hasGamma(level))
      return patterngammas->getGamma(level);
    else
      return 1.0;
  }
  else
  {
    float *gammas=this->getStandardGamma(featclass);
    if (gammas!=NULL)
    {
      float gamma=gammas[level-1];
      if (gamma>0)
        return gamma;
      else
        return 1.0;
    }
    else
      return 1.0;
  }
}

float Features::getMoveGamma(Go::Board *board, Go::ObjectBoard<int> *cfglastdist, Go::ObjectBoard<int> *cfgsecondlastdist, Go::Move move, bool checkforvalidmove) const
{
  float g=1.0;
  
  if (checkforvalidmove && !board->validMove(move))
    return 0;
  
  g*=this->getFeatureGamma(Features::PASS,this->matchFeatureClass(Features::PASS,board,cfglastdist,cfgsecondlastdist,move,false));
  g*=this->getFeatureGamma(Features::CAPTURE,this->matchFeatureClass(Features::CAPTURE,board,cfglastdist,cfgsecondlastdist,move,false));
  g*=this->getFeatureGamma(Features::EXTENSION,this->matchFeatureClass(Features::EXTENSION,board,cfglastdist,cfgsecondlastdist,move,false));
  g*=this->getFeatureGamma(Features::SELFATARI,this->matchFeatureClass(Features::SELFATARI,board,cfglastdist,cfgsecondlastdist,move,false));
  g*=this->getFeatureGamma(Features::ATARI,this->matchFeatureClass(Features::ATARI,board,cfglastdist,cfgsecondlastdist,move,false));
  g*=this->getFeatureGamma(Features::BORDERDIST,this->matchFeatureClass(Features::BORDERDIST,board,cfglastdist,cfgsecondlastdist,move,false));
  g*=this->getFeatureGamma(Features::LASTDIST,this->matchFeatureClass(Features::LASTDIST,board,cfglastdist,cfgsecondlastdist,move,false));
  g*=this->getFeatureGamma(Features::SECONDLASTDIST,this->matchFeatureClass(Features::SECONDLASTDIST,board,cfglastdist,cfgsecondlastdist,move,false));
  g*=this->getFeatureGamma(Features::CFGLASTDIST,this->matchFeatureClass(Features::CFGLASTDIST,board,cfglastdist,cfgsecondlastdist,move,false));
  g*=this->getFeatureGamma(Features::CFGSECONDLASTDIST,this->matchFeatureClass(Features::CFGSECONDLASTDIST,board,cfglastdist,cfgsecondlastdist,move,false));
  g*=this->getFeatureGamma(Features::PATTERN3X3,this->matchFeatureClass(Features::PATTERN3X3,board,cfglastdist,cfgsecondlastdist,move,false));
  
  return g;
}

float Features::getBoardGamma(Go::Board *board, Go::ObjectBoard<int> *cfglastdist, Go::ObjectBoard<int> *cfgsecondlastdist, Go::Color col) const
{
  float total=0;
  
  for (int p=0;p<board->getPositionMax();p++)
  {
    Go::Move move=Go::Move(col,p);
    if (board->validMove(move))
      total+=this->getMoveGamma(board,cfglastdist,cfgsecondlastdist,move,false);
  }
  
  Go::Move passmove=Go::Move(col,Go::Move::PASS);
  total+=this->getMoveGamma(board,cfglastdist,cfgsecondlastdist,passmove,false);
  
  return total;
}

float Features::getBoardGammas(Go::Board *board, Go::ObjectBoard<int> *cfglastdist, Go::ObjectBoard<int> *cfgsecondlastdist, Go::Color col, Go::ObjectBoard<float> *gammas) const
{
  float total=0;
  
  for (int p=0;p<board->getPositionMax();p++)
  {
    Go::Move move=Go::Move(col,p);
    if (board->validMove(move))
    {
      float gamma=this->getMoveGamma(board,cfglastdist,cfgsecondlastdist,move,false);;
      gammas->set(p,gamma);
      total+=gamma;
    }
  }
  
  {
    Go::Move move=Go::Move(col,Go::Move::PASS);
    float gamma=this->getMoveGamma(board,cfglastdist,cfgsecondlastdist,move,false);;
    gammas->set(0,gamma);
    total+=gamma;
  }
  
  return total;
}

std::string Features::getFeatureClassName(Features::FeatureClass featclass) const
{
  switch (featclass)
  {
    case Features::PASS:
      return "pass";
    case Features::CAPTURE:
      return "capture";
    case Features::EXTENSION:
      return "extension";
    case Features::ATARI:
      return "atari";
    case Features::SELFATARI:
      return "selfatari";
    case Features::BORDERDIST:
      return "borderdist";
    case Features::LASTDIST:
      return "lastdist";
    case Features::SECONDLASTDIST:
      return "secondlastdist";
    case Features::CFGLASTDIST:
      return "cfglastdist";
    case Features::CFGSECONDLASTDIST:
      return "cfgsecondlastdist";
    case Features::PATTERN3X3:
      return "pattern3x3";
    default:
      return "";
  }
}

Features::FeatureClass Features::getFeatureClassFromName(std::string name) const
{
  if (name=="pass")
    return Features::PASS;
  else if (name=="capture")
    return Features::CAPTURE;
  else if (name=="extension")
    return Features::EXTENSION;
  else if (name=="atari")
    return Features::ATARI;
  else if (name=="selfatari")
    return Features::SELFATARI;
  else if (name=="borderdist")
    return Features::BORDERDIST;
  else if (name=="lastdist")
    return Features::LASTDIST;
  else if (name=="secondlastdist")
    return Features::SECONDLASTDIST;
  else if (name=="cfglastdist")
    return Features::CFGLASTDIST;
  else if (name=="cfgsecondlastdist")
    return Features::CFGSECONDLASTDIST;
  else if (name=="pattern3x3")
    return Features::PATTERN3X3;
  else
    return Features::INVALID;
}

bool Features::loadGammaFile(std::string filename)
{
  std::ifstream fin(filename.c_str());
  
  if (!fin)
    return false;
  
  std::string line;
  while (std::getline(fin,line))
  {
    if (!this->loadGammaLine(line))
    {
      fin.close();
      return false;
    }
  }
  
  fin.close();
  
  return true;
}

bool Features::loadGammaString(std::string lines)
{
  std::istringstream iss(lines);
  
  std::string line;
  while (getline(iss,line,'\n'))
  {
    if (!this->loadGammaLine(line))
      return false;
  }
  
  return true;
}

bool Features::loadGammaLine(std::string line)
{
  std::string id,classname,levelstring,gammastring;
  Features::FeatureClass featclass;
  unsigned int level;
  float gamma;
  
  std::transform(line.begin(),line.end(),line.begin(),::tolower);
  
  std::istringstream issline(line);
  if (!getline(issline,id,' '))
    return false;
  if (!getline(issline,gammastring,' '))
    return false;
  
  std::istringstream issid(id);
  if (!getline(issid,classname,':'))
    return false;
  if (!getline(issid,levelstring,':'))
    return false;
  
  if (levelstring.at(0)=='0' && levelstring.at(1)=='x')
  {
    level=0;
    std::string hex="0123456789abcdef";
    for (unsigned int i=2;i<levelstring.length();i++)
    {
      level=level*16+(unsigned int)hex.find(levelstring.at(i),0);
    }
  }
  else
  {
    std::istringstream isslevel(levelstring);
    if (!(isslevel >> level))
      return false;
  }
  
  std::istringstream issgamma(gammastring);
  if (!(issgamma >> gamma))
    return false;
  
  featclass=this->getFeatureClassFromName(classname);
  if (featclass==Features::INVALID)
    return false;
  
  return this->setFeatureGamma(featclass,level,gamma);
}

float *Features::getStandardGamma(Features::FeatureClass featclass) const
{
  switch (featclass)
  {
    case Features::PASS:
      return (float *)gammas_pass;
    case Features::CAPTURE:
      return (float *)gammas_capture;
    case Features::EXTENSION:
      return (float *)gammas_extension;
    case Features::ATARI:
      return (float *)gammas_atari;
    case Features::SELFATARI:
      return (float *)gammas_selfatari;
    case Features::BORDERDIST:
      return (float *)gammas_borderdist;
    case Features::LASTDIST:
      return (float *)gammas_lastdist;
    case Features::SECONDLASTDIST:
      return (float *)gammas_secondlastdist;
    case Features::CFGLASTDIST:
      return (float *)gammas_cfglastdist;
    case Features::CFGSECONDLASTDIST:
      return (float *)gammas_cfgsecondlastdist;
    default:
      return NULL;
  }
}

bool Features::setFeatureGamma(Features::FeatureClass featclass, unsigned int level, float gamma)
{
  if (featclass==Features::PATTERN3X3)
  {
    patterngammas->setGamma(level,gamma);
    this->updatePatternIds();
    return true;
  }
  else
  {
    float *gammas=this->getStandardGamma(featclass);
    if (gammas!=NULL)
    {
      gammas[level-1]=gamma;
      return true;
    }
    else
      return false;
  }
}

std::string Features::getMatchingFeaturesString(Go::Board *board, Go::ObjectBoard<int> *cfglastdist, Go::ObjectBoard<int> *cfgsecondlastdist, Go::Move move, bool pretty) const
{
  std::ostringstream ss;
  unsigned int level,base;
  
  base=0;
  
  level=this->matchFeatureClass(Features::PASS,board,cfglastdist,cfgsecondlastdist,move);
  if (level>0)
  {
    if (pretty)
      ss<<" pass:"<<level;
    else
      ss<<" "<<(level-1);
  }
  base+=PASS_LEVELS;
  
  level=this->matchFeatureClass(Features::CAPTURE,board,cfglastdist,cfgsecondlastdist,move);
  if (level>0)
  {
    if (pretty)
      ss<<" capture:"<<level;
    else
      ss<<" "<<(base+level-1);
  }
  base+=CAPTURE_LEVELS;
  
  level=this->matchFeatureClass(Features::EXTENSION,board,cfglastdist,cfgsecondlastdist,move);
  if (level>0)
  {
    if (pretty)
      ss<<" extension:"<<level;
    else
      ss<<" "<<(base+level-1);
  }
  base+=EXTENSION_LEVELS;
  
  level=this->matchFeatureClass(Features::SELFATARI,board,cfglastdist,cfgsecondlastdist,move);
  if (level>0)
  {
    if (pretty)
      ss<<" selfatari:"<<level;
    else
      ss<<" "<<(base+level-1);
  }
  base+=SELFATARI_LEVELS;
  
  level=this->matchFeatureClass(Features::ATARI,board,cfglastdist,cfgsecondlastdist,move);
  if (level>0)
  {
    if (pretty)
      ss<<" atari:"<<level;
    else
      ss<<" "<<(base+level-1);
  }
  base+=ATARI_LEVELS;
  
  level=this->matchFeatureClass(Features::BORDERDIST,board,cfglastdist,cfgsecondlastdist,move);
  if (level>0)
  {
    if (pretty)
      ss<<" borderdist:"<<level;
    else
      ss<<" "<<(base+level-1);
  }
  base+=BORDERDIST_LEVELS;
  
  level=this->matchFeatureClass(Features::LASTDIST,board,cfglastdist,cfgsecondlastdist,move);
  if (level>0)
  {
    if (pretty)
      ss<<" lastdist:"<<level;
    else
      ss<<" "<<(base+level-1);
  }
  base+=LASTDIST_LEVELS;
  
  level=this->matchFeatureClass(Features::SECONDLASTDIST,board,cfglastdist,cfgsecondlastdist,move);
  if (level>0)
  {
    if (pretty)
      ss<<" secondlastdist:"<<level;
    else
      ss<<" "<<(base+level-1);
  }
  base+=SECONDLASTDIST_LEVELS;
  
  level=this->matchFeatureClass(Features::CFGLASTDIST,board,cfglastdist,cfgsecondlastdist,move);
  if (level>0)
  {
    if (pretty)
      ss<<" cfglastdist:"<<level;
    else
      ss<<" "<<(base+level-1);
  }
  base+=CFGLASTDIST_LEVELS;
  
  level=this->matchFeatureClass(Features::CFGSECONDLASTDIST,board,cfglastdist,cfgsecondlastdist,move);
  if (level>0)
  {
    if (pretty)
      ss<<" cfgsecondlastdist:"<<level;
    else
      ss<<" "<<(base+level-1);
  }
  base+=CFGSECONDLASTDIST_LEVELS;
  
  level=this->matchFeatureClass(Features::PATTERN3X3,board,cfglastdist,cfgsecondlastdist,move);
  if (patterngammas->hasGamma(level) && !move.isPass() && !move.isResign())
  {
    if (pretty)
      ss<<" pattern3x3:0x"<<std::hex<<std::setw(4)<<std::setfill('0')<<level;
    else
      ss<<" "<<(int)patternids->getGamma(level);
  }
  
  return ss.str();
}

std::string Features::getFeatureIdList() const
{
  std::ostringstream ss;
  unsigned int id=0;
  
  for (unsigned int level=1;level<=PASS_LEVELS;level++)
    ss<<(id++)<<" pass:"<<level<<"\n";
  
  for (unsigned int level=1;level<=CAPTURE_LEVELS;level++)
    ss<<(id++)<<" capture:"<<level<<"\n";
  
  for (unsigned int level=1;level<=EXTENSION_LEVELS;level++)
    ss<<(id++)<<" extension:"<<level<<"\n";
  
  for (unsigned int level=1;level<=SELFATARI_LEVELS;level++)
    ss<<(id++)<<" selfatari:"<<level<<"\n";
  
  for (unsigned int level=1;level<=ATARI_LEVELS;level++)
    ss<<(id++)<<" atari:"<<level<<"\n";
  
  for (unsigned int level=1;level<=BORDERDIST_LEVELS;level++)
    ss<<(id++)<<" borderdist:"<<level<<"\n";
  
  for (unsigned int level=1;level<=LASTDIST_LEVELS;level++)
    ss<<(id++)<<" lastdist:"<<level<<"\n";
  
  for (unsigned int level=1;level<=SECONDLASTDIST_LEVELS;level++)
    ss<<(id++)<<" secondlastdist:"<<level<<"\n";
  
  for (unsigned int level=1;level<=CFGLASTDIST_LEVELS;level++)
    ss<<(id++)<<" cfglastdist:"<<level<<"\n";
  
  for (unsigned int level=1;level<=CFGSECONDLASTDIST_LEVELS;level++)
    ss<<(id++)<<" cfgsecondlastdist:"<<level<<"\n";
  
  for (unsigned int level=0;level<PATTERN_3x3_GAMMAS;level++)
  {
    if (patterngammas->hasGamma(level))
    {
      if (id==patternids->getGamma(level))
        ss<<std::dec<<(id++)<<" pattern3x3:0x"<<std::hex<<std::setw(4)<<std::setfill('0')<<level<<"\n";
      else
        fprintf(stderr,"WARNING! pattern id mismatch");
    }
  }
  
  return ss.str();
}

void Features::updatePatternIds()
{
  unsigned int id=PASS_LEVELS+CAPTURE_LEVELS+EXTENSION_LEVELS+SELFATARI_LEVELS+ATARI_LEVELS+BORDERDIST_LEVELS+LASTDIST_LEVELS+SECONDLASTDIST_LEVELS+CFGLASTDIST_LEVELS+CFGSECONDLASTDIST_LEVELS;
  
  for (unsigned int level=0;level<PATTERN_3x3_GAMMAS;level++)
  {
    if (patterngammas->hasGamma(level))
      patternids->setGamma(level,id++);
  }
}

void Features::computeCFGDist(Go::Board *board, Go::ObjectBoard<int> **cfglastdist, Go::ObjectBoard<int> **cfgsecondlastdist)
{
  if (!board->getLastMove().isPass() && !board->getLastMove().isResign())
    *cfglastdist=board->getCFGFrom(board->getLastMove().getPosition(),CFGLASTDIST_LEVELS);
  if (!board->getSecondLastMove().isPass() && !board->getSecondLastMove().isResign())
    *cfgsecondlastdist=board->getCFGFrom(board->getSecondLastMove().getPosition(),CFGSECONDLASTDIST_LEVELS);
}

