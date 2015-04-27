#ifndef CAEN_V265_H
#define CAEN_V265_H

#include "interface/StandardIncludes.hpp"
#include "interface/Board.hpp"
#include "interface/BoardConfig.hpp"

#define CAEN_V265_ADDRESSMODE cvA24_U_DATA
#define CAEN_V265_DATAWIDTH cvD16

#define CAEN_V265_CHANNELS              8

#define CAEN_V265_STATUS                0x0000
#define CAEN_V265_CLEAR                 0x0002
#define CAEN_V265_DAC                   0x0004
#define CAEN_V265_GATE                  0x0006
#define CAEN_V265_OUTPUT_BUFFER         0x0008
#define CAEN_V265_FIXED_CODE            0x00FA
#define CAEN_V265_MODEL                 0x00FC
#define CAEN_V265_SERIAL                0x00FE

#define CAEN_V265_VERSION_BITMASK 0xF000
#define CAEN_V265_SERIAL_BITMASK 0x0FFF
#define CAEN_V265_CAEN_BITMASK 0xFC00
#define CAEN_V265_MODULE_BITMASK 0x03FF
#define CAEN_V265_FIXEDCODE_BITMASK 0xFAF5

#define CAEN_V265_READY_BITMASK 0x8000
#define CAEN_V265_FULL_BITMASK 0x4000
#define CAEN_V265_RANGE_BITMASK 0x1000
#define CAEN_V265_CHANNEL_BITMASK 0xE000
#define CAEN_V265_ADCDATUM_BITMASK 0x0FFF

 
class CAEN_V265: public Board 
{
public:
  typedef enum  {
    ERR_NONE= 0,
    ERR_CONF_NOT_FOUND,
    ERR_OPEN,
    ERR_CONFIG,
    ERR_RESET,
    ERR_READ,
    ERR_DUMMY_LAST,
  } ERROR_CODES;
  

  typedef struct CAEN_V265_Config_t {
    unsigned int baseAddress;
  } CAEN_V265_Config_t;

  CAEN_V265(): Board(), handle_(-1) { type_="CAEN_V265"; };

  virtual int Init();
  virtual int Clear();
  virtual int BufferClear(); //reset the buffers
  virtual int ClearBusy(); //reset the buffers
  virtual int Print() { return 0; }
  virtual int Config(BoardConfig *bC);
  virtual int Read(vector<WORD> &v);
  virtual int SetHandle(int handle) { handle_=handle; return 0;};

  // --- TriggerBoard functionalities not yet implemented

  inline CAEN_V265_Config_t* GetConfiguration() { return &configuration_; };

private:

  int CheckStatusAfterRead();

  uint32_t handle_;
  CAEN_V265_Config_t configuration_;
};

#endif
