#ifndef DEF_OAKFOAM_PARAMETERS_H
#define DEF_OAKFOAM_PARAMETERS_H

#define UCB_C 0.44
#define UCB_INIT 1.2

#define RAVE_MOVES 3000

#define OUTPUTSGF_MAXCHILDREN 10

#include <string>

class Parameters
{
  public:
    Parameters();
    
    int board_size;
    
    float ucb_c;
    float ucb_init;
    
    int rave_moves;
    
    int outputsgf_maxchildren;
    
    class Manager
    {
      //todo
    };
};

#endif
