#ifndef DEF_OAKFOAM_PLAYOUT_H
#define DEF_OAKFOAM_PLAYOUT_H

#include "Go.h"
//from "Parameters.h":
class Parameters;
#include "../gtp/Gtp.h"

class Playout
{
  public:
    Playout(Parameters *prms);
    ~Playout();
    
    void doPlayout(Go::Board *board, float &finalscore, std::list<Go::Move> &playoutmoves, Go::Color colfirst, Go::BitBoard *firstlist, Go::BitBoard *secondlist);
    void getPlayoutMove(Go::Board *board, Go::Color col, Go::Move &move);
    
    void resetLGRF();
  
  private:
    Parameters *params;
    Gtp::Engine *gtpe;
    
    int *lgrf1,*lgrf2;
    int lgrfpositionmax;
    
    void getPlayoutMove(Go::Board *board, Go::Color col, Go::Move &move, int *posarray);
    
    int getLGRF1(Go::Color col, int pos1);
    int getLGRF2(Go::Color col, int pos1, int pos2);
    void setLGRF1(Go::Color col, int pos1, int val);
    void setLGRF2(Go::Color col, int pos1, int pos2, int val);
    bool hasLGRF1(Go::Color col, int pos1);
    bool hasLGRF2(Go::Color col, int pos1, int pos2);
    void clearLGRF1(Go::Color col, int pos1);
    void clearLGRF2(Go::Color col, int pos1, int pos2);
};
#endif
