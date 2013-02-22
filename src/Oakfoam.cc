#include "Oakfoam.h"

#include "config.h"
#include "build.h"

Oakfoam::Oakfoam()
{
  gtpe=new Gtp::Engine();
  engine=new Engine(gtpe,PACKAGE_NAME " : " PACKAGE_VERSION " (" BUILD_DATE " " BUILD_TIME ")");
  
  gtpe->addConstantCommand("name",PACKAGE_NAME);
  gtpe->addConstantCommand("version",PACKAGE_VERSION);
  
  book_autoload=true;
  web_interface=false;
  web_address="127.0.0.1";
  web_port=8000;
}

Oakfoam::~Oakfoam()
{
  gtpe->setPonderer(NULL,NULL,NULL);
  delete engine;
  delete gtpe;
}

void Oakfoam::run()
{
  engine->postCmdLineArgs(book_autoload);
  engine->run(web_interface,web_address,web_port);
}

