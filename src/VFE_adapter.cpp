#include "interface/VFE_adapter.hpp"

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <bitset>

#include "TString.h"

int VFE_adapter::Init()
{
    Log("[VFE_adapter::Init] entering...", 1);
    uhal::ConnectionManager manager (_manager_cfg);
    for (auto & d : _devices) {
        _dv.push_back(manager.getDevice(d));
    }

    uhal::ValWord<uint32_t> tmp;
    bool first = true;
    for (auto & hw : _dv) {
        // Read FW version to check :
        _fw_version = hw.getNode("FW_VER").read();
        // Switch to triggered mode + external trigger :
        hw.getNode("VICE_CTRL").write((_trigger_self_threshold<<16) + 8 * _trigger_self + 4 * _trigger_loop + 2 * _trigger_type);
        // Stop DAQ and ask for 16 _nsamples per frame (+timestamp) :
        int command = ((_nsamples + 1)<<16) + VFE_adapter_CAPTURE_STOP;
        hw.getNode("CAP_CTRL").write(command);
        // Add laser latency before catching data ~ 40 us
        hw.getNode("TRIG_DELAY").write((_sw_daq_delay<<16) + _hw_daq_delay);
        hw.dispatch();
        Log("[VFE_adapter::Init] entering 1...", 1);
        // Reset the reading base address :
        hw.getNode("CAP_ADDRESS").write(0);
        hw.dispatch();
        // check FW version
        if (first) {
            tmp = _fw_version;
            first = false;
        }
        if (_fw_version != tmp) {
            Log("[VFE_adapter::FwVersion] WARNING: not all the devices have the same firmware!!", 1);
        }
    }

    // init number of words needed for transfer
    int n_word = (_nsamples + 1) * 3; // 3*32 bits words per sample to get the 5 channels data
    _n_transfer = n_word / (VFE_adapter_MAX_PAYLOAD / 4); // max ethernet packet = 1536 bytes, max user payload = 1500 bytes
    _n_last = n_word - _n_transfer * (VFE_adapter_MAX_PAYLOAD / 4);
    Log(Form("[VFE_adapter::Init] Reading events by blocks of %dx32b-words, %d bits", n_word, n_word * 32), 1);
    Log(Form("[VFE_adapter::Init] Using %d transfers of %d words + 1 transfer of %d words", _n_transfer, VFE_adapter_MAX_PAYLOAD / 4, _n_last), 1);
    if(_n_transfer >= 246)
    {
        Log("[VFE_adapter::Init] Event size too big! Please reduce number of samples per frame.", 1);
        Log("[VFE_adapter::Init] Max frame size : 28672", 1);
    }
    Print();

    // init event header
    //
    // if different FW versions across devices, the version of the last device is written
    setHeadFwVersion(_fw_version); 
    setHeadNSamples(_nsamples);
    setHeadNDevices(_dv.size());
    setHeadFrequency(3); // FIXME: if FW changed to handle different frequency, implement a function to read it from the FW directly

    Log("[VFE_adapter::Init] ...returning.", 1);
    return 0;
}


int VFE_adapter::Clear()
{
    if (_debug) Log("[VFE_adapter::Clear] entering...", 3);
    int ret = BufferClear();
    if (_debug) Log("[VFE_adapter::Clear] ...returning.", 3);
    return ret;
}


int VFE_adapter::StartDAQ()
{
    if (_debug) Log("[VFE_adapter::StartDAQ] entering...", 3);
    // also reset memory
    for (auto & hw : _dv) {
        int command = ((_nsamples + 1)<<16) + VFE_adapter_CAPTURE_START;
        hw.getNode("CAP_CTRL").write(command);
        hw.dispatch();
    }
    if (_debug) Log("[VFE_adapter::StartDAQ] ...returning.", 3);
    return 0;
}


int VFE_adapter::StopDAQ()
{
    if (_debug) Log("[VFE_adapter::StopDAQ] entering...", 3);
    for (auto & hw : _dv) {
        int command = ((_nsamples + 1)<<16) + VFE_adapter_CAPTURE_STOP;
        hw.getNode("CAP_CTRL").write(command);
        hw.dispatch();
    }
    if (_debug) Log("[VFE_adapter::StopDAQ] ...returning.", 3);
    return 0;
}


int VFE_adapter::BufferClear()
{
    if (_debug) Log("[VFE_adapter::BufferClear] entering..." ,3);
    int ret = !(!StopDAQ() && !StartDAQ());
    if (_debug) Log("[VFE_adapter::BufferClear] ...returning.", 3);
    return ret;
}


