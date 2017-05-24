#include "interface/VFE_adapter.hpp"

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <bitset>


int VFE_adapter::Init()
{
        if (_debug) fprintf(_fl, "[VFE_adapter::Init] entering...\n");
        uhal::ConnectionManager manager (_manager_cfg);
        for (auto & d : _devices) {
                _dv.push_back(manager.getDevice(d));
        }

        for (auto & hw : _dv) {
                // Read FW version to check :
                uhal::ValWord<uint32_t> reg = hw.getNode("FW_VER").read();
                // Switch to triggered mode + external trigger :
                hw.getNode("VICE_CTRL").write((_trigger_self_threshold<<16) + 8 * _trigger_self + 4 * _trigger_loop + 2 * _trigger_type);
                // Stop DAQ and ask for 16 _nsamples per frame (+timestamp) :
                int command = ((_nsamples + 1)<<16) + VFE_adapter_CAPTURE_STOP;
                hw.getNode("CAP_CTRL").write(command);
                // Add laser latency before catching data ~ 40 us
                hw.getNode("TRIG_DELAY").write((_sw_daq_delay<<16) + _hw_daq_delay);
                hw.dispatch();

                // Reset the reading base address :
                hw.getNode("CAP_ADDRESS").write(0);

                hw.dispatch();
        }

        // init number of words needed for transfer
        int n_word = (_nsamples + 1) * 3; // 3*32 bits words per sample to get the 5 channels data
        _n_transfer = n_word / (VFE_adapter_MAX_PAYLOAD / 4); // max ethernet packet = 1536 bytes, max user payload = 1500 bytes
        _n_last = n_word - _n_transfer * (VFE_adapter_MAX_PAYLOAD / 4);
        fprintf(_fl, "Reading events by blocks of %dx32b-words, %d bits\n", n_word, n_word * 32);
        fprintf(_fl, "Using %d transfers of %d words + 1 transfer of %d words\n", _n_transfer, VFE_adapter_MAX_PAYLOAD / 4, _n_last);
        if(_n_transfer >= 246)
        {
                fprintf(_fl, "Event size too big! Please reduce number of samples per frame.\n");
                fprintf(_fl, "Max frame size : 28672\n");
        }
        Print();

        if (_debug) fprintf(_fl, "[VFE_adapter::Init] ...returning.\n");
        return 0;
}


int VFE_adapter::Clear()
{
        if (_debug) fprintf(_fl, "[VFE_adapter::Clear] entering...\n");
        int ret = BufferClear();
        if (_debug) fprintf(_fl, "[VFE_adapter::Clear] ...returning.\n");
        return ret;
}


int VFE_adapter::StartDAQ()
{
        if (_debug) fprintf(_fl, "[VFE_adapter::StartDAQ] entering...\n");
        // also reset memory
        for (auto & hw : _dv) {
                int command = ((_nsamples + 1)<<16) + VFE_adapter_CAPTURE_START;
                hw.getNode("CAP_CTRL").write(command);
                hw.dispatch();
        }
        Print();
        if (_debug) fprintf(_fl, "[VFE_adapter::StartDAQ] ...returning.\n");
        return 0;
}


int VFE_adapter::StopDAQ()
{
        if (_debug) fprintf(_fl, "[VFE_adapter::StopDAQ] entering...\n");
        for (auto & hw : _dv) {
                int command = ((_nsamples + 1)<<16) + VFE_adapter_CAPTURE_STOP;
                hw.getNode("CAP_CTRL").write(command);
                hw.dispatch();
        }
        if (_debug) fprintf(_fl, "[VFE_adapter::StopDAQ] ...returning.\n");
        return 0;
}


int VFE_adapter::BufferClear()
{
        if (_debug) fprintf(_fl, "[VFE_adapter::BufferClear] entering...\n");
        int ret = !(!StopDAQ() && !StartDAQ());
        if (_debug) fprintf(_fl, "[VFE_adapter::BufferClear] ...returning.\n");
        return ret;
}


int VFE_adapter::ClearBusy()
{  
        if (_debug) fprintf(_fl, "[VFE_adapter::ClearBusy] entering...\n");
        //int ret = BufferClear();
        if (_debug) fprintf(_fl, "[VFE_adapter::ClearBusy] ...returning.\n");
        return 0;
}


int VFE_adapter::Config(BoardConfig * bc)
{
        if (_debug) fprintf(_fl, "[VFE_adapter::Config] entering...\n");
        TriggerBoard::Config(bc);
        ParseConfiguration(bc);
        if (_debug) fprintf(_fl, "[VFE_adapter::Config] ...returning.\n");
        return 0;
}


