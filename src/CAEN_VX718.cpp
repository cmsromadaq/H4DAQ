#include "interface/CAEN_VX718.hpp"
#include <iostream>
#include <string>

int CAEN_VX718::Init()
{

  int status = 0;
  std::cout << "[VX718]::[INFO]::++++++ CAEN VX718 INIT ++++++" << std::endl;
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
  status |= CAENVME_EnableScalerGate(handle_); 
  if (status)
    return ERR_PROGRAM;

  PrintConfiguration();
  std::cout << "[VX718]::[INFO]::++++++ CAEN VX718 END INIT ++++++" << std::endl;  
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
  ParseConfiguration();
}

int CAEN_VX718::Read(vector<WORD> &v)
{
  //Nothing to read for VX718
}

int CAEN_VX718::TriggerReceived()
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
  
  //loop until scaler switch to 1
  bool trigger=false;
  uint32_t data;
  while(!trigger) //maybe implement also a trigger timeout later
    {
      status |= CAENVME_ReadRegister(handle_, cvScaler1, &data);
      if (status)
	return ERR_TRIGGER_READ;
      if (data)
	{
	  trigger=true;
	  if (data>1)
	    std::cout << "[VX718]::[WARNING]::SCALER >1" << std::endl;
	}
    }
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
  mainRegisters["Scaler0Conf"]=cvScaler0;
  mainRegisters["Scaler1Conf"]=cvScaler1;
  
  for (std::map<std::string,CVRegisters>::const_iterator myReg=mainRegisters.begin();myReg!=mainRegisters.end();++myReg)
    {
      status != CAENVME_ReadRegister(handle_, myReg->second, &data);
      if (status)
	return ERR_READ;
      std::cout << "[VX718]::[INFO]::Register " << myReg->first << " 0x" << std::hex << data << std::endl;
    }
  return 0;
}

int CAEN_VX718::ParseConfiguration()
{
}
