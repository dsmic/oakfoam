#include "Benson.h"

//for debugging:
#include <cstdio>

Benson::Benson(Go::Board *bd)
  : board(bd),
    size(board->getSize()),
    safepositions(new Go::ObjectBoard<Go::Color>(size))
{
  safepositions->fill(Go::EMPTY);
}

Benson::~Benson()
{
  delete safepositions;
  for(std::list<Benson::Chain*>::iterator iter=blackchains.begin();iter!=blackchains.end();++iter) 
  {
    delete (*iter);
  }
  for(std::list<Benson::Chain*>::iterator iter=whitechains.begin();iter!=whitechains.end();++iter) 
  {
    delete (*iter);
  }
  for(std::list<Benson::Region*>::iterator iter=blackregions.begin();iter!=blackregions.end();++iter) 
  {
    delete (*iter);
  }
  for(std::list<Benson::Region*>::iterator iter=whiteregions.begin();iter!=whiteregions.end();++iter) 
  {
    delete (*iter);
  }
}

void Benson::solve()
{
  this->updateChainsAndRegions();
  
  bool removedchains=true,removedregions=true;
  while (removedchains || removedregions)
  {
    this->updateVitalRegions();
    removedchains=this->removeNonVitalChains();
    removedregions=this->removeNonSurroundedRegions();
    //fprintf(stderr,"rmv: %d %d\n",removedchains,removedregions);
  }
  
  for (int p=0;p<board->getPositionMax();p++)
  {
    if (board->inGroup(p))
    {
      Go::Group *group=board->getGroup(p);
      bool foundchain=false;
      
      if (group->getColor()==Go::BLACK)
      {
        for(std::list<Benson::Chain*>::iterator iter=blackchains.begin();iter!=blackchains.end();++iter) 
        {
          if ((*iter)->group==group)
          {
            foundchain=true;
            break;
          }
        }
      }
      else
      {
        for(std::list<Benson::Chain*>::iterator iter=whitechains.begin();iter!=whitechains.end();++iter) 
        {
          if ((*iter)->group==group)
          {
            foundchain=true;
            break;
          }
        }
      }
      
      if (foundchain)
        safepositions->set(p,group->getColor());
    }
  }
  
  for(std::list<Benson::Region*>::iterator iter=blackregions.begin();iter!=blackregions.end();++iter) 
  {
    if ((*iter)->isvital && !(*iter)->hasstones)
    {
      for(std::list<int>::iterator iter2=(*iter)->positions.begin();iter2!=(*iter)->positions.end();++iter2) 
      {
        safepositions->set((*iter2),Go::BLACK);
      }
    }
  }
  
  for(std::list<Benson::Region*>::iterator iter=whiteregions.begin();iter!=whiteregions.end();++iter) 
  {
    if ((*iter)->isvital && !(*iter)->hasstones)
    {
      for(std::list<int>::iterator iter2=(*iter)->positions.begin();iter2!=(*iter)->positions.end();++iter2) 
      {
        if (safepositions->get((*iter2))==Go::EMPTY)
          safepositions->set((*iter2),Go::WHITE);
        else
          safepositions->set((*iter2),Go::EMPTY);
      }
    }
  }
}

void Benson::updateChainsAndRegions()
{
  Go::BitBoard *usedflags=new Go::BitBoard(size);
  
  std::set<Go::Group*> *allgroups=board->getGroups();
  for(std::set<Go::Group*>::iterator iter=allgroups->begin();iter!=allgroups->end();++iter) 
  {
    Benson::Chain *chain=new Benson::Chain();
    chain->group=(*iter);
    chain->col=(*iter)->getColor();
    chain->vitalregions=0;
    if (chain->col==Go::BLACK)
      blackchains.push_back(chain);
    else
      whitechains.push_back(chain);
  }
  
  usedflags->clear();
  for (int p=0;p<board->getPositionMax();p++)
  {
    if (!usedflags->get(p) && board->getColor(p)!=Go::BLACK && board->getColor(p)!=Go::OFFBOARD)
    {
      Benson::Region *region=new Benson::Region();
      region->col=Go::BLACK;
      region->isvital=false;
      region->hasstones=false;
      this->spreadRegion(region,p,usedflags);
      blackregions.push_back(region);
    }
  }
  
  usedflags->clear();
  for (int p=0;p<board->getPositionMax();p++)
  {
    if (!usedflags->get(p) && board->getColor(p)!=Go::WHITE && board->getColor(p)!=Go::OFFBOARD)
    {
      Benson::Region *region=new Benson::Region();
      region->col=Go::WHITE;
      region->isvital=false;
      region->hasstones=false;
      this->spreadRegion(region,p,usedflags);
      whiteregions.push_back(region);
    }
  }
  
  delete usedflags;
}

void Benson::spreadRegion(Benson::Region *region, int pos, Go::BitBoard *usedflags)
{
  if (usedflags->get(pos))
    return;
  
  usedflags->set(pos);
  region->positions.push_back(pos);
  if (board->getColor(pos)!=Go::EMPTY)
    region->hasstones=true;
  
  foreach_adjacent(pos,p,{
    if (board->getColor(p)==region->col)
      region->adjpositions.push_back(p);
    else if (board->getColor(p)!=Go::OFFBOARD)
      this->spreadRegion(region,p,usedflags);
  });
}

