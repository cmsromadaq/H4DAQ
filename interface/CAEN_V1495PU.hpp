#ifndef CAEN_V560_H
#define CAEN_V1495PU_H

#include "interface/StandardIncludes.hpp"
#include "interface/Board.hpp"
#include "interface/BoardConfig.hpp"



// #define CAEN_V1495PU_DATAWIDTH cvD16
#define CAEN_V1495PU_ADDRESSMODE cvA32_U_DATA

#define CAEN_V1495_PATTERNUNIT_MASKA_ADDRESS 0x1010
#define CAEN_V1495_PATTERNUNIT_MASKB_ADDRESS 0x1014
#define CAEN_V1495_PATTERNUNIT_MASKE_ADDRESS 0x101C
#define CAEN_V1495_PATTERNUNIT_MASKF_ADDRESS 0x1020
#define CAEN_V1495_PATTERNUNIT_CTRLREG_ADDRESS 0x1018
#define CAEN_V1495_PATTERNUNIT_DELAY_ADDRESS 0x1018

#define CAEN_V1495_PATTERNUNIT_PATTERNA_ADDRESS 0x103C
#define CAEN_V1495_PATTERNUNIT_PATTERNB_ADDRESS 0x1040
#define CAEN_V1495_PATTERNUNIT_PATTERNE_ADDRESS 0x1044
#define CAEN_V1495_PATTERNUNIT_PATTERNF_ADDRESS 0x1048
#define CAEN_V1495_PATTERNUNIT_STATUS_ADDRESS 0x1050

#define CAEN_V1495_PATTERNUNIT_MODULERESET_ADDRESS 0x800A

#define CAEN_V1495_PATTERNUNIT_VMEFPGA_FWVERSION_ADDRESS 0x800C
#define CAEN_V1495_PATTERNUNIT_VMEFPGAFWVERSION_MAJORNUMBER_BITMASK 0xFF
#define CAEN_V1495_PATTERNUNIT_VMEFPGAFWVERSION_MINORNUMBER_BITMASK 0xFF

#define CAEN_V1495_PATTERNUNIT_USERFPGA_FWVERSION_ADDRESS 0x100C
#define CAEN_V1495_PATTERNUNIT_USERFPGAFWVERSION_MAJORNUMBER_BITMASK 0xF
#define CAEN_V1495_PATTERNUNIT_USERFPGAFWVERSION_MINORNUMBER_BITMASK 0xF

class CAEN_V1495PU: public Board 
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
  
  typedef struct CAEN_V1495PU_Config_t {
    
    unsigned int baseAddress;
    
    unsigned int ctrlRegWord;
    unsigned int maskA;
    unsigned int maskB;
    unsigned int maskE;
    unsigned int maskF;
    unsigned int sigDelay;
  } CAEN_V1495PU_Config_t;

  CAEN_V1495PU(): Board(), handle_(-1) { type_="CAEN_V1495PU"; };

  virtual int Init();
  virtual int Clear();
  virtual int BufferClear(); //reset the scaler
  virtual int Print() { return 0; }
  virtual int Config(BoardConfig *bC);
  virtual int Read(vector<WORD> &v);
  virtual int SetHandle(int handle) { handle_=handle;};

  inline CAEN_V1495PU_Config_t* GetConfiguration() { return &configuration_; };

private:
  uint32_t handle_;
  CAEN_V1495PU_Config_t configuration_;
};

#endif
