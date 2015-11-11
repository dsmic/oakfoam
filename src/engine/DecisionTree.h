#ifndef DEF_OAKFOAM_DECISIONTREE_H
#define DEF_OAKFOAM_DECISIONTREE_H

#include <string>
#include <list>
#include <vector>
#include <map>
#include "Go.h"
//from "Parameters.h":
class Parameters;

/** Decision Tree for Features. */
class DecisionTree
{
  public:
    enum Type
    {
      STONE,
      INTERSECTION
    };

    class StoneGraph
    {
      public:
        StoneGraph(Go::Board *board);
        ~StoneGraph();

        std::string toString();

        Go::Board *getBoard() { return board; };
        unsigned int getNumNodes() { return nodes->size(); };

        int getNodePosition(unsigned int node) { return nodes->at(node)->pos; };
        Go::Color getNodeStatus(unsigned int node) { return nodes->at(node)->col; };
        int getNodeSize(unsigned int node) { return nodes->at(node)->size; };
        int getNodeLiberties(unsigned int node) { return nodes->at(node)->liberties; };
        int getEdgeWeight(unsigned int node1, unsigned int node2);

        unsigned int addAuxNode(int pos);
        void removeAuxNode();
        unsigned int getAuxNode() { return auxnode; };
        int getAuxPos() { return auxpos; };

        void compressChain();

      private:
        struct StoneNode
        {
          int pos;
          Go::Color col;
          int size;
          int liberties;
        };

        Go::Board *board;
        std::vector<StoneNode*> *nodes;
        std::vector<std::vector<int>*> *edges;
        unsigned int auxnode;
        int auxpos;
        std::map<int,int> *posregions;

        void mergeNodes(unsigned int n1, unsigned int n2);
        int lookupPosition(int pos);
    };

    class IntersectionGraph
    {
      public:
        IntersectionGraph(Go::Board *board);
        ~IntersectionGraph();

        std::string toString();

        Go::Board *getBoard() { return board; };
        unsigned int getNumNodes() { return nodes->size(); };

        int getNodePosition(unsigned int node) { return nodes->at(node)->pos; };
        Go::Color getNodeStatus(unsigned int node) { return nodes->at(node)->col; };
        int getNodeSize(unsigned int node) { return nodes->at(node)->size; };
        int getNodeLiberties(unsigned int node) { return nodes->at(node)->liberties; };
        bool hasEdge(unsigned int node1, unsigned int node2);
        int getEdgeConnectivity(unsigned int node1, unsigned int node2);
        int getEdgeDistance(unsigned int node1, unsigned int node2);
        std::vector<unsigned int> *getAdjacentNodes(unsigned int node);

        unsigned int addAuxNode(int pos);
        void removeAuxNode();
        unsigned int getAuxNode() { return auxnode; };
        int getAuxPos() { return auxpos; };

        void compressChain() { this->compress(true); };
        void compressEmpty() { this->compress(false); };

        void updateRegionSizes(IntersectionGraph *a, IntersectionGraph *b, IntersectionGraph *c);

      private:
        struct IntersectionEdge
        {
          unsigned int start;
          unsigned int end;
          int connectivity;
        };

        struct IntersectionNode
        {
          int pos;
          Go::Color col;
          int size;
          int liberties;
          std::vector<IntersectionEdge*> *edges;
        };

        Go::Board *board;
        std::vector<IntersectionNode*> *nodes;
        std::vector<std::vector<int>*> *distances;
        unsigned int auxnode;
        int auxpos;
        std::map<int,int> *posregions;

        void compress(bool chainnotempty);
        void mergeNodes(unsigned int n1, unsigned int n2);
        IntersectionEdge *getEdge(unsigned int node1, unsigned node2);

        int lookupPosition(int pos);
        void updateRegionSizes(std::map<int,unsigned int> *lookupMap, IntersectionGraph *other);
    };

    struct GraphTypes
    {
      bool stoneNone;
      bool stoneChain;
      bool intersectionNone;
      bool intersectionChain;
      bool intersectionEmpty;
      bool intersectionBoth;
    };

    class GraphCollection
    {
      public:
        GraphCollection(GraphTypes types, Go::Board *board);
        ~GraphCollection();

        Go::Board *getBoard() { return board; };

        StoneGraph *getStoneGraph(bool compressChain);
        IntersectionGraph *getIntersectionGraph(bool compressChain, bool compressEmpty);

