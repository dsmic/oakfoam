#include "Random.h"

#include <cmath>
#include <boost/timer.hpp>
#include <boost/random/linear_congruential.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/random/variate_generator.hpp>

Random::Random(long unsigned int s)
{
  seed=s;
}

long unsigned int Random::getRandomInt()
{
  //Park-Miller "Minimal Standard" PRNG
  
  long unsigned int hi, lo;
  
  lo= 16807 * (seed & 0xffff);
  hi= 16807 * (seed >> 16);
  
  lo+= (hi & 0x7fff) << 16;
  lo+= hi >> 15;
  
  if (lo >= 0x7FFFFFFF) lo-=0x7FFFFFFF;
  
  return (seed=lo);
}

long unsigned int Random::getRandomInt(long unsigned int max)
{
  return this->getRandomInt() % max; //XXX: not uniform, but good enough
}

float Random::getRandomReal()
{
  return (float)this->getRandomInt() / ((long unsigned int)(1) << 31);
}

