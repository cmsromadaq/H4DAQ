#include "interface/CAEN_V560.hpp"
#include <iostream>
#include <sstream>
#include <string>

//#define CAEN_V560_DEBUG

int CAEN_V560::Init()
{
  
}

int CAEN_V560::Clear()
{
  int status=0;
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;

  return 0;
}      

int CAEN_V560::BufferClear()
{
  int status=0;
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;

  return 0;
}      

int CAEN_V560::Config(BoardConfig *bC)
{
  Board::Config(bC);
  //here the parsing of the xmlnode...
  GetConfiguration()->baseAddress=Configurator::GetInt( bC->getElementContent("baseAddress"));
  return 0;
}

int CAEN_V560::Read(vector<WORD> &v)
{
  int status=0;
  v.clear();
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;

  return 0;
}
