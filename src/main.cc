#ifndef HAVE_CONFIG_H
  #error Error! "config.h" required
#endif
#include <config.h>
#include <iostream>

#include "Oakfoam.h"

void printusage()
{
  std::cout << "Usage: oakfoam [OPTIONS]\n";
  std::cout << "\n";
  std::cout << "Options:\n";
  std::cout << "  -h, --help        display usage and exit\n";
  std::cout << "  -V, --version     display version and exit\n";
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
    else
    {
      std::cout << "Invalid argument: " << arg << "\n";
      return 1;
    }
  }
  
  oakfoam.run();
  
  return 0;
}