int VFE_adapter::ClearBusy()
{  
    if (_debug) Log("[VFE_adapter::ClearBusy] entering...", 3);
    //int ret = BufferClear();
    if (_debug) Log("[VFE_adapter::ClearBusy] ...returning.", 3);
    return 0;
}


int VFE_adapter::Config(BoardConfig * bc)
{
    Log("[VFE_adapter::Config] entering...", 1);
    Board::Config(bc);
    ParseConfiguration(bc);
    Log("[VFE_adapter::Config] ...returning.", 1);
    return 0;
}


int VFE_adapter::Read(std::vector<WORD> &v)
{
    if (_debug) Log("[VFE_adapter::Read] entering...", 3);
    // the following are debugging lines:
    // ... free_mem = free memory on the FPGA buffer
    // ...  address = writing/reading addresses (16 bit each in a 32 bit word)
    ///free_mem = hw.getNode("CAP_FREE").read();
    ///address = hw.getNode("CAP_ADDRESS").read();
    ///hw.dispatch();
    ///if(debug>0)printf("address : 0x%8.8x, Free memory : %d",address.value(),free_mem.value());

    // write event header
    v.push_back(_header);

    // Read event samples from FPGA
    // loading into v

    // for debugging purposes, can dump the whole decoded event content
    for (auto & hw : _dv) {
        if (_debug > 1) _mem.clear();

        uhal::ValWord<uint32_t> free_mem = hw.getNode("CAP_FREE").read();
        hw.dispatch();
	Log(Form("     Free mem before      : 0x%8.8x", free_mem.value()), 1);

        //fprintf(_fl, "     Free memory           : 0x%8.8x", free_mem.value());
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
        free_mem = hw.getNode("CAP_FREE").read();
        hw.dispatch();
	Log(Form("     Free mem after       : 0x%8.8x", free_mem.value()), 1);
        //if(debug>0)printf("After reading address : 0x%8.8x, Free memory : %d",address.value(),free_mem.value());
        for(int is = 0; is < _n_last; ++is) {
            if (_debug > 1) _mem.push_back(_block[is]);
            v.push_back(_block[is]);
        }
#ifdef DEEPDEBUG //---use with caution
        if (_debug > 1) {
            _mem.valid(true);
            Log(Form("** Device id: %s  uri: %s", hw.id().c_str(), hw.uri().c_str()), 3);
            Decode();
        }
#endif
    }
    if (_debug) Log("[VFE_adapter::Read] ...returning.", 3);
    return 0;
}


int VFE_adapter::ParseConfiguration(BoardConfig * bc)
{
    Log("[VFE_adapter::ParseConfiguration] entering...", 1);
    _manager_cfg             = bc->getElementContent("ManagerCfg");
    _devices                 = bc->getElementVector("Device");
    _nsamples                = Configurator::GetInt(bc->getElementContent("Nsamples").c_str());
    _trigger_self            = Configurator::GetInt(bc->getElementContent("TriggerSelf").c_str());
    _trigger_self_threshold  = Configurator::GetInt(bc->getElementContent("TriggerSelfThreshold").c_str());
    _trigger_loop            = Configurator::GetInt(bc->getElementContent("TriggerLoop").c_str());
    _trigger_type            = Configurator::GetInt(bc->getElementContent("TriggerType").c_str());
    _hw_daq_delay            = Configurator::GetInt(bc->getElementContent("HwDAQDelay").c_str());
    _sw_daq_delay            = Configurator::GetInt(bc->getElementContent("SwDAQDelay").c_str());
    _debug                   = Configurator::GetInt(bc->getElementContent("DebugLevel").c_str());

    return 0;
}


void VFE_adapter::Decode()
{
    Log("[VFE_adapter::Decode] entering...", 1);
    // The first sample should have bit 70 at 1
    if((_mem[0]>>31) != 1) Log(Form("Sample 0 not a header : %8.8x", _mem[0]), 1);
    unsigned long int t1 =  _mem[0]     &0xFFFF;
    unsigned long int t2 =  _mem[1]     &0xFFFF;
    unsigned long int t3 = (_mem[1]>>16)&0xFFFF;
    unsigned long int t4 =  _mem[2]     &0xFFFF;
    unsigned long int t5 = (_mem[2]>>16)&0x00FF;
    unsigned long int timestamp = (t5<<56) + (t4<<42) + (t3<<28) + (t2<<14) + t1;
    Log(Form("timestamp : %8.8x %8.8x %8.8x",_mem[2],_mem[1],_mem[0]), 1);
    Log(Form("timestamp : %ld %4.4lx %4.4lx %4.4lx %4.4lx %4.4lx", timestamp, t5, t4, t3, t2, t1), 1);
    unsigned short int event[5]; // 5 channels per VFE
    for(int is = 0; is < _nsamples; ++is)
    {
        int j = (is + 1) * 3;
        event[0] =  _mem[j]       &0xFFFF;
        event[1] =  _mem[j+1]     &0xFFFF;
        event[2] = (_mem[j+1]>>16)&0xFFFF;
        event[3] =  _mem[j+2]     &0xFFFF;
        event[4] = (_mem[j+2]>>16)&0xFFFF;
        Log(Form("*** sample: %5d    blocks: %8.8x %8.8x %8.8x", is, _mem[j], _mem[j+1], _mem[j+2]), 1);
        Log(Form("--> sample: %5d  channels: %8d %8d %8d %8d %8d", is, event[0], event[1], event[2], event[3], event[4]), 1);
    }
    Log("[VFE_adapter::Decode] ...returning.", 1);
}


