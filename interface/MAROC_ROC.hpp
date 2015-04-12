#ifndef MAROC_ROC_H
#define MAROC_ROC_H

#include "interface/StandardIncludes.hpp"
#include "interface/Board.hpp"
#include "interface/BoardConfig.hpp"

#define MAROC_ROC_ADDRESSMODE cvA24_U_DATA
#define MAROC_ROC_DATAWIDTH cvD16

#define MAROC_ROC_FEB_CONFIG_BITSIZE 829

#define MAROC_ROC_BOARDDATA_REGISTER_SIZE 0x1000
#define MAROC_ROC_BOARDDATA_NWORD 72

#define MAROC_ROC_HOLD_REGISTER 0x9000
#define MAROC_ROC_CONF_REGISTER 0xC000
#define MAROC_ROC_FIFO_REGISTER 0xCC00
#define MAROC_ROC_OUTPUT_CONNECTOR_REGISTER 0xE000
#define MAROC_ROC_INPUT_CONNECTOR_REGISTER 0xF000
#define MAROC_ROC_SCALER_REGISTER 0xF200
#define MAROC_ROC_TS_LOW_REGISTER  0xF300
#define MAROC_ROC_TS_HIGH_REGISTER 0xF340
#define MAROC_ROC_CONF_TS_REGISTER 0xF380
#define MAROC_ROC_EVREG_REGISTER   0xF3C0
#define MAROC_ROC_INPUT_CONNECTOR_DAUGHTER_REGISTER 0xF400
#define MAROC_ROC_STATUS_REGISTER 0xF600

#define MAROC_ROC_TRIGGER_FALLING 0 
#define MAROC_ROC_TRIGGER_RISING 1 
#define MAROC_ROC_TRIGGER_FEBTRIG 2
#define MAROC_ROC_TRIGGER_VA1TATRIG 4
#define MAROC_ROC_TRIGGER_LEDTRIG 5
#define MAROC_ROC_TRIGGER_NIMTRIG 7 

#define MAROC_ROC_HOLD1_SUBADD 7
#define MAROC_ROC_HOLD2_SUBADD 9
#define MAROC_ROC_FEB_CONFIG_TRIG_SUBADD 2

class MAROC_ROC: public Board 
{
public:
  typedef enum  {
    ERR_NONE= 0,
    ERR_CONF_NOT_FOUND,
    ERR_OPEN,
    ERR_CONFIG,
    ERR_RESET,
    ERR_READ,
    ERR_FEB_COMM,
    ERR_DUMMY_LAST,
  } ERROR_CODES;

  typedef enum  {
    TTL_RISING= 0, 
    TTL_FALLING= 1, 
    FEB=2,     
    NIM=3,
    INTERNAL=4
  } MAROC_ROC_TriggerType_t;

  typedef struct MAROC_ROC_Config_t {
    unsigned int baseAddress;

    string configFile;

    bool zeroSuppressionMode;
    bool debugMode;

    unsigned int holdValue;
    unsigned int holdDeltaValue;

    MAROC_ROC_TriggerType_t triggerType;

    //unsigned int numBoard; //for the moment just 1 daughter board
  } MAROC_ROC_Config_t;

  MAROC_ROC(): Board(), handle_(-1) { type_="MAROC_ROC"; };

  virtual int Init();
  virtual int Clear();
  virtual int BufferClear(); //reset the buffers
  virtual int Print() { return 0; }
  virtual int Config(BoardConfig *bC);
  virtual int Read(vector<WORD> &v);
  virtual int SetHandle(int handle) { handle_=handle; return 0;};

  // --- FEB Trigger functionalities not yet implemented

  inline MAROC_ROC_Config_t* GetConfiguration() { return &configuration_; };

private:
  int CheckADCStatus(int& status);
  int ConfigFEBTrigger();
  int ConfigFEBReadoutHold();
  int ConfigROCTrigger();
  int ClearADCBusy();
  int ClockFEBBusPulse();
  int ClockReg();
  int GlobalReset();
  int InitADC();
  int LoadFEBConfiguration();
  int RegIn(bool upDown);
  int ResetTimeStamp();
  int ResetFIFO();
  int SendOnFEBBus(int address, int data);
  int SetADCSlowHold(int slowhold,int clock_sel);
  int SetADCClock(int nclk);
  int SetADCTestOff();
  int SetADCEnableDReset();
  int SetADCNormalReadoutMode();
  int SetADCZeroSuppressionMode();
  int SetMemoryMode(bool external);

  uint32_t handle_;
  MAROC_ROC_Config_t configuration_;
  uint32_t channels_;
};

#endif