int VFE_adapter::Read(std::vector<WORD> &v)
{
        if (_debug) fprintf(_fl, "[VFE_adapter::Read] entering...\n");
        // the following are debugging lines:
        // ... free_mem = free memory on the FPGA buffer
        // ...  address = writing/reading addresses (16 bit each in a 32 bit word)
        ///free_mem = hw.getNode("CAP_FREE").read();
        ///address = hw.getNode("CAP_ADDRESS").read();
        ///hw.dispatch();
        ///if(debug>0)printf("address : 0x%8.8x, Free memory : %d\n",address.value(),free_mem.value());

        // Read event samples from FPGA
        // loading into v

        // for debugging purposes, can dump the whole decoded event content
        for (auto & hw : _dv) {
                if (_debug > 1) _mem.clear();
                //uhal::ValWord<uint32_t> free_mem = hw.getNode("CAP_FREE").read();
                //hw.dispatch();
                //fprintf(_fl, "     Free memory           : 0x%8.8x\n", free_mem.value());
                for(int itrans = 0; itrans < _n_transfer; ++itrans)
                {
                        _block = hw.getNode ("CAP_DATA").readBlock(VFE_adapter_MAX_PAYLOAD / 4);
                        hw.dispatch();
                        for(int is = 0; is < VFE_adapter_MAX_PAYLOAD / 4; ++is) {
                                if (_debug > 1) _mem.push_back(_block[is]);
                                v.push_back(_block[is]);
                        }
                }
                _block = hw.getNode ("CAP_DATA").readBlock(_n_last);
                // for debugging
                //_address = hw.getNode("CAP_ADDRESS").read();
                //free_mem = hw.getNode("CAP_FREE").read();
                hw.dispatch();
                //if(debug>0)printf("After reading address : 0x%8.8x, Free memory : %d\n",address.value(),free_mem.value());
                for(int is = 0; is < _n_last; ++is) {
                        if (_debug > 1) _mem.push_back(_block[is]);
                        v.push_back(_block[is]);
                }
                if (_debug > 1) {
                        _mem.valid(true);
                        fprintf(_fl, "** Device id: %s  uri: %s\n", hw.id().c_str(), hw.uri().c_str());
                        Decode();
                }
        }
        if (_debug) fprintf(_fl, "[VFE_adapter::Read] ...returning.\n");
        return 0;
}


int VFE_adapter::ParseConfiguration(BoardConfig * bc)
{
        if (_debug) fprintf(_fl, "[VFE_adapter::ParseConfiguration] entering...\n");
        _manager_cfg             = bc->getElementContent("ManagerCfg");
        _devices                 = bc->getElementVector("Device");
        _nsamples                = atoi(bc->getElementContent("Nsamples").c_str());
        _trigger_self            = atoi(bc->getElementContent("TriggerSelf").c_str());
        _trigger_self_threshold  = atoi(bc->getElementContent("TriggerSelfThreshold").c_str());
        _trigger_loop            = atoi(bc->getElementContent("TriggerLoop").c_str());
        _trigger_type            = atoi(bc->getElementContent("TriggerType").c_str());
        _hw_daq_delay            = atoi(bc->getElementContent("HwDAQDelay").c_str());
        _sw_daq_delay            = atoi(bc->getElementContent("SwDAQDelay").c_str());
        _debug                   = atoi(bc->getElementContent("DebugLevel").c_str());
        _logger_type             = bc->getElementContent("LogType");
        if (_debug) fprintf(_fl, "[VFE_adapter::ParseConfiguration] ...returning.\n");
        Print();
        return 0;
}


