#include "Features.h"

Features::Features()
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
}

Features::~Features()
{
  delete patterngammas;
  delete patternids;
}

unsigned int Features::matchFeatureClass(Features::FeatureClass featclass, Go::Board *board, Go::Move move)
{
  if ((featclass!=Features::PASS && move.isPass()) || move.isResign())
    return 0;
  
  if (!board->validMove(move))
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
      int size=board->getSize();
      Go::Color col=move.getColor();
      int pos=move.getPosition();
      foreach_adjacent(pos,p,{
        if (board->inGroup(p))
        {
          Go::Group *group=board->getGroup(p);
          if (group->inAtari() && col!=group->getColor())
            return 1;
        }
      });
      return 0;
    }
    case Features::EXTENSION:
    {
      int size=board->getSize();
      Go::Color col=move.getColor();
      int pos=move.getPosition();
      bool foundgroupinatari=false;
      bool foundextension=false;
      foreach_adjacent(pos,p,{
        if (board->inGroup(p))
        {
          Go::Group *group=board->getGroup(p);
          if (group->inAtari() && col==group->getColor())
            foundgroupinatari=true;
          else if (col==group->getColor())
            foundextension=true;
        }
        else if (board->touchingEmpty(p)>1)
          foundextension=true;
      });
      if (foundgroupinatari && foundextension)
        return 1;
      else
        return 0;
    }
    case Features::SELFATARI:
    {
      int size=board->getSize();
      Go::Color col=move.getColor();
      int pos=move.getPosition();
      
      if (board->touchingEmpty(pos)>1)
        return 0;
      
      bool foundgroupwith2libsorless=false;
      bool foundconnection=false;
      Go::Group *grp2libs=NULL;
      foreach_adjacent(pos,p,{
        if (board->inGroup(p))
        {
          Go::Group *group=board->getGroup(p);
          if (col==group->getColor() && group->isOneOfTwoLiberties(pos))
          {
            if (!foundgroupwith2libsorless)
            {
              foundgroupwith2libsorless=true;
              grp2libs=group;
            }
            else if (group!=grp2libs)
              foundconnection=true;
          }
          else if (col==group->getColor() && !group->inAtari())
            foundconnection=true;
        }
      });
      if (!foundconnection)
        return 1;
      else
        return 0;
    }
    case Features::ATARI:
    {
      int size=board->getSize();
      Go::Color col=move.getColor();
      int pos=move.getPosition();
      foreach_adjacent(pos,p,{
        if (board->inGroup(p))
        {
          Go::Group *group=board->getGroup(p);
          if (col!=group->getColor() && group->isOneOfTwoLiberties(pos))
          {
            if (board->isCurrentSimpleKo())
              return 2;
            else
              return 1;
          }
        }
      });
      return 0;
    }
    case Features::BORDERDIST:
    {
      int size=board->getSize();
      int pos=move.getPosition();
      
      int x=Go::Position::pos2x(pos,size);
      int y=Go::Position::pos2y(pos,size);
      int ix=(size-x-1);
      int iy=(size-y-1);
      
      int dist=x;
      if (y<dist)
        dist=y;
      if (ix<dist)
        dist=ix;
      if (iy<dist)
        dist=iy;
      
      if (dist<BORDERDIST_LEVELS)
        return (dist+1);
      else
        return 0;
    }
    case Features::LASTDIST:
    {
      if (board->getLastMove().isPass() || board->getLastMove().isResign())
        return 0;
      
      int size=board->getSize();
      int pos1=move.getPosition();
      int pos2=board->getLastMove().getPosition();
      
      int x1=Go::Position::pos2x(pos1,size);
      int y1=Go::Position::pos2y(pos1,size);
      int x2=Go::Position::pos2x(pos2,size);
      int y2=Go::Position::pos2y(pos2,size);
      
      int dist=abs(x1-x2)+abs(y1-y2);
      
      if (dist<=LASTDIST_LEVELS)
        return dist;
      else
        return 0;
    }
    case Features::SECONDLASTDIST:
    {
      if (board->getSecondLastMove().isPass() || board->getSecondLastMove().isResign())
        return 0;
      
      int size=board->getSize();
      int pos1=move.getPosition();
      int pos2=board->getSecondLastMove().getPosition();
      
      int x1=Go::Position::pos2x(pos1,size);
      int y1=Go::Position::pos2y(pos1,size);
      int x2=Go::Position::pos2x(pos2,size);
      int y2=Go::Position::pos2y(pos2,size);
      
      int dist=abs(x1-x2)+abs(y1-y2);
      
      if (dist<=SECONDLASTDIST_LEVELS)
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

float Features::getFeatureGamma(Features::FeatureClass featclass, unsigned int level)
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

float Features::getMoveGamma(Go::Board *board, Go::Move move)
{
  float g=1.0;
  
  if (!board->validMove(move))
    return 0;
  
  g*=this->getFeatureGamma(Features::PASS,this->matchFeatureClass(Features::PASS,board,move));
  g*=this->getFeatureGamma(Features::CAPTURE,this->matchFeatureClass(Features::CAPTURE,board,move));
  g*=this->getFeatureGamma(Features::EXTENSION,this->matchFeatureClass(Features::EXTENSION,board,move));
  g*=this->getFeatureGamma(Features::SELFATARI,this->matchFeatureClass(Features::SELFATARI,board,move));
  g*=this->getFeatureGamma(Features::ATARI,this->matchFeatureClass(Features::ATARI,board,move));
  g*=this->getFeatureGamma(Features::BORDERDIST,this->matchFeatureClass(Features::BORDERDIST,board,move));
  g*=this->getFeatureGamma(Features::LASTDIST,this->matchFeatureClass(Features::LASTDIST,board,move));
  g*=this->getFeatureGamma(Features::SECONDLASTDIST,this->matchFeatureClass(Features::SECONDLASTDIST,board,move));
  g*=this->getFeatureGamma(Features::PATTERN3X3,this->matchFeatureClass(Features::PATTERN3X3,board,move));
  
  return g;
}

float Features::getBoardGamma(Go::Board *board, Go::Color col)
{
  float total=0;
  
  for (int p=0;p<board->getPositionMax();p++)
  {
    Go::Move move=Go::Move(col,p);
    if (board->validMove(move))
      total+=this->getMoveGamma(board,move);
  }
  
  Go::Move passmove=Go::Move(col,Go::Move::PASS);
  total+=this->getMoveGamma(board,passmove);
  
  return total;
}

std::string Features::getFeatureClassName(Features::FeatureClass featclass)
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
    case Features::PATTERN3X3:
      return "pattern3x3";
    default:
      return "";
  }
}

