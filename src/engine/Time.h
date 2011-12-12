#ifndef DEF_OAKFOAM_TIME_H
#define DEF_OAKFOAM_TIME_H

#include "Go.h"
//from "Parameters.h":
class Parameters;

/** Time Management.
 * Utility class for time management.
 */
class Time
{
  public:
    /** Create a new Time object with Canadian overtime. */
    Time(Parameters *prms, float main, float overtime, int stones);
    /** Create a new Time object with absolute time.
     * Setting @p main to 0, means there is actually no time limits.
     */
    Time(Parameters *prms, float main);
    //~Time();
    
    /** Are there no time limits? */
    bool isNoTiming() const { return (base_main==0 && (base_overtime==0 || base_stones==0)); };
    /** Are there absolute time limits? */
    bool isAbsoluteTiming() const { return (!this->isNoTiming() && (base_overtime==0 || base_stones==0)); };
    /** Are there Canadian overtime settings? */
    bool isCanadianOvertime() const { return (!this->isNoTiming() && base_overtime!=0 && base_stones!=0); };
    
    /** Currently in overtime? */
    bool inOvertime(Go::Color col) const { return (*(this->stonesLeftForColor(col))>0); }
    /** How much time is remaining? */
    float timeLeft(Go::Color col) const { return *(this->timeLeftForColor(col)); };
    /** How many stones are left for this period?
     * This is only relevant if Canadian overtime is used.
     */
    int stonesLeft(Go::Color col) const { return *(this->stonesLeftForColor(col)); };
    
    /** Reset the timing. */
    void resetAll() { this->setupTimeForColors(); };
    /** Update the time limit for a specific player. */
    void updateTimeLeft(Go::Color col, float time, int stones);
    /** Use up a portion of time for a player. */
    void useTime(Go::Color col, float timeused);
    
    /** Determine how much time a player should use for a turn. */
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
