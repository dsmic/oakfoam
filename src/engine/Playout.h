#ifndef DEF_OAKFOAM_PLAYOUT_H
#define DEF_OAKFOAM_PLAYOUT_H

#include "Go.h"
//from "Parameters.h":
class Parameters;
//from "Tree.h":
class Tree;
//from "Worker.h":
namespace Worker
{
  class Settings;
};
#include "../gtp/Gtp.h"

class Playout
{
  public:
    Playout(Parameters *prms);
    ~Playout();
    
    void doPlayout(Worker::Settings *settings, Go::Board *board, float &finalscore, Tree *playouttree, std::list<Go::Move> &playoutmoves, Go::Color colfirst, Go::BitBoard *firstlist, Go::BitBoard *secondlist);
    void getPlayoutMove(Worker::Settings *settings, Go::Board *board, Go::Color col, Go::Move &move);
    
    void resetLGRF();
  
  private:
    Parameters *const params;
    Gtp::Engine *gtpe;
    
    int *lgrf1,*lgrf2;
    int lgrfpositionmax;
    
    void getPlayoutMove(Worker::Settings *settings, Go::Board *board, Go::Color col, Go::Move &move, int *posarray);
    
    int getLGRF1(Go::Color col, int pos1) const;
    int getLGRF2(Go::Color col, int pos1, int pos2) const;
    void setLGRF1(Go::Color col, int pos1, int val);
    void setLGRF2(Go::Color col, int pos1, int pos2, int val);
    bool hasLGRF1(Go::Color col, int pos1) const;
    bool hasLGRF2(Go::Color col, int pos1, int pos2) const;
    void clearLGRF1(Go::Color col, int pos1);
    void clearLGRF2(Go::Color col, int pos1, int pos2);
};
#endif
