#ifndef DEF_OAKFOAM_OAKFOAM_H
#define DEF_OAKFOAM_OAKFOAM_H

#include <iostream>
#include <string>

#include "gtp/Gtp.h"

#include "engine/Engine.h"

class Oakfoam
{
  public:
    Oakfoam();
    ~Oakfoam();
    
    void run();
    
    Gtp::Engine *gtpe;
    Engine *engine;
    bool book_autoload;
};

#endif
