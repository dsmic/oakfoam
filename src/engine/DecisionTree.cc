#include "DecisionTree.h"

#include <cstdio>
#include <cmath>
#include <fstream>
#include <algorithm>
#include <boost/lexical_cast.hpp>
#include "Parameters.h"
#include "Engine.h"

#define TEXT "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789<>=!."
#define WHITESPACE " \t\r\n"

DecisionTree::DecisionTree(Parameters *p, DecisionTree::Type t, std::vector<std::string> *a, DecisionTree::Node *r)
{
  params = p;
  type = t;
  attrs = a;
  root = r;
  this->updateLeafIds();
}

DecisionTree::~DecisionTree()
{
  delete attrs;
  delete root;
}

void DecisionTree::updateLeafIds()
{
  leafmap.clear();
  root->populateLeafIds(leafmap);
}

void DecisionTree::setLeafWeight(int id, float w)
{
  //XXX: no error checking here
  leafmap[id]->setWeight(w);
}

unsigned int DecisionTree::getMaxNode(DecisionTree::Node *node)
{
  if (node->isRoot())
    return 0;

  DecisionTree::Option *opt = node->getParent();
  DecisionTree::Query *query = opt->getParent();
  bool addnode = false;
  switch (type)
  {
    case SPARSE:
      if (query->getLabel()=="NEW" && opt->getLabel()!="N")
        addnode = true;
      break;
  }
  
  DecisionTree::Node *topnode = query->getParent();
  return this->getMaxNode(topnode) + (addnode?1:0);
}

float DecisionTree::getWeight(Go::Board *board, Go::Move move, bool updatetree)
{
  if (!board->validMove(move) || !move.isNormal())
    return -1;

  if (type == SPARSE)
    return this->getSparseWeight(board,move,updatetree);
  else
    return -1;
}

void DecisionTree::updateDescent(Go::Board *board, Go::Move move)
{
  this->getWeight(board,move,true);
}

void DecisionTree::updateDescent(Go::Board *board)
{
  Go::Color col = board->nextToMove();
  for (int p=0; p<board->getPositionMax(); p++)
  {
    Go::Move move = Go::Move(col,p);
    if (board->validMove(move))
      this->updateDescent(board,move);
  }
}

void DecisionTree::collectionUpdateDescent(std::list<DecisionTree*> *trees, Go::Board *board)
{
  for (std::list<DecisionTree*>::iterator iter=trees->begin();iter!=trees->end();++iter)
  {
    (*iter)->updateDescent(board);
  }
}

float DecisionTree::getCollectionWeight(std::list<DecisionTree*> *trees, Go::Board *board, Go::Move move, bool updatetree)
{
  float weight = 1;

  if (!board->validMove(move) || !move.isNormal())
    return -1;

  for (std::list<DecisionTree*>::iterator iter=trees->begin();iter!=trees->end();++iter)
  {
    float w = (*iter)->getWeight(board, move, updatetree);
    if (w == -1)
      return -1;
    weight *= w;
  }

  return weight;
}

std::list<int> *DecisionTree::getLeafIds(Go::Board *board, Go::Move move)
{
  if (!board->validMove(move))
    return NULL;

  std::vector<int> *stones = new std::vector<int>();
  bool invert = (move.getColor() != Go::BLACK);
  stones->push_back(move.getPosition());

  std::list<DecisionTree::Node*> *nodes = NULL;
  if (type == SPARSE)
    nodes = this->getSparseLeafNodes(root,board,stones,invert,false);

  if (nodes == NULL)
    return NULL;

  std::list<int> *ids = new std::list<int>();
  for (std::list<DecisionTree::Node*>::iterator iter=nodes->begin();iter!=nodes->end();++iter)
  {
    ids->push_back((*iter)->getLeafId());
  }
  delete nodes;
  return ids;
}

std::list<int> *DecisionTree::getCollectionLeafIds(std::list<DecisionTree*> *trees, Go::Board *board, Go::Move move)
{
  std::list<int> *collids = new std::list<int>();
  int offset = 0;

  for (std::list<DecisionTree*>::iterator iter=trees->begin();iter!=trees->end();++iter)
  {
    std::list<int> *ids = (*iter)->getLeafIds(board, move);
    if (ids == NULL)
    {
      delete collids;
      return NULL;
    }
    for (std::list<int>::iterator iter2=ids->begin();iter2!=ids->end();++iter2)
    {
      collids->push_back(offset + (*iter2));
    }
    delete ids;
    offset += (*iter)->getLeafCount();
  }

  return collids;
}

int DecisionTree::getCollectionLeafCount(std::list<DecisionTree*> *trees)
{
  int count = 0;

  for (std::list<DecisionTree*>::iterator iter=trees->begin();iter!=trees->end();++iter)
  {
    count += (*iter)->getLeafCount();
  }

  return count;
}

void DecisionTree::setCollectionLeafWeight(std::list<DecisionTree*> *trees, int id, float w)
{
  int offset = 0;

  for (std::list<DecisionTree*>::iterator iter=trees->begin();iter!=trees->end();++iter)
  {
    int lc = (*iter)->getLeafCount();
    if (offset <= id && id < (offset+lc))
    {
      (*iter)->setLeafWeight(id-offset,w);
      break;
    }
    offset += lc;
  }
}

