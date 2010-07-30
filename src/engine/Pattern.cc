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

void Pattern::ThreeByThreeTable::updatePatternTransformed(bool addpattern, Pattern::ThreeByThree pattern, bool addinverted)
{
  Pattern::ThreeByThree currentpattern=pattern;
  Pattern::ThreeByThree currentpatterninverted=currentpattern.invert();
  
  for (int i=0;i<4;i++)
  {
    this->updatePattern(addpattern,currentpattern.hash());
    this->updatePattern(addpattern,currentpattern.flipHorizontal().hash());
    
    currentpattern=currentpattern.rotateRight();
    
    if (addinverted)
    {
      this->updatePattern(addpattern,currentpatterninverted.hash());
      this->updatePattern(addpattern,currentpatterninverted.flipHorizontal().hash());
      
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
    Pattern::ThreeByThree pattern=Pattern::ThreeByThree(gocols);
    if (toplay==Pattern::ThreeByThreeTable::BLACK)
      this->updatePatternTransformed(addpatterns,pattern,false);
    else if (toplay==Pattern::ThreeByThreeTable::WHITE)
      this->updatePatternTransformed(addpatterns,pattern.invert(),false);
    else
      this->updatePatternTransformed(addpatterns,pattern);
  }
}


