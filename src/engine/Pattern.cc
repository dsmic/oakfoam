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

std::string Pattern::ThreeByThree::toString()
{
  std::ostringstream ss;
  ss<<Go::colorToChar(colnw)<<Go::colorToChar(coln)<<Go::colorToChar(colne)<<"\n";
  ss<<Go::colorToChar(colw)<<'X'<<Go::colorToChar(cole)<<"\n";
  ss<<Go::colorToChar(colsw)<<Go::colorToChar(cols)<<Go::colorToChar(colse)<<"\n";
  return ss.str();
}

void Pattern::ThreeByThreeTable::addPatternTransformed(Pattern::ThreeByThree pattern, bool addinverted)
{
  Pattern::ThreeByThree currentpattern=pattern;
  Pattern::ThreeByThree currentpatterninverted=currentpattern.invert();
  
  for (int i=0;i<4;i++)
  {
    this->addPattern(currentpattern.hash());
    this->addPattern(currentpattern.flipHorizontal().hash());
    
    currentpattern=currentpattern.rotateRight();
    
    if (addinverted)
    {
      this->addPattern(currentpatterninverted.hash());
      this->addPattern(currentpatterninverted.flipHorizontal().hash());
      
      currentpatterninverted=currentpatterninverted.rotateRight();
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
    
    if (filtered.length()!=8) //all patterns are 8 chars long
    {
      fin.close();
      return false;
    }
      
    fprintf(stderr,"%s\n",filtered.c_str());
    
    std::transform(filtered.begin(),filtered.end(),filtered.begin(),::tolower);
    
    Pattern::ThreeByThree pattern=Pattern::ThreeByThree(fileCharToColor(filtered.at(0)),fileCharToColor(filtered.at(1)),fileCharToColor(filtered.at(2)),fileCharToColor(filtered.at(3)),fileCharToColor(filtered.at(4)),fileCharToColor(filtered.at(5)),fileCharToColor(filtered.at(6)),fileCharToColor(filtered.at(7)));
    this->addPatternTransformed(pattern);
  }
  
  fin.close();
  
  return true;
}