float DecisionTree::combineNodeWeights(std::list<DecisionTree::Node*> *nodes)
{
  float weight = 1;
  for (std::list<DecisionTree::Node*>::iterator iter=nodes->begin();iter!=nodes->end();++iter)
  {
    weight *= (*iter)->getWeight();
  }
  return weight;
}

int DecisionTree::getDistance(Go::Board *board, int p1, int p2)
{
  if (p1<0 && p2<0) // both sides
  {
    if ((p1+p2)%2==0) // opposite sides
      return board->getSize();
    else
      return 0;
  }
  else if (p1<0 || p2<0) // one side
  {
    if (p2 < 0)
    {
      int tmp = p1;
      p1 = p2;
      p2 = tmp;
    }
    // p1 is side

    int size = board->getSize();
    int x = Go::Position::pos2x(p2,size);
    int y = Go::Position::pos2y(p2,size);
    switch (p1)
    {
      case -1:
        return x;
      case -2:
        return y;
      case -3:
        return size-x-1;
      case -4:
        return size-y-1;
    }
    return 0;
  }
  else
    return board->getRectDistance(p1,p2);
}

float DecisionTree::getSparseWeight(Go::Board *board, Go::Move move, bool updatetree)
{
  std::vector<int> *stones = new std::vector<int>();
  bool invert = (move.getColor() != Go::BLACK);

  stones->push_back(move.getPosition());

  std::list<DecisionTree::Node*> *nodes = this->getSparseLeafNodes(root,board,stones,invert,updatetree);
  float w = -1;
  if (nodes != NULL)
  {
    w = DecisionTree::combineNodeWeights(nodes);
    delete nodes;
  }

  delete stones;
  return w;
}

