#include "Oakfoam.h"

#include <config.h>
#include <build.h>

Oakfoam::Oakfoam()
{
  gtpe=new Gtp::Engine();
  engine=new Engine(gtpe,PACKAGE_NAME " : " PACKAGE_VERSION " (" BUILD_DATE " " BUILD_TIME ")");
  
  gtpe->addConstantCommand("name",PACKAGE_NAME);
  gtpe->addConstantCommand("version",PACKAGE_VERSION);
  
  book_autoload=true;
}

Oakfoam::~Oakfoam()
{
  delete gtpe;
  delete engine;
}

void Oakfoam::run()
{
  engine->postCmdLineArgs(book_autoload);
  engine->run();
}

