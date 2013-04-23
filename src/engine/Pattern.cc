#include "Pattern.h"

#include <cstdlib>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <fstream>

unsigned int Pattern::ThreeByThree::makeHash(Go::Color colnw, Go::Color coln, Go::Color colne, Go::Color colw, Go::Color cole, Go::Color colsw, Go::Color cols, Go::Color colse)
{
  //it is used later, that the return value has only 16 bit, even if unsigned int is larger!
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

unsigned long Pattern::FiveByFiveBorder::makeHash(Go::Board *board, int pos)
{
  unsigned long hash=0;
  int size=board->getSize();
  if (board->getColor(pos+P_NW)!=Go::OFFBOARD)
    hash|=(Pattern::FiveByFiveBorder::hashColor(board->getColor(pos+P_NW+P_NW)) << 30);
  else
    hash|=(Pattern::FiveByFiveBorder::hashColor(Go::OFFBOARD) << 30);
  if (board->getColor(pos+P_N)!=Go::OFFBOARD)
  {
    hash|=(Pattern::FiveByFiveBorder::hashColor(board->getColor(pos+P_N+P_NW)) << 28);
    hash|=(Pattern::FiveByFiveBorder::hashColor(board->getColor(pos+P_N+P_N)) << 26);
    hash|=(Pattern::FiveByFiveBorder::hashColor(board->getColor(pos+P_N+P_NE)) << 24);
  }
  else
  {
    hash|=(Pattern::FiveByFiveBorder::hashColor(Go::OFFBOARD) << 28);
    hash|=(Pattern::FiveByFiveBorder::hashColor(Go::OFFBOARD) << 26);
    hash|=(Pattern::FiveByFiveBorder::hashColor(Go::OFFBOARD) << 24);
  }
  if (board->getColor(pos+P_NE)!=Go::OFFBOARD)
    hash|=(Pattern::FiveByFiveBorder::hashColor(board->getColor(pos+P_NE+P_NE)) << 22);
  else
    hash|=(Pattern::FiveByFiveBorder::hashColor(Go::OFFBOARD) << 22);
  if (board->getColor(pos+P_W)!=Go::OFFBOARD)
  {
    hash|=(Pattern::FiveByFiveBorder::hashColor(board->getColor(pos+P_W+P_NW)) << 20);
    hash|=(Pattern::FiveByFiveBorder::hashColor(board->getColor(pos+P_W+P_W)) << 18);
    hash|=(Pattern::FiveByFiveBorder::hashColor(board->getColor(pos+P_W+P_SW)) << 16);
  }
  else
  {
    hash|=(Pattern::FiveByFiveBorder::hashColor(Go::OFFBOARD) << 20);
    hash|=(Pattern::FiveByFiveBorder::hashColor(Go::OFFBOARD) << 18);
    hash|=(Pattern::FiveByFiveBorder::hashColor(Go::OFFBOARD) << 16);
  }
  if (board->getColor(pos+P_E)!=Go::OFFBOARD)
  {
    hash|=(Pattern::FiveByFiveBorder::hashColor(board->getColor(pos+P_E+P_NE)) << 14);
    hash|=(Pattern::FiveByFiveBorder::hashColor(board->getColor(pos+P_E+P_E)) << 12);
    hash|=(Pattern::FiveByFiveBorder::hashColor(board->getColor(pos+P_E+P_SE)) << 10);
  }
  else
  {
    hash|=(Pattern::FiveByFiveBorder::hashColor(Go::OFFBOARD) << 14);
    hash|=(Pattern::FiveByFiveBorder::hashColor(Go::OFFBOARD) << 12);
    hash|=(Pattern::FiveByFiveBorder::hashColor(Go::OFFBOARD) << 10);
  }
  if (board->getColor(pos+P_SW)!=Go::OFFBOARD)
    hash|=(Pattern::FiveByFiveBorder::hashColor(board->getColor(pos+P_SW+P_SW)) << 8);
  else
    hash|=(Pattern::FiveByFiveBorder::hashColor(Go::OFFBOARD) << 8);
  if (board->getColor(pos+P_S)!=Go::OFFBOARD)
  {
    hash|=(Pattern::FiveByFiveBorder::hashColor(board->getColor(pos+P_S+P_SW)) << 6);
    hash|=(Pattern::FiveByFiveBorder::hashColor(board->getColor(pos+P_S+P_S)) << 4);
    hash|=(Pattern::FiveByFiveBorder::hashColor(board->getColor(pos+P_S+P_SE)) << 2);
  }
  else
  {
    hash|=(Pattern::FiveByFiveBorder::hashColor(Go::OFFBOARD) << 6);
    hash|=(Pattern::FiveByFiveBorder::hashColor(Go::OFFBOARD) << 4);
    hash|=(Pattern::FiveByFiveBorder::hashColor(Go::OFFBOARD) << 2);
  }
  if (board->getColor(pos+P_SE)!=Go::OFFBOARD)
    hash|=(Pattern::FiveByFiveBorder::hashColor(board->getColor(pos+P_SE+P_SE)) << 0);
  else
    hash|=(Pattern::FiveByFiveBorder::hashColor(Go::OFFBOARD) << 0);
  return hash;
}

int Pattern::FiveByFiveBorder::hashColor(Go::Color col)
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

Pattern::CircularDictionary::CircularDictionary()
{
  for (int i=0;i<PATTERN_CIRC_32BITPARTS;i++)
  {
    rot_r2[i]=0;
    rot_r4[i]=0;
    rot_l6[i]=0;
    rot_l12[i]=0;
    flip_0[i]=0;
    flip_r2[i]=0;
    flip_r4[i]=0;
    flip_r6[i]=0;
    flip_r10[i]=0;
    flip_r14[i]=0;
    flip_l2[i]=0;
    flip_l4[i]=0;
    flip_l6[i]=0;
    flip_l10[i]=0;
    flip_l14[i]=0;
  }
  
  int base=0;
  for (int d=2;d<=PATTERN_CIRC_MAXSIZE;d++)
  {
    baseoffset[d]=base;
    int start=d/3+(d%3!=0?1:0);
    int end=d/2;
    for (int b=start;b<=end;b++)
    {
      int a=d-2*b;
      if (a>=0 && a<=b)
      {
        if (a==0)
        {
          dictx[d].push_back(0);
          dicty[d].push_back(b);
          this->setTrans(rot_r2,base);
          this->setTrans(flip_0,base);
          dictx[d].push_back(b);
          dicty[d].push_back(0);
          this->setTrans(rot_r2,base+1);
          this->setTrans(flip_r4,base+1);
          dictx[d].push_back(0);
          dicty[d].push_back(-b);
          this->setTrans(rot_r2,base+2);
          this->setTrans(flip_0,base+2);
          dictx[d].push_back(-b);
          dicty[d].push_back(0);
          this->setTrans(rot_l6,base+3);
          this->setTrans(flip_l4,base+3);
          
          base+=4;
        }
        else if (a==b)
        {
          dictx[d].push_back(b);
          dicty[d].push_back(b);
          this->setTrans(rot_r2,base);
          this->setTrans(flip_r6,base);
          dictx[d].push_back(b);
          dicty[d].push_back(-b);
          this->setTrans(rot_r2,base+1);
          this->setTrans(flip_r2,base+1);
          dictx[d].push_back(-b);
          dicty[d].push_back(-b);
          this->setTrans(rot_r2,base+2);
          this->setTrans(flip_l2,base+2);
          dictx[d].push_back(-b);
          dicty[d].push_back(b);
          this->setTrans(rot_l6,base+3);
          this->setTrans(flip_l6,base+3);
          
          base+=4;
        }
        else
        {
          dictx[d].push_back(a);
          dicty[d].push_back(b);
          this->setTrans(rot_r4,base);
          this->setTrans(flip_r14,base);
          dictx[d].push_back(b);
          dicty[d].push_back(a);
          this->setTrans(rot_r4,base+1);
          this->setTrans(flip_r10,base+1);
          dictx[d].push_back(b);
          dicty[d].push_back(-a);
          this->setTrans(rot_r4,base+2);
          this->setTrans(flip_r6,base+2);
          dictx[d].push_back(a);
          dicty[d].push_back(-b);
          this->setTrans(rot_r4,base+3);
          this->setTrans(flip_r2,base+3);
          
          dictx[d].push_back(-a);
          dicty[d].push_back(-b);
          this->setTrans(rot_r4,base+4);
          this->setTrans(flip_l2,base+4);
          dictx[d].push_back(-b);
          dicty[d].push_back(-a);
          this->setTrans(rot_r4,base+5);
          this->setTrans(flip_l6,base+5);
          dictx[d].push_back(-b);
          dicty[d].push_back(a);
          this->setTrans(rot_l12,base+6);
          this->setTrans(flip_l10,base+6);
          dictx[d].push_back(-a);
          dicty[d].push_back(b);
          this->setTrans(rot_l12,base+7);
          this->setTrans(flip_l14,base+7);
          
          base+=8;
        }
      }
    }
  }
}

Pattern::Circular::Circular(Pattern::CircularDictionary *dict, const Go::Board *board, int pos, int sz) : size(sz>PATTERN_CIRC_MAXSIZE?PATTERN_CIRC_MAXSIZE:sz)
{
  ldict=dict;
  for (int i=0;i<PATTERN_CIRC_32BITPARTS;i++)
  {
    hash[i]=0;
  }
  
  int x=Go::Position::pos2x(pos,board->getSize());
  int y=Go::Position::pos2y(pos,board->getSize());
  int base=0;
  
  for (int d=2;d<=size;d++)
  {
    std::list<int> *xoffsets=dict->getXOffsetsForSize(d);
    std::list<int> *yoffsets=dict->getYOffsetsForSize(d);
    std::list<int>::iterator iterx=xoffsets->begin();
    std::list<int>::iterator itery=yoffsets->begin();
    while(iterx!=xoffsets->end() && itery!=yoffsets->end())
    {
      int fx=x+(*iterx);
      int fy=y+(*itery);
      Go::Color col;
      
      if (fx<0 || fy<0 || fx>=board->getSize() || fy>=board->getSize())
        col=Go::OFFBOARD;
      else
        col=board->getColor(Go::Position::xy2pos(fx,fy,board->getSize()));
      
      this->initColor(base,col);
      
      ++iterx;
      ++itery;
      base++;
    }
  }
}

int Pattern::Circular::countStones(Pattern::CircularDictionary *dict)
{
  int numStones=0;
  int l=dict->getBaseOffset(size+1);
     
  for (int i=0;i<l;i++)
  {
    if (this->getColor(i)==Go::WHITE) numStones++;
    if (this->getColor(i)==Go::BLACK) numStones++;
  }

  return numStones;
}
      
int Pattern::Circular::hashColor(Go::Color col)
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

Go::Color Pattern::Circular::hash2Color(int hash)
{
  switch (hash)
  {
    case 0:
      return Go::EMPTY;
    case 1:
      return Go::BLACK;
    case 2:
      return Go::WHITE;
    case 3:
      return Go::OFFBOARD;
    default:
      return Go::OFFBOARD;
  }

}

void Pattern::Circular::initColor(int offset, Go::Color col)
{
  int part=offset/(32/2);
  int bitoffset=(offset%(32/2))*2;
  
  hash[part]|=Pattern::Circular::hashColor(col)<<(32-bitoffset-2);
}

Go::Color Pattern::Circular::getColor(int offset)
{
  int part=offset/(32/2);
  int bitoffset=(offset%(32/2))*2;
  
  uint32_t tmp=hash[part]>>(32-bitoffset-2);
//  fprintf(stderr,"%x %x\n",hash[part],tmp & 0x0003);
  return this->hash2Color(tmp & 0x0003);
}

void Pattern::Circular::resetColor(int offset)
{
  int part=offset/(32/2);
  int bitoffset=(offset%(32/2))*2;
  
  hash[part]&=~(((boost::uint_fast32_t)3<<(32-bitoffset-2)));
}

void Pattern::CircularDictionary::setTrans(boost::uint_fast32_t data[PATTERN_CIRC_32BITPARTS], int offset)
{
  int part=offset/(32/2);
  int bitoffset=(offset%(32/2))*2;
  
  data[part]|=(3)<<(32-bitoffset-2);
}

std::string Pattern::Circular::toString(Pattern::CircularDictionary *dict) const
{
  std::ostringstream ss;
  
  ss<<size;
  int len=0;
  if (size==PATTERN_CIRC_MAXSIZE)
    len=PATTERN_CIRC_32BITPARTS;
  else
  {
    int l=dict->getBaseOffset(size+1);
    len=l/(32/2)+(l%(32/2)>0?1:0);
  }
    
  for (int i=0;i<len;i++)
  {
    ss<<std::hex<<":"<<std::setw(8)<<std::setfill('0')<<hash[i];
  }
  
  return ss.str();
}

int Pattern::Circular::sizefromString(std::string patternstring)
{
  std::istringstream iss(patternstring);
  std::string dummy1;
  getline(iss,dummy1,':');
  return strtol(dummy1.c_str(),NULL,10);
}

Pattern::Circular::Circular(Pattern::CircularDictionary *dict, std::string fromString): size(sizefromString(fromString)>PATTERN_CIRC_MAXSIZE?PATTERN_CIRC_MAXSIZE:sizefromString(fromString))
{
  ldict=dict;
  for (int i=0;i<PATTERN_CIRC_32BITPARTS;i++)
  {
    hash[i]=0;
  }
  //fprintf(stderr,"the size is %d\n",size);
  std::string dummy1, dummy2, dummy3;
  std::istringstream iss(fromString);
  getline(iss,dummy1,':');

  int i=0;
  while (1)
  {
    dummy2="";
    getline(iss,dummy2,':');
    if (dummy2.length()==0)
      break;
    hash[i]=strtoul(dummy2.c_str(),NULL,16);
    //fprintf(stderr,"1 %s\n",dummy2.c_str());
    //fprintf(stderr,"2 %lx\n",strtoul(dummy2.c_str(),NULL,16));
    i++;
  }
}

Pattern::Circular Pattern::Circular::getSubPattern(Pattern::CircularDictionary *dict, int newsize) const
{
  int s=newsize;
  if (newsize>size)
    s=size;
  Pattern::Circular newpatt(s);
  
  for (int i=0;i<PATTERN_CIRC_32BITPARTS;i++)
  {
    newpatt.hash[i]=hash[i];
  }
  
  if (s==PATTERN_CIRC_MAXSIZE)
    return newpatt;
  
  int l=dict->getBaseOffset(s+1);
  //fprintf(stderr,"l: %d\n",l);
  for (int i=l;i<PATTERN_CIRC_32BITPARTS*(32/2);i++)
  {
    newpatt.resetColor(i);
  }
  
  return newpatt;
}

bool Pattern::Circular::operator==(const Pattern::Circular other) const
{
  if (size!=other.size)
    return false;
  
  for (int i=0;i<PATTERN_CIRC_32BITPARTS;i++)
  {
    if (hash[i]!=other.hash[i])
      return false;
  }
  return true;
}

bool Pattern::Circular::operator<(const Pattern::Circular other) const
{
  if (size<other.size)
    return true;
  else if(size>other.size)
    return false;

  for (int i=0;i<PATTERN_CIRC_32BITPARTS;i++)
  {
    if (hash[i]<other.hash[i])
      return true;
    else if (hash[i]>other.hash[i])
      return false;
  }
  return false;
}

bool Pattern::Circular::operator<(const Pattern::Circular *other) const
{
  if (size<other->size)
    return true;
  else if(size>other->size)
    return false;

  for (int i=0;i<PATTERN_CIRC_32BITPARTS;i++)
  {
    if (hash[i]<other->hash[i])
      return true;
    else if (hash[i]>other->hash[i])
      return false;
  }
  return false;
}

void Pattern::Circular::invert()
{
  for (int i=0;i<PATTERN_CIRC_32BITPARTS;i++)
  {
    boost::uint_fast32_t upper=hash[i]&(0xAAAAAAAA);
    boost::uint_fast32_t lower=hash[i]&(0x55555555);
    hash[i]=(upper>>1)|(lower<<1);
  }
}

void Pattern::Circular::rotateRight(Pattern::CircularDictionary *dict)
{
  boost::uint_fast32_t withnext=0;
  for (int i=0;i<PATTERN_CIRC_32BITPARTS;i++)
  {
    boost::uint_fast32_t orig=hash[i];
    boost::uint_fast32_t r2=dict->rot_r2[i];
    boost::uint_fast32_t r4=dict->rot_r4[i];
    boost::uint_fast32_t l6=dict->rot_l6[i];
    boost::uint_fast32_t l12=dict->rot_l12[i];
    
    if (i>0)
      hash[i-1]|=((orig&l6)>>(32-6))|((orig&l12)>>(32-12));  
    hash[i]=((orig&r2)>>2)|((orig&r4)>>4)|((orig&l6)<<6)|((orig&l12)<<12)|withnext;
    withnext=((orig&r2)<<(32-2))|((orig&r4)<<(32-4));
  }
}

void Pattern::Circular::flipHorizontal(Pattern::CircularDictionary *dict)
{
  boost::uint_fast32_t withnext=0;
  for (int i=0;i<PATTERN_CIRC_32BITPARTS;i++)
  {
    boost::uint_fast32_t orig=hash[i];
    boost::uint_fast32_t none=dict->flip_0[i];
    boost::uint_fast32_t r2=dict->flip_r2[i];
    boost::uint_fast32_t r4=dict->flip_r4[i];
    boost::uint_fast32_t r6=dict->flip_r6[i];
    boost::uint_fast32_t r10=dict->flip_r10[i];
    boost::uint_fast32_t r14=dict->flip_r14[i];
    boost::uint_fast32_t l2=dict->flip_l2[i];
    boost::uint_fast32_t l4=dict->flip_l4[i];
    boost::uint_fast32_t l6=dict->flip_l6[i];
    boost::uint_fast32_t l10=dict->flip_l10[i];
    boost::uint_fast32_t l14=dict->flip_l14[i];
    
    if (i>0)
      hash[i-1]|=((orig&l2)>>(32-2))|((orig&l4)>>(32-4))|((orig&l6)>>(32-6))|((orig&l10)>>(32-10))|((orig&l14)>>(32-14));  
    hash[i]=(orig&none)|((orig&r2)>>2)|((orig&r4)>>4)|((orig&r6)>>6)|((orig&r10)>>10)|((orig&r14)>>14)|((orig&l2)<<2)|((orig&l4)<<4)|((orig&l6)<<6)|((orig&l10)<<10)|((orig&l14)<<14)|withnext;
    withnext=((orig&r2)<<(32-2))|((orig&r4)<<(32-4))|((orig&r6)<<(32-6))|((orig&r10)<<(32-10))|((orig&r14)<<(32-14));
  }
}

void Pattern::Circular::convertToSmallestEquivalent(Pattern::CircularDictionary *dict)
{
  Pattern::Circular alternative=this->copy();
  
  //fprintf(stderr,"comparing: %s\n",this->toString(dict).c_str());
  
  for (int x=0;x<4;x++)
  {
    if (alternative<this)
    {
      //fprintf(stderr,"found: %s\n",alternative.toString(dict).c_str());
      for (int i=0;i<PATTERN_CIRC_32BITPARTS;i++)
      {
        hash[i]=alternative.hash[i];
      }
    }
    //else
    //  fprintf(stderr,"missed: %s\n",alternative.toString(dict).c_str());
    alternative.rotateRight(dict);
  }
  
  alternative.flipHorizontal(dict);
  
  for (int x=0;x<4;x++)
  {
    if (alternative<this)
    {
      //fprintf(stderr,"found: %s\n",alternative.toString(dict).c_str());
      for (int i=0;i<PATTERN_CIRC_32BITPARTS;i++)
      {
        hash[i]=alternative.hash[i];
      }
    }
    //else
    //  fprintf(stderr,"missed: %s\n",alternative.toString(dict).c_str());
    alternative.rotateRight(dict);
  }
}