bool DecisionTree::updateSparseNode(DecisionTree::Node *node, Go::Board *board, std::vector<int> *stones, bool invert)
{
  std::vector<DecisionTree::StatPerm*> *statperms = node->getStats()->getStatPerms();
  for (unsigned int i=0; i<statperms->size(); i++)
  {
    DecisionTree::StatPerm *sp = statperms->at(i);

    std::string spt = sp->getLabel();
    std::vector<std::string> *attrs = sp->getAttrs();
    int res = 0;
    int resmin = sp->getRange()->getStart();
    int resmax = sp->getRange()->getEnd();
    //fprintf(stderr,"[DT] SP type: '%s'\n",spt.c_str());
    if (spt=="NEW")
    {
      std::string cols = attrs->at(0);
      bool B = (cols.find('B') != std::string::npos);
      bool W = (cols.find('W') != std::string::npos);
      bool S = (cols.find('S') != std::string::npos);
      int center = stones->at(0);

      bool resfound = false;
      for (int s=0; s<resmax; s++)
      {
        if (B || W)
        {
          for (int ii=0; ii<board->getSize(); ii++)
          {
            for (int jj=0; jj<board->getSize(); jj++)
            {
              int p = Go::Position::xy2pos(ii,jj,board->getSize());
              Go::Color col = board->getColor(p);
              if (((invert?W:B) && col==Go::BLACK) || ((invert?B:W) && col==Go::WHITE))
              {
                if (DecisionTree::getDistance(board,center,p) <= s)
                {
                  bool found = false;
                  for (unsigned int iii=0; iii<stones->size(); iii++)
                  {
                    if (stones->at(iii) == p)
                    {
                      found = true;
                      break;
                    }
                  }
                  if (!found)
                  {
                    resfound = true;
                    res = s;
                    break;
                  }
                }
              }
            }
          }
        }
        else if (S)
        {
          for (int p=-1; p>=-4; p--)
          {
            if (DecisionTree::getDistance(board,center,p) <= s)
            {
              bool found = false;
              for (unsigned int ii=0; ii<stones->size(); ii++)
              {
                if (stones->at(ii) == p)
                {
                  found = true;
                  break;
                }
              }
              if (!found)
              {
                resfound = true;
                res = s;
                break;
              }
            }
          }
        }
        else
        {
          fprintf(stderr,"[DT] Error! Unknown attribute: '%s'\n",cols.c_str());
          return false;
        }
        if (resfound)
          break;
      }

      if (!resfound)
        res = resmax;
    }
    else if (spt=="DIST")
    {
      std::vector<std::string> *attrs = sp->getAttrs();
      int n0 = boost::lexical_cast<int>(attrs->at(0));
      int n1 = boost::lexical_cast<int>(attrs->at(1));

      res = DecisionTree::getDistance(board,stones->at(n0),stones->at(n1));
    }
    else if (spt=="ATTR")
    {
      std::vector<std::string> *attrs = sp->getAttrs();
      std::string type = attrs->at(0);
      int n = boost::lexical_cast<int>(attrs->at(1));

      int p = stones->at(n);
      if (p < 0) // side
        res = 0;
      else
      {
        if (type == "SIZE")
        {
          if (board->inGroup(p))
            res = board->getGroup(p)->numOfStones();
          else
            res = 0;
        }
        else if (type == "LIB")
        {
          if (board->inGroup(p))
          {
            if (board->getGroup(p)->inAtari())
              res = 1;
            else
              res = board->getGroup(p)->numOfPseudoLiberties();
          }
          else
            res = 0;
        }
      }
    }
    else
    {
      fprintf(stderr,"[DT] Error! Unknown stat perm type: '%s'\n",spt.c_str());
      return false;
    }

    if (res < resmin)
      res = resmin;
    else if (res > resmax)
      res = resmax;
    sp->getRange()->addVal(res,params->dt_range_divide);
  }

  if (node->isLeaf())
  {
    int descents = statperms->at(0)->getRange()->getThisVal();
    if (descents >= params->dt_split_after && (descents%10)==0)
    {
      //fprintf(stderr,"DT split now!\n");

      std::string bestlabel = "";
      std::vector<std::string> *bestattrs = NULL;
      float bestval = -1;

      for (unsigned int i=0; i<statperms->size(); i++)
      {
        DecisionTree::StatPerm *sp = statperms->at(i);
        DecisionTree::Range *range = sp->getRange();

        float m = range->getExpectedMedian();
        int m1 = floor(m);
        int m2 = ceil(m);
        float p1l = range->getExpectedPercentageLessThan(m1);
        float p1e = range->getExpectedPercentageEquals(m1);
        float p2l = (m1==m2?p1l:range->getExpectedPercentageLessThan(m2));
        float p2e = (m1==m2?p1e:range->getExpectedPercentageEquals(m2));

        float v1l = DecisionTree::percentageToVal(p1l);
        float v2l = DecisionTree::percentageToVal(p2l);
        float v1e = DecisionTree::percentageToVal(p1e);
        float v2e = DecisionTree::percentageToVal(p2e);

        /*fprintf(stderr,"DT stat: %s",sp->getLabel().c_str());
        for (unsigned int j=0; j<sp->getAttrs()->size(); j++)
          fprintf(stderr,"%c%s",(j==0?'[':'|'),sp->getAttrs()->at(j).c_str());
        fprintf(stderr,"] \t %.3f %d %d %.3f %.3f %.3f %.3f\n",m,m1,m2,p1l,p1e,p2l,p2e);*/
        //fprintf(stderr,"DT range:\n%s\n",range->toString().c_str());

        bool foundbest = false;
        int localm = 0;
        bool lne = false;

        if (sp->getLabel()=="NEW")
        {
          if (v1l > bestval)
          {
            bestval = v1l;
            localm = m1-1;
            foundbest = true;
          }
          if (v2l > bestval)
          {
            bestval = v2l;
            localm = m2-1;
            foundbest = true;
          }
        }
        else
        {
          if (v1l > bestval)
          {
            bestval = v1l;
            localm = m1;
            lne = true;
            foundbest = true;
          }
          if (v2l > bestval)
          {
            bestval = v2l;
            localm = m2;
            lne = true;
            foundbest = true;
          }
          if (v1e > bestval)
          {
            bestval = v1e;
            localm = m1;
            lne = false;
            foundbest = true;
          }
          if (v2e > bestval)
          {
            bestval = v2e;
            localm = m2;
            lne = false;
            foundbest = true;
          }
        }

        if (foundbest)
        {
          bestlabel = sp->getLabel();
          if (bestattrs != NULL)
            delete bestattrs;
          std::vector<std::string> *attrs = new std::vector<std::string>();
          for (unsigned int j=0; j<sp->getAttrs()->size(); j++)
          {
            attrs->push_back(sp->getAttrs()->at(j));
          }
          if (bestlabel=="DIST" || bestlabel=="ATTR")
            attrs->push_back(lne?"<":"=");
          attrs->push_back(boost::lexical_cast<std::string>(localm));
          bestattrs = attrs;
        }
      }

      if (bestval != -1)
      {
        if (bestval >= params->dt_split_threshold)
        {
          int maxnode = this->getMaxNode(node);

          DecisionTree::Query *query = new DecisionTree::Query(type,bestlabel,bestattrs,maxnode);
          query->setParent(node);
          node->setQuery(query);

          /*params->engine->getGtpEngine()->getOutput()->printfDebug("DT best stat: %s",bestlabel.c_str());
          for (unsigned int j=0; j<bestattrs->size(); j++)
            params->engine->getGtpEngine()->getOutput()->printfDebug("%c%s",(j==0?'[':'|'),bestattrs->at(j).c_str());
          params->engine->getGtpEngine()->getOutput()->printfDebug("] %.2f\n",bestval);*/
        }
        else
          delete bestattrs;
      }
    }
  }

  return true;
}

float DecisionTree::percentageToVal(float p)
{
  float val = (p-0.5)/0.5;
  if (val < 0)
    val = -val;
  val = 1 - val;
  return val;
}

