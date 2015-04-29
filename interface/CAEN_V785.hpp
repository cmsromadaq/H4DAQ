#ifndef CAEN_V785_H
#define CAEN_V785_H

#include "interface/StandardIncludes.hpp"
#include "interface/Board.hpp"
#include "interface/CAEN_V792.hpp"
#include "interface/BoardConfig.hpp"

class CAEN_V785: public CAEN_V792
{
public:

  CAEN_V785(): CAEN_V792() { type_="CAEN_V785"; };

  // virtual int Init();
  // virtual int Clear();
  // virtual int BufferClear(); //reset the buffers
  // virtual int Print() { return 0; }
  // virtual int Config(BoardConfig *bC);
  // virtual int Read(vector<WORD> &v);
  // virtual int SetHandle(int handle) { handle_=handle; return 0;};

  // // --- TriggerBoard functionalities not yet implemented

  // inline CAEN_V792_Config_t* GetConfiguration() { return &configuration_; };

private:

  // int CheckStatusAfterRead();

  // uint32_t handle_;
  // CAEN_V792_Config_t configuration_;
  // uint32_t channels_;
};

#endif
