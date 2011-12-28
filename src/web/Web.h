#ifndef DEF_OAKFOAM_WEB_H
#define DEF_OAKFOAM_WEB_H

#include "config.h"
#ifdef HAVE_WEB

//from "../engine/Engine.h":
class Engine;

/** Web interface.
 * Alternative interface for engine.
 */
class Web
{
  public:
    /** Create a new Web object on a port. */
    Web(Engine *eng, int p);

    /** Return the engine. */
    Engine *getEngine() const { return engine; };
    /** Return the port. */
    int getPort() const { return port; };

    /** Run the web interface handler. */
    void run();
    
  private:
    Engine *engine;
    int port;
};

#endif
#endif