std::list<DecisionTree::Node*> *DecisionTree::getSparseLeafNodes(DecisionTree::Node *node, Go::Board *board, std::vector<int> *stones, bool invert, bool updatetree)
{
  if (updatetree)
  {
    if (!this->updateSparseNode(node,board,stones,invert))
      return NULL;
  }

  if (node->isLeaf())
  {
    std::list<DecisionTree::Node*> *list = new std::list<DecisionTree::Node*>();
    list->push_back(node);
    return list;
  }

  DecisionTree::Query *q = node->getQuery();

  //TODO: handle compress chains param
  
  if (q->getLabel() == "NEW")
  {
    int center = stones->at(0);

    std::vector<std::string> *attrs = q->getAttrs();
    bool B = (attrs->at(0).find('B') != std::string::npos);
    bool W = (attrs->at(0).find('W') != std::string::npos);
    bool S = (attrs->at(0).find('S') != std::string::npos);
    std::string valstr = attrs->at(1);
    int val = boost::lexical_cast<int>(valstr);

    std::list<int> matches;
    for (int s=0; s<=val; s++)
    {
      //XXX: need more efficient way of finding new nearby points?
      if (B || W)
      {
        for (int i=0; i<board->getSize(); i++)
        {
          for (int j=0; j<board->getSize(); j++)
          {
            int p = Go::Position::xy2pos(i,j,board->getSize());
            Go::Color col = board->getColor(p);
            if (((invert?W:B) && col==Go::BLACK) || ((invert?B:W) && col==Go::WHITE))
            {
              if (DecisionTree::getDistance(board,center,p) <= s)
              {
                bool found = false;
                for (unsigned int i=0; i<stones->size(); i++)
                {
                  if (stones->at(i) == p)
                  {
                    found = true;
                    break;
                  }
                }
                if (!found)
                  matches.push_back(p);
              }
            }
          }
        }
      }
      if (S)
      {
        for (int p=-1; p>=-4; p--)
        {
          if (DecisionTree::getDistance(board,center,p) <= s)
          {
            bool found = false;
            for (unsigned int i=0; i<stones->size(); i++)
            {
              if (stones->at(i) == p)
              {
                found = true;
                break;
              }
            }
            if (!found)
              matches.push_back(p);
          }
        }
      }
      if (matches.size() > 0)
        break;
    }

    if (matches.size() > 1) // try break ties (B...W...S)
    {
      std::list<int> m;
      for (std::list<int>::iterator iter=matches.begin();iter!=matches.end();++iter)
      {
        int p = (*iter);
        if (invert)
        {
          if (p>=0 && board->getColor(p)==Go::WHITE)
            m.push_back(p);
        }
        else
        {
          if (p>=0 && board->getColor(p)==Go::BLACK)
            m.push_back(p);
        }
      }
      if (m.size()==0)
      {
        for (std::list<int>::iterator iter=matches.begin();iter!=matches.end();++iter)
        {
          int p = (*iter);
          if (invert)
          {
            if (p>=0 && board->getColor(p)==Go::BLACK)
              m.push_back(p);
          }
          else
          {
            if (p>=0 && board->getColor(p)==Go::WHITE)
              m.push_back(p);
          }
        }
        if (m.size()==0)
        {
          for (std::list<int>::iterator iter=matches.begin();iter!=matches.end();++iter)
          {
            int p = (*iter);
            if (p<0) // side
              m.push_back(p);
          }
        }
      }
      matches = m;

      if (matches.size() > 1) // still tied (dist to newest node)
      {
        for (int i=stones->size()-1; i>0; i--) // implicitly checked distance to 0'th node
        {
          int mindist = DecisionTree::getDistance(board,stones->at(i),matches.front());
          for (std::list<int>::iterator iter=matches.begin();iter!=matches.end();++iter)
          {
            int dist = DecisionTree::getDistance(board,stones->at(i),(*iter));
            if (dist < mindist)
              mindist = dist;
          }

          std::list<int> m;
          for (std::list<int>::iterator iter=matches.begin();iter!=matches.end();++iter)
          {
            int dist = DecisionTree::getDistance(board,stones->at(i),(*iter));
            if (dist == mindist)
              m.push_back((*iter));
          }
          matches = m;

          if (matches.size() == 1)
            break;
        }

        if (matches.size() > 1) // yet still tied (largest size)
        {
          int maxsize = 0;
          for (std::list<int>::iterator iter=matches.begin();iter!=matches.end();++iter)
          {
            int p = (*iter);
            int size = 0;
            if (p>0 && board->inGroup(p))
              size = board->getGroup(p)->numOfStones();
            if (size > maxsize)
              maxsize = size;
          }

          std::list<int> m;
          for (std::list<int>::iterator iter=matches.begin();iter!=matches.end();++iter)
          {
            int p = (*iter);
            int size = 0;
            if (p>0 && board->inGroup(p))
              size = board->getGroup(p)->numOfStones();
            if (size == maxsize)
              m.push_back(p);
          }
          matches = m;

          if (matches.size() > 1) // yet yet still tied (most liberties)
          {
            int maxlib = 0;
            for (std::list<int>::iterator iter=matches.begin();iter!=matches.end();++iter)
            {
              int p = (*iter);
              int lib = 0;
              if (p>0 && board->inGroup(p))
              {
                if (board->getGroup(p)->inAtari())
                  lib = 1;
                else
                  lib = board->getGroup(p)->numOfPseudoLiberties();
              }
              if (lib > maxlib)
                maxlib = lib;
            }

            std::list<int> m;
            for (std::list<int>::iterator iter=matches.begin();iter!=matches.end();++iter)
            {
              int p = (*iter);
              int lib = 0;
              if (p>0 && board->inGroup(p))
              {
                if (board->getGroup(p)->inAtari())
                  lib = 1;
                else
                  lib = board->getGroup(p)->numOfPseudoLiberties();
              }
              if (lib == maxlib)
                m.push_back(p);
            }
            matches = m;
          }
        }
      }
    }

    //fprintf(stderr,"[DT] matches: %lu\n",matches.size());
    if (matches.size() > 0)
    {
      std::list<DecisionTree::Node*> *nodes = NULL;
      for (std::list<int>::iterator iter=matches.begin();iter!=matches.end();++iter)
      {
        std::list<DecisionTree::Node*> *subnodes = NULL;
        int newpos = (*iter);
        Go::Color col;
        if (newpos < 0) // side
          col = Go::OFFBOARD;
        else
          col = board->getColor(newpos);
        stones->push_back(newpos);

        std::vector<DecisionTree::Option*> *options = q->getOptions();
        for (unsigned int i=0; i<options->size(); i++)
        {
          std::string l = options->at(i)->getLabel();
          if (col==Go::BLACK && l==(invert?"W":"B"))
            subnodes = this->getSparseLeafNodes(options->at(i)->getNode(),board,stones,invert,updatetree);
          else if (col==Go::WHITE && l==(invert?"B":"W"))
            subnodes = this->getSparseLeafNodes(options->at(i)->getNode(),board,stones,invert,updatetree);
          else if (col==Go::OFFBOARD && l=="S")
            subnodes = this->getSparseLeafNodes(options->at(i)->getNode(),board,stones,invert,updatetree);
        }
        stones->pop_back();

        if (subnodes == NULL)
        {
          if (nodes != NULL)
            delete nodes;
          return NULL;
        }

        if (nodes == NULL)
          nodes = subnodes;
        else
        {
          nodes->splice(nodes->begin(),*subnodes);
          delete subnodes;
        }
      }

      /*if (nodes->size()>1)
      {
        int minid = -1;
        for (std::list<DecisionTree::Node*>::iterator iter=nodes->begin();iter!=nodes->end();++iter)
        {
          if (minid==-1 || (*iter)->getLeafId()<minid)
            minid = (*iter)->getLeafId();
        }
        std::list<DecisionTree::Node*> *newnodes = new std::list<DecisionTree::Node*>();;
        for (std::list<DecisionTree::Node*>::iterator iter=nodes->begin();iter!=nodes->end();++iter)
        {
          if ((*iter)->getLeafId()==minid)
          {
            newnodes->push_back((*iter));
            break;
          }
        }
        delete nodes;
        nodes = newnodes;
      }*/
      return nodes;
    }
    else
    {
      std::vector<DecisionTree::Option*> *options = q->getOptions();
      for (unsigned int i=0; i<options->size(); i++)
      {
        std::string l = options->at(i)->getLabel();
        if (l=="N")
          return this->getSparseLeafNodes(options->at(i)->getNode(),board,stones,invert,updatetree);
      }
      return NULL;
    }
  }
  else if (q->getLabel() == "DIST")
  {
    std::vector<std::string> *attrs = q->getAttrs();
    int n0 = boost::lexical_cast<int>(attrs->at(0));
    int n1 = boost::lexical_cast<int>(attrs->at(1));
    bool eq = attrs->at(2) == "=";
    int val = boost::lexical_cast<int>(attrs->at(3));

    int dist = DecisionTree::getDistance(board,stones->at(n0),stones->at(n1));
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
        return this->getSparseLeafNodes(options->at(i)->getNode(),board,stones,invert,updatetree);
      else if (!res && l=="N")
        return this->getSparseLeafNodes(options->at(i)->getNode(),board,stones,invert,updatetree);
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

    int p = stones->at(n);
    int attr = 0;
    if (p < 0) // side
      attr = 0;
    else
    {
      if (type == "SIZE")
      {
        if (board->inGroup(p))
          attr = board->getGroup(p)->numOfStones();
        else
          attr = 0;
      }
      else if (type == "LIB")
      {
        if (board->inGroup(p))
        {
          if (board->getGroup(p)->inAtari())
            attr = 1;
          else
            attr = board->getGroup(p)->numOfPseudoLiberties();
        }
        else
          attr = 0;
      }
    }

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
        return this->getSparseLeafNodes(options->at(i)->getNode(),board,stones,invert,updatetree);
      else if (!res && l=="N")
        return this->getSparseLeafNodes(options->at(i)->getNode(),board,stones,invert,updatetree);
    }
    return NULL;
  }
  else
  {
    fprintf(stderr,"[DT] Error! Invalid query type: '%s'\n",q->getLabel().c_str());
    return NULL;
  }
}