      private:
        Go::Board *board;
        StoneGraph *stoneNone;
        StoneGraph *stoneChain;
        IntersectionGraph *intersectionNone;
        IntersectionGraph *intersectionChain;
        IntersectionGraph *intersectionEmpty;
        IntersectionGraph *intersectionBoth;
    };

    ~DecisionTree();

    std::string toString(bool ignorestats = false, int leafoffset = 0);

    Type getType() { return type; };
    bool getCompressChain() { return compressChain; };
    bool getCompressEmpty() { return compressEmpty; };
    std::vector<std::string> *getAttrs() { return attrs; };
    float getWeight(GraphCollection *graphs, Go::Move move, bool updatetree = false, bool win = false);
    std::list<int> *getLeafIds(GraphCollection *graphs, Go::Move move);
    void updateLeafIds();
    int getLeafCount() { return leafmap.size(); };
    void setLeafWeight(int id, float w);
    void updateDescent(GraphCollection *graphs, Go::Move move, bool win);
    void updateDescent(GraphCollection *graphs, Go::Move winmove);
    void getTreeStats(int &treenodes, int &leaves, int &maxdepth, int &sumdepth, int &sumsqdepth, int &maxnodes, int &sumnodes, int &sumsqnodes, float &sumlogweights, float &sumsqlogweights);
    std::string getLeafPath(int id);

    static std::list<DecisionTree*> *parseString(Parameters *params, std::string rawdata, unsigned long pos = 0);
    static std::list<DecisionTree*> *loadFile(Parameters *params, std::string filename);
    static bool saveFile(std::list<DecisionTree*> *trees, std::string filename, bool ignorestats = false);

    static float getCollectionWeight(std::list<DecisionTree*> *trees, GraphCollection *graphs, Go::Move move, bool updatetree = false);
    static std::list<int> *getCollectionLeafIds(std::list<DecisionTree*> *trees, GraphCollection *graphs, Go::Move move);
    static int getCollectionLeafCount(std::list<DecisionTree*> *trees);
    static void setCollectionLeafWeight(std::list<DecisionTree*> *trees, int id, float w);
    static void collectionUpdateDescent(std::list<DecisionTree*> *trees, GraphCollection *graphs, Go::Move winmove);
    static std::string getCollectionLeafPath(std::list<DecisionTree*> *trees, int id);
    static GraphTypes getCollectionTypes(std::list<DecisionTree*> *trees);
  
  private:
    class Range
    {
      public:
        Range(int s, int e);
        ~Range();

        std::string toString(int indent = 0);
        int getStart() { return start; };
        int getEnd() { return end; };
        void addVal(int v);
        int getSum() { return sum; };

        int getEquals(int v);
        int getLessThan(int v);
        int getLessThanEquals(int v) { return this->getEquals(v) + this->getLessThan(v); };

      private:
        int start, end, sum;
        int *data;
    };

    class StatPerm
    {
      public:
        StatPerm(std::string l, std::vector<std::string> *a, Range *d, Range *w, Range *d2 = NULL, Range *w2 = NULL, Range *d3 = NULL, Range *w3 = NULL);
        ~StatPerm();

        std::string toString(int indent = 0);
        std::string getLabel() { return label; };
        std::vector<std::string> *getAttrs() { return attrs; };

        Range *getDescents() { return descents; };
        Range *getWins() { return wins; };
        Range *getDescents2() { return descents2; };
        Range *getWins2() { return wins2; };
        Range *getDescents3() { return descents3; };
        Range *getWins3() { return wins3; };

        float getQuality(Parameters *params, bool lne, int v);

      private:
        std::string label;
        std::vector<std::string> *attrs;
        Range *descents, *wins;
        Range *descents2, *wins2, *descents3, *wins3;
    };

    class Node;
    class Query;

    class Stats
    {
      public:
        Stats(Type type, unsigned int maxnode);
        Stats(std::vector<StatPerm*> *sp);
        ~Stats();

        std::string toString(int indent = 0);
        std::vector<StatPerm*> *getStatPerms() { return statperms; };

        Query *getBestQuery(Parameters *params, Type type, int maxnode);

      private:
        std::vector<StatPerm*> *statperms;
    };

    class Option
    {
      public:
        Option(std::string l, Node *n);
        Option(Type type, std::string l, unsigned int maxnode);
        ~Option();

        std::string toString(int indent = 0, bool ignorestats = false, int leafoffset = 0);
        Query *getParent() { return parent; };
        void setParent(Query *p) { parent = p; };
        std::string getLabel() { return label; };
        Node *getNode() { return node; };

