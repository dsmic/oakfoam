#include "Oakfoam.h"

void cmdTest(Gtp::Engine* gtpe, Gtp::Arguments* args)
{
  std::cout << "= test complete\n";
}

Oakfoam::Oakfoam()
{
  gtpe=new Gtp::Engine();
  engine=new Engine();
  
  engine->init();
  this->addGtpCommands();
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

void Oakfoam::addGtpCommands() //TODO: move into engine
{
  gtpe->addConstantCommand("protocol_version","2");
  gtpe->addConstantCommand("name",PACKAGE_NAME);
  gtpe->addConstantCommand("version",PACKAGE_VERSION);
  
  gtpe->addFunctionCommand("test",&cmdTest);
}

