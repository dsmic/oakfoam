#include "DecisionTree.h"

#include <cstdio>
#include <cmath>
#include <limits>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <map>
#include <boost/lexical_cast.hpp>
#include "Parameters.h"
#include "Engine.h"

#define TEXT "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789<>=!."
#define WHITESPACE " \t\r\n"
#define COMMENT "#"

DecisionTree::DecisionTree(Parameters *p, DecisionTree::Type t, bool cmpC, bool cmpE, std::vector<std::string> *a, DecisionTree::Node *r)
{
  params = p;
  type = t;
  compressChain = cmpC;
  compressEmpty = cmpE;
  attrs = a;
  root = r;
  statsallocated = false;
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

std::string DecisionTree::getLeafPath(int id)
{
  //XXX: no error checking here
  return leafmap[id]->getPath();
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
    case STONE:
    case INTERSECTION:
      if (query->getLabel()=="NEW" && opt->getLabel()!="N")
        addnode = true;
      break;
  }
  
  DecisionTree::Node *topnode = query->getParent();
  return this->getMaxNode(topnode) + (addnode?1:0);
}

float DecisionTree::getWeight(DecisionTree::GraphCollection *graphs, Go::Move move, bool updatetree, bool win)
{
  if (!graphs->getBoard()->validMove(move) || !move.isNormal())
    return -1;

  std::list<DecisionTree::Node*> *nodes = this->getLeafNodes(graphs,move,updatetree,win);
  float w = -1;
  if (nodes != NULL)
  {
    w = DecisionTree::combineNodeWeights(nodes);
    delete nodes;
  }

  return w;
}

void DecisionTree::updateDescent(DecisionTree::GraphCollection *graphs, Go::Move move, bool win)
{
  std::list<DecisionTree::Node*> *nodes = this->getLeafNodes(graphs,move,true,win);
  if (nodes != NULL)
    delete nodes;
}

void DecisionTree::updateDescent(DecisionTree::GraphCollection *graphs, Go::Move winmove)
{
  Go::Board *board = graphs->getBoard();
  Go::Color col = board->nextToMove();
  for (int p=0; p<board->getPositionMax(); p++)
  {
    Go::Move move = Go::Move(col,p);
    if (board->validMove(move))
      this->updateDescent(graphs,move,(move==winmove));
  }
}

void DecisionTree::collectionUpdateDescent(std::list<DecisionTree*> *trees, DecisionTree::GraphCollection *graphs, Go::Move winmove)
{
  for (std::list<DecisionTree*>::iterator iter=trees->begin();iter!=trees->end();++iter)
  {
    (*iter)->updateDescent(graphs,winmove);
  }
}

float DecisionTree::getCollectionWeight(std::list<DecisionTree*> *trees, DecisionTree::GraphCollection *graphs, Go::Move move, bool updatetree)
{
  float weight = 1;

  if (!graphs->getBoard()->validMove(move) || !move.isNormal())
    return -1;

  for (std::list<DecisionTree*>::iterator iter=trees->begin();iter!=trees->end();++iter)
  {
    float w = (*iter)->getWeight(graphs, move, updatetree);
    if (w == -1)
      return -1;
    weight *= w;
  }

  return weight;
}