std::string DecisionTree::toString(bool ignorestats)
{
  std::string r = "(DT[";
  for (unsigned int i=0;i<attrs->size();i++)
  {
    if (i!=0)
      r += "|";
    r += attrs->at(i);
  }
  r += "]\n";

  r += root->toString(2,ignorestats);

  r += ")";
  return r;
}

std::string DecisionTree::Node::toString(int indent, bool ignorestats)
{
  std::string r = "";

  if (ignorestats)
  {
    for (int i=0;i<indent;i++)
      r += " ";
    r += "(STATS:)\n";
  }
  else
    r += stats->toString(indent);

  if (query == NULL)
  {
    for (int i=0;i<indent;i++)
      r += " ";
    r += "(WEIGHT[";
    r += boost::lexical_cast<std::string>(weight); //TODO: make sure this is correctly formatted
    r += "])\n";
  }
  else
    r += query->toString(indent,ignorestats);

  return r;
}

std::string DecisionTree::Query::toString(int indent, bool ignorestats)
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
    r += options->at(i)->toString(indent+2,ignorestats);
  }
  
  for (int i=0;i<indent;i++)
    r += " ";
  r += ")\n";

  return r;
}

std::string DecisionTree::Option::toString(int indent, bool ignorestats)
{
  std::string r = "";

  for (int i=0;i<indent;i++)
    r += " ";
  r += label + ":\n";

  r += node->toString(indent+2,ignorestats);

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

std::list<DecisionTree*> *DecisionTree::parseString(Parameters *params, std::string rawdata, unsigned long pos)
{
  std::string data = DecisionTree::stripWhitespace(rawdata);
  std::transform(data.begin(),data.end(),data.begin(),::toupper);
  //fprintf(stderr,"[DT] parsing: '%s'\n",data.c_str());

  pos = data.find("(DT",pos);
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

  Type type;
  std::string typestr = attrs->at(0);
  if (typestr == "SPARSE")
    type = SPARSE;
  else
  {
    fprintf(stderr,"[DT] Error! Invalid type: '%s'\n",typestr.c_str());
    delete attrs;
    return NULL;
  }

  if (attrs->size()!=2 || attrs->at(0)!="SPARSE" || attrs->at(1)!="NOCOMP") //TODO: extend to more tree types
  {
    fprintf(stderr,"[DT] Error! Invalid attributes for tree\n");
    delete attrs;
    return NULL;
  }

  DecisionTree::Node *root = DecisionTree::parseNode(type,data,pos);
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

  root->populateEmptyStats(type);

  DecisionTree *dt = new DecisionTree(params,type,attrs,root);
  
  bool isnext = false;
  if (pos < data.size())
  {
    pos = data.find("(DT",pos);
    if (pos != std::string::npos && pos < data.size())
      isnext = true;
  }
  if (isnext)
  {
    std::list<DecisionTree*> *trees = DecisionTree::parseString(params,data,pos);
    if (trees == NULL)
    {
      delete dt;
      return NULL;
    }
    trees->push_front(dt);
    return trees;
  }
  else
  {
    std::list<DecisionTree*> *trees = new std::list<DecisionTree*>();
    trees->push_back(dt);
    return trees;
  }
}

std::vector<std::string> *DecisionTree::parseAttrs(std::string data, unsigned long &pos)
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

DecisionTree::Node *DecisionTree::parseNode(DecisionTree::Type type, std::string data, unsigned long &pos)
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

    std::vector<DecisionTree::Option*> *options = DecisionTree::parseOptions(type,data,pos);
    if (options == NULL)
    {
      delete stats;
      delete attrs;
      return NULL;
    }

    if (data[pos] != ')')
    {
      fprintf(stderr,"[DT] Error! Expected ')' at '%s'\n",data.substr(pos).c_str());
      delete stats;
      return NULL;
    }
    pos++;

    DecisionTree::Query *query = new DecisionTree::Query(label,attrs,options);
    return new DecisionTree::Node(stats,query);
  }
}