Features::FeatureClass Features::getFeatureClassFromName(std::string name)
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
      return false;
  }
  
  fin.close();
  
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

float *Features::getStandardGamma(Features::FeatureClass featclass)
{
  switch (featclass)
  {
    case Features::PASS:
      return gammas_pass;
    case Features::CAPTURE:
      return gammas_capture;
    case Features::EXTENSION:
      return gammas_extension;
    case Features::ATARI:
      return gammas_atari;
    case Features::SELFATARI:
      return gammas_selfatari;
    case Features::BORDERDIST:
      return gammas_borderdist;
    case Features::LASTDIST:
      return gammas_lastdist;
    case Features::SECONDLASTDIST:
      return gammas_secondlastdist;
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

std::string Features::getMatchingFeaturesString(Go::Board *board, Go::Move move, bool pretty)
{
  std::ostringstream ss;
  unsigned int level;
  
  level=this->matchFeatureClass(Features::PASS,board,move);
  if (level>0)
  {
    if (pretty)
      ss<<" pass:"<<level;
    else
      ss<<" "<<(level-1);
  }
  
  level=this->matchFeatureClass(Features::CAPTURE,board,move);
  if (level>0)
  {
    if (pretty)
      ss<<" capture:"<<level;
    else
      ss<<" "<<(PASS_LEVELS+level-1);
  }
  
  level=this->matchFeatureClass(Features::EXTENSION,board,move);
  if (level>0)
  {
    if (pretty)
      ss<<" extension:"<<level;
    else
      ss<<" "<<(PASS_LEVELS+CAPTURE_LEVELS+level-1);
  }
  
  level=this->matchFeatureClass(Features::SELFATARI,board,move);
  if (level>0)
  {
    if (pretty)
      ss<<" selfatari:"<<level;
    else
      ss<<" "<<(PASS_LEVELS+CAPTURE_LEVELS+EXTENSION_LEVELS+level-1);
  }
  
  level=this->matchFeatureClass(Features::ATARI,board,move);
  if (level>0)
  {
    if (pretty)
      ss<<" atari:"<<level;
    else
      ss<<" "<<(PASS_LEVELS+CAPTURE_LEVELS+EXTENSION_LEVELS+SELFATARI_LEVELS+level-1);
  }
  
  level=this->matchFeatureClass(Features::BORDERDIST,board,move);
  if (level>0)
  {
    if (pretty)
      ss<<" borderdist:"<<level;
    else
      ss<<" "<<(PASS_LEVELS+CAPTURE_LEVELS+EXTENSION_LEVELS+SELFATARI_LEVELS+ATARI_LEVELS+level-1);
  }
  
  level=this->matchFeatureClass(Features::LASTDIST,board,move);
  if (level>0)
  {
    if (pretty)
      ss<<" lastdist:"<<level;
    else
      ss<<" "<<(PASS_LEVELS+CAPTURE_LEVELS+EXTENSION_LEVELS+SELFATARI_LEVELS+ATARI_LEVELS+BORDERDIST_LEVELS+level-1);
  }
  
  level=this->matchFeatureClass(Features::SECONDLASTDIST,board,move);
  if (level>0)
  {
    if (pretty)
      ss<<" secondlastdist:"<<level;
    else
      ss<<" "<<(PASS_LEVELS+CAPTURE_LEVELS+EXTENSION_LEVELS+SELFATARI_LEVELS+ATARI_LEVELS+BORDERDIST_LEVELS+LASTDIST_LEVELS+level-1);
  }
  
  level=this->matchFeatureClass(Features::PATTERN3X3,board,move);
  if (patterngammas->hasGamma(level) && !move.isPass() && !move.isResign())
  {
    if (pretty)
      ss<<" pattern3x3:0x"<<std::hex<<std::setw(4)<<std::setfill('0')<<level;
    else
      ss<<" "<<(int)patternids->getGamma(level);
  }
  
  return ss.str();
}

std::string Features::getFeatureIdList()
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
  unsigned int id=PASS_LEVELS+CAPTURE_LEVELS+EXTENSION_LEVELS+SELFATARI_LEVELS+ATARI_LEVELS+BORDERDIST_LEVELS+LASTDIST_LEVELS+SECONDLASTDIST_LEVELS;
  
  for (unsigned int level=0;level<PATTERN_3x3_GAMMAS;level++)
  {
    if (patterngammas->hasGamma(level))
      patternids->setGamma(level,id++);
  }
}