std::list<int> *DecisionTree::getLeafIds(DecisionTree::GraphCollection *graphs, Go::Move move)
{
  if (!graphs->getBoard()->validMove(move))
    return NULL;

  std::list<DecisionTree::Node*> *nodes = this->getLeafNodes(graphs,move,false,false);
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

std::list<int> *DecisionTree::getCollectionLeafIds(std::list<DecisionTree*> *trees, DecisionTree::GraphCollection *graphs, Go::Move move)
{
  std::list<int> *collids = new std::list<int>();
  int offset = 0;

  for (std::list<DecisionTree*>::iterator iter=trees->begin();iter!=trees->end();++iter)
  {
    std::list<int> *ids = (*iter)->getLeafIds(graphs, move);
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

std::string DecisionTree::getCollectionLeafPath(std::list<DecisionTree*> *trees, int id)
{
  int offset = 0;

  for (std::list<DecisionTree*>::iterator iter=trees->begin();iter!=trees->end();++iter)
  {
    int lc = (*iter)->getLeafCount();
    if (offset <= id && id < (offset+lc))
      return (*iter)->getLeafPath(id-offset);
    offset += lc;
  }

  return "";
}

DecisionTree::GraphTypes DecisionTree::getCollectionTypes(std::list<DecisionTree*> *trees)
{
  DecisionTree::GraphTypes types;

  types.stoneNone = false;
  types.stoneChain = false;
  types.intersectionNone = false;
  types.intersectionChain = false;
  types.intersectionEmpty = false;
  types.intersectionBoth = false;

  for (std::list<DecisionTree*>::iterator iter=trees->begin();iter!=trees->end();++iter)
  {
    DecisionTree *tree = (*iter);
    switch (tree->getType())
    {
      case STONE:
        {
          if (tree->getCompressChain())
            types.stoneChain = true;
          else
            types.stoneNone = true;
          break;
        }
      case INTERSECTION:
        {
          if (tree->getCompressChain())
          {
            if (tree->getCompressEmpty())
              types.intersectionBoth = true;
            else
              types.intersectionChain = true;
          }
          else
          {
            if (tree->getCompressEmpty())
              types.intersectionEmpty = true;
            else
              types.intersectionNone = true;
          }
          break;
        }
    }
  }

  return types;
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

void DecisionTree::getTreeStats(int &treenodes, int &leaves, int &maxdepth, int &sumdepth, int &sumsqdepth, int &maxnodes, int &sumnodes, int &sumsqnodes, float &sumlogweights, float &sumsqlogweights)
{
  treenodes = 0;
  leaves = 0;
  maxdepth = 0;
  sumdepth = 0;
  sumsqdepth = 0;
  maxnodes = 0;
  sumnodes = 0;
  sumsqnodes = 0;
  sumlogweights = 0;
  sumsqlogweights = 0;

  root->getTreeStats(type,1,1,treenodes,leaves,maxdepth,sumdepth,sumsqdepth,maxnodes,sumnodes,sumsqnodes,sumlogweights,sumsqlogweights);
}

void DecisionTree::Node::getTreeStats(DecisionTree::Type type, int depth, int nodes, int &treenodes, int &leaves, int &maxdepth, int &sumdepth, int &sumsqdepth, int &maxnodes, int &sumnodes, int &sumsqnodes, float &sumlogweights, float &sumsqlogweights)
{
  treenodes++;

  if (this->isLeaf())
  {
    leaves++;

    if (depth > maxdepth)
      maxdepth = depth;
    sumdepth += depth;
    sumsqdepth += depth*depth;

    if (nodes > maxnodes)
      maxnodes = nodes;
    sumnodes += nodes;
    sumsqnodes += nodes*nodes;

    float logweight = log(weight);
    sumlogweights += logweight;
    sumsqlogweights += logweight*logweight;
  }
  else if (query!=NULL)
  {
    std::vector<DecisionTree::Option*> *options = query->getOptions();
    for (unsigned int i=0; i<options->size(); i++)
    {
      DecisionTree::Option *opt = options->at(i);
      bool addnode = false;
      switch (type)
      {
        case STONE:
        case INTERSECTION:
          if (query->getLabel()=="NEW" && opt->getLabel()!="N")
            addnode = true;
          break;
      }
      opt->getNode()->getTreeStats(type,depth+1,nodes+(addnode?1:0),treenodes,leaves,maxdepth,sumdepth,sumsqdepth,maxnodes,sumnodes,sumsqnodes,sumlogweights,sumsqlogweights);
    }
  }
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

bool DecisionTree::updateStoneNode(DecisionTree::Node *node, DecisionTree::StoneGraph *graph, std::vector<unsigned int> *stones, bool invert, bool win)
{
  if (node->isLeaf()) // only update stats until the node is split
  {
    std::vector<DecisionTree::StatPerm*> *statperms = node->getStats()->getStatPerms();
    for (unsigned int i=0; i<statperms->size(); i++)
    {
      DecisionTree::StatPerm *sp = statperms->at(i);

      std::string spt = sp->getLabel();
      std::vector<std::string> *attrs = sp->getAttrs();
      int res = 0;
      int res2 = -1;
      int res3 = -1;
      int resmin = sp->getDescents()->getStart();
      int resmax = sp->getDescents()->getEnd();

      //fprintf(stderr,"[DT] SP type: '%s'\n",spt.c_str());
      if (spt=="NEW")
      {
        unsigned int auxnode = stones->at(0);

        std::string cols = attrs->at(0);
        bool B = (cols.find('B') != std::string::npos);
        bool W = (cols.find('W') != std::string::npos);
        bool S = (cols.find('S') != std::string::npos);
        if (!B && !W && !S)
        {
          fprintf(stderr,"[DT] Error! Unknown attribute: '%s'\n",cols.c_str());
          return false;
        }

        bool resfoundB = false;
        bool resfoundW = false;
        bool resfoundS = false;
        int resB = resmax;
        int resW = resmax;
        int resS = resmax;
        for (int s=0; s<=resmax; s++)
        {
          for (unsigned int i=0; i<graph->getNumNodes(); i++)
          {
            bool valid = false;
            Go::Color col = graph->getNodeStatus(i);
            if ((invert?W:B) && col==Go::BLACK)
              valid = true;
            else if ((invert?B:W) && col==Go::WHITE)
              valid = true;
            else if (S && col==Go::OFFBOARD)
              valid = true;

            if (valid)
            {
              if (graph->getEdgeWeight(auxnode,i) <= s)
              {
                bool found = false;
                for (unsigned int j=0; j<stones->size(); j++)
                {
                  if (stones->at(j) == i)
                  {
                    found = true;
                    break;
                  }
                }
                if (!found)
                {
                  if (!resfoundB && col==(invert?Go::WHITE:Go::BLACK))
                  {
                    resfoundB = true;
                    resB = s;
                  }
                  else if (!resfoundW && col==(invert?Go::BLACK:Go::WHITE))
                  {
                    resfoundW = true;
                    resW = s;
                  }
                  else if (!resfoundS && col==Go::OFFBOARD)
                  {
                    resfoundS = true;
                    resS = s;
                  }
                }
              }
            }
          }

          if (resfoundB && resfoundW && resfoundS)
            break;
        }

        if (!resfoundB)
          resB = resmax;
        if (!resfoundW)
          resW = resmax;
        if (!resfoundS)
          resS = resmax;

        if (resB <= resW)
          resW = resmax;
        if (resW <= resS)
          resS = resmax;

        if (B && W)
        {
          res = resB;
          res2 = resW;
          if (S)
            res3 = resS;
        }
        else if (B)
        {
          res = resB;
          if (S)
            res2 = resS;
        }
        else if (W)
        {
          res = resW;
          if (S)
            res2 = resS;
        }
        else if (S)
          res = resS;
      }
      else if (spt=="DIST")
      {
        std::vector<std::string> *attrs = sp->getAttrs();
        int n0 = boost::lexical_cast<int>(attrs->at(0));
        int n1 = boost::lexical_cast<int>(attrs->at(1));

        res = graph->getEdgeWeight(stones->at(n0),stones->at(n1));
      }
      else if (spt=="ATTR")
      {
        std::vector<std::string> *attrs = sp->getAttrs();
        std::string type = attrs->at(0);
        int n = boost::lexical_cast<int>(attrs->at(1));

        int node = stones->at(n);
        if (type == "SIZE")
          res = graph->getNodeSize(node);
        else if (type == "LIB")
          res = graph->getNodeLiberties(node);
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

      sp->getDescents()->addVal(res);
      if (win)
        sp->getWins()->addVal(res);
      if (res2 != -1)
      {
        sp->getDescents2()->addVal(res2);
        if (win)
          sp->getWins2()->addVal(res2);
      }
      if (res3 != -1)
      {
        sp->getDescents3()->addVal(res3);
        if (win)
          sp->getWins3()->addVal(res3);
      }
    }
  }

  if (node->isLeaf())
  {
    std::vector<DecisionTree::StatPerm*> *statperms = node->getStats()->getStatPerms();
    int descents = statperms->at(0)->getDescents()->getSum();
    if (descents >= params->dt_split_after && (descents%10)==0)
    {
      //fprintf(stderr,"DT split now!\n");

      int maxnode = this->getMaxNode(node);
      DecisionTree::Query *query = node->getStats()->getBestQuery(params,type,maxnode);
      if (query != NULL)
      {
        query->setParent(node);
        node->setQuery(query);

        // free stats memory
        node->clearStats();
      }
    }
  }

  return true;
}

bool DecisionTree::updateIntersectionNode(DecisionTree::Node *node, DecisionTree::IntersectionGraph *graph, std::vector<unsigned int> *stones, bool invert, bool win)
{
  if (node->isLeaf()) // only update stats until the node is split
  {
    std::vector<DecisionTree::StatPerm*> *statperms = node->getStats()->getStatPerms();
    for (unsigned int i=0; i<statperms->size(); i++)
    {
      DecisionTree::StatPerm *sp = statperms->at(i);

      std::string spt = sp->getLabel();
      std::vector<std::string> *attrs = sp->getAttrs();
      int res = 0;
      int res2 = -1;
      int res3 = -1;
      int resmin = sp->getDescents()->getStart();
      int resmax = sp->getDescents()->getEnd();

      //fprintf(stderr,"[DT] SP type: '%s'\n",spt.c_str());
      if (spt=="NEW")
      {
        if (resmin!=0 || resmax!=1)
        {
          fprintf(stderr,"[DT] Error! Bad range (%d,%d), expected (0,1)\n",resmin,resmax);
          return false;
        }

        std::string cols = attrs->at(0);
        bool B = (cols.find('B') != std::string::npos);
        bool W = (cols.find('W') != std::string::npos);
        bool E = (cols.find('E') != std::string::npos);
        if (!B && !W && !E)
        {
          fprintf(stderr,"[DT] Error! Unknown attribute: '%s'\n",cols.c_str());
          return false;
        }
        int n = boost::lexical_cast<int>(attrs->at(1));

        bool resfoundB = false;
        bool resfoundW = false;
        bool resfoundE = false;
        std::vector<unsigned int> *adjnodes = graph->getAdjacentNodes(stones->at(n));
        if (adjnodes != NULL)
        {
          for (unsigned int i=0; i<adjnodes->size(); i++)
          {
            unsigned int adjn = adjnodes->at(i);

            bool valid = false;
            Go::Color col = graph->getNodeStatus(adjn);
            if ((invert?W:B) && col==Go::BLACK)
              valid = true;
            else if ((invert?B:W) && col==Go::WHITE)
              valid = true;
            else if (E && col==Go::EMPTY)
              valid = true;

            if (valid)
            {
              bool found = false;
              for (unsigned int j=0; j<stones->size(); j++)
              {
                if (stones->at(j) == adjn)
                {
                  found = true;
                  break;
                }
              }
              if (!found)
              {
                if (!resfoundB && col==(invert?Go::WHITE:Go::BLACK))
                  resfoundB = true;
                else if (!resfoundW && col==(invert?Go::BLACK:Go::WHITE))
                  resfoundW = true;
                else if (!resfoundE && col==Go::EMPTY)
                  resfoundE = true;
              }
            }

            if (resfoundB && resfoundW && resfoundE)
              break;
          }
        }

        if (adjnodes != NULL)
          delete adjnodes;

        if (B)
        {
          res = resfoundB?1:0;
          if (W)
          {
            res2 = resfoundW?1-res:0;
            if (E)
              res3 = resfoundE?1-res-res2:0;
          }
          else if (E)
            res2 = resfoundE?1-res:0;
        }
        else if (W)
        {
          res = resfoundW?1:0;
          if (E)
            res2 = resfoundE?1-res:0;
        }
        else if (E)
          res = resfoundE?1:0;
      }
      else if (spt=="EDGE")
      {
        std::vector<std::string> *attrs = sp->getAttrs();
        std::string type = attrs->at(0);
        int n0 = boost::lexical_cast<int>(attrs->at(1));
        int n1 = boost::lexical_cast<int>(attrs->at(2));

        if (type == "DIST")
          res = graph->getEdgeDistance(stones->at(n0),stones->at(n1));
        else if (type == "CONN")
          res = graph->getEdgeConnectivity(stones->at(n0),stones->at(n1));
      }
      else if (spt=="ATTR")
      {
        std::vector<std::string> *attrs = sp->getAttrs();
        std::string type = attrs->at(0);
        int n = boost::lexical_cast<int>(attrs->at(1));

        int node = stones->at(n);
        if (type == "SIZE")
          res = graph->getNodeSize(node);
        else if (type == "LIB")
          res = graph->getNodeLiberties(node);
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

      sp->getDescents()->addVal(res);
      if (win)
        sp->getWins()->addVal(res);
      if (res2 != -1)
      {
        sp->getDescents2()->addVal(res2);
        if (win)
          sp->getWins2()->addVal(res2);
      }
      if (res3 != -1)
      {
        sp->getDescents3()->addVal(res3);
        if (win)
          sp->getWins3()->addVal(res3);
      }
    }
  }

  if (node->isLeaf())
  {
    std::vector<DecisionTree::StatPerm*> *statperms = node->getStats()->getStatPerms();
    int descents = statperms->at(0)->getDescents()->getSum();
    if (descents >= params->dt_split_after && (descents%10)==0)
    {
      //fprintf(stderr,"DT split now!\n");

      int maxnode = this->getMaxNode(node);
      DecisionTree::Query *query = node->getStats()->getBestQuery(params,type,maxnode);
      if (query != NULL)
      {
        query->setParent(node);
        node->setQuery(query);

        // free stats memory
        node->clearStats();
      }
    }
  }

  return true;
}

std::list<DecisionTree::Node*> *DecisionTree::getLeafNodes(DecisionTree::GraphCollection *graphs, Go::Move move, bool updatetree, bool win)
{
  if (!move.isNormal())
    return NULL;

  if (updatetree && !statsallocated)
  {
    root->populateEmptyStats(this->getType());
    statsallocated = true;
  }

  std::list<DecisionTree::Node*> *nodes = NULL;

  std::vector<unsigned int> *stones = new std::vector<unsigned int>();
  bool invert = (move.getColor() != Go::BLACK);

  switch (type)
  {
    case STONE:
      {
        DecisionTree::StoneGraph *graph = graphs->getStoneGraph(this->getCompressChain());

        if (move.getPosition()!=graph->getAuxPos())
        {
          if (graph->getAuxNode()!=(unsigned int)-1)
            graph->removeAuxNode();

          graph->addAuxNode(move.getPosition());
        }
        unsigned int auxnode = graph->getAuxNode();

        stones->push_back(auxnode);
        nodes = this->getStoneLeafNodes(root,graph,stones,invert,updatetree,win);

        break;
      }

    case INTERSECTION:
      {
        DecisionTree::IntersectionGraph *graph = graphs->getIntersectionGraph(this->getCompressChain(), this->getCompressEmpty());

        if (move.getPosition()!=graph->getAuxPos())
        {
          if (graph->getAuxNode()!=(unsigned int)-1)
            graph->removeAuxNode();

          graph->addAuxNode(move.getPosition());
        }
        unsigned int auxnode = graph->getAuxNode();

        stones->push_back(auxnode);
        nodes = this->getIntersectionLeafNodes(root,graph,stones,invert,updatetree,win);

        break;
      }
  }

  delete stones;

  if (nodes!=NULL && params->dt_solo_leaf)
  {
    if (nodes->size()>1)
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
    }
  }

  return nodes;
}

std::list<DecisionTree::Node*> *DecisionTree::getStoneLeafNodes(DecisionTree::Node *node, DecisionTree::StoneGraph *graph, std::vector<unsigned int> *stones, bool invert, bool updatetree, bool win)
{
  if (updatetree)
  {
    if (!this->updateStoneNode(node,graph,stones,invert,win))
      return NULL;
  }

  if (node->isLeaf())
  {
    std::list<DecisionTree::Node*> *list = new std::list<DecisionTree::Node*>();
    list->push_back(node);
    return list;
  }

  DecisionTree::Query *q = node->getQuery();
  
  if (q->getLabel() == "NEW")
  {
    unsigned int auxnode = stones->at(0);

    std::vector<std::string> *attrs = q->getAttrs();
    bool B = (attrs->at(0).find('B') != std::string::npos);
    bool W = (attrs->at(0).find('W') != std::string::npos);
    bool S = (attrs->at(0).find('S') != std::string::npos);
    std::string valstr = attrs->at(1);
    int val = boost::lexical_cast<int>(valstr);

    std::list<unsigned int> matches;
    for (int s=0; s<=val; s++)
    {
      for (unsigned int i=0; i<graph->getNumNodes(); i++)
      {
        bool valid = false;
        if ((invert?W:B) && graph->getNodeStatus(i)==Go::BLACK)
          valid = true;
        else if ((invert?B:W) && graph->getNodeStatus(i)==Go::WHITE)
          valid = true;
        else if (S && graph->getNodeStatus(i)==Go::OFFBOARD)
          valid = true;

        if (valid)
        {
          if (graph->getEdgeWeight(auxnode,i) <= s)
          {
            bool found = false;
            for (unsigned int j=0; j<stones->size(); j++)
            {
              if (stones->at(j) == i)
              {
                found = true;
                break;
              }
            }
            if (!found)
              matches.push_back(i);
          }
        }
      }

      if (matches.size() > 0)
        break;
    }

    if (matches.size() > 1) // try break ties (B...W...S)
    {
      std::list<unsigned int> m;
      for (std::list<unsigned int>::iterator iter=matches.begin();iter!=matches.end();++iter)
      {
        unsigned int n = (*iter);
        if (invert)
        {
          if (graph->getNodeStatus(n)==Go::WHITE)
            m.push_back(n);
        }
        else
        {
          if (graph->getNodeStatus(n)==Go::BLACK)
            m.push_back(n);
        }
      }
      if (m.size()==0)
      {
        for (std::list<unsigned int>::iterator iter=matches.begin();iter!=matches.end();++iter)
        {
          unsigned int n = (*iter);
          if (invert)
          {
            if (graph->getNodeStatus(n)==Go::BLACK)
              m.push_back(n);
          }
          else
          {
            if (graph->getNodeStatus(n)==Go::WHITE)
              m.push_back(n);
          }
        }
        if (m.size()==0)
        {
          for (std::list<unsigned int>::iterator iter=matches.begin();iter!=matches.end();++iter)
          {
            unsigned int n = (*iter);
            if (graph->getNodeStatus(n)==Go::OFFBOARD)
              m.push_back(n);
          }
        }
      }
      matches = m;

      if (matches.size() > 1) // still tied (dist to newest node)
      {
        for (unsigned int i=stones->size()-1; i>0; i--) // implicitly checked distance to 0'th node
        {
          int mindist = graph->getEdgeWeight(stones->at(i),matches.front());
          for (std::list<unsigned int>::iterator iter=matches.begin();iter!=matches.end();++iter)
          {
            int dist = graph->getEdgeWeight(stones->at(i),(*iter));
            if (dist < mindist)
              mindist = dist;
          }

          std::list<unsigned int> m;
          for (std::list<unsigned int>::iterator iter=matches.begin();iter!=matches.end();++iter)
          {
            int dist = graph->getEdgeWeight(stones->at(i),(*iter));
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
          for (std::list<unsigned int>::iterator iter=matches.begin();iter!=matches.end();++iter)
          {
            int size = graph->getNodeSize((*iter));
            if (size > maxsize)
              maxsize = size;
          }

          std::list<unsigned int> m;
          for (std::list<unsigned int>::iterator iter=matches.begin();iter!=matches.end();++iter)
          {
            int size = graph->getNodeSize((*iter));
            if (size == maxsize)
              m.push_back((*iter));
          }
          matches = m;

          if (matches.size() > 1) // yet yet still tied (most liberties)
          {
            int maxlib = 0;
            for (std::list<unsigned int>::iterator iter=matches.begin();iter!=matches.end();++iter)
            {
              int lib = graph->getNodeLiberties((*iter));
              if (lib > maxlib)
                maxlib = lib;
            }

            std::list<unsigned int> m;
            for (std::list<unsigned int>::iterator iter=matches.begin();iter!=matches.end();++iter)
            {
              int lib = graph->getNodeLiberties((*iter));
              if (lib == maxlib)
                m.push_back((*iter));
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
      for (std::list<unsigned int>::iterator iter=matches.begin();iter!=matches.end();++iter)
      {
        std::list<DecisionTree::Node*> *subnodes = NULL;
        unsigned int newnode = (*iter);
        Go::Color col = graph->getNodeStatus(newnode);
        stones->push_back(newnode);

        std::vector<DecisionTree::Option*> *options = q->getOptions();
        for (unsigned int i=0; i<options->size(); i++)
        {
          std::string l = options->at(i)->getLabel();
          if (col==Go::BLACK && l==(invert?"W":"B"))
            subnodes = this->getStoneLeafNodes(options->at(i)->getNode(),graph,stones,invert,updatetree,win);
          else if (col==Go::WHITE && l==(invert?"B":"W"))
            subnodes = this->getStoneLeafNodes(options->at(i)->getNode(),graph,stones,invert,updatetree,win);
          else if (col==Go::OFFBOARD && l=="S")
            subnodes = this->getStoneLeafNodes(options->at(i)->getNode(),graph,stones,invert,updatetree,win);
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

      return nodes;
    }
    else
    {
      std::vector<DecisionTree::Option*> *options = q->getOptions();
      for (unsigned int i=0; i<options->size(); i++)
      {
        std::string l = options->at(i)->getLabel();
        if (l=="N")
          return this->getStoneLeafNodes(options->at(i)->getNode(),graph,stones,invert,updatetree,win);
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

    int dist = graph->getEdgeWeight(stones->at(n0),stones->at(n1));
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
        return this->getStoneLeafNodes(options->at(i)->getNode(),graph,stones,invert,updatetree,win);
      else if (!res && l=="N")
        return this->getStoneLeafNodes(options->at(i)->getNode(),graph,stones,invert,updatetree,win);
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

    unsigned int node = stones->at(n);
    int attr = 0;
    if (type == "SIZE")
      attr = graph->getNodeSize(node);
    else if (type == "LIB")
      attr = graph->getNodeLiberties(node);

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
        return this->getStoneLeafNodes(options->at(i)->getNode(),graph,stones,invert,updatetree,win);
      else if (!res && l=="N")
        return this->getStoneLeafNodes(options->at(i)->getNode(),graph,stones,invert,updatetree,win);
    }
    return NULL;
  }
  else
  {
    fprintf(stderr,"[DT] Error! Invalid query type: '%s'\n",q->getLabel().c_str());
    return NULL;
  }
}

std::list<DecisionTree::Node*> *DecisionTree::getIntersectionLeafNodes(DecisionTree::Node *node, DecisionTree::IntersectionGraph *graph, std::vector<unsigned int> *stones, bool invert, bool updatetree, bool win)
{
  if (updatetree)
  {
    if (!this->updateIntersectionNode(node,graph,stones,invert,win))
      return NULL;
  }

  if (node->isLeaf())
  {
    std::list<DecisionTree::Node*> *list = new std::list<DecisionTree::Node*>();
    list->push_back(node);
    return list;
  }

  DecisionTree::Query *q = node->getQuery();
  
  if (q->getLabel() == "NEW")
  {
    std::vector<std::string> *attrs = q->getAttrs();
    bool B = (attrs->at(0).find('B') != std::string::npos);
    bool W = (attrs->at(0).find('W') != std::string::npos);
    bool E = (attrs->at(0).find('E') != std::string::npos);
    int n = boost::lexical_cast<int>(attrs->at(1));

    std::list<unsigned int> matches;
    std::vector<unsigned int> *adjnodes = graph->getAdjacentNodes(stones->at(n));
    if (adjnodes != NULL)
    {
      for (unsigned int i=0; i<adjnodes->size(); i++)
      {
        unsigned int adjn = adjnodes->at(i);

        bool valid = false;
        if ((invert?W:B) && graph->getNodeStatus(adjn)==Go::BLACK)
          valid = true;
        else if ((invert?B:W) && graph->getNodeStatus(adjn)==Go::WHITE)
          valid = true;
        else if (E && graph->getNodeStatus(adjn)==Go::EMPTY)
          valid = true;

        if (valid)
        {
          bool found = false;
          for (unsigned int j=0; j<stones->size(); j++)
          {
            if (stones->at(j) == adjn)
            {
              found = true;
              break;
            }
          }
          if (!found)
            matches.push_back(adjn);
        }
      }
      delete adjnodes;
    }

    if (matches.size() > 1) // try break ties (B...W...E)
    {
      std::list<unsigned int> m;
      for (std::list<unsigned int>::iterator iter=matches.begin();iter!=matches.end();++iter)
      {
        unsigned int n = (*iter);
        if (invert)
        {
          if (graph->getNodeStatus(n)==Go::WHITE)
            m.push_back(n);
        }
        else
        {
          if (graph->getNodeStatus(n)==Go::BLACK)
            m.push_back(n);
        }
      }
      if (m.size()==0)
      {
        for (std::list<unsigned int>::iterator iter=matches.begin();iter!=matches.end();++iter)
        {
          unsigned int n = (*iter);
          if (invert)
          {
            if (graph->getNodeStatus(n)==Go::BLACK)
              m.push_back(n);
          }
          else
          {
            if (graph->getNodeStatus(n)==Go::WHITE)
              m.push_back(n);
          }
        }
        if (m.size()==0)
        {
          for (std::list<unsigned int>::iterator iter=matches.begin();iter!=matches.end();++iter)
          {
            unsigned int n = (*iter);
            if (graph->getNodeStatus(n)==Go::EMPTY)
              m.push_back(n);
          }
        }
      }
      matches = m;

      if (matches.size() > 1) // still tied (min dist to newest node)
      {
        for (unsigned int i=stones->size()-1; i>=0 && i!=(unsigned int)-1; i--)
        {
          int mindist = graph->getEdgeDistance(stones->at(i),matches.front());
          for (std::list<unsigned int>::iterator iter=matches.begin();iter!=matches.end();++iter)
          {
            int dist = graph->getEdgeDistance(stones->at(i),(*iter));
            if (dist < mindist)
              mindist = dist;
          }

          std::list<unsigned int> m;
          for (std::list<unsigned int>::iterator iter=matches.begin();iter!=matches.end();++iter)
          {
            int dist = graph->getEdgeDistance(stones->at(i),(*iter));
            if (dist == mindist)
              m.push_back((*iter));
          }
          matches = m;

          if (matches.size() == 1)
            break;
        }

        if (matches.size() > 1) // still tied (max conn to newest node)
        {
          for (unsigned int i=stones->size()-1; i>=0 && i!=(unsigned int)-1; i--)
          {
            int maxconn = graph->getEdgeConnectivity(stones->at(i),matches.front());
            for (std::list<unsigned int>::iterator iter=matches.begin();iter!=matches.end();++iter)
            {
              int conn = graph->getEdgeConnectivity(stones->at(i),(*iter));
              if (conn > maxconn)
                maxconn = conn;
            }

            std::list<unsigned int> m;
            for (std::list<unsigned int>::iterator iter=matches.begin();iter!=matches.end();++iter)
            {
              int conn = graph->getEdgeConnectivity(stones->at(i),(*iter));
              if (conn == maxconn)
                m.push_back((*iter));
            }
            matches = m;

            if (matches.size() == 1)
              break;
          }

          if (matches.size() > 1) // yet still tied (largest size)
          {
            int maxsize = 0;
            for (std::list<unsigned int>::iterator iter=matches.begin();iter!=matches.end();++iter)
            {
              int size = graph->getNodeSize((*iter));
              if (size > maxsize)
                maxsize = size;
            }

            std::list<unsigned int> m;
            for (std::list<unsigned int>::iterator iter=matches.begin();iter!=matches.end();++iter)
            {
              int size = graph->getNodeSize((*iter));
              if (size == maxsize)
                m.push_back((*iter));
            }
            matches = m;

            if (matches.size() > 1) // yet yet still tied (most liberties)
            {
              int maxlib = 0;
              for (std::list<unsigned int>::iterator iter=matches.begin();iter!=matches.end();++iter)
              {
                int lib = graph->getNodeLiberties((*iter));
                if (lib > maxlib)
                  maxlib = lib;
              }

              std::list<unsigned int> m;
              for (std::list<unsigned int>::iterator iter=matches.begin();iter!=matches.end();++iter)
              {
                int lib = graph->getNodeLiberties((*iter));
                if (lib == maxlib)
                  m.push_back((*iter));
              }
              matches = m;
            }
          }
        }
      }
    }

    //fprintf(stderr,"[DT] matches: %lu\n",matches.size());
    if (matches.size() > 0)
    {
      std::list<DecisionTree::Node*> *nodes = NULL;
      for (std::list<unsigned int>::iterator iter=matches.begin();iter!=matches.end();++iter)
      {
        std::list<DecisionTree::Node*> *subnodes = NULL;
        unsigned int newnode = (*iter);
        Go::Color col = graph->getNodeStatus(newnode);
        stones->push_back(newnode);

        std::vector<DecisionTree::Option*> *options = q->getOptions();
        for (unsigned int i=0; i<options->size(); i++)
        {
          std::string l = options->at(i)->getLabel();
          if (col==Go::BLACK && l==(invert?"W":"B"))
            subnodes = this->getIntersectionLeafNodes(options->at(i)->getNode(),graph,stones,invert,updatetree,win);
          else if (col==Go::WHITE && l==(invert?"B":"W"))
            subnodes = this->getIntersectionLeafNodes(options->at(i)->getNode(),graph,stones,invert,updatetree,win);
          else if (col==Go::EMPTY && l=="E")
            subnodes = this->getIntersectionLeafNodes(options->at(i)->getNode(),graph,stones,invert,updatetree,win);
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

      return nodes;
    }
    else
    {
      std::vector<DecisionTree::Option*> *options = q->getOptions();
      for (unsigned int i=0; i<options->size(); i++)
      {
        std::string l = options->at(i)->getLabel();
        if (l=="N")
          return this->getIntersectionLeafNodes(options->at(i)->getNode(),graph,stones,invert,updatetree,win);
      }
      return NULL;
    }
  }
  else if (q->getLabel() == "EDGE")
  {
    std::vector<std::string> *attrs = q->getAttrs();
    std::string type = attrs->at(0);
    int n0 = boost::lexical_cast<int>(attrs->at(1));
    int n1 = boost::lexical_cast<int>(attrs->at(2));
    bool eq = attrs->at(3) == "=";
    int val = boost::lexical_cast<int>(attrs->at(4));

    int dist = 0;
    if (type == "DIST")
      dist = graph->getEdgeDistance(stones->at(n0),stones->at(n1));
    else if (type == "CONN")
      dist = graph->getEdgeConnectivity(stones->at(n0),stones->at(n1));

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
        return this->getIntersectionLeafNodes(options->at(i)->getNode(),graph,stones,invert,updatetree,win);
      else if (!res && l=="N")
        return this->getIntersectionLeafNodes(options->at(i)->getNode(),graph,stones,invert,updatetree,win);
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

    unsigned int node = stones->at(n);
    int attr = 0;
    if (type == "SIZE")
      attr = graph->getNodeSize(node);
    else if (type == "LIB")
      attr = graph->getNodeLiberties(node);

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
        return this->getIntersectionLeafNodes(options->at(i)->getNode(),graph,stones,invert,updatetree,win);
      else if (!res && l=="N")
        return this->getIntersectionLeafNodes(options->at(i)->getNode(),graph,stones,invert,updatetree,win);
    }
    return NULL;
  }
  else
  {
    fprintf(stderr,"[DT] Error! Invalid query type: '%s'\n",q->getLabel().c_str());
    return NULL;
  }
}

void DecisionTree::swapChildren(int &d0, int &w0, float &r0, int &d1, int &w1, float &r1)
{
  int ti = d0;
  d0 = d1;
  d1 = ti;

  ti = w0;
  w0 = w1;
  w1 = ti;

  float tf = r0;
  r0 = r1;
  r1 = tf;
}

float DecisionTree::sortedQueryQuality(Parameters *params, int d0, int w0, int d1, int w1, int d2, int w2, int d3, int w3)
{
  // Sort outcomes by winrate, with empty outcomes at the end

  float r0 = (d0 <= 0) ? -1 : ((float)w0) / d0;
  float r1 = (d1 <= 0) ? -1 : ((float)w1) / d1;
  float r2 = (d2 <= 0) ? -1 : ((float)w2) / d2;
  float r3 = (d3 <= 0) ? -1 : ((float)w3) / d3;

  // find first non-empty outcome for r0
  if (r0==-1)
    DecisionTree::swapChildren(d0,w0,r0,d1,w1,r1);
  if (r0==-1)
    DecisionTree::swapChildren(d0,w0,r0,d2,w2,r2);
  if (r0==-1) // shouldn't ever go this far
    DecisionTree::swapChildren(d0,w0,r0,d3,w3,r3);

  // resolve r0
  if (r0 < r1)
    DecisionTree::swapChildren(d0,w0,r0,d1,w1,r1);
  if (r2!=-1 && r0 < r2)
    DecisionTree::swapChildren(d0,w0,r0,d2,w2,r2);
  if (r3!=-1 && r0 < r3)
    DecisionTree::swapChildren(d0,w0,r0,d3,w3,r3);

  // find first non-empty outcome for r1
  if (r1==-1)
    DecisionTree::swapChildren(d1,w1,r1,d2,w2,r2);
  if (r1==-1)
    DecisionTree::swapChildren(d1,w1,r1,d3,w3,r3);

  // resolve r1
  if (r2!=-1 && r1 < r2)
    DecisionTree::swapChildren(d1,w1,r1,d2,w2,r2);
  if (r3!=-1 && r1 < r3)
    DecisionTree::swapChildren(d1,w1,r1,d3,w3,r3);

  // find first non-empty outcome for r2
  if (r2==-1)
    DecisionTree::swapChildren(d2,w2,r2,d3,w3,r3);

  // resolve r2
  if (r2!=-1 && r3!=-1  && r2 < r3)
    DecisionTree::swapChildren(d2,w2,r2,d3,w3,r3);

  return DecisionTree::computeQueryQuality(params,d0,w0,d1,w1,d2,w2,d3,w3);
}

float DecisionTree::computeQueryQuality(Parameters *params, int d0, int w0, int d1, int w1, int d2, int w2, int d3, int w3)
{
  int l0 = d0 - w0;
  int l1 = d1 - w1;
  int l2 = (d2 == -1) ? -1 : d2 - w2;
  int l3 = (d3 == -1) ? -1 : d3 - w3;

  float r0 = (d0 <= 0) ? -1 : ((float)w0) / d0;
  float r1 = (d1 <= 0) ? -1 : ((float)w1) / d1;
  float r2 = (d2 <= 0) ? -1 : ((float)w2) / d2;
  float r3 = (d3 <= 0) ? -1 : ((float)w3) / d3;

  int D = d0 + d1 + ((d2==-1)?0:d2) + ((d3==-1)?0:d3);
  int W = w0 + w1 + ((w2==-1)?0:w2) + ((w3==-1)?0:w3);
  int L = l0 + l1 + ((l2==-1)?0:l2) + ((l3==-1)?0:l3);
  float R = (D <= 0) ? -1 : ((float)W) / D;

  float pd0 = (d0 <= 0) ? -1 : ((float)d0) / D;
  float pd1 = (d1 <= 0) ? -1 : ((float)d1) / D;
  float pd2 = (d2 <= 0) ? -1 : ((float)d2) / D;
  float pd3 = (d3 <= 0) ? -1 : ((float)d3) / D;

  float pw0, pw1, pw2, pw3;
  if (W > 0)
  {
    pw0 = (d0 <= 0) ? -1 : ((float)w0) / W;
    pw1 = (d1 <= 0) ? -1 : ((float)w1) / W;
    pw2 = (d2 <= 0) ? -1 : ((float)w2) / W;
    pw3 = (d3 <= 0) ? -1 : ((float)w3) / W;
  }
  else
  {
    pw0 = -1;
    pw1 = -1;
    pw2 = -1;
    pw3 = -1;
  }

  float pl0, pl1, pl2, pl3;
  if (L > 0)
  {
    pl0 = (d0 <= 0) ? -1 : ((float)l0) / L;
    pl1 = (d1 <= 0) ? -1 : ((float)l1) / L;
    pl2 = (d2 <= 0) ? -1 : ((float)l2) / L;
    pl3 = (d3 <= 0) ? -1 : ((float)l3) / L;
  }
  else
  {
    pl0 = -1;
    pl1 = -1;
    pl2 = -1;
    pl3 = -1;
  }

  bool sorted = true;
  if (r0 < r1)
    sorted = false;
  else if (r2!=-1 && r1 < r2)
    sorted = false;
  else if (r2!=-1 && r3!=-1 && r2 < r3)
    sorted = false;

  int C = 2 + (d2==-1?0:1) + (d3==-1?0:1);

  float q = -std::numeric_limits<float>::infinity();

  // sensibility conditions
  bool sens1 = (d0==-1||d0<D) && (d1==-1||d1<D) && (d2==-1||d2<D) && (d3==-1||d3<D); // all descents must not be down one child
  bool sens2 = W>1 && L>1; // must be some wins and losses
  bool sens3 = (w0==-1||w0<W) && (w1==-1||w1<W) && (w2==-1||w2<W) && (w3==-1||w3<W); // all wins must not be down one child
  bool sens4 = (l0==-1||l0<L) && (l1==-1||l1<L) && (l2==-1||l2<L) && (l3==-1||l3<L); // all losses must not be down one child

  if (sens1)
  {
    switch (params->dt_selection_policy)
    {
      case Parameters::SP_WIN_LOSS_SEPARATE:
        {
          if (!sens2)
            break;

          q = 0;

          if (d0 > 0)
            q += fabs(r0 - R);
          if (d1 > 0)
            q += fabs(r1 - R);
          if (d2 > 0)
            q += fabs(r2 - R);
          if (d3 > 0)
            q += fabs(r3 - R);

          break;
        }
      case Parameters::SP_WEIGHTED_WIN_LOSS_SEPARATE:
        {
          if (!sens2)
            break;

          q = 0;

          if (d0 > 0)
            q += pd0 * fabs(r0 - R);
          if (d1 > 0)
            q += pd1 * fabs(r1 - R);
          if (d2 > 0)
            q += pd2 * fabs(r2 - R);
          if (d3 > 0)
            q += pd3 * fabs(r3 - R);

          break;
        }
      case Parameters::SP_WINRATE_ENTROPY:
        {
          if (!sens2)
            break;

          if (sorted)
          {
            float W = log(r0);

            float L = 0;
            if (r3!=-1)
              L = log(1 - r3);
            else if (r2!=-1)
              L = log(1 - r2);
            else
              L = log(1 - r1);

            q = W + L;
          }
          else
            q = DecisionTree::sortedQueryQuality(params,d0,w0,d1,w1,d2,w2,d3,w3);

          break;
        }
      case Parameters::SP_SYMMETRIC_SEPARATE:
        {
          if (!sens2)
            break;

          q = 0;

          if (d0 > 0)
            q -= r0<R? r0/R : (1-r0)/(1-R);
          if (d1 > 0)
            q -= r1<R? r1/R : (1-r1)/(1-R);
          if (d2 > 0)
            q -= r2<R? r2/R : (1-r2)/(1-R);
          if (d3 > 0)
            q -= r3<R? r3/R : (1-r3)/(1-R);

          break;
        }
      case Parameters::SP_WEIGHTED_SYMMETRIC_SEPARATE:
        {
          if (!sens2)
            break;

          q = 0;

          if (d0 > 0)
            q -= pd0*(r0<R? r0/R : (1-r0)/(1-R));
          if (d1 > 0)
            q -= pd1*(r1<R? r1/R : (1-r1)/(1-R));
          if (d2 > 0)
            q -= pd2*(r2<R? r2/R : (1-r2)/(1-R));
          if (d3 > 0)
            q -= pd3*(r3<R? r3/R : (1-r3)/(1-R));

          break;
        }
      case Parameters::SP_WEIGHTED_WINRATE_ENTROPY:
        {
          if (!sens2)
            break;

          float wr0 = r0>0?(pd0 * log(r0)):1;
          float wr1 = r1>0?(pd1 * log(r1)):1;
          float wr2 = r2>0?(pd2 * log(r2)):1;
          float wr3 = r3>0?(pd3 * log(r3)):1;

          float W = 1;
          int Wc = -1;
          if (W==1 || (wr0<1 && wr0>W))
          {
            W = wr0;
            Wc = 0;
          }
          if (W==1 || (wr1<1 && wr1>W))
          {
            W = wr1;
            Wc = 1;
          }
          if (W==1 || (wr2<1 && wr2>W))
          {
            W = wr2;
            Wc = 2;
          }
          if (W==1 || (wr3<1 && wr3>W))
          {
            W = wr3;
            Wc = 3;
          }

          float lr0 = (r0!=-1 && r0<1 && Wc!=0)?(pd0 * log(1 - r0)):1;
          float lr1 = (r1!=-1 && r1<1 && Wc!=1)?(pd1 * log(1 - r1)):1;
          float lr2 = (r2!=-1 && r2<1 && Wc!=2)?(pd2 * log(1 - r2)):1;
          float lr3 = (r3!=-1 && r3<1 && Wc!=3)?(pd3 * log(1 - r3)):1;

          float L = 1;
          if (L==1 || (lr0<1 && lr0>L))
            L = lr0;
          if (L==1 || (lr1<1 && lr1>L))
            L = lr1;
          if (L==1 || (lr2<1 && lr2>L))
            L = lr2;
          if (L==1 || (lr3<1 && lr3>L))
            L = lr3;

          q = W + L;

          break;
        }
      case Parameters::SP_CLASSIFICATION_SEPARATE:
        {
          if (!sens2)
            break;

          if (sorted)
          {
            q = 0;

            int neC = (d0>0?1:0) + (d1>0?1:0) + (d2>0?1:0) + (d3>0?1:0);

            if (neC==2)
              q = (float)(w0 + l1) / (d0 + d1);
            else if (neC==3)
            {
              float W1n = w0;
              float W1d = d0;
              float W2n = W1n + w1;
              float W2d = W1d + d1;
              float L1n = w2;
              float L1d = d2;
              float L2n = W1n + w1;
              float L2d = W1d + d1;

              q = (W1n + L1n)/(W1d + L1d);
              if ((W1n + L2n)/(W1d + L2d) > q)
                q = (W1n + L2n)/(W1d + L2d);
              if ((W2n + L1n)/(W2d + L1d) > q)
                q = (W2n + L1n)/(W2d + L1d);
            }
            else // C==4
            {
              float W1n = w0;
              float W1d = d0;
              float W2n = W1n + w1;
              float W2d = W1d + d1;
              float W3n = W2n + w2;
              float W3d = W2d + d2;
              float L1n = w3;
              float L1d = d3;
              float L2n = W1n + w2;
              float L2d = W1d + d2;
              float L3n = W2n + w1;
              float L3d = W2d + d1;

              q = (W1n + L1n)/(W1d + L1d);
              if ((W1n + L2n)/(W1d + L2d) > q)
                q = (W1n + L2n)/(W1d + L2d);
              if ((W1n + L3n)/(W1d + L3d) > q)
                q = (W1n + L3n)/(W1d + L3d);
              if ((W2n + L1n)/(W2d + L1d) > q)
                q = (W2n + L1n)/(W2d + L1d);
              if ((W2n + L2n)/(W2d + L2d) > q)
                q = (W2n + L2n)/(W2d + L2d);
              if ((W3n + L1n)/(W3d + L1d) > q)
                q = (W3n + L1n)/(W3d + L1d);
            }
          }
          else
            q = DecisionTree::sortedQueryQuality(params,d0,w0,d1,w1,d2,w2,d3,w3);

          break;
        }
      case Parameters::SP_ROBUST_DESCENT_SPLIT:
        {
          q = 0;
          float cr = 1.0 / C;

          if (d0 >= 0)
            q -= fabs(cr - pd0);
          if (d1 >= 0)
            q -= fabs(cr - pd1);
          if (d2 >= 0)
            q -= fabs(cr - pd2);
          if (d3 >= 0)
            q -= fabs(cr - pd3);

          break;
        }
      case Parameters::SP_ROBUST_WIN_SPLIT:
        {
          if (!sens3)
            break;

          q = 0;
          float cr = 1.0 / C;

          if (d0 >= 0)
            q -= fabs(cr - pw0);
          if (d1 >= 0)
            q -= fabs(cr - pw1);
          if (d2 >= 0)
            q -= fabs(cr - pw2);
          if (d3 >= 0)
            q -= fabs(cr - pw3);

          break;
        }
      case Parameters::SP_ROBUST_LOSS_SPLIT:
        {
          if (!sens4)
            break;

          q = 0;
          float cr = 1.0 / C;

          if (d0 >= 0)
            q -= fabs(cr - pl0);
          if (d1 >= 0)
            q -= fabs(cr - pl1);
          if (d2 >= 0)
            q -= fabs(cr - pl2);
          if (d3 >= 0)
            q -= fabs(cr - pl3);

          break;
        }
      case Parameters::SP_ENTROPY_DESCENT_SPLIT:
        {
          q = 0;

          if (pd0 > 0)
            q -= pd0 * log(pd0);
          if (pd1 > 0)
            q -= pd1 * log(pd1);
          if (pd2 > 0)
            q -= pd2 * log(pd2);
          if (pd3 > 0)
            q -= pd3 * log(pd3);

          break;
        }
      case Parameters::SP_ENTROPY_WIN_SPLIT:
        {
          if (!sens3)
            break;

          q = 0;

          if (pw0 > 0)
            q -= pw0 * log(pw0);
          if (pw1 > 0)
            q -= pw1 * log(pw1);
          if (pw2 > 0)
            q -= pw2 * log(pw2);
          if (pw3 > 0)
            q -= pw3 * log(pw3);

          break;
        }
      case Parameters::SP_ENTROPY_LOSS_SPLIT:
        {
          if (!sens4)
            break;

          q = 0;

          if (pl0 > 0)
            q -= pl0 * log(pl0);
          if (pl1 > 0)
            q -= pl1 * log(pl1);
          if (pl2 > 0)
            q -= pl2 * log(pl2);
          if (pl3 > 0)
            q -= pl3 * log(pl3);

          break;
        }
      case Parameters::SP_WINRATE_SPLIT:
        {
          if (!sens2)
            break;

          q = 0;

          if (d0 >= 0)
            q -= fabs(r0 - R);
          if (d1 >= 0)
            q -= fabs(r1 - R);
          if (d2 >= 0)
            q -= fabs(r2 - R);
          if (d3 >= 0)
            q -= fabs(r3 - R);

          break;
        }
      case Parameters::SP_DESCENT_SPLIT:
        {
          q = 1 - 2*fabs(((float)d0)/D - 0.5);
          break;
        }
    }
  }

  // fprintf(stderr,"[DT] quality of %d(%d:%d)[%.2f] %d(%d:%d)[%.2f] %d(%d:%d)[%.2f] %d(%d:%d)[%.2f] %d(%d:%d)[%.2f] = %.4f\n",D,W,L,R,d0,w0,l0,r0,d1,w1,l1,r1,d2,w2,l2,r2,d3,w3,l3,r3,q);
  return q;
}

std::string DecisionTree::toString(bool ignorestats, int leafoffset)
{
  this->updateLeafIds();

  std::string r = "(DT[";
  for (unsigned int i=0;i<attrs->size();i++)
  {
    if (i!=0)
      r += "|";
    r += attrs->at(i);
  }
  r += "]\n";
  //r += " # leaves: " + boost::lexical_cast<std::string>(leafmap.size()) + "\n";

  r += root->toString(2,ignorestats,leafoffset);

  r += ")";
  return r;
}

std::string DecisionTree::Node::toString(int indent, bool ignorestats, int leafoffset)
{
  std::string r = "";

  if (ignorestats || stats == NULL)
  {
    // empty (STATS:) not required
    // for (int i=0;i<indent;i++)
    //   r += " ";
    // r += "(STATS:)\n";
  }
  else
    r += stats->toString(indent);

  if (query == NULL)
  {
    for (int i=0;i<indent;i++)
      r += " ";
    r += "(WEIGHT[";
    r += boost::lexical_cast<std::string>(weight); //TODO: make sure this is correctly formatted
    r += "]) # id: " + boost::lexical_cast<std::string>(leafoffset+leafid) + "\n";
  }
  else
    r += query->toString(indent,ignorestats,leafoffset);

  return r;
}

std::string DecisionTree::Node::getPath()
{
  std::string r = "";

  if (!this->isRoot())
  {
    DecisionTree::Option *opt = this->getParent();
    DecisionTree::Query *query = opt->getParent();
    DecisionTree::Node *node = query->getParent();

    r = node->getPath();
    r += query->getLabel() + "[";
    for (unsigned int i=0;i<query->getAttrs()->size();i++)
    {
      if (i!=0)
        r += "|";
      r += query->getAttrs()->at(i);
    }
    r += "]->";
    r += opt->getLabel() + ":";
  }

  if (query == NULL)
  {
    r += "WEIGHT[";
    r += boost::lexical_cast<std::string>(weight); //TODO: make sure this is correctly formatted
    r += "]";
  }

  return r;
}

std::string DecisionTree::Query::toString(int indent, bool ignorestats, int leafoffset)
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
    r += options->at(i)->toString(indent+2,ignorestats,leafoffset);
  }
  
  for (int i=0;i<indent;i++)
    r += " ";
  r += ")\n";

  return r;
}

std::string DecisionTree::Option::toString(int indent, bool ignorestats, int leafoffset)
{
  std::string r = "";

  for (int i=0;i<indent;i++)
    r += " ";
  r += label + ":\n";

  r += node->toString(indent+2,ignorestats,leafoffset);

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

  r += descents->toString(indent+2);
  r += wins->toString(indent+2);
  if (descents2 != NULL)
  {
    r += descents2->toString(indent+2);
    r += wins2->toString(indent+2);
  }
  if (descents3 != NULL)
  {
    r += descents3->toString(indent+2);
    r += wins3->toString(indent+2);
  }

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
  for (int i = start; i <= end; i++)
  {
    r += boost::lexical_cast<std::string>(this->getEquals(i));
    if (i < end)
      r += ":";
  }
  r += "])\n";

  return r;
}

std::list<DecisionTree*> *DecisionTree::parseString(Parameters *params, std::string rawdata, unsigned long pos)
{
  std::string data = DecisionTree::stripWhitespace(DecisionTree::stripComments(rawdata));
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
  bool compressChain = false;
  bool compressEmpty = false;

  std::string typestr = attrs->at(0); // tree type must be first attribute
  if (typestr == "STONE" || typestr=="SPARSE") // SPARSE is legacy
    type = STONE;
  else if (typestr == "INTERSECTION")
    type = INTERSECTION;
  else
  {
    fprintf(stderr,"[DT] Error! Invalid type: '%s'\n",typestr.c_str());
    delete attrs;
    return NULL;
  }

  for (unsigned int i=1; i<attrs->size(); i++)
  {
    if (attrs->at(i) == "CHAINCOMP")
      compressChain = true;
    else if (attrs->at(i) == "EMPTYCOMP")
      compressEmpty = true;
    else if (attrs->at(i) == "BOTHCOMP")
    {
      compressChain = true;
      compressEmpty = true;
    }
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

  // root->populateEmptyStats(type); // lazy allocation

  DecisionTree *dt = new DecisionTree(params,type,compressChain,compressEmpty,attrs,root);
  
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
  if (data.substr(pos,7) == "(STATS:") // legacy stats parsing
  {
    if (data.substr(pos,8) == "(STATS:)") // empty stats: lazy allocation
      pos += 8;
    else
    {
      fprintf(stderr,"[DT] Error! Stats parsing is deprecated\n");
      return NULL;
    }
  }
  // else: assume no stats are present

  if (data.substr(pos,8) == "(WEIGHT[")
  {
    pos += 8;
    float *num = DecisionTree::parseNumber(data,pos);
    if (num == NULL)
      return NULL;

    if (data.substr(pos,2) != "])")
    {
      fprintf(stderr,"[DT] Error! Expected '])' at '%s'\n",data.substr(pos).c_str());
      delete num;
      return NULL;
    }
    pos += 2;

    DecisionTree::Node *node = new DecisionTree::Node(*num);
    //fprintf(stderr,"[DT] !!!\n");

    delete num;
    return node;
  }
  else
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

    std::vector<DecisionTree::Option*> *options = DecisionTree::parseOptions(type,data,pos);
    if (options == NULL)
    {
      delete attrs;
      return NULL;
    }

    if (data[pos] != ')')
    {
      fprintf(stderr,"[DT] Error! Expected ')' at '%s'\n",data.substr(pos).c_str());
      return NULL;
    }
    pos++;

    DecisionTree::Query *query = new DecisionTree::Query(label,attrs,options);
    return new DecisionTree::Node(query);
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

  //TODO: update

  if (data[pos] != ')')
  {
    fprintf(stderr,"[DT] Error! Expected ')' at '%s'\n",data.substr(pos).c_str());
    delete attrs;
    delete range;
    return NULL;
  }
  pos++;

  DecisionTree::StatPerm *sp = new StatPerm(label,attrs,range,NULL); //XXX: quick 'fix'
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

  //TODO: update ?

  if (data[pos] != ')')
  {
    fprintf(stderr,"[DT] Error! Expected ')' at '%s'\n",data.substr(pos).c_str());
    delete ps;
    delete pe;
    delete pv;
    return NULL;
  }
  pos++;

  return new DecisionTree::Range(*ps,*pe);
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

  int leafoffset = 0;
  for (std::list<DecisionTree*>::iterator iter=trees->begin();iter!=trees->end();++iter)
  {
    fout << (*iter)->toString(ignorestats,leafoffset) << "\n\n";
    leafoffset += (*iter)->getLeafCount();
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

std::string DecisionTree::stripComments(std::string in)
{
  std::string comment = COMMENT;
  std::string newline = "\r\n";
  bool incomment = false;
  std::string out = "";
  for (unsigned int i = 0; i < in.length(); i++)
  {
    if (comment.find(in[i]) != std::string::npos)
      incomment = true;
    else if (newline.find(in[i]) != std::string::npos)
    {
      incomment = false;
      out += in[i];
    }
    else if (!incomment)
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

DecisionTree::Node::Node(Query *q)
{
  parent = NULL;
  stats = NULL;
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

DecisionTree::Node::Node(float w)
{
  parent = NULL;
  stats = NULL;
  query = NULL;
  leafid = -1;
  weight = w;
}

DecisionTree::Node::~Node()
{
  if (stats != NULL)
    delete stats;
  if (query != NULL)
    delete query;
}

void DecisionTree::Node::populateEmptyStats(DecisionTree::Type type, unsigned int maxnode)
{
  if (stats == NULL || stats->getStatPerms()->size() == 0)
  {
    if (stats != NULL)
      delete stats;
    if (this->isLeaf()) // no need to collect stats on internal nodes
      stats = new DecisionTree::Stats(type,maxnode);
    else
      stats = NULL;
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
        case STONE:
        case INTERSECTION:
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
    case STONE:
      {
        statperms = new std::vector<DecisionTree::StatPerm*>();
        float rangemin = 0;
        float rangemax = 100;

        // ATTR
        for (unsigned int i=0; i<=maxnode; i++)
        {
          if (i>0) // don't need to keep stats on 0'th node
          {
            {
              std::vector<std::string> *attrs = new std::vector<std::string>();
              attrs->push_back("LIB");
              attrs->push_back(boost::lexical_cast<std::string>(i));
              statperms->push_back(new DecisionTree::StatPerm("ATTR",attrs,new DecisionTree::Range(rangemin,rangemax),new DecisionTree::Range(rangemin,rangemax)));
            }
            {
              std::vector<std::string> *attrs = new std::vector<std::string>();
              attrs->push_back("SIZE");
              attrs->push_back(boost::lexical_cast<std::string>(i));
              statperms->push_back(new DecisionTree::StatPerm("ATTR",attrs,new DecisionTree::Range(rangemin,rangemax),new DecisionTree::Range(rangemin,rangemax)));
            }
          }
        }

        // DIST
        for (unsigned int i=0; i<=maxnode; i++)
        {
          for (unsigned int j=i+1; j<=maxnode; j++)
          {
            std::vector<std::string> *attrs = new std::vector<std::string>();
            attrs->push_back(boost::lexical_cast<std::string>(i));
            attrs->push_back(boost::lexical_cast<std::string>(j));
            statperms->push_back(new DecisionTree::StatPerm("DIST",attrs,new DecisionTree::Range(rangemin,rangemax),new DecisionTree::Range(rangemin,rangemax)));
          }

        }

        // NEW
        std::string colslist = "BWS";
        for (unsigned int i=1; i<((unsigned int)1<<colslist.size()); i++)
        {
          std::string cols = "";
          for (unsigned int j=0; j<colslist.size(); j++)
          {
            if ((i>>j)&0x01)
              cols += colslist[j];
          }
          std::vector<std::string> *attrs = new std::vector<std::string>();
          attrs->push_back(cols);

          int children = cols.size() + 1;
          if (children == 4)
          {
            DecisionTree::Range *d1 = new DecisionTree::Range(rangemin,rangemax);
            DecisionTree::Range *w1 = new DecisionTree::Range(rangemin,rangemax);
            DecisionTree::Range *d2 = new DecisionTree::Range(rangemin,rangemax);
            DecisionTree::Range *w2 = new DecisionTree::Range(rangemin,rangemax);
            DecisionTree::Range *d3 = new DecisionTree::Range(rangemin,rangemax);
            DecisionTree::Range *w3 = new DecisionTree::Range(rangemin,rangemax);
            statperms->push_back(new DecisionTree::StatPerm("NEW",attrs,d1,w1,d2,w2,d3,w3));
          }
          else if (children == 3)
          {
            DecisionTree::Range *d1 = new DecisionTree::Range(rangemin,rangemax);
            DecisionTree::Range *w1 = new DecisionTree::Range(rangemin,rangemax);
            DecisionTree::Range *d2 = new DecisionTree::Range(rangemin,rangemax);
            DecisionTree::Range *w2 = new DecisionTree::Range(rangemin,rangemax);
            statperms->push_back(new DecisionTree::StatPerm("NEW",attrs,d1,w1,d2,w2));
          }
          else if (children == 2)
          {
            DecisionTree::Range *d1 = new DecisionTree::Range(rangemin,rangemax);
            DecisionTree::Range *w1 = new DecisionTree::Range(rangemin,rangemax);
            statperms->push_back(new DecisionTree::StatPerm("NEW",attrs,d1,w1));
          }
          else
            delete attrs;
        }

        break;
      }

    case INTERSECTION:
      {
        statperms = new std::vector<DecisionTree::StatPerm*>();
        float rangemin = 0;
        float rangemax = 500;

        // ATTR
        for (unsigned int i=0; i<=maxnode; i++)
        {
          {
            std::vector<std::string> *attrs = new std::vector<std::string>();
            attrs->push_back("LIB");
            attrs->push_back(boost::lexical_cast<std::string>(i));
            statperms->push_back(new DecisionTree::StatPerm("ATTR",attrs,new DecisionTree::Range(rangemin,rangemax),new DecisionTree::Range(rangemin,rangemax)));
          }
          {
            std::vector<std::string> *attrs = new std::vector<std::string>();
            attrs->push_back("SIZE");
            attrs->push_back(boost::lexical_cast<std::string>(i));
            statperms->push_back(new DecisionTree::StatPerm("ATTR",attrs,new DecisionTree::Range(rangemin,rangemax),new DecisionTree::Range(rangemin,rangemax)));
          }
        }

        // EDGE
        for (unsigned int i=0; i<=maxnode; i++)
        {
          for (unsigned int j=i+1; j<=maxnode; j++)
          {
            {
              std::vector<std::string> *attrs = new std::vector<std::string>();
              attrs->push_back("DIST");
              attrs->push_back(boost::lexical_cast<std::string>(i));
              attrs->push_back(boost::lexical_cast<std::string>(j));
              statperms->push_back(new DecisionTree::StatPerm("EDGE",attrs,new DecisionTree::Range(rangemin,rangemax),new DecisionTree::Range(rangemin,rangemax)));
            }
            {
              std::vector<std::string> *attrs = new std::vector<std::string>();
              attrs->push_back("CONN");
              attrs->push_back(boost::lexical_cast<std::string>(i));
              attrs->push_back(boost::lexical_cast<std::string>(j));
              statperms->push_back(new DecisionTree::StatPerm("EDGE",attrs,new DecisionTree::Range(rangemin,rangemax),new DecisionTree::Range(rangemin,rangemax)));
            }
          }

        }

        // NEW
        for (unsigned int i=0; i<=maxnode; i++)
        {
          std::string colslist = "BWE";
          for (unsigned int j=1; j<((unsigned int)1<<colslist.size()); j++)
          {
            std::string cols = "";
            for (unsigned int k=0; k<colslist.size(); k++)
            {
              if ((j>>k)&0x01)
                cols += colslist[k];
            }
            std::vector<std::string> *attrs = new std::vector<std::string>();
            attrs->push_back(cols);
            attrs->push_back(boost::lexical_cast<std::string>(i));

            int children = cols.size() + 1;
            if (children == 4)
            {
              DecisionTree::Range *d1 = new DecisionTree::Range(0,1);
              DecisionTree::Range *w1 = new DecisionTree::Range(0,1);
              DecisionTree::Range *d2 = new DecisionTree::Range(0,1);
              DecisionTree::Range *w2 = new DecisionTree::Range(0,1);
              DecisionTree::Range *d3 = new DecisionTree::Range(0,1);
              DecisionTree::Range *w3 = new DecisionTree::Range(0,1);
              statperms->push_back(new DecisionTree::StatPerm("NEW",attrs,d1,w1,d2,w2,d3,w3));
            }
            else if (children == 3)
            {
              DecisionTree::Range *d1 = new DecisionTree::Range(0,1);
              DecisionTree::Range *w1 = new DecisionTree::Range(0,1);
              DecisionTree::Range *d2 = new DecisionTree::Range(0,1);
              DecisionTree::Range *w2 = new DecisionTree::Range(0,1);
              statperms->push_back(new DecisionTree::StatPerm("NEW",attrs,d1,w1,d2,w2));
            }
            else if (children == 2)
            {
              DecisionTree::Range *d1 = new DecisionTree::Range(0,1);
              DecisionTree::Range *w1 = new DecisionTree::Range(0,1);
              statperms->push_back(new DecisionTree::StatPerm("NEW",attrs,d1,w1));
            }
            else
              delete attrs;
          }
        }

        break;
      }
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

DecisionTree::StatPerm::StatPerm(std::string l, std::vector<std::string> *a, Range *d, Range *w, Range *d2, Range *w2, Range *d3, Range *w3)
{
  label = l;
  attrs = a;
  descents = d;
  wins = w;
  descents2 = d2;
  wins2 = w2;
  descents3 = d3;
  wins3 = w3;
}

DecisionTree::StatPerm::~StatPerm()
{
  delete attrs;
  delete descents;
  delete wins;
  delete descents2;
  delete wins2;
  delete descents3;
  delete wins3;
}

float DecisionTree::StatPerm::getQuality(Parameters *params, bool lne, int v)
{
  int D = descents->getSum();
  int W = wins->getSum();

  int d1 = 0;
  int w1 = 0;
  int d2 = 0;
  int w2 = 0;
  int d3 = 0;
  int w3 = 0;

  if (lne)
  {
    d1 = descents->getLessThan(v);
    w1 = wins->getLessThan(v);
    if (descents2 != NULL)
    {
      d2 = descents2->getLessThan(v);
      w2 = wins2->getLessThan(v);
    }
    if (descents3 != NULL)
    {
      d3 = descents3->getLessThan(v);
      w3 = wins3->getLessThan(v);
    }
  }
  else
  {
    d1 = descents->getEquals(v);
    w1 = wins->getEquals(v);
    if (descents2 != NULL)
    {
      d2 = descents2->getEquals(v);
      w2 = wins2->getEquals(v);
    }
    if (descents3 != NULL)
    {
      d3 = descents3->getEquals(v);
      w3 = wins3->getEquals(v);
    }
  }

  int d0 = D - d1 - d2 - d3;
  int w0 = W - w1 - w2 - w3;

  if (descents2 == NULL)
  {
    d2 = -1;
    w2 = -1;
  }
  if (descents3 == NULL)
  {
    d3 = -1;
    w3 = -1;
  }

  // fprintf(stderr,"[DT] qq[v=%d]: %d,%d %d,%d %d,%d %d,%d\n",v,d0,w0,d1,w1,d2,w2,d3,w3);
  return DecisionTree::computeQueryQuality(params,d0,w0,d1,w1,d2,w2,d3,w3);
}

DecisionTree::Query *DecisionTree::Stats::getBestQuery(Parameters *params, Type type, int maxnode)
{
  float bestquality = -std::numeric_limits<float>::max();
  std::string bestlabel = "";
  std::vector<std::string> *bestattrs = NULL;

  // fprintf(stderr,"[DT] find best in:\n%s\n",this->toString(0).c_str());

  std::vector<DecisionTree::StatPerm*> *statperms = this->getStatPerms();
  for (unsigned int i=0; i<statperms->size(); i++)
  {
    DecisionTree::StatPerm *sp = statperms->at(i);

    if (type == DecisionTree::INTERSECTION && sp->getLabel()=="NEW")
    {
      float q = sp->getQuality(params,false,1);
      if (q > bestquality)
      {
        bestquality = q;
        bestlabel = sp->getLabel();
        if (bestattrs != NULL)
          delete bestattrs;
        std::vector<std::string> *attrs = new std::vector<std::string>();
        for (unsigned int j=0; j<sp->getAttrs()->size(); j++)
        {
          attrs->push_back(sp->getAttrs()->at(j));
        }
      }
    }
    else
    {
      int min = sp->getDescents()->getStart();
      int max = sp->getDescents()->getEnd();
      bool foundbest = false;
      int bestval = 0;
      bool bestlne = false;

      for (int v = min; v < max; v++) // max is used to group not found
      {
        float ql = sp->getQuality(params,true,v);
        if (ql > bestquality)
        {
          foundbest = true;
          bestquality = ql;
          bestval = v;
          bestlne = true;
        }

        if (sp->getLabel() != "NEW")
        {
          float qe = sp->getQuality(params,false,v);
          if (qe > bestquality)
          {
            foundbest = true;
            bestquality = qe;
            bestval = v;
            bestlne = false;
          }
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
        if (bestlabel=="NEW")
          attrs->push_back(boost::lexical_cast<std::string>(bestval-1));
        else
        {
          attrs->push_back(bestlne?"<":"=");
          attrs->push_back(boost::lexical_cast<std::string>(bestval));
        }
        bestattrs = attrs;
      }
    }
  }

  if (bestattrs != NULL && bestquality > -std::numeric_limits<float>::max())
  {
    // fprintf(stderr,"[DT] best stat: %s",bestlabel.c_str());
    // for (unsigned int j=0; j<bestattrs->size(); j++)
    //   fprintf(stderr,"%c%s",(j==0?'[':'|'),bestattrs->at(j).c_str());
    // fprintf(stderr,"] %.4f\n",bestquality);

    return new DecisionTree::Query(type,bestlabel,bestattrs,maxnode);
  }
  else
  {
    if (bestattrs != NULL)
      delete bestattrs;
    return NULL;
  }
}

DecisionTree::Range::Range(int s, int e)
{
  start = s;
  end = e;
  sum = 0;
  data = new int[e-s+1];
  for (int i = 0; i < (e-s+1); i++)
    data[i] = 0;
}

DecisionTree::Range::~Range()
{
  delete[] data;
}

void DecisionTree::Range::addVal(int v)
{
  if (v<start || v>end)
    return;

  data[v-start]++;
  sum++;
}

int DecisionTree::Range::getEquals(int v)
{
  if (v<start || v>end)
    return 0;

  return data[v-start];
}

int DecisionTree::Range::getLessThan(int v)
{
  int t = 0;
  for (int i = start; i < v && i <=end; i++)
  {
    t += data[i-start];
  }
  return t;
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
    case STONE:
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

    case INTERSECTION:
      if (l=="NEW")
      {
        bool B = (attrs->at(0).find('B') != std::string::npos);
        bool W = (attrs->at(0).find('W') != std::string::npos);
        bool E = (attrs->at(0).find('E') != std::string::npos);
        if (B)
          options->push_back(new DecisionTree::Option(type,"B",maxnode+1));
        if (W)
          options->push_back(new DecisionTree::Option(type,"W",maxnode+1));
        if (E)
          options->push_back(new DecisionTree::Option(type,"E",maxnode+1));
        options->push_back(new DecisionTree::Option(type,"N",maxnode));
      }
      else if (l=="EDGE" || l=="ATTR")
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

DecisionTree::StoneGraph::StoneGraph(Go::Board *board)
{
  this->board = board;
  nodes = new std::vector<DecisionTree::StoneGraph::StoneNode*>();
  edges = new std::vector<std::vector<int>*>();
  posregions = new std::map<int,int>();

  for (int p = 0; p < board->getPositionMax(); p++)
  {
    if (board->inGroup(p))
    {
      DecisionTree::StoneGraph::StoneNode *node = new DecisionTree::StoneGraph::StoneNode();

      node->pos = p;
      node->col = board->getColor(p);
      Go::Group *group = board->getGroup(p);
      node->size = group->numOfStones();
      if (group->inAtari())
        node->liberties = 1;
      else
        node->liberties = group->numOfPseudoLiberties();

      nodes->push_back(node);
      (*posregions)[p] = p;
    }
  }

  for (int i = -1; i >= -4; i--)
  {
    DecisionTree::StoneGraph::StoneNode *node = new DecisionTree::StoneGraph::StoneNode();

    node->pos = i;
    node->col = Go::OFFBOARD;
    node->size = 0;
    node->liberties = 0;

    nodes->push_back(node);
  }

  unsigned int N = nodes->size();
  for (unsigned int i=0; i<N; i++)
  {
    edges->push_back(new std::vector<int>());

    int p1 = nodes->at(i)->pos;
    for (unsigned int j=0; j<i; j++)
    {
      int p2 = nodes->at(j)->pos;
      int d = DecisionTree::getDistance(board,p1,p2);
      edges->at(i)->push_back(d);
    }
  }

  auxnode = -1;
  auxpos = -1;
}

DecisionTree::StoneGraph::~StoneGraph()
{
  for (unsigned int i = 0; i < nodes->size(); i++)
  {
    delete nodes->at(i);
    delete edges->at(i);
  }
  delete nodes;
  delete edges;
  delete posregions;
}

std::string DecisionTree::StoneGraph::toString()
{
  std::ostringstream ss;

  for (unsigned int i = 0; i < (3+1 + 1+1 + 2+1 + 2+3); i++)
    ss << " ";
  for (unsigned int i = 0; i < this->getNumNodes(); i++)
    ss << " " << std::setw(3)<<i;
  ss << "\n";

  for (unsigned int i = 0; i < this->getNumNodes(); i++)
  {
    ss << std::setw(3)<<i << " " << Go::colorToChar(this->getNodeStatus(i)) << " " << std::setw(2)<<this->getNodeSize(i) << " " << std::setw(2)<<this->getNodeLiberties(i) << "   ";
  
    for (unsigned int j = 0; j < this->getNumNodes(); j++)
    {
      if (i==j)
        ss << "   -";
      else
        ss << " " << std::setw(3)<<this->getEdgeWeight(i,j);
    }

    ss << "\n";
  }

  return ss.str();
}

int DecisionTree::StoneGraph::getEdgeWeight(unsigned int node1, unsigned int node2)
{
  if (node1 == node2)
    return 0;
  else if (node1 < node2)
    return edges->at(node2)->at(node1);
  else
    return edges->at(node1)->at(node2);
}

unsigned int DecisionTree::StoneGraph::addAuxNode(int pos)
{
  if (auxnode != (unsigned int)-1)
    throw "Aux node already present";

  DecisionTree::StoneGraph::StoneNode *node = new DecisionTree::StoneGraph::StoneNode();

  node->pos = pos;
  node->col = Go::EMPTY;
  node->size = 0;
  node->liberties = 0;

  nodes->push_back(node);
  edges->push_back(new std::vector<int>());

  unsigned int N = nodes->size();
  unsigned int i = N - 1;

  std::map<int,int> distMap;
  for (int p = 0; p < board->getPositionMax(); p++)
  {
    if (board->inGroup(p))
    {
      int d = DecisionTree::getDistance(board,pos,p);
      int rp = this->lookupPosition(p);

      int cdist = -1;
      if (distMap.count(rp) > 0)
        cdist = distMap[rp];

      if (cdist==-1 || d<cdist)
        distMap[rp] = d;
    }
  }

  for (unsigned int j=0; j<i; j++)
  {
    int p = nodes->at(j)->pos;
    int d = 0;
    if (p < 0) // side
      d = DecisionTree::getDistance(board,pos,p);
    else
    {
      int rp = this->lookupPosition(p);
      if (distMap.count(rp) > 0)
        d = distMap[rp];
    }
    edges->at(i)->push_back(d);
  }

  auxnode = i;
  auxpos = pos;

  return i;
}

void DecisionTree::StoneGraph::removeAuxNode()
{
  if (auxnode == (unsigned int)-1)
    throw "Aux node not present";

  // assume that the last node added is the aux node
  unsigned int N = nodes->size();
  unsigned int i = N - 1;

  delete nodes->at(i);
  delete edges->at(i);

  nodes->resize(N-1);
  edges->resize(N-1);

  auxnode = -1;
}

void DecisionTree::StoneGraph::compressChain()
{
  bool change = true;
  while (change)
  {
    change = false;
    for (unsigned int i=0; i<this->getNumNodes(); i++)
    {
      Go::Color col = this->getNodeStatus(i);
      if (col==Go::BLACK || col==Go::WHITE)
      {
        for (unsigned int j=i+1; j<this->getNumNodes(); j++)
        {
          int d = this->getEdgeWeight(i,j);
          if (d==1 && this->getNodeStatus(i)==this->getNodeStatus(j))
          {
            this->mergeNodes(i,j);
            change = true;
            j--; // j was removed by the merge
            continue;
          }
        }
      }
    }
  }
}

int DecisionTree::StoneGraph::lookupPosition(int pos)
{
  if (posregions->at(pos) == pos)
    return pos;
  else
    return this->lookupPosition(posregions->at(pos)); // could use path compression
}

void DecisionTree::StoneGraph::mergeNodes(unsigned int n1, unsigned int n2)
{
  if (n1==n2)
    return;
  else if (n2<n1)
  {
    this->mergeNodes(n2,n1);
    return;
  }

  DecisionTree::StoneGraph::StoneNode *node1 = nodes->at(n1);
  DecisionTree::StoneGraph::StoneNode *node2 = nodes->at(n2);
  // int pos = node1->pos;
  // Go::Color col = node1->col;
  // int size = node1->size; // no reduction required
  // int liberties = node1->liberties; // no reduction required

  // Merge regions
  int p1 = this->lookupPosition(node1->pos);
  int p2 = this->lookupPosition(node2->pos);
  (*posregions)[p2] = p1;

  // Merge edges of n1 and n2
  for (unsigned int i=0; i<n1; i++)
  {
    int dist1 = edges->at(n1)->at(i);
    int dist2 = edges->at(n2)->at(i);

    int mindist = (dist1<dist2?dist1:dist2);

    edges->at(n1)->at(i) = mindist;
  }

  // Merge edges between n1 and n2
  for (unsigned int i=n1+1; i<n2; i++)
  {
    int dist1 = edges->at(i)->at(n1);
    int dist2 = edges->at(n2)->at(i);

    int mindist = (dist1<dist2?dist1:dist2);

    edges->at(i)->at(n1) = mindist;
  }

  // Merge edges after n2
  for (unsigned int i=n2+1; i<nodes->size(); i++)
  {
    int dist1 = edges->at(i)->at(n1);
    int dist2 = edges->at(i)->at(n2);

    int mindist = (dist1<dist2?dist1:dist2);

    edges->at(i)->at(n1) = mindist;
    edges->at(i)->erase(edges->at(i)->begin()+n2);
  }

  delete nodes->at(n2);
  nodes->erase(nodes->begin()+n2);

  delete edges->at(n2);
  edges->erase(edges->begin()+n2);
}

DecisionTree::IntersectionGraph::IntersectionGraph(Go::Board *board)
{
  this->board = board;
  nodes = new std::vector<DecisionTree::IntersectionGraph::IntersectionNode*>();
  distances = new std::vector<std::vector<int>*>();
  posregions = new std::map<int,int>();

  std::map<int,unsigned int> lookupNodes;

  for (int p = 0; p < board->getPositionMax(); p++)
  {
    if (board->onBoard(p))
    {
      DecisionTree::IntersectionGraph::IntersectionNode *node = new DecisionTree::IntersectionGraph::IntersectionNode();

      node->pos = p;
      node->col = board->getColor(p);
      node->edges = new std::vector<DecisionTree::IntersectionGraph::IntersectionEdge*>();
      node->size = 1;
      if (board->inGroup(p))
      {
        Go::Group *group = board->getGroup(p);
        if (group->inAtari())
          node->liberties = 1;
        else
          node->liberties = group->numOfPseudoLiberties();
      }
      else
        node->liberties = 0;

      nodes->push_back(node);
      unsigned int n = nodes->size() - 1;
      lookupNodes[p] = n;
      (*posregions)[p] = p;
    }
  }

  unsigned int N = nodes->size();

  int size = board->getSize();
  for (unsigned int i=0; i<N; i++)
  {
    int pos = nodes->at(i)->pos;
    foreach_adjacent(pos,p,{
      if (lookupNodes.count(p) > 0)
      {
        unsigned int j = lookupNodes[p];
        if (i < j)
        {
          DecisionTree::IntersectionGraph::IntersectionEdge *edge = new DecisionTree::IntersectionGraph::IntersectionEdge();

          edge->start = i;
          edge->end = j;
          edge->connectivity = 1;

          nodes->at(i)->edges->push_back(edge);
          nodes->at(j)->edges->push_back(edge);
        }
      }
    });
  }

  for (unsigned int i=0; i<N; i++)
  {
    distances->push_back(new std::vector<int>());

    int p1 = nodes->at(i)->pos;
    for (unsigned int j=0; j<i; j++)
    {
      int p2 = nodes->at(j)->pos;
      int d = board->getRectDistance(p1,p2);
      distances->at(i)->push_back(d);
    }
  }

  auxnode = -1;
  auxpos = -1;
}

DecisionTree::IntersectionGraph::~IntersectionGraph()
{
  for (unsigned int i = 0; i < nodes->size(); i++)
  {
    DecisionTree::IntersectionGraph::IntersectionNode *node = nodes->at(i);
    for (unsigned int j = 0; j < node->edges->size(); j++)
    {
      DecisionTree::IntersectionGraph::IntersectionEdge *edge = node->edges->at(j);
      if (edge->start == i && edge->end < i)
        delete edge;
      else if (edge->end == i && edge->start < i)
        delete edge;
    }
    delete node->edges;
    delete node;
    delete distances->at(i);
  }
  delete nodes;
  delete distances;
  delete posregions;
}

std::string DecisionTree::IntersectionGraph::toString()
{
  std::ostringstream ss;

  for (unsigned int i = 0; i < (3+1 + 1+1 + 2+1 + 2+3); i++)
    ss << " ";
  for (unsigned int i = 0; i < this->getNumNodes(); i++)
    ss << " " << std::setw(3)<<i;
  ss << "\n";

  for (unsigned int i = 0; i < this->getNumNodes(); i++)
  {
    ss << std::setw(3)<<i << " " << Go::colorToChar(this->getNodeStatus(i)) << " " << std::setw(2)<<this->getNodeSize(i) << " " << std::setw(2)<<this->getNodeLiberties(i) << "   ";
  
    for (unsigned int j = 0; j < this->getNumNodes(); j++)
    {
      if (i==j)
        ss << "   -";
      else
        ss << " " << std::setw(3)<<this->getEdgeDistance(i,j);
    }

    ss << "  ";
    for (unsigned int j = 0; j < this->getNumNodes(); j++)
    {
      if (this->hasEdge(i,j))
      {
        ss << " " << j << ":" << this->getEdgeConnectivity(i,j);
      }
    }

    // ss << " | " << nodes->at(i)->edges->size();
    // for (unsigned int j = 0; j < nodes->at(i)->edges->size(); j++)
    // {
    //     DecisionTree::IntersectionGraph::IntersectionEdge *edge = nodes->at(i)->edges->at(j);
    //     ss << " " << edge->start << "-" << edge->end << ":" << edge->connectivity;
    // }

    ss << "\n";
  }

  return ss.str();
}

int DecisionTree::IntersectionGraph::getEdgeDistance(unsigned int node1, unsigned int node2)
{
  if (node1 == node2)
    return 0;
  else if (node1 < node2)
    return distances->at(node2)->at(node1);
  else
    return distances->at(node1)->at(node2);
}

DecisionTree::IntersectionGraph::IntersectionEdge *DecisionTree::IntersectionGraph::getEdge(unsigned int node1, unsigned node2)
{
  if (node1 == node2)
    return NULL;

  if (nodes->at(node1)->edges->size() > nodes->at(node2)->edges->size())
    return this->getEdge(node2,node1);

  std::vector<DecisionTree::IntersectionGraph::IntersectionEdge*> *edges = nodes->at(node1)->edges;
  for (unsigned int i=0; i<edges->size(); i++)
  {
    DecisionTree::IntersectionGraph::IntersectionEdge *edge = edges->at(i);
    if (edge->start == node2 || edge->end == node2) // we already know it has an endpoint at node1
      return edge;
  }

  return NULL;
}

bool DecisionTree::IntersectionGraph::hasEdge(unsigned int node1, unsigned int node2)
{
  return this->getEdge(node1,node2) != NULL;
}

int DecisionTree::IntersectionGraph::getEdgeConnectivity(unsigned int node1, unsigned int node2)
{
  DecisionTree::IntersectionGraph::IntersectionEdge *edge = this->getEdge(node1,node2);
  if (edge==NULL)
    return 0;
  else
    return edge->connectivity;
}

unsigned int DecisionTree::IntersectionGraph::addAuxNode(int pos)
{
  if (auxnode != (unsigned int)-1)
    throw "Aux node already present";
  
  int regionpos = this->lookupPosition(pos);

  for (unsigned int i=0; i<nodes->size(); i++)
  {
    if (nodes->at(i)->pos == regionpos)
    {
      auxnode = i;
      auxpos = pos;
      return auxnode;
    }
  }

  return -1; // should never happen
}

void DecisionTree::IntersectionGraph::removeAuxNode()
{
  if (auxnode == (unsigned int)-1)
    throw "Aux node not present";

  auxnode = -1;
}

void DecisionTree::IntersectionGraph::compress(bool chainnotempty)
{
  bool change = true;
  while (change)
  {
    change = false;
    for (unsigned int i=0; i<this->getNumNodes(); i++)
    {
      Go::Color col = this->getNodeStatus(i);
      if ((chainnotempty && (col==Go::BLACK || col==Go::WHITE)) || (!chainnotempty && col==Go::EMPTY))
      {
        for (unsigned int j=i+1; j<this->getNumNodes(); j++)
        {
          int d = this->getEdgeDistance(i,j);
          if (d==1 && this->getNodeStatus(i)==this->getNodeStatus(j))
          {
            this->mergeNodes(i,j);
            change = true;
            j--; // j was removed by the merge
            continue;
          }
        }
      }
    }
  }
}

int DecisionTree::IntersectionGraph::lookupPosition(int pos)
{
  if (posregions->at(pos) == pos)
    return pos;
  else
    return this->lookupPosition(posregions->at(pos)); // could use path compression
}

void DecisionTree::IntersectionGraph::mergeNodes(unsigned int n1, unsigned int n2)
{
  if (n1==n2)
    return;
  else if (n2<n1)
  {
    this->mergeNodes(n2,n1);
    return;
  }

  DecisionTree::IntersectionGraph::IntersectionNode *node1 = nodes->at(n1);
  DecisionTree::IntersectionGraph::IntersectionNode *node2 = nodes->at(n2);
  // int pos = node1->pos;
  // Go::Color col = node1->col;
  // int size = node1->size; // no reduction required
  node1->size += node2->size;
  // int liberties = node1->liberties; // no reduction required

  // Merge regions (mostly for empty regions)
  int p1 = this->lookupPosition(node1->pos);
  int p2 = this->lookupPosition(node2->pos);
  (*posregions)[p2] = p1;

  // Merge edges of n1 and n2
  std::vector<DecisionTree::IntersectionGraph::IntersectionEdge*> *edges1 = node1->edges;
  std::vector<DecisionTree::IntersectionGraph::IntersectionEdge*> *edges2 = node2->edges;
  std::vector<DecisionTree::IntersectionGraph::IntersectionEdge*> *newedges = new std::vector<DecisionTree::IntersectionGraph::IntersectionEdge*>();

  for (unsigned int i=0; i<edges1->size(); i++)
  {
    DecisionTree::IntersectionGraph::IntersectionEdge *edge = edges1->at(i);
    if (edge->start == n2 || edge->end == n2) // n1-n2 edge
      continue;
    else
      newedges->push_back(edge);
  }

  unsigned int N1 = newedges->size();

  for (unsigned int i=0; i<edges2->size(); i++)
  {
    DecisionTree::IntersectionGraph::IntersectionEdge *edge = edges2->at(i);
    if (edge->start == n1 || edge->end == n1) // n1-n2 edge
      delete edge;
    else
    {
      unsigned int othernode = edge->start==n2 ? edge->end : edge->start;
      DecisionTree::IntersectionGraph::IntersectionEdge *otheredge = NULL;
      for (unsigned int j=0; j<N1; j++)
      {
        DecisionTree::IntersectionGraph::IntersectionEdge *e = newedges->at(j);
        if (e->start == othernode || e->end == othernode)
        {
          otheredge = e;
          break;
        }
      }

      if (otheredge != NULL)
      {
        otheredge->connectivity += edge->connectivity;

        std::vector<DecisionTree::IntersectionGraph::IntersectionEdge*> *otheredges = nodes->at(othernode)->edges;
        for (unsigned int j=0; j<otheredges->size(); j++)
        {
          if (otheredges->at(j) == edge)
          {
            otheredges->erase(otheredges->begin()+j);
            break;
          }
        }
        
        delete edge;
      }
      else
      {
        if (edge->start == n2)
          edge->start = n1;
        else if (edge->end == n2)
          edge->end = n1;
        newedges->push_back(edge);
      }
    }
  }

  delete edges1;
  delete edges2;
  node1->edges = newedges;

  // Adjust edges to nodes after n2
  for (unsigned int i=0; i<nodes->size(); i++)
  {
    if (i!=n2)
    {
      DecisionTree::IntersectionGraph::IntersectionNode *en1 = nodes->at(i);
      for (unsigned int j=0; j<en1->edges->size(); j++)
      {
        DecisionTree::IntersectionGraph::IntersectionEdge *edge = en1->edges->at(j);
        if (i == edge->start)
        {
          if (edge->start > n2)
            edge->start--;
          if (edge->end > n2)
            edge->end--;
        }
      }
    }
  }
  
  // Merge distances of n1 and n2
  for (unsigned int i=0; i<n1; i++)
  {
    int dist1 = distances->at(n1)->at(i);
    int dist2 = distances->at(n2)->at(i);

    int mindist = (dist1<dist2?dist1:dist2);

    distances->at(n1)->at(i) = mindist;
  }

  // Merge distances between n1 and n2
  for (unsigned int i=n1+1; i<n2; i++)
  {
    int dist1 = distances->at(i)->at(n1);
    int dist2 = distances->at(n2)->at(i);

    int mindist = (dist1<dist2?dist1:dist2);

    distances->at(i)->at(n1) = mindist;
  }

  // Merge distances after n2
  for (unsigned int i=n2+1; i<nodes->size(); i++)
  {
    int dist1 = distances->at(i)->at(n1);
    int dist2 = distances->at(i)->at(n2);

    int mindist = (dist1<dist2?dist1:dist2);

    distances->at(i)->at(n1) = mindist;
    distances->at(i)->erase(distances->at(i)->begin()+n2);
  }

  delete distances->at(n2);
  distances->erase(distances->begin()+n2);

  delete nodes->at(n2);
  nodes->erase(nodes->begin()+n2);
}

void DecisionTree::IntersectionGraph::updateRegionSizes(IntersectionGraph *a, IntersectionGraph *b, IntersectionGraph *c)
{
  std::map<int,unsigned int> *lookupMap = new std::map<int,unsigned int>();

  for (unsigned int i=0; i<nodes->size(); i++)
  {
    DecisionTree::IntersectionGraph::IntersectionNode *node = nodes->at(i);
    (*lookupMap)[node->pos] = i;
  }

  if (a != NULL)
    this->updateRegionSizes(lookupMap,a);
  if (b != NULL)
    this->updateRegionSizes(lookupMap,b);
  if (c != NULL)
    this->updateRegionSizes(lookupMap,c);

  delete lookupMap;
}

void DecisionTree::IntersectionGraph::updateRegionSizes(std::map<int,unsigned int> *lookupMap, IntersectionGraph *other)
{
  for (unsigned int i=0; i<other->nodes->size(); i++)
  {
    DecisionTree::IntersectionGraph::IntersectionNode *node = other->nodes->at(i);

    int regionpos = this->lookupPosition(node->pos);
    unsigned int regionnode = lookupMap->at(regionpos);
    node->size = nodes->at(regionnode)->size;
  }
}

std::vector<unsigned int> *DecisionTree::IntersectionGraph::getAdjacentNodes(unsigned int node)
{
  std::vector<unsigned int> *adjnodes = new std::vector<unsigned int>();

  std::vector<DecisionTree::IntersectionGraph::IntersectionEdge*> *edges = nodes->at(node)->edges;
  for (unsigned int i=0; i<edges->size(); i++)
  {
    DecisionTree::IntersectionGraph::IntersectionEdge *edge = edges->at(i);
    if (edge->start==node)
      adjnodes->push_back(edge->end);
    else
      adjnodes->push_back(edge->start);
  }

  return adjnodes;
}

DecisionTree::GraphCollection::GraphCollection(GraphTypes types, Go::Board *board)
{
  this->board = board;

  if (types.stoneNone)
  {
    stoneNone = new DecisionTree::StoneGraph(board);
  }
  else
    stoneNone = NULL;

  if (types.stoneChain)
  {
    stoneChain = new DecisionTree::StoneGraph(board);
    stoneChain->compressChain();
  }
  else
    stoneChain = NULL;

  if (types.intersectionNone)
  {
    intersectionNone = new DecisionTree::IntersectionGraph(board);
  }
  else
    intersectionNone = NULL;

  if (types.intersectionChain)
  {
    intersectionChain = new DecisionTree::IntersectionGraph(board);
    intersectionChain->compressChain();
  }
  else
    intersectionChain = NULL;

  if (types.intersectionEmpty)
  {
    intersectionEmpty = new DecisionTree::IntersectionGraph(board);
    intersectionEmpty->compressEmpty();
  }
  else
    intersectionEmpty = NULL;

  if (types.intersectionBoth || types.intersectionEmpty || types.intersectionChain || types.intersectionNone) // also used to update regions for other types
  {
    intersectionBoth = new DecisionTree::IntersectionGraph(board);
    intersectionBoth->compressEmpty();
    intersectionBoth->compressChain();

    intersectionBoth->updateRegionSizes(intersectionNone,intersectionChain,intersectionEmpty);
  }
  else
    intersectionBoth = NULL;

  // fprintf(stderr,"stoneNone:\n%s\n",stoneNone->toString().c_str());
  // fprintf(stderr,"stoneChain:\n%s\n",stoneChain->toString().c_str());
  // fprintf(stderr,"intersectionNone:\n%s\n",intersectionNone->toString().c_str());
  // fprintf(stderr,"intersectionChain:\n%s\n",intersectionChain->toString().c_str());
  // fprintf(stderr,"intersectionEmpty:\n%s\n",intersectionEmpty->toString().c_str());
  // fprintf(stderr,"intersectionBoth:\n%s\n",intersectionBoth->toString().c_str());
}

DecisionTree::GraphCollection::~GraphCollection()
{
  delete stoneNone;
  delete stoneChain;

  delete intersectionNone;
  delete intersectionChain;
  delete intersectionEmpty;
  delete intersectionBoth;
}

DecisionTree::StoneGraph *DecisionTree::GraphCollection::getStoneGraph(bool compressChain)
{
  if (compressChain)
    return stoneChain;
  else
    return stoneNone;
}

DecisionTree::IntersectionGraph *DecisionTree::GraphCollection::getIntersectionGraph(bool compressChain, bool compressEmpty)
{
  if (compressChain)
  {
    if (compressEmpty)
      return intersectionBoth;
    else
      return intersectionChain;
  }
  else
  {
    if (compressEmpty)
      return intersectionEmpty;
    else
      return intersectionNone;
  }
}

