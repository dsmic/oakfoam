#include "Util.h"

Util::MoveTree::MoveTree(int rm, Go::Move mov, Util::MoveTree *p)
{
  parent=p;
  children=new std::list<Util::MoveTree*>();
  move=mov;
  playouts=0;
  ratio=0;
  mean=0;
  raveplayouts=0;
  raveratio=0;
  rm=ravemoves;
}

Util::MoveTree::~MoveTree()
{
  for(std::list<Util::MoveTree*>::iterator iter=children->begin();iter!=children->end();++iter) 
  {
    delete (*iter);
  }
  delete children;
}

void Util::MoveTree::addChild(Util::MoveTree *node)
{
  children->push_back(node);
  node->parent=this;
}

void Util::MoveTree::addWin(float score)
{
  int wins=ratio*playouts;
  float totalscore=mean*playouts;
  wins++;
  totalscore+=score;
  playouts++;
  ratio=(float)wins/playouts;
  mean=(float)totalscore/playouts;
  this->passPlayoutUp();
}

void Util::MoveTree::addLose(float score)
{
  int wins=ratio*playouts;
  float totalscore=mean*playouts;
  totalscore+=score;
  playouts++;
  ratio=(float)wins/playouts;
  mean=(float)totalscore/playouts;
  this->passPlayoutUp();
}

void Util::MoveTree::addRAVEWin()
{
  int ravewins=raveratio*raveplayouts;
  ravewins++;
  raveplayouts++;
  raveratio=(float)ravewins/raveplayouts;
  this->passPlayoutUp();
}

void Util::MoveTree::addRAVELose()
{
  int ravewins=raveratio*raveplayouts;
  raveplayouts++;
  raveratio=(float)ravewins/raveplayouts;
  this->passPlayoutUp();
}

Util::MoveTree *Util::MoveTree::getChild(Go::Move move)
{
  for(std::list<Util::MoveTree*>::iterator iter=children->begin();iter!=children->end();++iter) 
  {
    if ((*iter)->getMove().getColor()==move.getColor() && (*iter)->getMove().getX()==move.getX() && (*iter)->getMove().getY()==move.getY()) 
      return (*iter);
  }
  return NULL;
}

float Util::MoveTree::makeRAVERatio(float ratio, float raveratio, int playouts, int ravemoves)
{
  float alpha;
  if (ravemoves>0)
    alpha=(ravemoves-playouts)/ravemoves;
  else
    alpha=0;
  
  if (alpha<0)
    alpha=0;
  return raveratio*alpha + ratio*(1-alpha);
}

float Util::MoveTree::getRAVERatio()
{
  return Util::MoveTree::makeRAVERatio(ratio,raveratio,playouts,ravemoves);
}

void Util::MoveTree::updateFromChildPlayout()
{ 
  float currentrr=0;
  ratio=0;
  raveratio=0;
  mean=0;
  playouts++;
  
  for(std::list<Util::MoveTree*>::iterator iter=children->begin();iter!=children->end();++iter) 
  {
    if ((*iter)->playouts>0 || (*iter)->raveplayouts>0)
    {
      float childratio=(*iter)->ratio;
      float childraveratio=(*iter)->raveratio;
      float childmean=(*iter)->mean;
      float childrr=Util::MoveTree::makeRAVERatio(1-childratio,1-childraveratio,playouts,ravemoves);
      
      if (childrr>currentrr)
      {
        ratio=1-childratio;
        raveratio=1-childraveratio;
        mean=childmean;
        currentrr=Util::MoveTree::makeRAVERatio(ratio,raveratio,playouts,ravemoves);
      }
    }
  }
  
  this->passPlayoutUp();
}

void Util::MoveTree::passPlayoutUp()
{
  if (parent!=NULL)
    parent->updateFromChildPlayout();
}

