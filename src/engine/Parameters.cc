#include "Parameters.h"

Parameters::Parameters()
{
}

Parameters::~Parameters()
{
  for(std::list<Parameters::Parameter *>::iterator iter=paramlist.begin();iter!=paramlist.end();++iter)
  {
    if ((*iter)->type==Parameters::LIST)
      delete (*iter)->options;
    delete (*iter);
  }
}

void Parameters::addParameter(std::string id, int *ptr, int def, Parameters::UpdateFunction func, void *instance)
{
  Parameters::Parameter *param=new Parameters::Parameter();
  param->id=id;
  param->ptr=ptr;
  param->type=Parameters::INTEGER;
  param->func=func;
  param->instance=instance;
  paramlist.push_back(param);
  (*ptr)=def;
}

void Parameters::addParameter(std::string id, float *ptr, float def, Parameters::UpdateFunction func, void *instance)
{
  Parameters::Parameter *param=new Parameters::Parameter();
  param->id=id;
  param->ptr=ptr;
  param->type=Parameters::FLOAT;
  param->func=func;
  param->instance=instance;
  paramlist.push_back(param);
  (*ptr)=def;
}

void Parameters::addParameter(std::string id, bool *ptr, bool def, Parameters::UpdateFunction func, void *instance)
{
  Parameters::Parameter *param=new Parameters::Parameter();
  param->id=id;
  param->ptr=ptr;
  param->type=Parameters::BOOLEAN;
  param->func=func;
  param->instance=instance;
  paramlist.push_back(param);
  (*ptr)=def;
}

void Parameters::addParameter(std::string id, std::string *ptr, std::string def, Parameters::UpdateFunction func, void *instance)
{
  Parameters::Parameter *param=new Parameters::Parameter();
  param->id=id;
  param->ptr=ptr;
  param->type=Parameters::STRING;
  param->func=func;
  param->instance=instance;
  paramlist.push_back(param);
  (*ptr)=def;
}

void Parameters::addParameter(std::string id, std::string *ptr, std::list<std::string> *options, std::string def, Parameters::UpdateFunction func, void *instance)
{
  Parameters::Parameter *param=new Parameters::Parameter();
  param->id=id;
  param->ptr=ptr;
  param->type=Parameters::LIST;
  param->options=options;
  param->func=func;
  param->instance=instance;
  paramlist.push_back(param);
  (*ptr)=def;
}

bool Parameters::setParameter(std::string id, std::string val)
{
  for(std::list<Parameters::Parameter *>::iterator iter=paramlist.begin();iter!=paramlist.end();++iter)
  {
    if ((*iter)->id==id)
    {
      bool ret=false;
      switch ((*iter)->type)
      {
        case INTEGER:
          ret=this->setParameterInteger((*iter),val);
          break;
        case FLOAT:
          ret=this->setParameterFloat((*iter),val);
          break;
        case BOOLEAN:
          ret=this->setParameterBoolean((*iter),val);
          break;
        case STRING:
          ret=this->setParameterString((*iter),val);
          break;
        case LIST:
          ret=this->setParameterList((*iter),val);
          break;
      }
      if (ret && (*iter)->func!=NULL)
        (*(*iter)->func)((*iter)->instance,(*iter)->id);
      return ret;
    }
  }
  
  return false;
}

bool Parameters::setParameterInteger(Parameters::Parameter *param, std::string val)
{
  std::istringstream iss(val);
  int v;
  
  if (iss >> v)
  {
    (*(int*)(param->ptr))=v;
    return true;
  }
  else
    return false;
}

bool Parameters::setParameterFloat(Parameters::Parameter *param, std::string val)
{
  std::istringstream iss(val);
  float v;
  
  if (iss >> v)
  {
    (*(float*)(param->ptr))=v;
    return true;
  }
  else
    return false;
}

bool Parameters::setParameterBoolean(Parameters::Parameter *param, std::string val)
{
  std::istringstream iss(val);
  int v;
  
  if (iss >> v)
  {
    (*(bool*)(param->ptr))=(v==1);
    return true;
  }
  else
    return false;
}

bool Parameters::setParameterString(Parameters::Parameter *param, std::string val)
{
  (*(std::string*)(param->ptr))=val;
  return true;
}

bool Parameters::setParameterList(Parameters::Parameter *param, std::string val)
{
  std::transform(val.begin(),val.end(),val.begin(),::tolower);
  for(std::list<std::string>::iterator iter=param->options->begin();iter!=param->options->end();++iter)
  {
    if ((*iter)==val)
    {
      (*(std::string*)(param->ptr))=val;
      return true;
    }
  }
  return false;
}

