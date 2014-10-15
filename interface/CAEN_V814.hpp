#ifndef CAEN_V814_H
#define CAEN_V814_H

#include "interface/StandardIncludes.hpp"
#include "interface/Board.hpp"
#include "interface/BoardConfig.hpp"

//---- CAEN V814 static define
#define CAEN_V814_CHANNELS 16

#define CAEN_V814_DATAWIDTH cvD16
#define CAEN_V814_ADDRESSMODE cvA32_U_DATA

//Main registers
#define CAEN_V814_THRESHOLD_ADD		0x0000	/* Threshold 0 register relative address */
#define CAEN_V814_OUT_WIDTH_0_7_ADD	0x0040	/* Output width register Ch 0-7 relative address */
#define CAEN_V814_OUT_WIDTH_8_15_ADD	0x0042	/* Output width register Ch 8-15 relative address */
#define CAEN_V814_MAJORITY_ADD		0x0048	/* CVT_V814_MAJORITY threshold register relative address */
#define CAEN_V814_PATTERN_INHIBIT_ADD	0x004A	/* Pattern inhibit register relative address */
#define CAEN_V814_MANUFACTURER_ADD	0x00FC	/* Manufacturer and Module type register relative address */
#define CAEN_V814_VERSION_ADD		0x00FD	/* Version and serial number register relative address */

class CAEN_V814: public Board 
{
public:

  typedef enum  {
    ERR_NONE= 0,
    ERR_CONF_NOT_FOUND,
    ERR_OPEN,
    ERR_CONFIG,
    ERR_RESET,
    ERR_DUMMY_LAST,
  } ERROR_CODES;
  
  typedef struct CAEN_V814_Config_t {
    unsigned int baseAddress;
    unsigned int patternMask;
    unsigned int outputWidth;
    unsigned int majorityThreshold;
    unsigned int commonThreshold;
    unsigned int chThreshold[CAEN_V814_CHANNELS];
  } CAEN_V814_Config_t;

  CAEN_V814(): Board(), handle_(-1) { type_="CAEN_V814"; };

  virtual int Init();
  virtual int Clear();
  virtual int BufferClear(); //reset the scaler
  virtual int Print() { return 0; }
  virtual int Config(BoardConfig *bC);
  virtual int Read(vector<WORD> &v);
  virtual int SetHandle(int handle) { handle_=handle;};

  inline CAEN_V814_Config_t* GetConfiguration() { return &configuration_; };

private:

  int SetThreshold(int channel);
  int SetOutputWidth();
  int SetMajorityThreshold();
  int SetPatternInhibit();
  
  uint32_t handle_;
  CAEN_V814_Config_t configuration_;
};

#endif
