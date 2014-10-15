#include "interface/CAEN_V814.hpp"
#include <iostream>
#include <sstream>
#include <string>

#define CAEN_V814_DEBUG

int CAEN_V814::Init()
{
  int status=0;
  ostringstream s; s << "[CAEN_V814]::[INFO]::++++++ CAEN V814 INIT ++++++";
  Log(s.str(),1);
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;
  if (!IsConfigured())
    return ERR_CONF_NOT_FOUND;

  //Read Version to check connection
  WORD data=0;
  status |= CAENVME_ReadCycle(handle_,configuration_.baseAddress+CAEN_V814_VERSION_ADD,&data,CAEN_V814_ADDRESSMODE,CAEN_V814_DATAWIDTH);
  if (status)
    {
      s.str(""); s << "[CAEN_V814]::[ERROR]::Cannot open V814 board @0x" << std::hex << configuration_.baseAddress << std::dec << " " << status; 
      Log(s.str(),1);
      return ERR_OPEN;
    }    

  int version = (data&0xF000)>>12;
  int serial =  (data&0x0FFF);

  s.str(""); s << "[CAEN_V814]::[INFO]::Open V814 board @0x" << std::hex << configuration_.baseAddress << std::dec << " Version " << version << " S/N " << serial; 
  Log(s.str(),1);

  status |= SetPatternInhibit();

#ifdef CAEN_V814_DEBUG
  s.str(""); s << "[CAEN_V814]::[DEBUG]::Pattern Mask has been set";
  Log(s.str(),3);
#endif
  status |= SetThreshold(-1);
#ifdef CAEN_V814_DEBUG
  s.str(""); s << "[CAEN_V814]::[DEBUG]::All Thresholds have been set";
  Log(s.str(),3);
#endif

  for (unsigned i=0;i<CAEN_V814_CHANNELS;++i)
    if (configuration_.chThreshold[i]>0)
      {
	status |= SetThreshold(i);
#ifdef CAEN_V814_DEBUG
	s.str(""); s << "[CAEN_V814]::[DEBUG]::Set specific threshold for Ch" <<i;
	Log(s.str(),3);
#endif
      }

  status|=SetMajorityThreshold(); // single 0x6 double 0x13 triple 0x1F

#ifdef CAEN_V814_DEBUG
  s.str(""); s << "[CAEN_V814]::[DEBUG]::Majority Threshold has been set";
  Log(s.str(),3);
#endif

  status|=SetOutputWidth();

#ifdef CAEN_V814_DEBUG
  s.str(""); s << "[CAEN_V814]::[DEBUG]::Output Width has been set";
  Log(s.str(),3);
#endif

  if (status)
    {
      s.str(""); s << "[CAEN_V814]::[ERROR]::Cannot config V814 @0x" << std::hex << configuration_.baseAddress << std::dec; 
      Log(s.str(),1);
      return ERR_OPEN;
    }    
  
  s.str(""); s << "[CAEN_V814]::[INFO]::++++++ CAEN V814 CONFIGURED ++++++";  
  Log(s.str(),1);
  
  return 0;
}

int CAEN_V814::Clear()
{
  int status=0;
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;

  return Init();
}      

int CAEN_V814::BufferClear()
{
  return 0;
}      

int CAEN_V814::Config(BoardConfig *bC)
{
  Board::Config(bC);
  //Set All Configuration to 0
  for (unsigned int i=0;i<CAEN_V814_CHANNELS;++i)
    GetConfiguration()->chThreshold[i]=0;
  //here the parsing of the xmlnode...
  GetConfiguration()->baseAddress=Configurator::GetInt( bC->getElementContent("baseAddress"));
  GetConfiguration()->patternMask=Configurator::GetInt( bC->getElementContent("patternMask"));
  GetConfiguration()->outputWidth=Configurator::GetInt( bC->getElementContent("outputWidth"));
  GetConfiguration()->majorityThreshold=Configurator::GetInt( bC->getElementContent("majorityThreshold"));
  GetConfiguration()->commonThreshold=Configurator::GetInt( bC->getElementContent("commonThreshold"));


  //Now threshold per channel if they exist in configuration
  char chThresholdKey[100];
  for (unsigned int i=0;i<CAEN_V814_CHANNELS;++i)
    {
      sprintf(chThresholdKey,"ch%dThreshold",i);
      if (bC->getElementContent(chThresholdKey) == "NULL")
	continue;
      GetConfiguration()->chThreshold[i]==(int)Configurator::GetInt( bC->getElementContent(chThresholdKey) );
    }

  return 0;
}