void Parameters::printParametersForGTP(Gtp::Engine *gtpe)
{
  for(std::list<Parameters::Parameter *>::iterator iter=paramlist.begin();iter!=paramlist.end();++iter)
  {
    this->printParameterForGTP(gtpe,(*iter));
  }
}

void Parameters::printParameterForGTP(Gtp::Engine *gtpe, Parameters::Parameter *param)
{
  switch (param->type)
  {
    case INTEGER:
      this->printParameterIntegerForGTP(gtpe,param);
      break;
    case FLOAT:
      this->printParameterFloatForGTP(gtpe,param);
      break;
    case BOOLEAN:
      this->printParameterBooleanForGTP(gtpe,param);
      break;
    case STRING:
      this->printParameterStringForGTP(gtpe,param);
      break;
    case LIST:
      this->printParameterListForGTP(gtpe,param);
      break;
  }
}

void Parameters::printParameterIntegerForGTP(Gtp::Engine *gtpe, Parameters::Parameter *param)
{
  int val=(*(int*)(param->ptr));
  gtpe->getOutput()->printf("[string] %s %d\n",param->id.c_str(),val);
}

void Parameters::printParameterFloatForGTP(Gtp::Engine *gtpe, Parameters::Parameter *param)
{
  float val=(*(float*)(param->ptr));
  gtpe->getOutput()->printf("[string] %s %.3f\n",param->id.c_str(),val);
}

void Parameters::printParameterBooleanForGTP(Gtp::Engine *gtpe, Parameters::Parameter *param)
{
  bool val=(*(bool*)(param->ptr));
  gtpe->getOutput()->printf("[bool] %s %d\n",param->id.c_str(),val);
}

void Parameters::printParameterStringForGTP(Gtp::Engine *gtpe, Parameters::Parameter *param)
{
  std::string val=(*(std::string*)(param->ptr));
  gtpe->getOutput()->printf("[string] %s %s\n",param->id.c_str(),val.c_str());
}

void Parameters::printParameterListForGTP(Gtp::Engine *gtpe, Parameters::Parameter *param)
{
  std::string val=(*(std::string*)(param->ptr));
  gtpe->getOutput()->printf("[list");
  for(std::list<std::string>::iterator iter=param->options->begin();iter!=param->options->end();++iter)
  {
    gtpe->getOutput()->printf("/%s",(*iter).c_str());
  }  
  gtpe->getOutput()->printf("] %s %s\n",param->id.c_str(),val.c_str());
}

void Parameters::printParametersForDescription(Gtp::Engine *gtpe)
{
  for(std::list<Parameters::Parameter *>::iterator iter=paramlist.begin();iter!=paramlist.end();++iter)
  {
    this->printParameterForDescription(gtpe,(*iter));
  }
}

void Parameters::printParameterForDescription(Gtp::Engine *gtpe, Parameters::Parameter *param)
{
  switch (param->type)
  {
    case INTEGER:
      this->printParameterIntegerForDescription(gtpe,param);
      break;
    case FLOAT:
      this->printParameterFloatForDescription(gtpe,param);
      break;
    case BOOLEAN:
      this->printParameterBooleanForDescription(gtpe,param);
      break;
    case STRING:
      this->printParameterStringForDescription(gtpe,param);
      break;
    case LIST:
      this->printParameterListForDescription(gtpe,param);
      break;
  }
}

void Parameters::printParameterIntegerForDescription(Gtp::Engine *gtpe, Parameters::Parameter *param)
{
  int val=(*(int*)(param->ptr));
  gtpe->getOutput()->printf("  %s %d\n",param->id.c_str(),val);
}

void Parameters::printParameterFloatForDescription(Gtp::Engine *gtpe, Parameters::Parameter *param)
{
  float val=(*(float*)(param->ptr));
  gtpe->getOutput()->printf("  %s %.3f\n",param->id.c_str(),val);
}

void Parameters::printParameterBooleanForDescription(Gtp::Engine *gtpe, Parameters::Parameter *param)
{
  bool val=(*(bool*)(param->ptr));
  gtpe->getOutput()->printf("  %s %d\n",param->id.c_str(),val);
}

void Parameters::printParameterStringForDescription(Gtp::Engine *gtpe, Parameters::Parameter *param)
{
  std::string val=(*(std::string*)(param->ptr));
  gtpe->getOutput()->printf("  %s %s\n",param->id.c_str(),val.c_str());
}

void Parameters::printParameterListForDescription(Gtp::Engine *gtpe, Parameters::Parameter *param)
{
  std::string val=(*(std::string*)(param->ptr));
  gtpe->getOutput()->printf("  %s %s\n",param->id.c_str(),val.c_str());
}


