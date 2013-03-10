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
     * @param[out] movereasons  List of reasons for making moves.
     */
    void doPlayout(Worker::Settings *settings, Go::Board *board, float &finalscore, Tree *playouttree, std::list<Go::Move> &playoutmoves, Go::Color colfirst, Go::BitBoard *firstlist, Go::BitBoard *secondlist, Go::BitBoard *earlyfirstlist, Go::BitBoard *earlysecondlist, std::list<std::string> *movereasons=NULL);
    /** Get a playout move for a given situation. */
    void getPlayoutMove(Worker::Settings *settings, Go::Board *board, Go::Color col, Go::Move &move, float critarray[], int passes=0, std::vector<int> *pool=NULL, std::vector<int> *poolcrit=NULL, std::string *reason=NULL,float *trylocal_p=NULL);
    /** Check for a useless move according to the Crazy Stone heuristic.
     * @todo Consider incorporating this into getPlayoutMove()
     */
    void checkUselessMove(Worker::Settings *settings, Go::Board *board, Go::Color col, Go::Move &move, std::string *reason=NULL);
    
    /** Reset LGRF values. */
    void resetLGRF();
  
  private:
    Parameters *const params;
    Gtp::Engine *gtpe;

    int *lgrf1,*lgrf1o,*lgrf2;
    bool *lbm; //last bad move
    unsigned char *lgrf1n;
    
    unsigned int *lgrf1hash,*lgrf1hash2,*lgrf2hash;
    int *lgrf1count,*lgrf2count;
    
    unsigned int *lgpf; // last good play with forgetting
    unsigned long int *lgpf_b;
    
    bool *badpassanswer;
    
    int lgrfpositionmax;
    
    void getPlayoutMove(Worker::Settings *settings, Go::Board *board, Go::Color col, Go::Move &move, int *posarray, float critarray[], int passes=0, std::vector<int> *pool=NULL, std::vector<int> *poolcrit=NULL, std::string *reason=NULL,float *trylocal_p=NULL);
    void checkUselessMove(Worker::Settings *settings, Go::Board *board, Go::Color col, Go::Move &move, int *posarray, std::string *reason=NULL);
    void getPoolRAVEMove(Worker::Settings *settings, Go::Board *board, Go::Color col, Go::Move &move, std::vector<int> *pool=NULL);
    void getLGRF2Move(Worker::Settings *settings, Go::Board *board, Go::Color col, Go::Move &move);
    void getLGRF1Move(Worker::Settings *settings, Go::Board *board, Go::Color col, Go::Move &move);
    void getLGRF1oMove(Worker::Settings *settings, Go::Board *board, Go::Color col, Go::Move &move);
    void getLGPFMove(Worker::Settings *settings, Go::Board *board, Go::Color col, Go::Move &move, int *posarray);
    void getFeatureMove(Worker::Settings *settings, Go::Board *board, Go::Color col, Go::Move &move);
    void getAnyCaptureMove(Worker::Settings *settings, Go::Board *board, Go::Color col, Go::Move &move, int *posarray);
    void getPatternMove(Worker::Settings *settings, Go::Board *board, Go::Color col, Go::Move &move, int *posarray, int passes);
    void getFillBoardMove(Worker::Settings *settings, Go::Board *board, Go::Color col, Go::Move &move, int *posarray, int passes, std::string *reason);
    void getFillBoardMoveBestPattern(Worker::Settings *settings, Go::Board *board, Go::Color col, Go::Move &move, int *posarray, int passes, std::string *reason);
    void getNakadeMove(Worker::Settings *settings, Go::Board *board, Go::Color col, Go::Move &move, int *posarray);
    void getLast2LibAtariMove(Worker::Settings *settings, Go::Board *board, Go::Color col, Go::Move &move, int *posarray);
    void getLastCaptureMove(Worker::Settings *settings, Go::Board *board, Go::Color col, Go::Move &move, int *posarray);
    void getLastAtariMove(Worker::Settings *settings, Go::Board *board, Go::Color col, Go::Move &move, int *posarray);
    void getAtariMove(Worker::Settings *settings, Go::Board *board, Go::Color col, Go::Move &move, int *posarray);
    void getNearbyMove(Worker::Settings *settings, Go::Board *board, Go::Color col, Go::Move &move);

    void checkEyeMove(Worker::Settings *settings, Go::Board *board, Go::Color col, Go::Move &move, int *posarray, Go::Move &replacemove);
    void checkAntiEyeMove(Worker::Settings *settings, Go::Board *board, Go::Color col, Go::Move &move, int *posarray, Go::Move &replacemove);

    bool isBadMove(Worker::Settings *settings, Go::Board *board, Go::Color col, int pos, float p=0.0, float p2=0.0, int passes=0);
    bool isEyeFillMove(Go::Board *board, Go::Color col, int pos);
    int getTwoLibertyMoveLevel(Go::Board *board, Go::Move move, Go::Group *group);
    
    int getLGRF1(Go::Color col, int pos1) const;
    unsigned int getLGRF1hash(Go::Color col, int pos1) const;
    unsigned int getLGRF1hash2(Go::Color col, int pos1) const;
    //int getLGRF1n_l(Go::Color col, int pos1) const;
    int getLGRF1o(Go::Color col, int pos1) const;
    int getLGRF2(Go::Color col, int pos1, int pos2) const;
    unsigned int getLGRF2hash(Go::Color col, int pos1, int pos2) const;
    void setLGRF1(Go::Color col, int pos1, int val);
    void setLGRF1(Go::Color col, int pos1, int val, unsigned int hash, unsigned int hash2);
    void setLGRF1n(Go::Color col, int pos1, int val);
    void setLGRF1o(Go::Color col, int pos1, int val);
    void setLGRF2(Go::Color col, int pos1, int pos2, int val);
    void setLGRF2(Go::Color col, int pos1, int pos2, int val, unsigned int hash);
    void setLGPF(Worker::Settings *settings, Go::Color col, int pos1, unsigned int val);
    void setLGPF(Worker::Settings *settings, Go::Color col, int pos1, unsigned int val, unsigned long int val_b);
    bool hasLGRF1(Go::Color col, int pos1) const;
    bool hasLGRF1n(Go::Color col, int pos1, int pos) const;
    bool hasLBM(Go::Color col, int val) const;
    bool hasLGRF1o(Go::Color col, int pos1) const;
    bool hasLGRF2(Go::Color col, int pos1, int pos2) const;
    bool hasLGPF(Go::Color col, int pos1, unsigned int hash) const;
    bool hasLGPF(Go::Color col, int pos1, unsigned int hash, unsigned long int hash_b) const;
    void clearLGRF1(Go::Color col, int pos1);
    //void clearLGRF1n(Go::Color col, int pos1);
    void clearLGRF1n(Go::Color col, int pos1, int val);
    void clearLGRF1o(Go::Color col, int pos1);
    void clearLGRF2(Go::Color col, int pos1, int pos2);
    void clearLGPF(Go::Color col, int pos1);
    void clearLGPF(Go::Color col, int pos1, unsigned int hash);
    void clearLGPF(Go::Color col, int pos1, unsigned int hash, unsigned long int hash_b);
    
    void clearBadPassAnswer(Go::Color col, int pos1);
    void setBadPassAnswer(Go::Color col, int pos1);
    bool isBadPassAnswer(Go::Color col, int pos1);
    
    void replaceWithApproachMove(Worker::Settings *settings, Go::Board *board, Go::Color col, int &pos);
};
#endif
