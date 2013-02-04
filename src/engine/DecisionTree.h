#ifndef DEF_OAKFOAM_DECISIONTREE_H
#define DEF_OAKFOAM_DECISIONTREE_H

#include <string>
#include <list>
#include <vector>
#include "Go.h"

/** Decision Tree for Feature Ensemble Method. */
class DecisionTree
{
  public:
    ~DecisionTree();

    std::string toString();

    float getWeight(Go::Board *board, Go::Move move);

    static DecisionTree *parseString(std::string rawdata);
    static DecisionTree *loadFile(std::string filename);
  
  private:
    class Range
    {
      public:
        Range(float s, float e, float v, Range *l, Range *r);
        Range(float s, float e, float v);
        ~Range();

        std::string toString(int indent);

      private:
        float start, end, val;
        Range *left;
        Range *right;
    };

    class StatPerm
    {
      public:
        StatPerm(std::string l, std::vector<std::string> *a, Range *r);
        ~StatPerm();

        std::string toString(int indent);

      private:
        std::string label;
        std::vector<std::string> *attrs;
        Range *range;
    };

    class Stats
    {
      public:
        Stats(std::vector<StatPerm*> *sp);
        ~Stats();

        std::string toString(int indent);

      private:
        std::vector<StatPerm*> *statperms;
    };

    class Node;

    class Option
    {
      public:
        Option(std::string l, Node *n);
        ~Option();

        std::string toString(int indent);
        std::string getLabel() { return label; };
        Node *getNode() { return node; };

      private:
        std::string label;
        Node *node;
    };

    class Query
    {
      public:
        Query(std::string l, std::vector<std::string> *a, std::vector<Option*> *o);
        ~Query();

        std::string toString(int indent);
        std::string getLabel() { return label; };
        std::vector<std::string> *getAttrs() { return attrs; };
        std::vector<Option*> *getOptions() { return options; };

      private:
        std::string label;
        std::vector<std::string> *attrs;
        std::vector<Option*> *options;
    };

    class Node
    {
      public:
        Node(Stats *s, Query *q);
        Node(Stats *s, float w);
        ~Node();

        std::string toString(int indent);
        float getWeight() { return weight; };
        bool isLeaf() { return query == NULL; };
        Query *getQuery() { return query; };

      private:
        Stats *stats;
        Query *query;
        float weight;
    };

    std::vector<std::string> *attrs;
    Node *root;

    DecisionTree(std::vector<std::string> *a, DecisionTree::Node *r);

    float getSparseWeight(Go::Board *board, Go::Move move);
    std::list<Node*> *getSparseLeafNodes(Node *node, Go::Board *board, std::vector<int> *stones, bool invert);

    static float combineNodeWeights(std::list<Node*> *nodes);
    static int getDistance(Go::Board *board, int p1, int p2);

    static std::string stripWhitespace(std::string in);
    static std::vector<std::string> *parseAttrs(std::string data, unsigned int &pos);
    static Node *parseNode(std::string data, unsigned int &pos);
    static Stats *parseStats(std::string data, unsigned int &pos);
    static std::vector<StatPerm*> *parseStatPerms(std::string data, unsigned int &pos);
    static Range *parseRange(std::string data, unsigned int &pos);
    static std::vector<Option*> *parseOptions(std::string data, unsigned int &pos);
    static float *parseNumber(std::string data, unsigned int &pos);
    static bool isText(char c);
};

#endif
