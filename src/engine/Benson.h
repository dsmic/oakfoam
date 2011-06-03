#ifndef DEF_OAKFOAM_BENSON_H
#define DEF_OAKFOAM_BENSON_H

#include "Go.h"
#include <list>

class Benson
{
  public:
    Benson(Go::Board *bd);
    ~Benson();
    
    void solve();
    Go::ObjectBoard<Go::Color> *getSafePositions() { return safepositions; };
  
  private:
    Go::Board *board;
    int size;
    Go::ObjectBoard<Go::Color> *safepositions;
    
    struct Chain
    {
      Go::Color col;
      Go::Group *group;
      int vitalregions;
    };
    
    struct Region
    {
      Go::Color col;
      std::list<int> positions;
      std::list<int> adjpositions;
      bool isvital;
      bool hasstones;
    };
    
    std::list<Benson::Chain*> blackchains,whitechains;
    std::list<Benson::Region*> blackregions,whiteregions;
    
    void updateChainsAndRegions();
    void spreadRegion(Benson::Region *region, int pos, Go::BitBoard *usedflags);
    bool isVitalTo(Benson::Region *region, Benson::Chain *chain);
    void updateVitalRegions();
    bool removeNonVitalChains();
    bool removeNonSurroundedRegions();
};

#endif
