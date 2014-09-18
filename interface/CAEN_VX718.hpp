#include "interface/HwManager.hpp"
#include "interface/BoardConfig.hpp"

#ifdef __linux__
#define LINUX
#endif

#include "CAENVMElib.h"
#include "CAENVMEtypes.h" 
#include "CAENVMEoslib.h"

class CAEN_VX718: public Board 
{
public:
  typedef enum  {
    ERR_NONE= 0,
    ERR_CONF_NOT_FOUND= 2,
    ERR_OPEN,
    ERR_PROGRAM,
    ERR_TRIGGER_READ,
    ERR_RESTART,
    ERR_DUMMY_LAST,
  } ERROR_CODES;
  
  typedef enum VX718_DAQ_Signals 
    {
      DAQ_CLEAR_BUSY=0,
      DAQ_TRIG_ACK,
      DAQ_LAST_SIGNAL,
    } VX718_DAQ_Signals;

  typedef struct CAEN_VX718_Config_t {
    CVBoardTypes boardType;

    uint32_t baseAddress;

    int32_t LinkType;
    int32_t LinkNum;

    CVOutputSelect clearBusyOutputBit;
    CVOutputSelect trigAckOutputBit;
    CVInputSelect  triggerInputBit;

    uint32_t outputMaskWord; 
    uint32_t outputMuxWord; 
    uint32_t scalerConfWord; 
    uint32_t controlRegWord;
    
    CVIOPolarity Output0Polarity;
    CVLEDPolarity Output0LedPolarity;
    CVIOSources Output0Source;

    CVIOPolarity Output1Polarity;
    CVLEDPolarity Output1LedPolarity;
    CVIOSources Output1Source;

    CVIOPolarity Output2Polarity;
    CVLEDPolarity Output2LedPolarity;
    CVIOSources Output2Source;

    CVIOPolarity Output3Polarity;
    CVLEDPolarity Output3LedPolarity;
    CVIOSources Output3Source;

    CVIOPolarity Output4Polarity;
    CVLEDPolarity Output4LedPolarity;
    CVIOSources Output4Source;

    CVIOPolarity Input0Polarity;
    CVLEDPolarity Input0LedPolarity;

    CVIOPolarity Input1Polarity;
    CVLEDPolarity Input1LedPolarity;
    
    uint32_t ScalerLimit;
    uint32_t ScalerAutoReset;
    CVIOSources ScalerSignalInput;
    CVIOSources ScalerGateInput;
    CVIOSources ScalerResetInput;

    CVTimeUnits PulserATimeUnit;
    uint32_t PulserATimeWidth;
    uint32_t PulserATimePulses;
    CVIOSources PulserAStartInput;
    CVIOSources PulserAResetInput;

  } CAEN_VX718_Config_t;

  CAEN_VX718(): handle_(-1){};

  virtual int Init();
  virtual int Clear();
  virtual int BufferClear(); //reset the scaler
  virtual int Config(BoardConfig *bC);
  virtual int Read(vector<WORD> &v);
  virtual int SetHandle(vector<WORD> &v);
  virtual int TriggerReceived();

private:
  int SendSignal(VX718_DAQ_Signals);
  int PrintConfiguration();
  int ParseConfiguration();
					
  uint32_t handle_;
  CAEN_VX718_Config_t configuration_;
};
