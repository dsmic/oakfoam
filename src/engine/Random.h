#ifndef DEF_OAKFOAM_RANDOM_H
#define DEF_OAKFOAM_RANDOM_H

class Random
{
  public:
    Random() {};
    Random(long unsigned int s);
    
    long unsigned int getRandomInt();
    long unsigned int getRandomInt(long unsigned int max);
    float getRandomReal();
  
  private:
    long unsigned int seed;
};

#endif
