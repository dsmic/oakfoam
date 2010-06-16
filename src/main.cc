#ifndef HAVE_CONFIG_H
  #error Error! "config.h" required
#endif

#include <config.h>
#include <iostream>
#include <fstream>

#include "Oakfoam.h"

void printusage()
{
  std::cout << "Usage: oakfoam [OPTIONS]\n";
  std::cout << "\n";
  std::cout << "Options:\n";
  std::cout << "  -c, --config FILE     execute the GTP commands in FILE first (no output)\n";
  std::cout << "  -h, --help            display this help and exit\n";
  std::cout << "  -V, --version         display version and exit\n";
  std::cout << "\n";
  std::cout << "Report bugs to: " << PACKAGE_BUGREPORT << "\n";
}

int main(int argc, char* argv[])
{
  Oakfoam oakfoam;
  
  for(int i=1;i<argc;i++) 
  {
    std::string arg = argv[i];
    if (arg=="-h" || arg=="--help" )
    {
      printusage();
      return 0;
    }
    else if (arg=="-V" || arg=="--version" )
    {
      std::cout << PACKAGE_STRING << "\n";
      return 0;
    }
    else if (arg=="-c" || arg=="--config" )
    {
      i++;
      std::ifstream fin(argv[i]);
      
      if (!fin)
      {
        std::cerr << "error opening file: " << argv[i] << "\n";
        return 1;
      }
      
      oakfoam.gtpe->getOutput()->setOutputOn(false);
      
      std::string line;
      while (std::getline(fin,line))
      {
        if (!oakfoam.gtpe->executeCommand(line))
        {
          fin.close();
          return 0;
        }
      }
      
      fin.close();
      
      oakfoam.gtpe->getOutput()->setOutputOn(true);
    }
    else
    {
      std::cout << "Invalid argument: " << arg << "\n";
      return 1;
    }
  }
  
  oakfoam.run();
  
  return 0;
}
