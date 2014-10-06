#include "interface/CAEN_VX718.hpp"
#include <iostream>
#include <sstream>
#include <string>

//#define CAEN_VX718_DEBUG_IOSIG

int CAEN_VX718::Init()
{

  int status = 0;
  ostringstream s; s << "[CAEN_VX718]::[INFO]::++++++ CAEN VX718 INIT ++++++";
  Log(s.str(),1);
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;
  if (!IsConfigured())
    return ERR_CONF_NOT_FOUND;
  status |= CAENVME_SystemReset(handle_);
  if (status)
    return ERR_RESET;

  status |= CAENVME_WriteRegister(handle_,cvVMEControlReg,configuration_.controlRegWord);
  /*  setting the output lines */
  status |= CAENVME_SetOutputConf(handle_,cvOutput0,configuration_.Output0Polarity,configuration_.Output0LedPolarity,configuration_.Output0Source);
  status |= CAENVME_SetOutputConf(handle_,cvOutput1,configuration_.Output1Polarity,configuration_.Output1LedPolarity,configuration_.Output1Source);
  status |= CAENVME_SetOutputConf(handle_,cvOutput2,configuration_.Output2Polarity,configuration_.Output2LedPolarity,configuration_.Output2Source);
  status |= CAENVME_SetOutputConf(handle_,cvOutput3,configuration_.Output3Polarity,configuration_.Output3LedPolarity,configuration_.Output3Source);
  status |= CAENVME_SetOutputConf(handle_,cvOutput4,configuration_.Output4Polarity,configuration_.Output4LedPolarity,configuration_.Output4Source);
  status |= CAENVME_WriteRegister(handle_,cvOutMuxRegSet,configuration_.outputMuxWord);
  /* setting which output line must be pulsed  */
  outputRegister_=configuration_.outputMaskWord;
  status |= CAENVME_SetOutputRegister(handle_,configuration_.outputMaskWord);
  //setting the input lines
  status |= CAENVME_SetInputConf(handle_,cvInput0,configuration_.Input0Polarity,configuration_.Input0LedPolarity);
  status |= CAENVME_SetInputConf(handle_,cvInput1,configuration_.Input1Polarity,configuration_.Input1LedPolarity);
  //setting up the pulser
  status |= CAENVME_SetPulserConf(handle_,
				  cvPulserA,
				  configuration_.PulserATimePeriod,
				  configuration_.PulserATimeWidth,
				  configuration_.PulserATimeUnit,
				  configuration_.PulserATimePulses,
				  configuration_.PulserAStartInput,
				  configuration_.PulserAResetInput
				  );
  //setting up scaler
  status |= CAENVME_WriteRegister(handle_,cvScaler0,configuration_.scalerConfWord);
  status |= CAENVME_SetScalerConf(handle_,
   				  configuration_.ScalerLimit,
   				  configuration_.ScalerAutoReset,
   				  configuration_.ScalerSignalInput,
   				  configuration_.ScalerGateInput,
   				  configuration_.ScalerResetInput
   				  );
  PrintConfiguration();

  status |= CAENVME_EnableScalerGate(handle_); 
  if (status)
    return ERR_PROGRAM;

  s.str(""); s << "[CAEN_VX718]::[INFO]::++++++ CAEN VX718 END INIT ++++++";  
  Log(s.str(),1);

#ifdef CAEN_VX718_DEBUG_IOSIG
  uint32_t data;
  for(int i=0;i<50000;i++){
	SendSignal(DAQ_BUSY_OFF);
	SendSignal(DAQ_CLEAR_BUSY);
	BufferClear();
        status |= CAENVME_EnableScalerGate(handle_); 
	while ( !TriggerReceived() ) usleep(1);
	SendSignal(DAQ_BUSY_ON);
	usleep(500);
	Log("Loop",1);
	}	
#endif
  return 0;
}

int CAEN_VX718::Clear()
{
  int status = 0;
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;
  status |= CAENVME_SystemReset(handle_);
  if (status)
    return ERR_RESET;
  return 0;
}

