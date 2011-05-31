#ifndef DEF_OAKFOAM_RANDOM_H
#define DEF_OAKFOAM_RANDOM_H

class Random
{
  public:
    Random() {};
    Random(unsigned long s);
    
    unsigned long getRandomInt();
    unsigned long getRandomInt(unsigned long max);
    float getRandomReal();
  
  private:
    unsigned long seed;
};

#endif
