#ifndef DEF_OAKFOAM_RANDOM_H
#define DEF_OAKFOAM_RANDOM_H

/** Pseudo Random Number Generator.
 * Uses of a Park-Miller PRNG.
 */
class Random
{
  public:
    /** Create a new Random object with specific seeds. */
    Random(unsigned long s=0, int threadid=0);
    /** Return the current seed. */
    unsigned long getSeed() const { return seed; };
    
    /** Generate a random integer. */
    inline unsigned long getRandomInt()
      {
        //Park-Miller "Minimal Standard" PRNG
        
        unsigned long hi, lo;
        
        lo= 16807 * (seed & 0xffff);
        hi= 16807 * (seed >> 16);
        
        lo+= (hi & 0x7fff) << 16;
        lo+= hi >> 15;
        
        if (lo >= 0x7FFFFFFF) lo-=0x7FFFFFFF;

        return (seed=lo);
      };
    /** Generate a random integer in a range. */
    inline unsigned long getRandomInt(unsigned long max)
      {
        return this->getRandomInt() % max; //XXX: not uniform, but good enough
      };
    /** Generate with distribution function. */
    unsigned long getRandomInt(unsigned long max, float a);
    /** Generate a random float in the range (0,1). */
    float getRandomReal();
    
    /** Create a new seed. */
    static unsigned long makeSeed(int threadid=0);
  
  private:
    unsigned long seed;
};

#endif
