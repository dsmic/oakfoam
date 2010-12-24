#include "Pattern.h"

unsigned int Pattern::ThreeByThree::makeHash(Go::Color colnw, Go::Color coln, Go::Color colne, Go::Color colw, Go::Color cole, Go::Color colsw, Go::Color cols, Go::Color colse)
{
  unsigned int hash=0;
  
  hash|=(Pattern::ThreeByThree::hashColor(colnw) << 14);
  hash|=(Pattern::ThreeByThree::hashColor(coln) << 12);
  hash|=(Pattern::ThreeByThree::hashColor(colne) << 10);
  
  hash|=(Pattern::ThreeByThree::hashColor(colw) << 8);
  hash|=(Pattern::ThreeByThree::hashColor(cole) << 6);
  
  hash|=(Pattern::ThreeByThree::hashColor(colsw) << 4);
  hash|=(Pattern::ThreeByThree::hashColor(cols) << 2);
  hash|=(Pattern::ThreeByThree::hashColor(colse));
  
  return hash;
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
      return 3;
  }
}

unsigned int Pattern::ThreeByThree::invert(unsigned int hash)
{
 unsigned int oddbits=  hash&(0x5555);
 unsigned int evenbits= hash&(0xAAAA);
 return ((evenbits>>1) | (oddbits<<1));
}

unsigned int Pattern::ThreeByThree::rotateRight(unsigned int hash)
{
  unsigned int right2=  hash&(0xC0C0);
  unsigned int right3=  hash&(0x3000);
  unsigned int right5=  hash&(0x0C00);
  unsigned int left2=   hash&(0x0303);
  unsigned int left3=   hash&(0x000C);
  unsigned int left5=   hash&(0x0030);
  return ((right2>>4) | (right3>>6) | (right5>>10) | (left2<<4) | (left3<<6) | (left5<<10));
}

unsigned int Pattern::ThreeByThree::flipHorizontal(unsigned int hash)
{
  unsigned int same=    hash&(0x300C);
  unsigned int right1=  hash&(0x0300);
  unsigned int right2=  hash&(0xC030);
  unsigned int left1=   hash&(0x00C0);
  unsigned int left2=   hash&(0x0C03);
  return (same | (right1>>2) | (right2>>4) | (left1<<2) | (left2<<4));
}

unsigned int Pattern::ThreeByThree::smallestEquivalent(unsigned int hash)
{
  unsigned int currenthash=hash;
  unsigned int smallesthash=hash;
  
  for (int i=0;i<4;i++)
  {
    if (currenthash<smallesthash)
      smallesthash=currenthash;
    unsigned int flippedcurrent=Pattern::ThreeByThree::flipHorizontal(currenthash);
    if (flippedcurrent<smallesthash)
      smallesthash=flippedcurrent;
    
    currenthash=Pattern::ThreeByThree::rotateRight(currenthash);
  }
  
  return smallesthash;
}

void Pattern::ThreeByThreeTable::updatePatternTransformed(bool addpattern, unsigned int pattern, bool addinverted)
{
  unsigned int currentpattern=pattern;
  unsigned int currentpatterninverted=Pattern::ThreeByThree::invert(pattern);
  
  for (int i=0;i<4;i++)
  {
    this->updatePattern(addpattern,currentpattern);
    this->updatePattern(addpattern,Pattern::ThreeByThree::flipHorizontal(currentpattern));
    
    currentpattern=Pattern::ThreeByThree::rotateRight(currentpattern);
    
    if (addinverted)
    {
      this->updatePattern(addpattern,currentpatterninverted);
      this->updatePattern(addpattern,Pattern::ThreeByThree::flipHorizontal(currentpatterninverted));
      
      currentpatterninverted=Pattern::ThreeByThree::rotateRight(currentpatterninverted);
    }
  }
}

bool Pattern::ThreeByThreeTable::loadPatternFile(std::string patternfilename)
{
  std::ifstream fin(patternfilename.c_str());
  
  if (!fin)
    return false;
  
  std::string line;
  while (std::getline(fin,line))
  {
    std::string filtered="";
    for (int i=0;i<(int)line.length();i++)
    {
      if (line.at(i)=='#')
        break;
      else if (line.at(i)!=' ' && line.at(i)!='\t')
        filtered+=line.at(i);
    }
    
    if (filtered.length()==0)
      continue;
    
    if (filtered.length()!=10) //all patterns are 10 chars long
    {
      fin.close();
      return false;
    }
    
    this->processFilePattern(filtered);
  }
  
  fin.close();
  
  return true;
}