int CAEN_VX718::BufferClear()
{
  //Reset the scaler counter
  int status = 0;
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;
  status |= CAENVME_ResetScalerCount(handle_);
  if (status)
    return ERR_RESET;
  return 0;

}

int CAEN_VX718::Config(BoardConfig *bC)
{
  Board::Config(bC);
  //GetConfiguration()->AAA  =Configurator::GetInt( bC->getElementContent("AAA"));  // prototype
  // fix cast
  GetConfiguration()->boardType= static_cast<CVBoardTypes> (Configurator::GetInt( bC->getElementContent("boardType")) );  
  GetConfiguration()->LinkType =Configurator::GetInt( bC->getElementContent("LinkType")); 
  GetConfiguration()->LinkNum  =Configurator::GetInt( bC->getElementContent("LinkNum")); 
  GetConfiguration()->clearBusyOutputBit= static_cast<CVOutputRegisterBits>(Configurator::GetInt( bC->getElementContent("clearBusyOutputBit")) ); 
  GetConfiguration()->daqBusyOutputBit= static_cast<CVOutputRegisterBits>(Configurator::GetInt( bC->getElementContent("daqBusyOutputBit")) ); 
  GetConfiguration()->trigAckOutputBit  = static_cast<CVOutputRegisterBits>(Configurator::GetInt( bC->getElementContent("trigAckOutputBit")) ); 
  GetConfiguration()->triggerInputBit   = static_cast<CVInputSelect> (Configurator::GetInt( bC->getElementContent("triggerInputBit")) ); 
      GetConfiguration()->outputMaskWord 		= Configurator::GetInt(bC->getElementContent("outputMaskWord"));	// uint32_t--> 
      GetConfiguration()->outputMuxWord 		= Configurator::GetInt(bC->getElementContent("outputMuxWord"));	// uint32_t--> 
      GetConfiguration()->scalerConfWord 		= Configurator::GetInt(bC->getElementContent("scalerConfWord"));	// uint32_t--> 
      GetConfiguration()->controlRegWord 		= Configurator::GetInt(bC->getElementContent("controlRegWord"));	// uint32_t-->
      GetConfiguration()->Output0Polarity 	= static_cast<CVIOPolarity>(Configurator::GetInt(bC->getElementContent("Output0Polarity")) );	// CVIOPolarity-->
      GetConfiguration()->Output0LedPolarity 	= static_cast<CVLEDPolarity>(Configurator::GetInt(bC->getElementContent("Output0LedPolarity")) );	// CVLEDPolarity-->
      GetConfiguration()->Output0Source 	= static_cast<CVIOSources> (Configurator::GetInt(bC->getElementContent("Output0Source")));	// CVIOSources-->
      GetConfiguration()->Output1Polarity 	= static_cast<CVIOPolarity> (Configurator::GetInt(bC->getElementContent("Output1Polarity")) );	// CVIOPolarity-->
      GetConfiguration()->Output1LedPolarity 	= static_cast<CVLEDPolarity> (Configurator::GetInt(bC->getElementContent("Output1LedPolarity")));	// CVLEDPolarity-->
      GetConfiguration()->Output1Source 	= static_cast<CVIOSources> (Configurator::GetInt(bC->getElementContent("Output1Source")));	// CVIOSources-->
      GetConfiguration()->Output2Polarity 	= static_cast<CVIOPolarity> (Configurator::GetInt(bC->getElementContent("Output2Polarity")) );	// CVIOPolarity-->
      GetConfiguration()->Output2LedPolarity 	= static_cast<CVLEDPolarity> (Configurator::GetInt(bC->getElementContent("Output2LedPolarity")) );	// CVLEDPolarity-->
      GetConfiguration()->Output2Source 	= static_cast<CVIOSources> (Configurator::GetInt(bC->getElementContent("Output2Source"))  );	// CVIOSources-->
      GetConfiguration()->Output3Polarity 	= static_cast<CVIOPolarity> (Configurator::GetInt(bC->getElementContent("Output3Polarity")) );	// CVIOPolarity-->
      GetConfiguration()->Output3LedPolarity 	= static_cast<CVLEDPolarity> (Configurator::GetInt(bC->getElementContent("Output3LedPolarity")) );	// CVLEDPolarity-->
      GetConfiguration()->Output3Source 	= static_cast<CVIOSources> (Configurator::GetInt(bC->getElementContent("Output3Source")) );	// CVIOSources-->
      GetConfiguration()->Output4Polarity 	= static_cast<CVIOPolarity> (Configurator::GetInt(bC->getElementContent("Output4Polarity")));	// CVIOPolarity-->
      GetConfiguration()->Output4LedPolarity 	= static_cast<CVLEDPolarity> (Configurator::GetInt(bC->getElementContent("Output4LedPolarity")) );	// CVLEDPolarity-->
      GetConfiguration()->Output4Source 	= static_cast<CVIOSources> ( Configurator::GetInt(bC->getElementContent("Output4Source")) );	// CVIOSources-->
      GetConfiguration()->Input0Polarity 	= static_cast<CVIOPolarity> ( Configurator::GetInt(bC->getElementContent("Input0Polarity"))  );	// CVIOPolarity-->
      GetConfiguration()->Input0LedPolarity 	= static_cast<CVLEDPolarity> (Configurator::GetInt(bC->getElementContent("Input0LedPolarity")) );	// CVLEDPolarity-->
      GetConfiguration()->Input1Polarity 	= static_cast<CVIOPolarity> (Configurator::GetInt(bC->getElementContent("Input1Polarity")));	// CVIOPolarity-->
      GetConfiguration()->Input1LedPolarity 	= static_cast<CVLEDPolarity> (Configurator::GetInt(bC->getElementContent("Input1LedPolarity") ));	// CVLEDPolarity-->
      GetConfiguration()->ScalerLimit 		= Configurator::GetInt(bC->getElementContent("ScalerLimit"));	// uint32_t-->
      GetConfiguration()->ScalerAutoReset 	= Configurator::GetInt(bC->getElementContent("ScalerAutoReset"));	// uint32_t-->
      GetConfiguration()->ScalerSignalInput 	= static_cast<CVIOSources> (Configurator::GetInt(bC->getElementContent("ScalerSignalInput")));	// CVIOSources-->
      GetConfiguration()->ScalerGateInput 	= static_cast<CVIOSources> (Configurator::GetInt(bC->getElementContent("ScalerGateInput")));	// CVIOSources-->
      GetConfiguration()->ScalerResetInput 	= static_cast<CVIOSources> (Configurator::GetInt(bC->getElementContent("ScalerResetInput")));	// CVIOSources-->
      GetConfiguration()->PulserATimeUnit 	= static_cast<CVTimeUnits> (Configurator::GetInt(bC->getElementContent("PulserATimeUnit")));	// CVTimeUnits-->
      GetConfiguration()->PulserATimeWidth 	= Configurator::GetInt(bC->getElementContent("PulserATimeWidth"));	// uint32_t-->
      GetConfiguration()->PulserATimePeriod 	= Configurator::GetInt(bC->getElementContent("PulserATimePeriod"));	// uint32_t-->
      GetConfiguration()->PulserATimePulses 	= Configurator::GetInt(bC->getElementContent("PulserATimePulses"));	// uint32_t-->
      GetConfiguration()->PulserAStartInput 	= static_cast<CVIOSources> (Configurator::GetInt(bC->getElementContent("PulserAStartInput")));	// CVIOSources-->
      GetConfiguration()->PulserAResetInput 	= static_cast<CVIOSources> (Configurator::GetInt(bC->getElementContent("PulserAResetInput")));	// CVIOSources-->
}

