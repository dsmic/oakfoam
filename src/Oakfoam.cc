#include "Oakfoam.h"

void Oakfoam::run()
{
  //std::cout << PACKAGE_STRING << "\n";
  
  Gtp gtp;
  Engine engine;
  
  engine.init();
  
  gtp.setEngine(engine);
  gtp.run();
}
