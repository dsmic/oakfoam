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

float DecisionTree::getWeight(Go::Board *board, Go::Move move)
{
  if (!board->validMove(move))
    return -1;

  std::string type = attrs->at(0);
  fprintf(stderr,"[DT] type: '%s'\n",type.c_str());
  if (type == "SPARSE")
    return this->getSparseWeight(board,move);
  else
    return -1;
}

float DecisionTree::getSparseWeight(Go::Board *board, Go::Move move)
{
  std::vector<int> *stones = new std::vector<int>();
  bool invert = (move.getColor() != Go::BLACK);

  stones->push_back(move.getPosition());

  DecisionTree::Node *n = this->getSparseLeafNode(root,board,stones,invert);
  float w = -1;
  if (n != NULL)
    w = n->getWeight();

  delete stones;
  return w;
}

DecisionTree::Node *DecisionTree::getSparseLeafNode(DecisionTree::Node *node, Go::Board *board, std::vector<int> *stones, bool invert)
{
  if (node->isLeaf())
    return node;

  int center = stones->at(0);
  DecisionTree::Query *q = node->getQuery();

  //TODO: handle compress chains param
  
  if (q->getLabel() == "NEW")
  {
    std::vector<std::string> *attrs = q->getAttrs();
    bool B = (attrs->at(0).find('B') != std::string::npos);
    bool W = (attrs->at(0).find('W') != std::string::npos);
    bool S = (attrs->at(0).find('S') != std::string::npos);
    std::string valstr = attrs->at(1);
    int val = boost::lexical_cast<int>(valstr);

    int newpos = -1;
    for (int s=0; s<val; s++)
    {
      //XXX: need more efficient way of finding new points
      std::list<int> matches;
      for (int p=0; p<board->getPositionMax(); p++)
      {
        Go::Color col = board->getColor(p);
        if (((invert?W:B) && col==Go::BLACK) || ((invert?B:W) && col==Go::WHITE) || (S && col==Go::OFFBOARD))
        {
          if (board->getRectDistance(center,p) <= s)
            matches.push_back(p);
        }
      }
      if (matches.size() > 0)
      {
        newpos = matches.front(); // TODO: break ties properly!
        break;
      }
    }

    Go::Color col = Go::EMPTY;
    if (newpos != -1)
    {
      stones->push_back(newpos);
      col = board->getColor(newpos);
    }

    std::vector<DecisionTree::Option*> *options = q->getOptions();
    for (unsigned int i=0; i<options->size(); i++)
    {
      std::string l = options->at(i)->getLabel();
      if (col==Go::BLACK && l=="B")
        return this->getSparseLeafNode(options->at(i)->getNode(),board,stones,invert);
      else if (col==Go::WHITE && l=="W")
        return this->getSparseLeafNode(options->at(i)->getNode(),board,stones,invert);
      else if (col==Go::OFFBOARD && l=="S")
        return this->getSparseLeafNode(options->at(i)->getNode(),board,stones,invert);
      else if (col==Go::EMPTY && l=="N")
        return this->getSparseLeafNode(options->at(i)->getNode(),board,stones,invert);
    }
    return NULL;
  }
  else if (q->getLabel() == "DIST")
  {
    std::vector<std::string> *attrs = q->getAttrs();
    int n0 = boost::lexical_cast<int>(attrs->at(0));
    int n1 = boost::lexical_cast<int>(attrs->at(1));
    bool eq = attrs->at(2) == "=";
    int val = boost::lexical_cast<int>(attrs->at(3));

    int dist = board->getRectDistance(stones->at(n0),stones->at(n1));
    bool res;
    if (eq)
      res = dist == val;
    else
      res = dist < val;

    std::vector<DecisionTree::Option*> *options = q->getOptions();
    for (unsigned int i=0; i<options->size(); i++)
    {
      std::string l = options->at(i)->getLabel();
      if (res && l=="Y")
        return this->getSparseLeafNode(options->at(i)->getNode(),board,stones,invert);
      else if (!res && l=="N")
        return this->getSparseLeafNode(options->at(i)->getNode(),board,stones,invert);
    }
    return NULL;
  }
  else if (q->getLabel() == "ATTR")
  {
    std::vector<std::string> *attrs = q->getAttrs();
    std::string type = attrs->at(0);
    int n = boost::lexical_cast<int>(attrs->at(1));
    bool eq = attrs->at(2) == "=";
    int val = boost::lexical_cast<int>(attrs->at(3));

    int attr = 0;
    if (type == "SIZE")
    {
      int p = stones->at(n);
      if (board->inGroup(p))
        attr = board->getGroup(p)->numOfStones();
      else
        attr = 0;
    }
    else if (type == "LIB")
      attr = board->getGroup(stones->at(n))->numOfPseudoLiberties();

    bool res;
    if (eq)
      res = attr == val;
    else
      res = attr < val;

    std::vector<DecisionTree::Option*> *options = q->getOptions();
    for (unsigned int i=0; i<options->size(); i++)
    {
      std::string l = options->at(i)->getLabel();
      if (res && l=="Y")
        return this->getSparseLeafNode(options->at(i)->getNode(),board,stones,invert);
      else if (!res && l=="N")
        return this->getSparseLeafNode(options->at(i)->getNode(),board,stones,invert);
    }
    return NULL;
  }
  else
  {
    fprintf(stderr,"[DT] Error! Invalid query type: '%s'\n",q->getLabel().c_str());
    return NULL;
  }
}