void VFE_adapter::Trigger()
{
    if(_debug) Log("[VFE_adapter::Trigger] sending trigger...", 3);
    for (auto & hw : _dv) {
        hw.getNode("FW_VER").write(1);
        hw.dispatch();
        // free_mem = hw.getNode("CAP_FREE").read();
        // hw.dispatch();
        // Log(Form("     Free memory after trigger : 0x%8.8x", free_mem.value()),1);
    }
}


int VFE_adapter::Print()
{
    if (_debug) Log("[VFE_adapter::Print] entering...", 3);
    Log(Form("**** Parameters read from xml config file: %s", _manager_cfg.c_str()), 1);
    Log("  **  Device list:", 1);
    for (auto & d : _devices) {
        Log(Form("      .. %s", d.c_str()), 1);
    }
    Log(Form("  **  Number of samples      : %d", _nsamples              ), 1);
    Log(Form("  **  Self trigger           : %d", _trigger_self          ), 1);
    Log(Form("  **  Trigger loop           : %d", _trigger_loop          ), 1);
    Log(Form("  **  Trigger type           : %d", _trigger_type          ), 1);
    Log(Form("  **  Self trigger threshold : %d", _trigger_self_threshold), 1);
    Log(Form("  **  HW DAQ delay           : %d", _hw_daq_delay          ), 1);
    Log(Form("  **  SW DAQ delay           : %d", _sw_daq_delay          ), 1);

    Log("**** Parameters read from registers of the single devices:", 1);
    for (auto & hw : _dv) {
        Log(Form("  ** Device: %s   uri: %s", hw.id().c_str(), hw.uri().c_str()), 1);
        // Read FW version to check :
        uhal::ValWord<uint32_t> reg = hw.getNode("FW_VER").read();
        // Cross-check the initialization
        // Read back delay values :
        uhal::ValWord<uint32_t> delays = hw.getNode("TRIG_DELAY").read();
        // Read back the read/write base address
        _address = hw.getNode("CAP_ADDRESS").read();
        _buffer_size = hw.getNode("CAP_FREE").read();
        uhal::ValWord<uint32_t> trig_reg = hw.getNode("VICE_CTRL").read();
        hw.dispatch();

        // Print init values
        Log(Form("     Firmware version      : %8.8x",   reg.value()), 1);
        Log(Form("     Delays                : %8.8x",   delays.value()), 1);
        Log(Form("     Initial R/W addresses : 0x%8.8x", _address.value()), 1);
        Log(Form("     Buffer Size           : 0x%8.8x", _buffer_size.value()), 1);
        Log(Form("     Trigger mode          : 0x%8.8x", trig_reg.value()), 1);
    }
    if (_debug) Log(Form("[VFE_adapter::Print] ...returning."), 3);
    return 0;
}


bool VFE_adapter::TriggerReceived()
{
    //--external trigger
    if (_trigger_loop == 0) {
        while (1) {
            //check free memory
            for (auto & hw : _dv) {
                uhal::ValWord<uint32_t> free_mem = hw.getNode("CAP_FREE").read();
                hw.dispatch();
		Log(Form("     Free mem           : 0x%8.8x", free_mem.value()), 1);
                if (free_mem.value() != _buffer_size.value()) //for the moment just signaling a trigger using an event present in memory
                    return 1;
            }
            usleep(10);
        }
    } else if(_trigger_loop == 1) { //--self trigger
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
    if (triggerStatus == TRIG_ON) {
        status |= StopDAQ();
        status |= StartDAQ();
    } else if (triggerStatus == TRIG_OFF) {
        status |= StopDAQ();
    }
    return status;
}
