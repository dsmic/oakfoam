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

/** Playouts. */
class Playout
{
  public:
    /** Create a playout instance with given parameters. */
    Playout(Parameters *prms);
    ~Playout();
    
    /** Perform a playout.
     * @param[in] settings      Settings of this worker thread.
     * @param[in] board         Board to perform the playout on.
     * @param[out] finalscore   Final score of the playout.
     * @param[in] playouttree   Tree leaf node that the playout is started from.
     * @param[in] playoutmoves  List of moves leading up to the start of the playout.
     * @param[in] colfirst      Color to move first.
     * @param[out] firstlist    List of location where the first color played.
     * @param[out] secondlist   List of location where the other color played.
     */
    void doPlayout(Worker::Settings *settings, Go::Board *board, float &finalscore, Tree *playouttree, std::list<Go::Move> &playoutmoves, Go::Color colfirst, Go::BitBoard *firstlist, Go::BitBoard *secondlist, std::list<std::string> *movereasons=NULL);
    /** Get a playout move for a given situation. */
    void getPlayoutMove(Worker::Settings *settings, Go::Board *board, Go::Color col, Go::Move &move, std::string *reason=NULL);
    /** Check for a useless move accoridng to the Crazy Stone heuristic.
     * @todo Consider incorporating this into getPlayoutMove()
     */
    void checkUselessMove(Worker::Settings *settings, Go::Board *board, Go::Color col, Go::Move &move, std::string *reason=NULL);
    
    /** Reset LGRF values. */
    void resetLGRF();
  
  private:
    Parameters *const params;
    Gtp::Engine *gtpe;
    
    int *lgrf1,*lgrf2;
    int lgrfpositionmax;
    
    void getPlayoutMove(Worker::Settings *settings, Go::Board *board, Go::Color col, Go::Move &move, int *posarray, std::string *reason=NULL);
    void checkUselessMove(Worker::Settings *settings, Go::Board *board, Go::Color col, Go::Move &move, int *posarray, std::string *reason=NULL);
    void getLGRF2Move(Go::Board *board, Go::Color col, Go::Move &move);
    void getLGRF1Move(Go::Board *board, Go::Color col, Go::Move &move);
    void getFeatureMove(Worker::Settings *settings, Go::Board *board, Go::Color col, Go::Move &move);
    void getAnyCaptureMove(Worker::Settings *settings, Go::Board *board, Go::Color col, Go::Move &move, int *posarray);
    void getPatternMove(Worker::Settings *settings, Go::Board *board, Go::Color col, Go::Move &move, int *posarray);
    void getFillBoardMove(Worker::Settings *settings, Go::Board *board, Go::Color col, Go::Move &move);
    void getNakadeMove(Worker::Settings *settings, Go::Board *board, Go::Color col, Go::Move &move, int *posarray);
    void getLast2LibAtariMove(Worker::Settings *settings, Go::Board *board, Go::Color col, Go::Move &move, int *posarray);
    void getLastCaptureMove(Worker::Settings *settings, Go::Board *board, Go::Color col, Go::Move &move, int *posarray);
    void getLastAtariMove(Worker::Settings *settings, Go::Board *board, Go::Color col, Go::Move &move, int *posarray);
    void getAtariMove(Worker::Settings *settings, Go::Board *board, Go::Color col, Go::Move &move, int *posarray);

    void checkEyeMove(Worker::Settings *settings, Go::Board *board, Go::Color col, Go::Move &move, int *posarray, Go::Move &replacemove);

    bool isBadMove(Go::Board *board, Go::Color col, int pos);
    int getTwoLibertyMoveLevel(Go::Board *board, Go::Move move, Go::Group *group);
    
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