int CAEN_VX718::Read(vector<WORD> &v)
{
  //Nothing to read for VX718
  return 0;
}

int CAEN_VX718::ClearBusy()
{
  int status = 0;
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;

  //Clear scaler
  status = BufferClear();
  if (status)
    return status;
  //send DAQ_CLEAR_BUSY
  status = SendSignal(DAQ_CLEAR_BUSY);
  if  (status)
    return status;
  status |= CAENVME_EnableScalerGate(handle_); 
  if (status)
    return status;
  return 0;
}

int CAEN_VX718::SetBusyOn()
{
  int status = 0;
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;

  //send DAQ_CLEAR_BUSY
  status = SendSignal(DAQ_BUSY_ON);
  if  (status)
    return status;
  return 0;
}

int CAEN_VX718::SetBusyOff()
{
  int status = 0;
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;

  //send DAQ_CLEAR_BUSY
  status = SendSignal(DAQ_BUSY_OFF);
  if  (status)
    return status;
  return 0;
}

bool CAEN_VX718::TriggerReceived()
{
  int status=0;
  //loop until scaler switch to 1
  bool trigger=false;
  uint32_t data;

  if (handle_<0)
    return false;

  status |= CAENVME_ReadRegister(handle_, cvScaler1, &data);
  if (status)
    return false;
  if (data)
    {
      if (data>1)
	{
	  ostringstream s; s << "[CAEN_VX718]::[WARNING]::SCALER >1";
	  Log(s.str(),1);
	}
      return true;
    }
  else
    {
      return false;
    }
}

