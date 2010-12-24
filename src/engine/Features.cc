#include "Features.h"

int Features::matchFeatureClass(Features::FeatureClass featclass, Go::Board *board, Go::Move move)
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
            return true;
        }
      });
      return 0;
    }
    default:
      return 0;
  }
}

float Features::getFeatureGamma(Features::FeatureClass featclass, int level)
{
  if (level==0)
    return 1.0;
  
  // values hard-coded for initial testing
  
  switch (featclass)
  {
    case Features::PASS:
    {
      if (level==1)
        return 0.17;
      else if (level==2)
        return 24.37;
      else
        return 1.0;
    }
    case Features::CAPTURE:
    {
      if (level==1)
        return 30.68;
      else
        return 1.0;
    }
    case Features::EXTENSION:
    {
      if (level==1)
        return 11.37;
      else
        return 1.0;
    }
    case Features::ATARI:
    {
      if (level==1)
        return 2.3;
      else
        return 1.0;
    }
    default:
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
  g*=this->getFeatureGamma(Features::ATARI,this->matchFeatureClass(Features::ATARI,board,move));
  
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

