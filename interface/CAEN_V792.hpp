#ifndef CAEN_V792_H
#define CAEN_V792_H

#include "interface/StandardIncludes.hpp"
#include "interface/Board.hpp"
#include "interface/BoardConfig.hpp"

#define CAEN_V792_ADDRESSMODE cvA32_U_DATA
#define CAEN_V792_DATAWIDTH cvD16

#define CAEN_V792_OUTPUT_BUFFER         0x0000
#define CAEN_V792_FW                    0x1000
#define CAEN_V792_REG1_STATUS           0x100e
#define CAEN_V792_REG2_STATUS           0x1022
#define CAEN_V792_REG1_CONTROL          0x1010
#define CAEN_V792_BIT_SET1              0x1006
#define CAEN_V792_BIT_CLEAR1            0x1008
#define CAEN_V792_BIT_SET2              0x1032
#define CAEN_V792_BIT_CLEAR2            0x1034

#define CAEN_V792_IPED                  0x1060

#define CAEN_V792_ZS_THR                0x1080

#define CAEN_V792_MCST_CBLT_ADDRESS     0x1004
#define CAEN_V792_MCST_CBLT_CTRL        0x101a

#define CAEN_V792_INTERRUPT_LEVEL       0x100a
#define CAEN_V792_INTERRUPT_VECTOR      0x100c

#define CAEN_V792_ADER_HIGH             0x1012
#define CAEN_V792_ADER_LOW              0x1014

#define CAEN_V792_EVENT_TRIGGER_REG     0x1020

#define CAEN_V792_EVENT_COUNTER_L       0x1024
#define CAEN_V792_EVENT_COUNTER_H       0x1026

#define CAEN_V792_FCLR_WINDOW           0x102e

#define CAEN_V792_W_MEM_TEST_ADDRESS    0x1036
#define CAEN_V792_MEM_TEST_WORD_HIGH    0x1038
#define CAEN_V792_CRATE_SELECT          0x103c

#define CAEN_V792_R_TEST_ADDRESS        0x1064
#define CAEN_V792_SS_RESET_REG          0x1016

#define CAEN_V792_RDY_BITMASK 0x1
#define CAEN_V792_BUSY_BITMASK 0x4
#define CAEN_V792_FULL_BITMASK 0x4
#define CAEN_V792_EMPTY_BITMASK 0x2
#define CAEN_V792_DATARESET_BITMASK 0x4
#define CAEN_V792_SOFTRESET_BITMASK 0x80
#define CAEN_V792_BLKEND_BITMASK 0x40
#define CAEN_V792_ZS_BITMASK 0x0010
#define CAEN_V792_EMPTYEN_BITMASK 0x1000
#define CAEN_V792_OVERRANGE_BITMASK 0x0008

#define CAEN_V792_EVENT_WORDTYPE_BITMASK 0x7000000
#define CAEN_V792_EVENT_DATA 0x0
#define CAEN_V792_EVENT_BOE 0x2
#define CAEN_V792_EVENT_EOE 0x4

class CAEN_V792: public Board 
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

  typedef enum  {
    CAEN_V792_E= 0, //32ch version (IDC inputs)
    CAEN_V792_N= 1  //16ch version (LEMO inputs)
  } CAEN_V792_Model_t;

  typedef enum  {
    
  } CAEN_V792_Bitmask_t;
  

  typedef struct CAEN_V792_Config_t {
    unsigned int baseAddress;

    CAEN_V792_Model_t model;

    bool blkEnd;
    bool zeroSuppression;
    bool emptyEnable;
    bool overRange;
    
    unsigned int iped;
    unsigned int zsThreshold;

  } CAEN_V792_Config_t;

  CAEN_V792(): Board(), handle_(-1) { type_="CAEN_V792"; };

  virtual int Init();
  virtual int Clear();
  virtual int BufferClear(); //reset the buffers
  virtual int Print() { return 0; }
  virtual int Config(BoardConfig *bC);
  virtual int Read(vector<WORD> &v);
  virtual int SetHandle(int handle) { handle_=handle; return 0;};

  // --- TriggerBoard functionalities not yet implemented

  inline CAEN_V792_Config_t* GetConfiguration() { return &configuration_; };

private:

  int CheckStatusAfterRead();

  uint32_t handle_;
  CAEN_V792_Config_t configuration_;
  uint32_t channels_;
};

#endif
