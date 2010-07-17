#ifndef DEF_OAKFOAM_RANDOM_H
#define DEF_OAKFOAM_RANDOM_H

#include <cmath>
#include <boost/timer.hpp>
#include <boost/random/linear_congruential.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/random/variate_generator.hpp>

class Random
{
  public:
    Random() {};
    Random(long unsigned int s);
    
    long unsigned int getRandomInt();
    float getRandomReal();
  
  private:
    long unsigned int seed;
};

#endif
