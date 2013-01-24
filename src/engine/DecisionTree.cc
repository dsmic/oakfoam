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
  if (attr.length() == 0)
  {
    fprintf(stderr,"[DT] Error! Unexpected '%c' at '%s'\n",data[pos],data.substr(pos).c_str());
    return NULL;
  }

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

  std::vector<DecisionTree::StatPerm*> *sp = DecisionTree::parseStatPerms(data,pos);
  if (sp == NULL)
    return NULL;

  if (data[pos] != ')')
  {
    fprintf(stderr,"[DT] Error! Expected ')' at '%s'\n",data.substr(pos).c_str());
    return NULL;
  }
  pos++;

  return new DecisionTree::Stats(sp);
}

std::vector<DecisionTree::StatPerm*> *DecisionTree::parseStatPerms(std::string data, unsigned int &pos)
{
  if (data[pos] != '(')
  {
    fprintf(stderr,"[DT] Error! Expected '(' at '%s'\n",data.substr(pos).c_str());
    return NULL;
  }
  pos++;

  std::string label = "";
  while (pos<data.length() && DecisionTree::isText(data[pos]))
  {
    label += data[pos];
    pos++;
  }
  if (label.length() == 0)
  {
    fprintf(stderr,"[DT] Error! Unexpected '%c' at '%s'\n",data[pos],data.substr(pos).c_str());
    return NULL;
  }

  if (data[pos] != '[')
  {
    fprintf(stderr,"[DT] Error! Expected '[' at '%s'\n",data.substr(pos).c_str());
    return NULL;
  }
  pos++;

  std::vector<std::string> *attrs = DecisionTree::parseAttrs(data,pos);
  if (attrs == NULL)
    return NULL;

  DecisionTree::Range *range = DecisionTree::parseRange(data,pos);
  if (range == NULL)
  {
    delete attrs;
    return NULL;
  }

  if (data[pos] != ')')
  {
    fprintf(stderr,"[DT] Error! Expected ')' at '%s'\n",data.substr(pos).c_str());
    delete attrs;
    delete range;
    return NULL;
  }
  pos++;

  DecisionTree::StatPerm *sp = new StatPerm(label,attrs,range);
  if (data[pos] == '(')
  {
    std::vector<DecisionTree::StatPerm*> *spstail = DecisionTree::parseStatPerms(data,pos);
    if (spstail == NULL)
    {
      delete attrs;
      delete sp;
      return NULL;
    }
    std::vector<DecisionTree::StatPerm*> *sps = new std::vector<DecisionTree::StatPerm*>();
    sps->push_back(sp);
    for (unsigned int i=0;i<spstail->size();i++)
    {
      sps->push_back(spstail->at(i));
    }
    delete spstail;

    return sps;
  }
  else
  {
    std::vector<DecisionTree::StatPerm*> *sps = new std::vector<DecisionTree::StatPerm*>();
    sps->push_back(sp);
    return sps;
  }
}

DecisionTree::Range *DecisionTree::parseRange(std::string data, unsigned int &pos)
{
  if (data[pos] != '(')
  {
    fprintf(stderr,"[DT] Error! Expected '(' at '%s'\n",data.substr(pos).c_str());
    return NULL;
  }
  pos++;

  float *ps = DecisionTree::parseNumber(data,pos);
  if (ps == NULL)
    return NULL;

  if (data[pos] != ':')
  {
    fprintf(stderr,"[DT] Error! Expected ':' at '%s'\n",data.substr(pos).c_str());
    delete ps;
    return NULL;
  }
  pos++;

  float *pe = DecisionTree::parseNumber(data,pos);
  if (pe == NULL)
  {
    delete ps;
    return NULL;
  }

  if (data[pos] != '[')
  {
    fprintf(stderr,"[DT] Error! Expected '[' at '%s'\n",data.substr(pos).c_str());
    delete ps;
    delete pe;
    return NULL;
  }
  pos++;

  float *pv = DecisionTree::parseNumber(data,pos);
  if (pv == NULL)
  {
    delete ps;
    delete pe;
    return NULL;
  }

  if (data[pos] != ']')
  {
    fprintf(stderr,"[DT] Error! Expected ']' at '%s'\n",data.substr(pos).c_str());
    delete ps;
    delete pe;
    delete pv;
    return NULL;
  }
  pos++;

  if (data[pos] == '(')
  {
    DecisionTree::Range *left = DecisionTree::parseRange(data,pos);
    if (left == NULL)
    {
      delete ps;
      delete pe;
      delete pv;
      return NULL;
    }

    DecisionTree::Range *right = DecisionTree::parseRange(data,pos);
    if (right == NULL)
    {
      delete ps;
      delete pe;
      delete pv;
      delete left;
      return NULL;
    }

    if (data[pos] != ')')
    {
      fprintf(stderr,"[DT] Error! Expected ')' at '%s'\n",data.substr(pos).c_str());
      delete ps;
      delete pe;
      delete pv;
      delete left;
      delete right;
      return NULL;
    }
    pos++;

    return new DecisionTree::Range(*ps,*pe,*pv,left,right);
  }
  else
  {
    if (data[pos] != ')')
    {
      fprintf(stderr,"[DT] Error! Expected ')' at '%s'\n",data.substr(pos).c_str());
      delete ps;
      delete pe;
      delete pv;
      return NULL;
    }
    pos++;

    return new DecisionTree::Range(*ps,*pe,*pv);
  }
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
    fprintf(stderr,"[DT] Error! '%s' isn't a valid number\n",s.c_str());
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

DecisionTree::Stats::Stats(std::vector<StatPerm*> *sp)
{
  statperms = sp;
}

DecisionTree::Stats::~Stats()
{
  for (unsigned int i=0;i<statperms->size();i++)
  {
    delete statperms->at(i);
  }
  delete statperms;
}

DecisionTree::StatPerm::StatPerm(std::string l, std::vector<std::string> *a, Range *r)
{
  label = l;
  attrs = a;
  range = r;
}

DecisionTree::StatPerm::~StatPerm()
{
  delete attrs;
  delete range;
}

DecisionTree::Range::Range(float s, float e, float v, Range *l, Range *r)
{
  start = s;
  end = e;
  val = v;
  left = l;
  right = r;
}

DecisionTree::Range::Range(float s, float e, float v)
{
  start = s;
  end = e;
  val = v;
  left = NULL;
  right = NULL;
}

DecisionTree::Range::~Range()
{
  if (left != NULL)
    delete left;
  if (right != NULL)
    delete right;
}

