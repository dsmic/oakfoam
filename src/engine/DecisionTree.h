#ifndef DEF_OAKFOAM_DECISIONTREE_H
#define DEF_OAKFOAM_DECISIONTREE_H

#include <string>
#include <list>

/** Decision Tree for Feature Ensemble Method. */
class DecisionTree
{
  public:
    /** Create a new decision tree. */
    DecisionTree();

    static DecisionTree *parseString(std::string rawdata);
    static DecisionTree *loadFile(std::string filename);
  
  private:
    static std::string stripWhitespace(std::string in);
    static std::list<std::string> *parseAttrs(std::string data, unsigned int &pos);
    static bool isText(char c);
};

#endif
