#ifndef LECROY_1182_H
#define LECROY_1182_H

#include "interface/StandardIncludes.hpp"
#include "interface/Board.hpp"
#include "interface/BoardConfig.hpp"

#define LECROY_1182_ADDRESSMODE cvA24_U_DATA
#define LECROY_1182_DATAWIDTH cvD16

#define LECROY_1182_CLR0_REG              0x0000

#define LECROY_1182_DATA_START_REG        0x0100 //start of event0 for channel0
#define LECROY_1182_DATA_REG_SIZE         0x0002 //channel size in bytes (this is a 12bit ADC)

#define LECROY_1182_RDY_BITMASK           0x001
#define LECROY_1182_BUSY_BITMASK          0x002
#define LECROY_1182_FULL_BITMASK          0x008

#define LECROY_1182_EVTBUFFER_BITMASK     0x0F0
#define LECROY_1182_DATARESET_BITMASK     0x100


class LECROY_1182: public Board 
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

  typedef struct LECROY_1182_Config_t {
    unsigned int baseAddress;
    uint32_t clrRegWord;
  } LECROY_1182_Config_t;

  LECROY_1182(): Board(), handle_(-1), channels_(8) { type_="LECROY_1182"; };

  virtual int Init();
  virtual int Clear();
  virtual int BufferClear(); //reset the buffers
  virtual int Print() { return 0; }
  virtual int Config(BoardConfig *bC);
  virtual int Read(vector<WORD> &v);
  virtual int SetHandle(int handle) { handle_=handle; return 0;};

  // --- TriggerBoard functionalities not yet implemented

  inline LECROY_1182_Config_t* GetConfiguration() { return &configuration_; };

private:

  int CheckStatusAfterRead();

  uint32_t handle_;
  LECROY_1182_Config_t configuration_;
  uint32_t channels_;
};

#endif