int CAEN_V814::Read(vector<WORD> &v)
{
  return 0;
}

int CAEN_V814::SetThreshold(int channel)
{
  int status=0;

  if (channel<0)
    {
      WORD data=configuration_.commonThreshold;
      for (unsigned int i=0;i<CAEN_V814_CHANNELS;++i)
	status |= CAENVME_WriteCycle(handle_, configuration_.baseAddress +  i*0x2 , &data, CAEN_V814_ADDRESSMODE,CAEN_V814_DATAWIDTH);
    }
  else
    {
      WORD data=configuration_.chThreshold[channel];
      status |= CAENVME_WriteCycle(handle_, configuration_.baseAddress +  channel*0x2 , &data, CAEN_V814_ADDRESSMODE,CAEN_V814_DATAWIDTH);
    }

  if (status)
    {
      ostringstream s; s << "[CAEN_V814]::[ERROR]::Cannot set threshold for V814 @0x" << std::hex << configuration_.baseAddress << std::dec; 
      Log(s.str(),1);
      return ERR_CONFIG;
    }

  return 0;
}

int CAEN_V814::SetOutputWidth()
{
  int status=0;

  WORD data=configuration_.outputWidth;
  status |= CAENVME_WriteCycle(handle_, configuration_.baseAddress +  CAEN_V814_OUT_WIDTH_0_7_ADD , &data, CAEN_V814_ADDRESSMODE,CAEN_V814_DATAWIDTH);
  status |= CAENVME_WriteCycle(handle_, configuration_.baseAddress +  CAEN_V814_OUT_WIDTH_8_15_ADD , &data, CAEN_V814_ADDRESSMODE,CAEN_V814_DATAWIDTH);

  if (status)
    {
      ostringstream s; s << "[CAEN_V814]::[ERROR]::Cannot set output width for V814 @0x" << std::hex << configuration_.baseAddress << std::dec; 
      Log(s.str(),1);
      return ERR_CONFIG;
    }

  return 0;
}


int CAEN_V814::SetMajorityThreshold()
{
  int status=0;

  WORD data=configuration_.majorityThreshold;
  status |= CAENVME_WriteCycle(handle_, configuration_.baseAddress +  CAEN_V814_MAJORITY_ADD , &data, CAEN_V814_ADDRESSMODE,CAEN_V814_DATAWIDTH);
  if (status)
    {
      ostringstream s; s << "[CAEN_V814]::[ERROR]::Cannot set majority threshold for V814 @0x" << std::hex << configuration_.baseAddress << std::dec; 
      Log(s.str(),1);
      return ERR_CONFIG;
    }

  return 0;
}

int CAEN_V814::SetPatternInhibit()
{
  int status=0;

  WORD data=configuration_.patternMask;
  status |= CAENVME_WriteCycle(handle_, configuration_.baseAddress +  CAEN_V814_PATTERN_INHIBIT_ADD , &data, CAEN_V814_ADDRESSMODE,CAEN_V814_DATAWIDTH);
  if (status)
    {
      ostringstream s; s << "[CAEN_V814]::[ERROR]::Cannot set pattern mask for V814 @0x" << std::hex << configuration_.baseAddress << std::dec; 
      Log(s.str(),1);
      return ERR_CONFIG;
    }

  return 0;
}