void VFE_adapter::Decode()
{
        if (_debug) fprintf(_fl, "[VFE_adapter::Decode] entering...\n");
        // The first sample should have bit 70 at 1
        if((_mem[0]>>31) != 1) fprintf(_fl, "Sample 0 not a header : %8.8x\n", _mem[0]);
        unsigned long int t1 =  _mem[0]     &0xFFFF;
        unsigned long int t2 =  _mem[1]     &0xFFFF;
        unsigned long int t3 = (_mem[1]>>16)&0xFFFF;
        unsigned long int t4 =  _mem[2]     &0xFFFF;
        unsigned long int t5 = (_mem[2]>>16)&0x00FF;
        unsigned long int timestamp = (t5<<56) + (t4<<42) + (t3<<28) + (t2<<14) + t1;
        fprintf(_fl, "timestamp : %8.8x %8.8x %8.8x\n",_mem[2],_mem[1],_mem[0]);
        fprintf(_fl, "timestamp : %ld %4.4lx %4.4lx %4.4lx %4.4lx %4.4lx\n", timestamp, t5, t4, t3, t2, t1);
        unsigned short int event[5]; // 5 channels per VFE
        for(int is = 0; is < _nsamples; ++is)
        {
                int j = (is + 1) * 3;
                event[0] =  _mem[j]       &0xFFFF;
                event[1] =  _mem[j+1]     &0xFFFF;
                event[2] = (_mem[j+1]>>16)&0xFFFF;
                event[3] =  _mem[j+2]     &0xFFFF;
                event[4] = (_mem[j+2]>>16)&0xFFFF;
                fprintf(_fl, "*** sample: %5d    blocks: %8.8x %8.8x %8.8x\n", is, _mem[j], _mem[j+1], _mem[j+2]);
                fprintf(_fl, "--> sample: %5d  channels: %8d %8d %8d %8d %8d\n", is, event[0], event[1], event[2], event[3], event[4]);
        }
        if (_debug) fprintf(_fl, "[VFE_adapter::Decode] ...returning.\n");
}


void VFE_adapter::Trigger()
{
        for (auto & hw : _dv) {
                hw.getNode("FW_VER").write(1);
                hw.dispatch();
        }
}


int VFE_adapter::Print()
{
        if (_debug) fprintf(_fl, "[VFE_adapter::Print] entering...\n");
        fprintf(_fl, "**** Parameters read from xml config file: %s\n", _manager_cfg.c_str());
        fprintf(_fl, "  **  Device list:\n");
        for (auto & d : _devices) {
                fprintf(_fl, "      .. %s\n", d.c_str());
        }
        fprintf(_fl, "  **  Number of samples      : %d\n", _nsamples              );
        fprintf(_fl, "  **  Self trigger           : %d\n", _trigger_self          );
        fprintf(_fl, "  **  Trigger loop           : %d\n", _trigger_loop          );
        fprintf(_fl, "  **  Trigger type           : %d\n", _trigger_type          );
        fprintf(_fl, "  **  Self trigger threshold : %d\n", _trigger_self_threshold);
        fprintf(_fl, "  **  HW DAQ delay           : %d\n", _hw_daq_delay          );
        fprintf(_fl, "  **  SW DAQ delay           : %d\n", _sw_daq_delay          );

        fprintf(_fl, "**** Parameters read from registers of the single devices:\n");
        for (auto & hw : _dv) {
                fprintf(_fl, "  ** Device: %s   uri: %s\n", hw.id().c_str(), hw.uri().c_str());
                // Read FW version to check :
                uhal::ValWord<uint32_t> reg = hw.getNode("FW_VER").read();
                // Cross-check the initialization
                // Read back delay values :
                uhal::ValWord<uint32_t> delays = hw.getNode("TRIG_DELAY").read();
                // Read back the read/write base address
                _address = hw.getNode("CAP_ADDRESS").read();
                uhal::ValWord<uint32_t> free_mem = hw.getNode("CAP_FREE").read();
                uhal::ValWord<uint32_t> trig_reg = hw.getNode("VICE_CTRL").read();
                hw.dispatch();

                // Print init values
                fprintf(_fl, "     Firmware version      : %8.8x\n",   reg.value());
                fprintf(_fl, "     Delays                : %8.8x\n",   delays.value());
                fprintf(_fl, "     Initial R/W addresses : 0x%8.8x\n", _address.value());
                fprintf(_fl, "     Free memory           : 0x%8.8x\n", free_mem.value());
                fprintf(_fl, "     Trigger mode          : 0x%8.8x\n", trig_reg.value());
        }
        if (_debug) fprintf(_fl, "[VFE_adapter::Print] ...returning.\n");
        return 0;
}

bool VFE_adapter::TriggerReceived()
{
  //--external trigger
  if(_trigger_loop == 0)
    {
      return 0;
    }
    //--self trigger
  else if(_trigger_loop == 1)
    {
      Trigger();
      return 1;
    }
  return 0;
}

int VFE_adapter::SetBusyOn()
{
  return 0;
}

int VFE_adapter::SetBusyOff()
{
  return 0;
}

int VFE_adapter::TriggerAck()
{
  return 0;
}

int VFE_adapter::SetTriggerStatus(TRG_t triggerType, TRG_STATUS_t triggerStatus)
{
  int status=0;
  if (triggerStatus == TRIG_ON)
    {
      status |= StopDAQ();
      status |= StartDAQ();
    }
  else if (triggerStatus == TRIG_OFF)
    status |= StopDAQ();

  return status;
}
