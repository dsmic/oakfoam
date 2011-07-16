#include "Time.h"

#include "Parameters.h"

Time::Time(Parameters *prms, float main, float overtime, int stones)
  : params(prms),
    base_main(main),
    base_overtime(overtime),
    base_stones(stones)
{
  this->setupTimeForColors();
}

Time::Time(Parameters *prms, float main)
  : params(prms),
    base_main(main),
    base_overtime(0),
    base_stones(0)
{
  this->setupTimeForColors();
}

void Time::setupTimeForColors()
{
  if (base_main>0)
  {
    black_time_left=base_main;
    white_time_left=base_main;
    black_stones_left=0;
    white_stones_left=0;
  }
  else if (this->isCanadianOvertime())
  {
    black_time_left=base_overtime;
    white_time_left=base_overtime;
    black_stones_left=base_stones;
    white_stones_left=base_stones;
  }
  else
  {
    black_time_left=0;
    white_time_left=0;
    black_stones_left=0;
    white_stones_left=0;
  }
}

float *Time::timeLeftForColor(Go::Color col) const
{
  if (col==Go::BLACK)
    return (float *)&black_time_left;
  else
    return (float *)&white_time_left;
}

int *Time::stonesLeftForColor(Go::Color col) const
{
  if (col==Go::BLACK)
    return (int *)&black_stones_left;
  else
    return (int *)&white_stones_left;
}

void Time::useTime(Go::Color col, float timeused)
{
  if (timeused>0 && !this->isNoTiming())
  {
    float *timeleft=this->timeLeftForColor(col);
    *timeleft-=timeused;
    if (*timeleft<0) // time finished or starting overtime
    {
      if (this->isAbsoluteTiming() || this->inOvertime(col)) // time run out
        *timeleft=1;
      else // entering overtime
      {
        *timeleft+=base_overtime;
        *(this->stonesLeftForColor(col))=base_stones;
      }
    }
    else if (this->inOvertime(col))
    {
      (*(this->stonesLeftForColor(col)))--;
      if (*(this->stonesLeftForColor(col))==0)
      {
        *timeleft=base_overtime;
        *(this->stonesLeftForColor(col))=base_stones;
      }
    }
  }
}

void Time::updateTimeLeft(Go::Color col, float time, int stones)
{
  *(this->timeLeftForColor(col))=time;
  *(this->stonesLeftForColor(col))=stones;
}

float Time::getAllocatedTimeForNextTurn(Go::Color col) const
{
  if (this->isNoTiming())
    return 0;
  else
  {
    if (this->inOvertime(col))
    {
      float time_per_move=((this->timeLeft(col)-params->time_buffer)/this->stonesLeft(col));
      if (time_per_move<params->time_move_minimum)
        time_per_move=params->time_move_minimum;
      return time_per_move;
    }
    else
    {
      float time_left=this->timeLeft(col);
      time_left-=params->time_buffer;
      float time_per_move=time_left/params->time_k; //allow much more time in beginning
      if (time_per_move<params->time_move_minimum)
        time_per_move=params->time_move_minimum;
      return time_per_move;
    }
  }
}

