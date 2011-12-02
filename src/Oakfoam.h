#ifndef DEF_OAKFOAM_OAKFOAM_H
#define DEF_OAKFOAM_OAKFOAM_H

#include <iostream>
#include <string>

#include "gtp/Gtp.h"

#include "engine/Engine.h"

/** The Oakfoam player.
 * Combines the Oakfoam engine with a GTP engine that is
 * connected with standard I/O.
 */
class Oakfoam
{
  public:
    Oakfoam();
    ~Oakfoam();
    
    void run();
    
    Gtp::Engine *gtpe;
    Engine *engine;

    /** Should we try and automatically load an opening book? */
    bool book_autoload;
};

#endif
