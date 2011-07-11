#include "Random.h"

#include <cmath>
#include <cstdlib>
#include <boost/timer.hpp>
#include <boost/random/linear_congruential.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/random/variate_generator.hpp>

Random::Random(unsigned long s)
{
  if (s==0)
    seed=((unsigned long)std::time(0))^((unsigned long)getpid());
  else
    seed=s;
}

unsigned long Random::getRandomInt()
{
  //Park-Miller "Minimal Standard" PRNG
  
  unsigned long hi, lo;
  
  lo= 16807 * (seed & 0xffff);
  hi= 16807 * (seed >> 16);
  
  lo+= (hi & 0x7fff) << 16;
  lo+= hi >> 15;
  
  if (lo >= 0x7FFFFFFF) lo-=0x7FFFFFFF;
  
  return (seed=lo);
}

unsigned long Random::getRandomInt(unsigned long max)
{
  return this->getRandomInt() % max; //XXX: not uniform, but good enough
}

float Random::getRandomReal()
{
  return (float)this->getRandomInt() / ((unsigned long)(1) << 31);
}