DecisionTree::Stats *DecisionTree::parseStats(std::string data, unsigned long &pos)
{
  if (data.substr(pos,7) != "(STATS:")
  {
    fprintf(stderr,"[DT] Error! Expected '(STATS:' at '%s'\n",data.substr(pos).c_str());
    return NULL;
  }
  pos += 7;

  std::vector<DecisionTree::StatPerm*> *sp;
  if (data[pos] == '(')
  {
    sp = DecisionTree::parseStatPerms(data,pos);
    if (sp == NULL)
      return NULL;
  }
  else
    sp = new std::vector<DecisionTree::StatPerm*>();

  if (data[pos] != ')')
  {
    fprintf(stderr,"[DT] Error! Expected ')' at '%s'\n",data.substr(pos).c_str());
    return NULL;
  }
  pos++;

  return new DecisionTree::Stats(sp);
}

std::vector<DecisionTree::StatPerm*> *DecisionTree::parseStatPerms(std::string data, unsigned long &pos)
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

DecisionTree::Range *DecisionTree::parseRange(std::string data, unsigned long &pos)
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

std::vector<DecisionTree::Option*> *DecisionTree::parseOptions(DecisionTree::Type type, std::string data, unsigned long &pos)
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

  DecisionTree::Node *node = DecisionTree::parseNode(type,data,pos);
  if (node == NULL)
    return NULL;

  DecisionTree::Option *opt = new DecisionTree::Option(label,node);
  if (DecisionTree::isText(data[pos]))
  {
    std::vector<DecisionTree::Option*> *optstail = DecisionTree::parseOptions(type,data,pos);
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

float *DecisionTree::parseNumber(std::string data, unsigned long &pos)
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

std::list<DecisionTree*> *DecisionTree::loadFile(Parameters *params, std::string filename)
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
  
  return DecisionTree::parseString(params,data);
}

