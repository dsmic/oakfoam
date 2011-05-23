#ifndef DEF_OAKFOAM_OAKFOAM_H
#define DEF_OAKFOAM_OAKFOAM_H

#include <config.h>
#include <build.h>

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
};

#endif
