#include "interface/CAEN_V513.hpp"
#include <iostream>
#include <sstream>
#include <string>

#define CAEN_V513_DEBUG

int CAEN_V513::Init()
{
  int status=0;
  ostringstream s; s << "[CAEN_V513]::[INFO]::++++++ CAEN V513 INIT ++++++";
  Log(s.str(),1);
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;
  if (!IsConfigured())
    return ERR_CONF_NOT_FOUND;

  //Read Version to check connection
  WORD data=0;
  status |= CAENVME_ReadCycle(handle_,configuration_.baseAddress+CAEN_V513_VERSION_REGISTER,&data,CAEN_V513_ADDRESSMODE,CAEN_V513_DATAWIDTH);
  if (status)
    {
      s.str(""); s << "[CAEN_V513]::[ERROR]::Cannot open V513 board @0x" << std::hex << configuration_.baseAddress << std::dec; 
      Log(s.str(),1);
      return ERR_OPEN;
    }    

  int version = data&0xF000;
  int serial = data&0x0FFF;
  s.str(""); s << "[CAEN_V513]::[INFO]::Open V513 board @0x" << std::hex << configuration_.baseAddress << std::dec << " Version " << version << " S/N " << serial; 
  Log(s.str(),1);
  
  //Reset module
  data=0xFF;
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+CAEN_V513_RESET_REGISTER,&data,CAEN_V513_ADDRESSMODE,CAEN_V513_DATAWIDTH);
  if (status)
    {
      s.str(""); s << "[CAEN_V513]::[ERROR]::Cannot reset V513 board " << status << std::dec; 
      Log(s.str(),1);
      return ERR_RESET;
    }    
  
  //Set strobe config
  data=configuration_.strobePolarity & CAEN_V513_STROBE_POLARITY_BITMASK;
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+CAEN_V513_STROBE_REGISTER,&data,CAEN_V513_ADDRESSMODE,CAEN_V513_DATAWIDTH);

  for (unsigned int i=0;i<CAEN_V513_CHANNELS;++i)
    {
      int channelDirection=(configuration_.channelsDirectionWord & (1 << i));
      int channelPolarity=(configuration_.channelsPolarityWord & (1 << i));
      int channelInputMode=(configuration_.channelsInputModeWord & (1 << i));
      int channelTransferMode=(configuration_.channelsTransferModeWord & (1 << i));
      data= (channelDirection*CAEN_V513_CHANNEL_DIR_BITMASK) | (channelPolarity*CAEN_V513_CHANNEL_POL_BITMASK) | (channelInputMode*CAEN_V513_CHANNEL_IM_BITMASK) | (channelTransferMode*CAEN_V513_CHANNEL_TM_BITMASK);
      status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+CAEN_V513_CHANNEL0_STATUS_REGISTER+i*CAEN_V513_CHANNEL_STATUS_SIZE,&data,CAEN_V513_ADDRESSMODE,CAEN_V513_DATAWIDTH);
    }
  
  //Now set all the output channels to 1 (Will enable trigger veto)
  for (unsigned int i=0;i<CAEN_V513_CHANNELS;++i)
    {
      if ( (configuration_.channelsDirectionWord & (1 << i)) == CAEN_V513_OUTPUT  && (configuration_.channelsPolarityWord & (1 << i)) == CAEN_V513_POS_POL ) 
	dataRegister_ |= 1>>i;
    }

  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+CAEN_V513_OUTPUT_REGISTER,&dataRegister_,CAEN_V513_ADDRESSMODE,CAEN_V513_DATAWIDTH);

  if (status)
    {
      s.str(""); s << "[CAEN_V513]::[ERROR]::Config error" << status;
      Log(s.str(),1);
      return ERR_CONFIG;
    } 
  s.str(""); s << "[CAEN_V513]::[INFO]::++++++ CAEN V513 CONFIGURED ++++++";  
  Log(s.str(),1);
  return 0;
}

int CAEN_V513::Clear()
{
  int status=0;
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;

  WORD data=0xFF;
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+CAEN_V513_RESET_REGISTER,&data,CAEN_V513_ADDRESSMODE,CAEN_V513_DATAWIDTH);
  if (status)
    {
      ostringstream s; s << "[CAEN_V513]::[ERROR]::Cannot reset V513 board " << status << std::dec; 
      Log(s.str(),1);
      return ERR_RESET;
    }  
  return 0;
}      

int CAEN_V513::BufferClear()
{
  //Do nothing we do not want to change the I/O register bit values
  return 0;
}      

