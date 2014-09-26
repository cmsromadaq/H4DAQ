#ifndef CAEN_V1742_H
#define CAEN_V1742_H

#include "interface/HwManager.hpp"
#include "interface/BoardConfig.hpp"

#include <CAENDigitizer.h>
#define CAEN_V1742_MAXCH  64          /* max. number of channels */
#define CAEN_V1742_MAXSET 8           /* max. number of independent settings */
#define CAEN_V1742_MAXGW  1000        /* max. number of generic write commads */

#define CAEN_V1742_VME_INTERRUPT_LEVEL      1
#define CAEN_V1742_VME_INTERRUPT_STATUS_ID  0xAAAA

#define CAEN_V1742_INTERRUPT_MODE           CAEN_DGTZ_IRQ_MODE_ROAK
#define CAEN_V1742_INTERRUPT_TIMEOUT        200  // ms


class CAEN_V1742: public Board {

public:

  typedef enum  {
    ERR_NONE= 0,
    ERR_CONF_NOT_FOUND,
    ERR_DGZ_OPEN,
    ERR_BOARD_INFO_READ,
    ERR_INVALID_BOARD_TYPE,
    ERR_DGZ_PROGRAM,
    ERR_MALLOC,
    ERR_RESTART,
    ERR_INTERRUPT,
    ERR_READOUT,
    ERR_EVENT_BUILD,
    ERR_HISTO_MALLOC,
    ERR_UNHANDLED_BOARD,
    ERR_MISMATCH_EVENTS,
    ERR_FREE_BUFFER,
    ERR_DUMMY_LAST,
  } ERROR_CODES;


  typedef struct CAEN_V1742_Config_t {
    unsigned int baseAddress;

    int LinkType;
    int LinkNum;
    int ConetNode;

    int Nch;
    int Nbit;
    float Ts;

    int RecordLength;
    unsigned int PostTrigger;

    int NumEvents;
    int InterruptNumEvents; 
    int TestPattern;
    int DesMode;
    int TriggerEdge;
    int FPIOtype;
    
    CAEN_DGTZ_TriggerMode_t ExtTriggerMode;
    
    uint8_t EnableMask;
    
    CAEN_DGTZ_TriggerMode_t ChannelTriggerMode[CAEN_V1742_MAXSET];
    
    uint32_t DCoffset[CAEN_V1742_MAXSET];
    int32_t  DCoffsetGrpCh[CAEN_V1742_MAXSET][CAEN_V1742_MAXSET];
    uint32_t Threshold[CAEN_V1742_MAXSET];
    uint8_t GroupTrgEnableMask[CAEN_V1742_MAXSET];
    
    uint32_t FTDCoffset[CAEN_V1742_MAXSET];
    uint32_t FTThreshold[CAEN_V1742_MAXSET];
    
    CAEN_DGTZ_TriggerMode_t	FastTriggerMode;
    uint32_t	 FastTriggerEnabled;
    
    int GWn;
    uint32_t GWaddr[CAEN_V1742_MAXGW];
    uint32_t GWdata[CAEN_V1742_MAXGW];
    
    int useCorrections;
    
  } CAEN_V1742_Config_t;

  CAEN_V1742():digitizerHandle_(-1), Board() { type_="CAEN_V1742"; };

  virtual int Init();
  virtual int Clear();
  virtual int BufferClear();
  virtual int Config(BoardConfig *bC);
  // --- Actually the size in bit of int is 16/32 on 32 bit and 64 on 64bit machines
  virtual int Read(vector<WORD> &v);
  virtual int SetHandle(int digitizerHandle) { digitizerHandle_ = digitizerHandle; };

  inline CAEN_V1742_Config_t* GetConfiguration() { return &digitizerConfiguration_; };
private:

  int getMoreBoardInfo();
  int programDigitizer();
  int writeEventToOutputBuffer(vector<WORD>& CAEN_V1742_rawDataBinaryBuffer, CAEN_DGTZ_EventInfo_t* eventInfo, CAEN_DGTZ_X742_EVENT_t* event);
  int ParseConfiguration();
					
  uint32_t digitizerHandle_;
  CAEN_V1742_Config_t digitizerConfiguration_;
  CAEN_DGTZ_BoardInfo_t boardInfo_;  
  
};

#endif
