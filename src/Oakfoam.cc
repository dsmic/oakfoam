#include "Oakfoam.h"

Oakfoam::Oakfoam()
{
  gtpe=new Gtp::Engine();
  engine=new Engine(gtpe);
  
  gtpe->addConstantCommand("name",PACKAGE_NAME);
  gtpe->addConstantCommand("version",PACKAGE_VERSION);
}

Oakfoam::~Oakfoam()
{
  delete engine;
  delete gtpe;
}

void Oakfoam::run()
{
  gtpe->run();
}

