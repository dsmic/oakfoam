#include "DecisionTree.h"

#include <cstdio>
#include <fstream>
#include <algorithm>

#define TEXT "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789<>=!."
#define WHITESPACE " \t\r\n"

DecisionTree::DecisionTree()
{
}

DecisionTree *DecisionTree::parseString(std::string rawdata)
{
  std::string data = DecisionTree::stripWhitespace(rawdata);
  std::transform(data.begin(),data.end(),data.begin(),::toupper);
  fprintf(stderr,"parsing: '%s'\n",data.c_str());

  unsigned int pos = data.find("(DT");
  if (pos == std::string::npos)
  {
    fprintf(stderr,"[DT] Error! Missing '(DT'\n");
    return NULL;
  }
  pos += 3;

  if (data[pos] != '[')
  {
    fprintf(stderr,"[DT] Error! Expected '[' at '%s'\n",data.substr(pos).c_str());
    return NULL;
  }
  pos++;

  std::list<std::string> *attrs = DecisionTree::parseAttrs(data,pos);
  if (attrs == NULL)
    return NULL;

  //TODO: parse NODE

  if (data[pos] != ')')
  {
    fprintf(stderr,"[DT] Error! Expected ')' at '%s'\n",data.substr(pos).c_str());
    return NULL;
  }

  //TODO: create new object
  return NULL;
}

std::list<std::string> *DecisionTree::parseAttrs(std::string data, unsigned int &pos)
{
  if (data[pos] == ']')
  {
    pos++;
    return new std::list<std::string>();
  }

  std::string attr = "";
  while (DecisionTree::isText(data[pos]))
  {
    attr += data[pos];
    pos++;
  }
  fprintf(stderr,"[DT] attr:'%s'\n",attr.c_str());

  if (data[pos] == '|' || data[pos] == ']')
  {
    if (data[pos] == '|')
      pos++;
    std::list<std::string> *attrs = DecisionTree::parseAttrs(data,pos);
    if (attrs == NULL)
      return NULL;
    attrs->push_front(attr);
    return attrs;
  }
  else
  {
    fprintf(stderr,"[DT] Error! Unexpected '%c' at '%s'\n",data[pos],data.substr(pos).c_str());
    return NULL;
  }
}

DecisionTree *DecisionTree::loadFile(std::string filename)
{
  std::ifstream fin(filename.c_str());
  
  if (!fin)
    return NULL;
  
  std::string data = "";
  std::string line;
  while (std::getline(fin,line))
  {
    data += line + "\n";
  }
  
  fin.close();
  
  return DecisionTree::parseString(data);
}

bool DecisionTree::isText(char c)
{
  std::string text = TEXT;
  return (text.find(c) != std::string::npos);
}

std::string DecisionTree::stripWhitespace(std::string in)
{
  std::string whitespace = WHITESPACE;
  std::string out = "";
  for (unsigned int i = 0; i < in.length(); i++)
  {
    if (whitespace.find(in[i]) == std::string::npos)
      out += in[i];
  }
  return out;
}