bool DecisionTree::saveFile(std::list<DecisionTree*> *trees, std::string filename, bool ignorestats)
{
  std::ofstream fout(filename.c_str());

  if (!fout)
    return false;

  for (std::list<DecisionTree*>::iterator iter=trees->begin();iter!=trees->end();++iter)
  {
    fout << (*iter)->toString(ignorestats) << "\n\n";
  }

  fout.close();
  return true;
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
  parent = NULL;
  stats = s;
  query = q;
  leafid = -1;
  weight = 0;

  query->setParent(this);
}

DecisionTree::Node::Node(Stats *s, float w)
{
  parent = NULL;
  stats = s;
  query = NULL;
  leafid = -1;
  weight = w;
}

DecisionTree::Node::~Node()
{
  delete stats;
  if (query != NULL)
    delete query;
}

void DecisionTree::Node::populateEmptyStats(DecisionTree::Type type, unsigned int maxnode)
{
  if (stats->getStatPerms()->size() == 0)
  {
    delete stats;
    stats = new DecisionTree::Stats(type,maxnode);
  }

  if (query != NULL)
  {
    std::vector<DecisionTree::Option*> *options = query->getOptions();
    for (unsigned int i=0; i<options->size(); i++)
    {
      DecisionTree::Option *opt = options->at(i);
      bool addnode = false;
      switch (type)
      {
        case SPARSE:
          if (query->getLabel()=="NEW" && opt->getLabel()!="N")
            addnode = true;
          break;
      }
      opt->getNode()->populateEmptyStats(type,maxnode+(addnode?1:0));
    }
  }
}

void DecisionTree::Node::populateLeafIds(std::vector<DecisionTree::Node*> &leafmap)
{
  if (this->isLeaf())
  {
    leafid = leafmap.size();
    leafmap.push_back(this);;
  }
  else
  {
    std::vector<DecisionTree::Option*> *options = query->getOptions();
    for (unsigned int i=0; i<options->size(); i++)
    {
      options->at(i)->getNode()->populateLeafIds(leafmap);
    }
  }
}

DecisionTree::Stats::Stats(std::vector<StatPerm*> *sp)
{
  statperms = sp;
}

