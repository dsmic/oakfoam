#ifndef DEF_OAKFOAM_RANDOM_H
#define DEF_OAKFOAM_RANDOM_H

class Random
{
  public:
    Random(unsigned long s=0);
    unsigned long getSeed() const { return seed; };
    
    unsigned long getRandomInt();
    unsigned long getRandomInt(unsigned long max);
    float getRandomReal();
  
  private:
    unsigned long seed;
};

#endif
