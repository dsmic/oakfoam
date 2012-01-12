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
    
    /** Run the engine. */
    void run();
    
    /** The GTP engine to use. */
    Gtp::Engine *gtpe;
    /** The core engine. */
    Engine *engine;

    /** Should we try and automatically load an opening book? */
    bool book_autoload;

    /** Should the web interface be used? (versus the GTP interface) */
    bool web_interface;
    /** Web interface bind address. */
    std::string web_address;
    /** Web interface port number. */
    int web_port;
};

#endif