bool Pattern::ThreeByThreeTable::loadPatternString(std::string patternstring)
{
  std::istringstream iss(patternstring);
  
  std::string line;
  while (iss>>line) //XXX: can fail on some input
  {
    std::string filtered="";
    for (int i=0;i<(int)line.length();i++)
    {
      if (line.at(i)=='#')
        break;
      else if (line.at(i)!=' ' && line.at(i)!='\t')
        filtered+=line.at(i);
    }
    
    if (filtered.length()==0)
      continue;
    
    if (filtered.length()!=10) //all patterns are 10 chars long
      return false;
    
    this->processFilePattern(filtered);
  }
  
  return true;
}

void Pattern::ThreeByThreeTable::processFilePattern(std::string pattern)
{
  bool addpatterns=(pattern.at(0)=='+');
  Pattern::ThreeByThreeTable::Color toplay;
  if (pattern.at(1)=='B')
    toplay=Pattern::ThreeByThreeTable::BLACK;
  else if (pattern.at(1)=='W')
    toplay=Pattern::ThreeByThreeTable::WHITE;
  else
    toplay=Pattern::ThreeByThreeTable::ANY;
  
  Pattern::ThreeByThreeTable::Color colors[8];
  for (int i=0;i<8;i++)
  {
    colors[i]=this->fileCharToColor(pattern.at(i+2));
  }
  this->processPermutations(addpatterns,toplay,colors);
}

void Pattern::ThreeByThreeTable::processPermutations(bool addpatterns, Pattern::ThreeByThreeTable::Color toplay, Pattern::ThreeByThreeTable::Color colors[], int pos)
{
  if (pos<8)
  {
    Pattern::ThreeByThreeTable::Color savedcol=colors[pos];
    if (colors[pos]==Pattern::ThreeByThreeTable::BLACK || colors[pos]==Pattern::ThreeByThreeTable::WHITE || colors[pos]==Pattern::ThreeByThreeTable::EMPTY || colors[pos]==Pattern::ThreeByThreeTable::OFFBOARD)
      this->processPermutations(addpatterns,toplay,colors,pos+1);
    else if (colors[pos]==Pattern::ThreeByThreeTable::NOTBLACK)
    {
      colors[pos]=Pattern::ThreeByThreeTable::WHITE;
      this->processPermutations(addpatterns,toplay,colors,pos+1);
      colors[pos]=Pattern::ThreeByThreeTable::EMPTY;
      this->processPermutations(addpatterns,toplay,colors,pos+1);
    }
    else if (colors[pos]==Pattern::ThreeByThreeTable::NOTWHITE)
    {
      colors[pos]=Pattern::ThreeByThreeTable::BLACK;
      this->processPermutations(addpatterns,toplay,colors,pos+1);
      colors[pos]=Pattern::ThreeByThreeTable::EMPTY;
      this->processPermutations(addpatterns,toplay,colors,pos+1);
    }
    else if (colors[pos]==Pattern::ThreeByThreeTable::NOTEMPTY)
    {
      colors[pos]=Pattern::ThreeByThreeTable::BLACK;
      this->processPermutations(addpatterns,toplay,colors,pos+1);
      colors[pos]=Pattern::ThreeByThreeTable::WHITE;
      this->processPermutations(addpatterns,toplay,colors,pos+1);
    }
    else if (colors[pos]==Pattern::ThreeByThreeTable::ANY)
    {
      colors[pos]=Pattern::ThreeByThreeTable::BLACK;
      this->processPermutations(addpatterns,toplay,colors,pos+1);
      colors[pos]=Pattern::ThreeByThreeTable::WHITE;
      this->processPermutations(addpatterns,toplay,colors,pos+1);
      colors[pos]=Pattern::ThreeByThreeTable::EMPTY;
      this->processPermutations(addpatterns,toplay,colors,pos+1);
    }
    colors[pos]=savedcol;
  }
  else
  {
    Go::Color gocols[8];
    for (int i=0;i<8;i++)
    {
      gocols[i]=this->colorToGoColor(colors[i]);
    }
    unsigned int pattern=Pattern::ThreeByThree::makeHash(gocols);
    if (toplay==Pattern::ThreeByThreeTable::BLACK)
      this->updatePatternTransformed(addpatterns,pattern,false);
    else if (toplay==Pattern::ThreeByThreeTable::WHITE)
      this->updatePatternTransformed(addpatterns,Pattern::ThreeByThree::invert(pattern),false);
    else
      this->updatePatternTransformed(addpatterns,pattern);
  }
}