DecisionTree::Stats::Stats(DecisionTree::Type type, unsigned int maxnode)
{
  switch (type)
  {
    case SPARSE:
      statperms = new std::vector<DecisionTree::StatPerm*>();
      float rangemin = 0;
      float rangemax = 100;

      // NEW
      std::string colslist = "BWS";
      for (unsigned int i=1; i<(1<<colslist.size()); i++)
      {
        std::string cols = "";
        for (unsigned int j=0; j<colslist.size(); j++)
        {
          if ((i>>j)&0x01)
            cols += colslist[j];
        }
        std::vector<std::string> *attrs = new std::vector<std::string>();
        attrs->push_back(cols);
        statperms->push_back(new DecisionTree::StatPerm("NEW",attrs,new DecisionTree::Range(rangemin,rangemax)));
      }

      for (unsigned int i=0; i<=maxnode; i++)
      {
        // DIST
        for (unsigned int j=i+1; j<=maxnode; j++)
        {
          std::vector<std::string> *attrs = new std::vector<std::string>();
          attrs->push_back(boost::lexical_cast<std::string>(i));
          attrs->push_back(boost::lexical_cast<std::string>(j));
          statperms->push_back(new DecisionTree::StatPerm("DIST",attrs,new DecisionTree::Range(rangemin,rangemax)));
        }

        // ATTR
        if (i>0) // don't need to keep stats on 0'th node
        {
          {
            std::vector<std::string> *attrs = new std::vector<std::string>();
            attrs->push_back("LIB");
            attrs->push_back(boost::lexical_cast<std::string>(i));
            statperms->push_back(new DecisionTree::StatPerm("ATTR",attrs,new DecisionTree::Range(rangemin,rangemax)));
          }
          {
            std::vector<std::string> *attrs = new std::vector<std::string>();
            attrs->push_back("SIZE");
            attrs->push_back(boost::lexical_cast<std::string>(i));
            statperms->push_back(new DecisionTree::StatPerm("ATTR",attrs,new DecisionTree::Range(rangemin,rangemax)));
          }
        }
      }

      break;
  }
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

DecisionTree::Range::Range(int s, int e, int v, Range *l, Range *r)
{
  start = s;
  end = e;
  val = v;
  parent = NULL;
  left = l;
  right = r;

  left->setParent(this);
  right->setParent(this);
}

DecisionTree::Range::Range(int s, int e, int v)
{
  start = s;
  end = e;
  val = v;
  parent = NULL;
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

void DecisionTree::Range::addVal(int v, int div)
{
  if (v<start || v>end)
    return;

  if (this->isTerminal() && start!=end && (val>div || this->isRoot()))
  {
    int mid = start + (end - start)/2; //XXX: consider choosing mid according to a log scale
    left = new DecisionTree::Range(start,mid);
    right = new DecisionTree::Range(mid+1,end);
    left->setParent(this);
    right->setParent(this);
  }

  val++;

  if (left!=NULL)
    left->addVal(v,div);
  if (right!=NULL)
    right->addVal(v,div);
}

float DecisionTree::Range::getExpectedMedian(float vl, float vr)
{
  if (left==NULL || right==NULL)
  {
    float t = vl + val + vr;
    float rem = 0.5 - vl/t;
    if (rem <= 0)
      return start;
    else if (rem >= 1)
      return end;
    else
    {
      float p = rem * val/t;
      return start + p*(end-start);
    }
  }
  else
  {
    float s = 1.0f * (left->val + right->val)/val; // scale factor for next level
    float l = s*vl + left->val; // left val relative to next level
    float r = s*vr + right->val; // right val relative to next level
    if (l == r)
      return start + 0.5f*(end-start);
    else if (l > r)
      return left->getExpectedMedian(s*vl,r);
    else
      return right->getExpectedMedian(l,s*vr);
  }
}

float DecisionTree::Range::getExpectedPercentageLessThan(int v)
{
  if (left==NULL || right==NULL)
  {
    if (v <= start)
      return 0;
    else if (v > end)
      return 1;
    else
      return 1.0f * (v-start)/(end-start+1);
  }
  else
  {
    int t = left->val + right->val;
    if (t==0)
      return 1.0f * (v-start)/(end-start+1);

    if (v <= left->end)
    {
      float p = left->getExpectedPercentageLessThan(v);
      return p * left->val/t;
    }
    else if (v >= right->start)
    {
      float p = right->getExpectedPercentageLessThan(v);
      return 1.0f * left->val/t + p * right->val/t;
    }
    else
      return 1.0f * (v-start)/(end-start+1);
  }
}

float DecisionTree::Range::getExpectedPercentageEquals(int v)
{
  if (left==NULL || right==NULL)
  {
    if (v < start)
      return 0;
    else if (v > end)
      return 0;
    else
      return 1.0f/(end-start+1);
  }
  else
  {
    int t = left->val + right->val;
    if (t==0)
      return 1.0f/(end-start+1);

    if (v <= left->end)
    {
      float p = left->getExpectedPercentageEquals(v);
      return p * left->val/t;
    }
    else if (v >= right->start)
    {
      float p = right->getExpectedPercentageEquals(v);
      return p * right->val/t;
    }
    else
      return 1.0f/(end-start+1);
  }
}

DecisionTree::Query::Query(std::string l, std::vector<std::string> *a, std::vector<Option*> *o)
{
  parent = NULL;
  label = l;
  attrs = a;
  options = o;

  for (unsigned int i=0;i<options->size();i++)
  {
    options->at(i)->setParent(this);
  }
}

DecisionTree::Query::Query(DecisionTree::Type type, std::string l, std::vector<std::string> *a, unsigned int maxnode)
{
  parent = NULL;
  label = l;
  attrs = a;

  options = new std::vector<DecisionTree::Option*>();
  switch (type)
  {
    case SPARSE:
      if (l=="NEW")
      {
        bool B = (attrs->at(0).find('B') != std::string::npos);
        bool W = (attrs->at(0).find('W') != std::string::npos);
        bool S = (attrs->at(0).find('S') != std::string::npos);
        if (B)
          options->push_back(new DecisionTree::Option(type,"B",maxnode+1));
        if (W)
          options->push_back(new DecisionTree::Option(type,"W",maxnode+1));
        if (S)
          options->push_back(new DecisionTree::Option(type,"S",maxnode+1));
        options->push_back(new DecisionTree::Option(type,"N",maxnode));
      }
      else if (l=="DIST" || l=="ATTR")
      {
        options->push_back(new DecisionTree::Option(type,"Y",maxnode));
        options->push_back(new DecisionTree::Option(type,"N",maxnode));
      }
      break;
  }

  for (unsigned int i=0;i<options->size();i++)
  {
    options->at(i)->setParent(this);
  }
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
  parent = NULL;
  label = l;
  node = n;
  node->setParent(this);
}

DecisionTree::Option::Option(DecisionTree::Type type, std::string l, unsigned int maxnode)
{
  parent = NULL;
  label = l;
  node = new DecisionTree::Node(new DecisionTree::Stats(type,maxnode),1);
  node->setParent(this);
}

DecisionTree::Option::~Option()
{
  delete node;
}