bool Benson::isVitalTo(Benson::Region *region, Benson::Chain *chain) const
{
  if (region->col!=chain->col)
    return false;
  
  for(std::list<int>::iterator iter=region->positions.begin();iter!=region->positions.end();++iter) 
  {
    if (board->getColor((*iter))==Go::EMPTY)
    {
      bool foundadj=false;
      foreach_adjacent((*iter),p,{
        if (board->inGroup(p) && board->getGroup(p)==chain->group)
          foundadj=true;
      });
      if (!foundadj)
        return false;
    }
  }
  
  region->isvital=true;
  
  return true;
}

void Benson::updateVitalRegions()
{
  for(std::list<Benson::Region*>::iterator iter=blackregions.begin();iter!=blackregions.end();++iter) 
  {
    (*iter)->isvital=false;
    for(std::list<Benson::Chain*>::iterator iter2=blackchains.begin();iter2!=blackchains.end();++iter2) 
    {
      if (this->isVitalTo((*iter),(*iter2)))
      {
        (*iter2)->vitalregions++;
        //fprintf(stderr,"vital++: %d %s\n",(*iter2)->vitalregions,Go::Position::pos2string((*iter2)->group->getPosition(),size).c_str());
      }
    }
  }
  
  for(std::list<Benson::Region*>::iterator iter=whiteregions.begin();iter!=whiteregions.end();++iter) 
  {
    (*iter)->isvital=false;
    for(std::list<Benson::Chain*>::iterator iter2=whitechains.begin();iter2!=whitechains.end();++iter2) 
    {
      if (this->isVitalTo((*iter),(*iter2)))
      {
        (*iter2)->vitalregions++;
        //fprintf(stderr,"vital++: %d %s\n",(*iter2)->vitalregions,Go::Position::pos2string((*iter2)->group->getPosition(),size).c_str());
      }
    }
  }
}

bool Benson::removeNonVitalChains()
{
  bool removed=false;
  
  for(std::list<Benson::Chain*>::iterator iter=blackchains.begin();iter!=blackchains.end();++iter) 
  {
    if ((*iter)->vitalregions<2)
    {
      std::list<Benson::Chain*>::iterator tmpiter=iter;
      --iter;
      delete (*tmpiter);
      blackchains.erase(tmpiter);
      removed=true;
    }
    else
      (*iter)->vitalregions=0;
  }
  
  for(std::list<Benson::Chain*>::iterator iter=whitechains.begin();iter!=whitechains.end();++iter) 
  {
    if ((*iter)->vitalregions<2)
    {
      std::list<Benson::Chain*>::iterator tmpiter=iter;
      --iter;
      delete (*tmpiter);
      whitechains.erase(tmpiter);
      removed=true;
    }
    else
      (*iter)->vitalregions=0;
  }
  
  return removed;
}

bool Benson::removeNonSurroundedRegions()
{
  bool removed=false;
  
  for(std::list<Benson::Region*>::iterator iter=blackregions.begin();iter!=blackregions.end();++iter) 
  {
    bool foundmissing=false;
    for(std::list<int>::iterator iter2=(*iter)->adjpositions.begin();iter2!=(*iter)->adjpositions.end();++iter2) 
    {
      if (!board->inGroup((*iter2)))
      {
        foundmissing=true;
        break;
      }
      else
      {
        Go::Group *group=board->getGroup((*iter2));
        bool foundgroup=false;
        
        for(std::list<Benson::Chain*>::iterator iter3=blackchains.begin();iter3!=blackchains.end();++iter3) 
        {
          if ((*iter3)->group==group)
          {
            foundgroup=true;
            break;
          }
        }
        
        if (!foundgroup)
        {
          foundmissing=true;
          break;
        }
      }
    }
    if (foundmissing)
    {
      std::list<Benson::Region*>::iterator tmpiter=iter;
      --iter;
      delete (*tmpiter);
      blackregions.erase(tmpiter);
      removed=true;
    }
  }
  
  for(std::list<Benson::Region*>::iterator iter=whiteregions.begin();iter!=whiteregions.end();++iter) 
  {
    bool foundmissing=false;
    for(std::list<int>::iterator iter2=(*iter)->adjpositions.begin();iter2!=(*iter)->adjpositions.end();++iter2) 
    {
      if (!board->inGroup((*iter2)))
      {
        foundmissing=true;
        break;
      }
      else
      {
        Go::Group *group=board->getGroup((*iter2));
        bool foundgroup=false;
        
        for(std::list<Benson::Chain*>::iterator iter3=whitechains.begin();iter3!=whitechains.end();++iter3) 
        {
          if ((*iter3)->group==group)
          {
            foundgroup=true;
            break;
          }
        }
        
        if (!foundgroup)
        {
          foundmissing=true;
          break;
        }
      }
    }
    if (foundmissing)
    {
      std::list<Benson::Region*>::iterator tmpiter=iter;
      --iter;
      delete (*tmpiter);
      whiteregions.erase(tmpiter);
      removed=true;
    }
  }
  
  return removed;
}

