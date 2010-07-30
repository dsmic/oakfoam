#include "Pattern.h"

Pattern::ThreeByThree Pattern::ThreeByThree::invert()
{
  return Pattern::ThreeByThree(
    Go::otherColor(colnw),Go::otherColor(coln),Go::otherColor(colne),
    Go::otherColor(colw),Go::otherColor(cole),
    Go::otherColor(colsw),Go::otherColor(cols),Go::otherColor(colse)
  );
}

Pattern::ThreeByThree Pattern::ThreeByThree::rotateRight()
{
  return Pattern::ThreeByThree(
    colsw,colw,colnw,
    cols,coln,
    colse,cole,colne
  );
}

Pattern::ThreeByThree Pattern::ThreeByThree::flipHorizontal()
{
  return Pattern::ThreeByThree(
    colne,coln,colnw,
    cole,colw,
    colse,cols,colsw
  );
}

int Pattern::ThreeByThree::hashColor(Go::Color col)
{
  switch (col)
  {
    case Go::EMPTY:
      return 0;
    case Go::BLACK:
      return 1;
    case Go::WHITE:
      return 2;
    case Go::OFFBOARD:
      return 3;
    default:
      return -1;
  }
}

int Pattern::ThreeByThree::hash()
{
  int hash=0;
  
  hash|=(this->hashColor(colnw) << 14);
  hash|=(this->hashColor(coln) << 12);
  hash|=(this->hashColor(colne) << 10);
  
  hash|=(this->hashColor(colw) << 8);
  hash|=(this->hashColor(cole) << 6);
  
  hash|=(this->hashColor(colsw) << 4);
  hash|=(this->hashColor(cols) << 2);
  hash|=(this->hashColor(colse));
  
  return hash;
}


