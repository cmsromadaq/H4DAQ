#ifndef CAEN_V513_H
#define CAEN_V513_H

#include "interface/StandardIncludes.hpp"
#include "interface/Board.hpp"
#include "interface/BoardConfig.hpp"


//---- CAEN V513 static define
#define CAEN_V513_CHANNELS 16
#define CAEN_V513_DATAWIDTH cvD16
#define CAEN_V513_ADDRESSMODE cvA32_U_DATA

//---- CAEN V513 Registers Start
#define CAEN_V513_VERSION_REGISTER 0xFE
#define CAEN_V513_MODTYPE_REGISTER 0xFC
#define CAEN_V513_FIXCODE_REGISTER 0xFA
#define CAEN_V513_CLEAR_INPUT_REGISTER 0x48
#define CAEN_V513_INIT_REGISTER 0x46
#define CAEN_V513_CLEAR_STROBE_REGISTER 0x44
#define CAEN_V513_RESET_REGISTER 0x42
#define CAEN_V513_CLEAR_VMEINT_REGISTER 0x40
#define CAEN_V513_CHANNEL0_STATUS_REGISTER 0x10 
#define CAEN_V513_CHANNEL_STATUS_SIZE 0x2 
#define CAEN_V513_STROBE_REGISTER 0x6
#define CAEN_V513_INPUT_REGISTER 0x4 //Only read operation
#define CAEN_V513_OUTPUT_REGISTER 0x4 //Only write operation
//---- VME INTERRUPT ADDRESSES (Not used at the moment)
#define CAEN_V513_INT_MASK_REGISTER 0x8
#define CAEN_V513_INT_LEVEL_REGISTER 0x2
#define CAEN_V513_INT_VECTOR_REGISTER 0x1
//---- CAEN V513 Registers End

//---- CAEN V513 BitMask Start
#define CAEN_V513_STROBE_STATUS_BITMASK 0x4
#define CAEN_V513_STROBE_INTERRUPT_BITMASK 0x2
#define CAEN_V513_STROBE_POLARITY_BITMASK 0x1

#define CAEN_V513_CHANNEL_TM_BITMASK 0x8
#define CAEN_V513_CHANNEL_IM_BITMASK 0x4
#define CAEN_V513_CHANNEL_POL_BITMASK 0x2
#define CAEN_V513_CHANNEL_DIR_BITMASK 0x1
//---- CAEN V513 BitMask End

class CAEN_V513: public IOControlBoard 
{
public:
  typedef enum  {
    ERR_NONE= 0,
    ERR_CONF_NOT_FOUND,
    ERR_OPEN,
    ERR_CONFIG,
    ERR_RESET,
    ERR_READ,
    ERR_WRITE,
    ERR_UNK_TRIGGER_TYPE,
    ERR_DUMMY_LAST,
  } ERROR_CODES;
  
  typedef enum {
    CAEN_V513_POS_POL=1,
    CAEN_V513_NEG_POL=0
  } CAEN_V513_Polarity_t;

  typedef enum {
    CAEN_V513_STROBE_POS_POL=0,
    CAEN_V513_STROBE_NEG_POL=1
  } CAEN_V513_Strobe_Polarity_t;

  typedef enum {
    CAEN_V513_OUTPUT=0,
    CAEN_V513_INPUT=1
  } CAEN_V513_ChannelDirection_t;

  typedef enum {
    CAEN_V513_NORMAL_INPUT=0,
    CAEN_V513_GLITCHED_INPUT=1
  } CAEN_V513_InputMode_t;

  typedef enum {
    CAEN_V513_TRANSPARENT_OUT=0,
    CAEN_V513_STROBED_OUT=1
  } CAEN_V513_TransferMode_t;

  typedef struct CAEN_V513_Config_t {
    unsigned int baseAddress;

    CAEN_V513_Strobe_Polarity_t strobePolarity;
    
    //Channels low-level configuration
    unsigned int channelsDirectionWord;
    unsigned int channelsPolarityWord;
    unsigned int channelsInputModeWord;
    unsigned int channelsTransferModeWord;

    //Signals bit (only among input channels)
    unsigned int WWEReadBitMask;
    unsigned int WEReadBitMask;
    unsigned int EEReadBitMask;

    //Trigger vetoes bit (only among output channels)
    unsigned int beamTriggerVetoBitMask;
    unsigned int pedTriggerVetoBitMask;
    unsigned int ledTriggerVetoBitMask;
  } CAEN_V513_Config_t;

  CAEN_V513(): IOControlBoard(), handle_(-1), dataRegister_(0) { type_="CAEN_V513"; };

  virtual int Init();
  virtual int Clear();
  virtual int BufferClear(); //reset the scaler
  virtual int Print() { return 0; }
  virtual int Config(BoardConfig *bC);
  virtual int Read(vector<WORD> &v);
  virtual int SetHandle(int handle) { handle_=handle;};

  // --- these are meaningful only for ioControl boards. 
  virtual bool  SignalReceived(CMD_t signal);
  virtual int   SetTriggerStatus(TRG_t triggerType, TRG_STATUS_t triggerStatus);

  // --- TriggerBoard functionalities not yet implemented

  inline CAEN_V513_Config_t* GetConfiguration() { return &configuration_; };

private:
  //low level access functions
  int ReadInput(WORD& data);
  int SetOutput(WORD data);

  uint32_t handle_;
  CAEN_V513_Config_t configuration_;
  WORD dataRegister_;
};

#endif