      private:
        Query *parent;
        std::string label;
        Node *node;
    };

    class Query
    {
      public:
        Query(std::string l, std::vector<std::string> *a, std::vector<Option*> *o);
        Query(Type type, std::string l, std::vector<std::string> *a, unsigned int maxnode);
        ~Query();

        std::string toString(int indent = 0, bool ignorestats = false, int leafoffset = 0);
        Node *getParent() { return parent; };
        void setParent(Node *p) { parent = p; };
        std::string getLabel() { return label; };
        std::vector<std::string> *getAttrs() { return attrs; };
        std::vector<Option*> *getOptions() { return options; };

      private:
        Node *parent;
        std::string label;
        std::vector<std::string> *attrs;
        std::vector<Option*> *options;
    };

    class Node
    {
      public:
        Node(Stats *s, Query *q);
        Node(Query *q);
        Node(Stats *s, float w);
        Node(float w);
        ~Node();

        std::string toString(int indent = 0, bool ignorestats = false, int leafoffset = 0);
        bool isRoot() { return parent==NULL; };
        Option *getParent() { return parent; };
        void setParent(Option *p) { parent = p; };
        Stats *getStats() { return stats; };
        float getWeight() { return weight; };
        void setWeight(float w) { weight = w; };
        bool isLeaf() { return query == NULL; };
        int getLeafId() { return leafid; };
        Query *getQuery() { return query; };
        void setQuery(Query *q) { query = q; };
        std::string getPath();

        void populateEmptyStats(Type type, unsigned int maxnode = 0);
        void populateLeafIds(std::vector<Node*> &leafmap);
        void getTreeStats(Type type, int depth, int nodes, int &treenodes, int &leaves, int &maxdepth, int &sumdepth, int &sumsqdepth, int &maxnodes, int &sumnodes, int &sumsqnodes, float &sumlogweights, float &sumsqlogweights);

        void clearStats() { delete stats; stats = NULL; };

      private:
        Option *parent;
        Stats *stats;
        Query *query;
        int leafid;
        float weight;
    };

    Parameters *params;
    Type type;
    bool compressChain, compressEmpty;
    std::vector<std::string> *attrs;
    Node *root;
    std::vector<Node*> leafmap;
    bool statsallocated;

    DecisionTree(Parameters *p, Type t, bool cmpC, bool cmpE, std::vector<std::string> *a, DecisionTree::Node *r);

    std::list<Node*> *getLeafNodes(GraphCollection *graphs, Go::Move move, bool updatetree, bool win);
    std::list<Node*> *getStoneLeafNodes(Node *node, StoneGraph *graph, std::vector<unsigned int> *stones, bool invert, bool updatetree, bool win);
    std::list<Node*> *getIntersectionLeafNodes(Node *node, IntersectionGraph *graph, std::vector<unsigned int> *stones, bool invert, bool updatetree, bool win);
    bool updateStoneNode(Node *node, StoneGraph *graph, std::vector<unsigned int> *stones, bool invert, bool win);
    bool updateIntersectionNode(Node *node, IntersectionGraph *graph, std::vector<unsigned int> *stones, bool invert, bool win);
    unsigned int getMaxNode(Node *node);

    static float combineNodeWeights(std::list<Node*> *nodes);
    static int getDistance(Go::Board *board, int p1, int p2);
    static float computeQueryQuality(Parameters *params, int d0, int w0, int d1, int w1, int d2 = -1, int w2 = -1, int d3 = -1, int w3 = -1);
    static float sortedQueryQuality(Parameters *params, int d0, int w0, int d1, int w1, int d2 = -1, int w2 = -1, int d3 = -1, int w3 = -1);
    static void swapChildren(int &d0, int &w0, float &r0, int &d1, int &w1, float &r1);

    static std::string stripWhitespace(std::string in);
    static std::string stripComments(std::string in);
    static std::vector<std::string> *parseAttrs(std::string data, unsigned long &pos);
    static Node *parseNode(Type type, std::string data, unsigned long &pos);
    static Stats *parseStats(std::string data, unsigned long &pos);
    static std::vector<StatPerm*> *parseStatPerms(std::string data, unsigned long &pos);
    static Range *parseRange(std::string data, unsigned long &pos);
    static std::vector<Option*> *parseOptions(Type type, std::string data, unsigned long &pos);
    static float *parseNumber(std::string data, unsigned long &pos);
    static bool isText(char c);
};

#endif
