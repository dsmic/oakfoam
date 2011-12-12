#ifndef DEF_OAKFOAM_BENSON_H
#define DEF_OAKFOAM_BENSON_H

#include "Go.h"
#include <list>

/** Benson Algorithm.
 * Check a board for pass-safe stones.
 */
class Benson
{
public:
    /** Create a Benson object. */
    Benson(Go::Board *bd);
    ~Benson();
    
    /** Perform the Benson algorithm on the board. */
    void solve();
    /** Return the board positions that are safe for each color. */
    Go::ObjectBoard<Go::Color> *getSafePositions() const { return safepositions; };
  
  private:
    Go::Board *const board;
    const int size;
    Go::ObjectBoard<Go::Color> *const safepositions;
    
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
    bool isVitalTo(Benson::Region *region, Benson::Chain *chain) const;
    void updateVitalRegions();
    bool removeNonVitalChains();
    bool removeNonSurroundedRegions();
};

#endif
