#ifndef HAVE_CONFIG_H
  #error Error! "config.h" required
#endif
#include <config.h>

#include "Oakfoam.h"

int main(int argc, char**argv)
{
    Oakfoam oakfoam;
    oakfoam.run();
    
    return 0;
}