std::string DecisionTree::toString()
{
  std::string r = "(DT[";
  for (unsigned int i=0;i<attrs->size();i++)
  {
    if (i!=0)
      r += "|";
    r += attrs->at(i);
  }
  r += "]\n";

  r += root->toString(2);

  r += ")";
  return r;
}

std::string DecisionTree::Node::toString(int indent)
{
  std::string r = "";

  r += stats->toString(indent);

  if (query == NULL)
  {
    for (int i=0;i<indent;i++)
      r += " ";
    r += "(WEIGHT[";
    r += boost::lexical_cast<std::string>(weight);
    r += "])\n";
  }
  else
    r += query->toString(indent);

  return r;
}

std::string DecisionTree::Query::toString(int indent)
{
  std::string r = "";

  for (int i=0;i<indent;i++)
    r += " ";
  r += "(" + label + "[";
  for (unsigned int i=0;i<attrs->size();i++)
  {
    if (i!=0)
      r += "|";
    r += attrs->at(i);
  }
  r += "]\n";

  for (unsigned int i=0;i<options->size();i++)
  {
    r += options->at(i)->toString(indent+2);
  }
  
  for (int i=0;i<indent;i++)
    r += " ";
  r += ")\n";

  return r;
}

std::string DecisionTree::Option::toString(int indent)
{
  std::string r = "";

  for (int i=0;i<indent;i++)
    r += " ";
  r += label + ":\n";

  r += node->toString(indent+2);

  return r;
}

std::string DecisionTree::Stats::toString(int indent)
{
  std::string r = "";

  for (int i=0;i<indent;i++)
    r += " ";
  r += "(STATS:\n";

  for (unsigned int i=0;i<statperms->size();i++)
  {
    r += statperms->at(i)->toString(indent+2);
  }

  for (int i=0;i<indent;i++)
    r += " ";
  r += ")\n";

  return r;
}

std::string DecisionTree::StatPerm::toString(int indent)
{
  std::string r = "";

  for (int i=0;i<indent;i++)
    r += " ";
  r += "(" + label + "[";
  for (unsigned int i=0;i<attrs->size();i++)
  {
    if (i!=0)
      r += "|";
    r += attrs->at(i);
  }
  r += "]\n";

  r += range->toString(indent+2);

  for (int i=0;i<indent;i++)
    r += " ";
  r += ")\n";

  return r;
}

