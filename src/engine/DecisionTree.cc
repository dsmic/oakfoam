#include "DecisionTree.h"

#include <cstdio>
#include <fstream>
#include <algorithm>
#include <boost/lexical_cast.hpp>

#define TEXT "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789<>=!."
#define WHITESPACE " \t\r\n"

DecisionTree::DecisionTree(std::vector<std::string> *a, DecisionTree::Node *r)
{
  attrs = a;
  root = r;
}

DecisionTree::~DecisionTree()
{
  delete attrs;
  delete root;
}

DecisionTree *DecisionTree::parseString(std::string rawdata)
{
  std::string data = DecisionTree::stripWhitespace(rawdata);
  std::transform(data.begin(),data.end(),data.begin(),::toupper);
  fprintf(stderr,"[DT] parsing: '%s'\n",data.c_str());

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

  std::vector<std::string> *attrs = DecisionTree::parseAttrs(data,pos);
  if (attrs == NULL)
    return NULL;

  if (attrs->size()!=2 || attrs->at(0)!="SPARSE" || attrs->at(1)!="NOCOMP") //TODO: extend to more tree types
  {
    fprintf(stderr,"[DT] Error! Invalid attributes for tree\n");
    delete attrs;
    return NULL;
  }

  DecisionTree::Node *root = DecisionTree::parseNode(data,pos);
  if (root == NULL)
  {
    delete attrs;
    return NULL;
  }

  if (data[pos] != ')')
  {
    fprintf(stderr,"[DT] Error! Expected ')' at '%s'\n",data.substr(pos).c_str());
    delete root;
    delete attrs;
    return NULL;
  }
  pos++;

  return new DecisionTree(attrs,root);
}

std::vector<std::string> *DecisionTree::parseAttrs(std::string data, unsigned int &pos)
{
  if (data[pos] == ']')
  {
    pos++;
    return new std::vector<std::string>();
  }

  std::string attr = "";
  while (pos<data.length() && DecisionTree::isText(data[pos]))
  {
    attr += data[pos];
    pos++;
  }
  //fprintf(stderr,"[DT] attr:'%s'\n",attr.c_str());

  if (data[pos] == '|' || data[pos] == ']')
  {
    if (data[pos] == '|')
      pos++;
    std::vector<std::string> *attrstail = DecisionTree::parseAttrs(data,pos);
    if (attrstail == NULL)
      return NULL;
    std::vector<std::string> *attrs = new std::vector<std::string>();
    attrs->push_back(attr);
    for (unsigned int i=0;i<attrstail->size();i++)
    {
      attrs->push_back(attrstail->at(i));
    }
    delete attrstail;
    return attrs;
  }
  else
  {
    fprintf(stderr,"[DT] Error! Unexpected '%c' at '%s'\n",data[pos],data.substr(pos).c_str());
    return NULL;
  }
}

DecisionTree::Node *DecisionTree::parseNode(std::string data, unsigned int &pos)
{
  DecisionTree::Stats *stats = DecisionTree::parseStats(data,pos);
  if (stats == NULL)
    return NULL;

  if (data.substr(pos,8) == "(WEIGHT[")
  {
    pos += 8;
    float *num = DecisionTree::parseNumber(data,pos);
    if (num == NULL)
    {
      delete stats;
      return NULL;
    }

    if (data.substr(pos,2) != "])")
    {
      fprintf(stderr,"[DT] Error! Expected '])' at '%s'\n",data.substr(pos).c_str());
      delete stats;
      delete num;
      return NULL;
    }
    pos += 2;

    DecisionTree::Node *node = new DecisionTree::Node(stats,*num);
    //fprintf(stderr,"[DT] !!!\n");

    delete num;
    return node;
  }
  else
  {
    //TODO: parse QUERY
    return NULL;
  }
}

DecisionTree::Stats *DecisionTree::parseStats(std::string data, unsigned int &pos)
{
  if (data.substr(pos,7) != "(STATS:")
  {
    fprintf(stderr,"[DT] Error! Expected '(STATS:' at '%s'\n",data.substr(pos).c_str());
    return NULL;
  }
  pos += 7;

  //TODO: parse STATPERMS

  if (data[pos] != ')')
  {
    fprintf(stderr,"[DT] Error! Expected ')' at '%s'\n",data.substr(pos).c_str());
    return NULL;
  }
  pos++;

  return new DecisionTree::Stats();
}

float *DecisionTree::parseNumber(std::string data, unsigned int &pos)
{
  std::string s = "";
  while (pos<data.length() && (data[pos]=='.' || (data[pos]>='0' && data[pos]<='9')))
  {
    s += data[pos];
    pos++;
  }

  float n;
  try
  {
    n = boost::lexical_cast<float>(s);
  }
  catch (boost::bad_lexical_cast &)
  {
    return NULL;
  }
  float *p = new float();
  *p = n;
  return p;
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

DecisionTree::Node::Node(Stats *s, float w)
{
  stats = s;
  weight = w;
}

DecisionTree::Node::~Node()
{
  delete stats;
}

