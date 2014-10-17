#include "interface/CAEN_V560.hpp"
#include <iostream>
#include <sstream>
#include <string>

#define CAEN_V560_DEBUG

int CAEN_V560::Init()
{
  int status=0;
  ostringstream s; s << "[CAEN_V560]::[INFO]::++++++ CAEN V560 INIT ++++++";
  Log(s.str(),1);
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;
  if (!IsConfigured())
    return ERR_CONF_NOT_FOUND;

  //Read Version to check connection
  WORD data=0;
  status |= CAENVME_ReadCycle(handle_,configuration_.baseAddress+CAEN_V560_VERSION_REGISTER,&data,CAEN_V560_ADDRESSMODE,cvD16);
  if (status)
    {
      s.str(""); s << "[CAEN_V560]::[ERROR]::Cannot open V560 board @0x" << std::hex << configuration_.baseAddress << std::dec << " " << status; 
      Log(s.str(),1);
      return ERR_OPEN;
    }    

  int version = (data&0xF000)>>12;
  int serial =  (data&0x0FFF);

  s.str(""); s << "[CAEN_V560]::[INFO]::Open V560 board @0x" << std::hex << configuration_.baseAddress << std::dec << " Version " << version << " S/N " << serial; 
  Log(s.str(),1);

  data=0;
  status |= CAENVME_ReadCycle(handle_,configuration_.baseAddress+CAEN_V560_REG_CLEAR,&data,CAEN_V560_ADDRESSMODE,cvD16);
  if (status)
    {
      s.str(""); s << "[CAEN_V560]::[ERROR]::Cannot reset board V560 @0x" << std::hex << configuration_.baseAddress << std::dec; 
      Log(s.str(),1);
      return ERR_OPEN;
    }    

  data=0;
  status |= CAENVME_ReadCycle(handle_,configuration_.baseAddress+CAEN_V560_REG_VETO_CLEAR,&data,CAEN_V560_ADDRESSMODE,cvD16);
  if (status)
    {
      s.str(""); s << "[CAEN_V560]::[ERROR]::Cannot reset veto for board V560 @0x" << std::hex << configuration_.baseAddress << std::dec; 
      Log(s.str(),1);
      return ERR_OPEN;
    }    

  s.str(""); s << "[CAEN_V560]::[INFO]::++++++ CAEN V560 CONFIGURED ++++++";  
  Log(s.str(),1);

  return 0;
}

int CAEN_V560::Clear()
{
  int status=0;
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;

  WORD data=0xFF;
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+CAEN_V560_REG_CLEAR,&data,CAEN_V560_ADDRESSMODE,cvD16);
  if (status)
    {
      ostringstream s; s << "[CAEN_V560]::[ERROR]::Cannot reset V560 board " << status << std::dec; 
      Log(s.str(),1);
      return ERR_RESET;
    }  

  return 0;
}      

int CAEN_V560::BufferClear()
{
  return 0;

  return Clear();
}      

int CAEN_V560::Config(BoardConfig *bC)
{
  Board::Config(bC);
  //here the parsing of the xmlnode...
  GetConfiguration()->baseAddress=Configurator::GetInt( bC->getElementContent("baseAddress"));
  GetConfiguration()->enabledChannels=Configurator::GetInt( bC->getElementContent("enabledChannels"));
  return 0;
}

int CAEN_V560::Read(vector<WORD> &v)
{
  int status=0;
  v.clear();
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;

  WORD data;
  for(int i=0; i<CAEN_V560_CHANNELS; ++i) {

    if ( ! (configuration_.enabledChannels & (1<<i) ) )
      continue; //skipping disabled channels

    status|= CAENVME_ReadCycle(handle_, configuration_.baseAddress + CAEN_V560_REG_COUNTER0 + i*0x04 ,&data, CAEN_V560_ADDRESSMODE ,cvD32);

    if (status)
    {
      ostringstream s; s << "[CAEN_V560]::[ERROR]::Cannot read V560 board " << status << std::dec; 
      Log(s.str(),1);
      v.clear();
      return ERR_READ;
    }  
    
#ifdef CAEN_V560_DEBUG
    ostringstream s;
    s << "[CAEN_V560]::[DEBUG]:: channel " << i << " has " << data << " counts";
    Log(s.str(),3);
#endif

    v.push_back(data);
  }  

  return 0;
}
