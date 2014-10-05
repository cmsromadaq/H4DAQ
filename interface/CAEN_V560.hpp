#ifndef CAEN_V560_H
#define CAEN_V560_H

#include "interface/StandardIncludes.hpp"
#include "interface/Board.hpp"
#include "interface/BoardConfig.hpp"


//---- CAEN V560 static define
#define CAEN_V560_CHANNELS 16

// #define CAEN_V560_DATAWIDTH cvD16
#define CAEN_V560_ADDRESSMODE cvA32_U_DATA

#define CAEN_V560_REG_STATUS       0x0058
#define CAEN_V560_REG_VETO_CLEAR   0x0054
#define CAEN_V560_CLEAR_VME_INTER  0x000c
#define CAEN_V560_SCALE_INCREMENT  0x0056
#define CAEN_V560_VERSION_REGISTER 0x00FE
#define CAEN_V560_MANEMOD_TYPE     0x00FC
#define CAEN_V560_FIXEDCODE        0x00FA
#define CAEN_V560_REG_VETO_SET     0x0052
#define CAEN_V560_REG_CLEAR        0x0050
#define CAEN_V560_REG_TEST         0x0056

#define CAEN_V560_REG_COUNTER0     0x0010
#define CAEN_V560_REG_COUNTER1     0x0014
#define CAEN_V560_REG_COUNTER2     0x0018
#define CAEN_V560_REG_COUNTER3     0x001C
#define CAEN_V560_REG_COUNTER4     0x0020
#define CAEN_V560_REG_COUNTER5     0x0024
#define CAEN_V560_REG_COUNTER6     0x0028
#define CAEN_V560_REG_COUNTER7     0x002C
#define CAEN_V560_REG_COUNTER8     0x0030
#define CAEN_V560_REG_COUNTER9     0x0034
#define CAEN_V560_REG_COUNTER10    0x0038
#define CAEN_V560_REG_COUNTER11    0x003C
#define CAEN_V560_REG_COUNTER12    0x0040
#define CAEN_V560_REG_COUNTER13    0x0044
#define CAEN_V560_REG_COUNTER14    0x0048
#define CAEN_V560_REG_COUNTER15    0x004C

class CAEN_V560: public Board 
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
  
  typedef struct CAEN_V560_Config_t {
    unsigned int baseAddress;
  } CAEN_V560_Config_t;

  CAEN_V560(): Board(), handle_(-1) { type_="CAEN_V560"; };

  virtual int Init();
  virtual int Clear();
  virtual int BufferClear(); //reset the scaler
  virtual int Print() { return 0; }
  virtual int Config(BoardConfig *bC);
  virtual int Read(vector<WORD> &v);
  virtual int SetHandle(int handle) { handle_=handle;};

  inline CAEN_V560_Config_t* GetConfiguration() { return &configuration_; };

private:
  uint32_t handle_;
  CAEN_V560_Config_t configuration_;
};

#endif