int CAEN_VX718::TriggerAck()
{
  int status = 0;
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;
  
  //send DAQ_TRIG_ACK
  status = SendSignal(DAQ_TRIG_ACK);
  if (status)
    return status;
  return 0;
}

int CAEN_VX718::SendSignal(VX718_DAQ_Signals sig)
{
  int status = 0;
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;

  if(sig==DAQ_CLEAR_BUSY)
    status |= CAENVME_PulseOutputRegister(handle_,configuration_.clearBusyOutputBit);
  else if(sig==DAQ_TRIG_ACK)
    status |= CAENVME_PulseOutputRegister(handle_,configuration_.trigAckOutputBit);
  else if(sig==DAQ_BUSY_ON)
    {
    outputRegister_ |= configuration_.daqBusyOutputBit;
    status |= CAENVME_SetOutputRegister(handle_,outputRegister_);
    }
  else if(sig==DAQ_BUSY_OFF)
    {
    outputRegister_ &= (~configuration_.daqBusyOutputBit);
    status |= CAENVME_SetOutputRegister(handle_,outputRegister_);
    }
  else
    return ERR_DAQ_SIGNAL_UNKNOWN;

  if (status)
    return ERR_DAQ_SIGNAL;

  return 0;
}

int CAEN_VX718::PrintConfiguration()
{
  uint32_t data;
  int status=0;

  std::map<std::string,CVRegisters> mainRegisters;
  mainRegisters["StatusReg"]=cvStatusReg;
  mainRegisters["VMEControlReg"]=cvVMEControlReg;
  mainRegisters["VMEIRQEnaReg"]=cvVMEIRQEnaReg;
  mainRegisters["InputReg"]=cvInputReg;
  mainRegisters["OutReg"]=cvOutRegSet;
  mainRegisters["InMuxReg"]=cvInMuxRegSet;
  mainRegisters["OutMuxReg"]=cvOutMuxRegSet;
  mainRegisters["PulserATime"]=cvPulserA0;
  mainRegisters["PulserAPulses"]=cvPulserA1;
  mainRegisters["PulserBTime"]=cvPulserB0;
  mainRegisters["Scaler0"]=cvScaler0;
  mainRegisters["Scaler1"]=cvScaler1;
  
  for (std::map<std::string,CVRegisters>::const_iterator myReg=mainRegisters.begin();myReg!=mainRegisters.end();++myReg)
    {
      status != CAENVME_ReadRegister(handle_, myReg->second, &data);
      if (status)
	return ERR_READ;
      ostringstream s; s << "[CAEN_VX718]::[INFO]::Register " << myReg->first << " 0x" << std::hex << data;
      Log(s.str(),1);
    }
  return 0;
}

int CAEN_VX718::ParseConfiguration()
{
}