std::string DecisionTree::Range::toString(int indent)
{
  std::string r = "";

  for (int i=0;i<indent;i++)
    r += " ";
  r += "(";
  r += boost::lexical_cast<std::string>(start);
  r += ":";
  r += boost::lexical_cast<std::string>(end);
  r += "[";
  r += boost::lexical_cast<std::string>(val);
  r += "]";

  if (left==NULL && right==NULL)
    r += ")\n";
  else
  {
    r += "\n";

    if (left != NULL)
      r += left->toString(indent+2);
    if (right != NULL)
      r += right->toString(indent+2);

    for (int i=0;i<indent;i++)
      r += " ";
    r += ")\n";
  }

  return r;
}

DecisionTree *DecisionTree::parseString(std::string rawdata)
{
  std::string data = DecisionTree::stripWhitespace(rawdata);
  std::transform(data.begin(),data.end(),data.begin(),::toupper);
  //fprintf(stderr,"[DT] parsing: '%s'\n",data.c_str());

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
    if (data[pos] != '(')
    {
      fprintf(stderr,"[DT] Error! Expected '(' at '%s'\n",data.substr(pos).c_str());
      delete stats;
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
      delete stats;
      return NULL;
    }

    if (data[pos] != '[')
    {
      fprintf(stderr,"[DT] Error! Expected '[' at '%s'\n",data.substr(pos).c_str());
      delete stats;
      return NULL;
    }
    pos++;

    std::vector<std::string> *attrs = DecisionTree::parseAttrs(data,pos);
    if (attrs == NULL)
    {
      delete stats;
      return NULL;
    }

    std::vector<DecisionTree::Option*> *options = DecisionTree::parseOptions(data,pos);
    if (options == NULL)
    {
      delete stats;
      delete attrs;
      return NULL;
    }

    DecisionTree::Query *query = new DecisionTree::Query(label,attrs,options);
    return new DecisionTree::Node(stats,query);
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

std::vector<DecisionTree::Option*> *DecisionTree::parseOptions(std::string data, unsigned int &pos)
{
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

  if (data[pos] != ':')
  {
    fprintf(stderr,"[DT] Error! Expected ':' at '%s'\n",data.substr(pos).c_str());
    return NULL;
  }
  pos++;

  DecisionTree::Node *node = DecisionTree::parseNode(data,pos);
  if (node == NULL)
    return NULL;

  DecisionTree::Option *opt = new DecisionTree::Option(label,node);
  if (DecisionTree::isText(data[pos]))
  {
    std::vector<DecisionTree::Option*> *optstail = DecisionTree::parseOptions(data,pos);
    if (optstail == NULL)
    {
      delete opt;
      return NULL;
    }
    std::vector<DecisionTree::Option*> *opts = new std::vector<DecisionTree::Option*>();
    opts->push_back(opt);
    for (unsigned int i=0;i<optstail->size();i++)
    {
      opts->push_back(optstail->at(i));
    }
    delete optstail;

    return opts;
  }
  else
  {
    std::vector<DecisionTree::Option*> *opts = new std::vector<DecisionTree::Option*>();
    opts->push_back(opt);
    return opts;
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

DecisionTree::Node::Node(Stats *s, Query *q)
{
  stats = s;
  query = q;
  weight = 0;
}

DecisionTree::Node::Node(Stats *s, float w)
{
  stats = s;
  query = NULL;
  weight = w;
}

DecisionTree::Node::~Node()
{
  delete stats;
  if (query != NULL)
    delete query;
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

DecisionTree::Query::Query(std::string l, std::vector<std::string> *a, std::vector<Option*> *o)
{
  label = l;
  attrs = a;
  options = o;
}

DecisionTree::Query::~Query()
{
  delete attrs;
  for (unsigned int i=0;i<options->size();i++)
  {
    delete options->at(i);
  }
  delete options;
}

DecisionTree::Option::Option(std::string l, Node *n)
{
  label = l;
  node = n;
}

DecisionTree::Option::~Option()
{
  delete node;
}