int CAEN_V513::Config(BoardConfig *bC)
{
  Board::Config(bC);
  //here the parsing of the xmlnode...
  GetConfiguration()->baseAddress=Configurator::GetInt( bC->getElementContent("baseAddress"));
  GetConfiguration()->strobePolarity=static_cast<CAEN_V513_Strobe_Polarity_t>(Configurator::GetInt( bC->getElementContent("strobePolarity")));
    
  //Channels low-level configuration
  GetConfiguration()->channelsDirectionWord=Configurator::GetInt( bC->getElementContent("channelsDirectionWord"));
  GetConfiguration()->channelsPolarityWord=Configurator::GetInt( bC->getElementContent("channelsPolarityWord"));
  GetConfiguration()->channelsInputModeWord=Configurator::GetInt( bC->getElementContent("channelsInputModeWord"));
  GetConfiguration()->channelsTransferModeWord=Configurator::GetInt( bC->getElementContent("channelsTransferModeWord"));
  
  //Signals bit
  GetConfiguration()->WWEReadBitMask=Configurator::GetInt( bC->getElementContent("WWEReadBitMask"));
  GetConfiguration()->WEReadBitMask=Configurator::GetInt( bC->getElementContent("WEReadBitMask"));
  GetConfiguration()->EEReadBitMask=Configurator::GetInt( bC->getElementContent("EEReadBitMask"));
  
  //Trigger vetoes bit
  GetConfiguration()->beamTriggerVetoBitMask=Configurator::GetInt( bC->getElementContent("beamTriggerVetoBitMask"));
  GetConfiguration()->pedTriggerVetoBitMask=Configurator::GetInt( bC->getElementContent("pedTriggerVetoBitMask"));
  GetConfiguration()->ledTriggerVetoBitMask=Configurator::GetInt( bC->getElementContent("ledTriggerVetoBitMask"));

  return 0;
}

int CAEN_V513::Read(vector<WORD> &v)
{
  v.clear();
  //Put in output the value of the I/O register
  WORD data=0;
  ReadInput(data);
#ifdef CAEN_V513_DEBUG
  ostringstream s; s << "[CAEN_V513]::[DEBUG] DATA PATTERN 0x"<< data ;
  Log(s.str(),3);
#endif
  v.push_back(data);
  return 0;
}

int CAEN_V513::ReadInput(WORD& data)
{
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;

  int status=0;
  status |= CAENVME_ReadCycle(handle_,configuration_.baseAddress+CAEN_V513_INPUT_REGISTER,&data,CAEN_V513_ADDRESSMODE,CAEN_V513_DATAWIDTH);
  if (status)
    {
      ostringstream s; s << "[CAEN_V513]::[ERROR]::Cannot read I/O register " << status; 
      Log(s.str(),1);
      return ERR_READ;
    }
  return 0;
}

int CAEN_V513::SetOutput(WORD data)
{
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;

  int status=0;
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+CAEN_V513_OUTPUT_REGISTER,&data,CAEN_V513_ADDRESSMODE,CAEN_V513_DATAWIDTH);
  if (status)
    {
      ostringstream s; s << "[CAEN_V513]::[ERROR]::Cannot write I/O register " << status; 
      Log(s.str(),1);
      return ERR_WRITE;
    }
  dataRegister_=data;
  return 0;
}

bool CAEN_V513::SignalReceived(CMD_t signal)
{
  WORD data;
  ReadInput(data);

  if (signal == WWE ) 
      {
	return data & configuration_.WWEReadBitMask;
      }
  else if (signal == WE ) 
      {
	return data & configuration_.WEReadBitMask;
      }
  else if (signal == EE ) 
      {
	return data & configuration_.EEReadBitMask;
      }
   else
      {
	ostringstream s; s << "[CAEN_V513]::[ERROR]::Signal " << signal << " not handled by CAEN_V513";
	Log(s.str(),1);
	return false;
      }
  return false;
}
     
int CAEN_V513::SetTriggerStatus(TRG_t triggerType, TRG_STATUS_t triggerStatus)
{
  //Assuming output bits to be VETO bits, clearing them when triggerStatus is TRIG_ON
  //All output bits set to 1 at INIT level
  if (triggerType == BEAM_TRIG)
    {
	if (triggerStatus == TRIG_ON)
	  dataRegister_ &= ~configuration_.beamTriggerVetoBitMask;
      }
  else if ( triggerType == PED_TRIG )
      {
	if (triggerStatus == TRIG_ON)
	  dataRegister_ &= ~configuration_.pedTriggerVetoBitMask;
      }
  else if ( triggerType == LED_TRIG ) 
      {
	if (triggerStatus == TRIG_ON)
	  dataRegister_ &= ~configuration_.ledTriggerVetoBitMask;
      }
  else 
    {
      ostringstream s; s << "[CAEN_V513]::[ERROR]::Trigger type " << triggerType << " not handled by CAEN_V513";
      Log(s.str(),1);
      return ERR_UNK_TRIGGER_TYPE;
    }

  int status=SetOutput(dataRegister_);
  return status;
}
      
      
