#ifndef DEF_OAKFOAM_TIME_H
#define DEF_OAKFOAM_TIME_H

#include "Go.h"
//from "Parameters.h":
class Parameters;

class Time
{
  public:
    Time(Parameters *prms, float main, float overtime, int stones);
    Time(Parameters *prms, float main);
    //~Time();
    
    bool isNoTiming() const { return (base_main==0 && (base_overtime==0 || base_stones==0)); };
    bool isAbsoluteTiming() const { return (!this->isNoTiming() && (base_overtime==0 || base_stones==0)); };
    bool isCanadianOvertime() const { return (!this->isNoTiming() && base_overtime!=0 && base_stones!=0); };
    
    bool inOvertime(Go::Color col) const { return (*(this->stonesLeftForColor(col))>0); }
    float timeLeft(Go::Color col) const { return *(this->timeLeftForColor(col)); };
    int stonesLeft(Go::Color col) const { return *(this->stonesLeftForColor(col)); };
    
    void resetAll() { this->setupTimeForColors(); };
    void updateTimeLeft(Go::Color col, float time, int stones);
    void useTime(Go::Color col, float timeused);
    
    float getAllocatedTimeForNextTurn(Go::Color col) const;
    
  private:
    Parameters *const params;
    const float base_main,base_overtime;
    const int base_stones;
    
    float black_time_left,white_time_left;
    int black_stones_left,white_stones_left;
    
    void setupTimeForColors();
    float *timeLeftForColor(Go::Color col) const;
    int *stonesLeftForColor(Go::Color col) const;
};

#endif
