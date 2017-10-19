#include "interface/VFE_adapter.hpp"

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <bitset>

#include "TString.h"

#define DEEPDEBUG 1

int VFE_adapter::Init()
{
    Log("[VFE_adapter::Init] entering...", 1);
    uhal::ConnectionManager manager (_manager_cfg);
    for (auto & d : _devices) {
        _dv.push_back(manager.getDevice(d));
    }

    Reset();
    SetLEDStatus(0);

    ProgramDAC();
    CalibrationTriggerSetting();

    uhal::ValWord<uint32_t> tmp;
    bool first = true;
    int command;
    for (auto & hw : _dv) {
        // Read FW version to check :
        _fw_version = hw.getNode("FW_VER").read();
        // Switch to triggered mode + external trigger :
        command = (VFE_adapter_SELF_TRIGGER_MASK      *(_trigger_self_mask&0x1F))        |
                  (VFE_adapter_SELF_TRIGGER_THRESHOLD *(_trigger_self_threshold&0x3FFF)) |
                   VFE_adapter_SELF_TRIGGER            *_trigger_self                     |
                   VFE_adapter_SOFT_TRIGGER            *_trigger_soft                     |
                   VFE_adapter_TRIGGER_MODE            *1                                 | // Always DAQ on trigger
                   VFE_adapter_RESET                   *0;
        hw.getNode("VICE_CTRL").write(command);
        // Stop DAQ and ask for NSAMPLE _nsamples per frame (+timestamp) :
        int command = ((_nsamples + 1)<<16) + VFE_adapter_CAPTURE_STOP;
        hw.getNode("CAP_CTRL").write(command);
        // Add laser latency before catching data ~ 40 us
        hw.getNode("TRIG_DELAY").write((_sw_daq_delay<<16) + _hw_daq_delay);
        command = VFE_adapter_LED_ON*0+VFE_adapter_GEN_100HZ*0+VFE_adapter_GEN_CALIB*0+VFE_adapter_GEN_TRIGGER*0;
        hw.getNode("FW_VER").write(command);
        hw.dispatch();
        Log("[VFE_adapter::Init] entering 1...", 1);
        // Reset the reading base address :
        hw.getNode("CAP_ADDRESS").write(0);
        command = ((_nsamples + 1)<<16) + VFE_adapter_CAPTURE_START;
        hw.getNode("CAP_CTRL").write(command);
        // Read back delay values :
        /////delays = hw.getNode("TRIG_DELAY").read(); // FIXME - remove?
        // Read back the read/write base address
        _address = hw.getNode("CAP_ADDRESS").read();
        uhal::ValWord<uint32_t> free_mem;
        uhal::ValWord<uint32_t> trig_reg;
        free_mem = hw.getNode("CAP_FREE").read();
        trig_reg = hw.getNode("VICE_CTRL").read();
        hw.dispatch();
        //old_address[i_vfe]=address>>16; // FIXME - remove?
        //if(old_address[i_vfe]==0x6fff)old_address[i_vfe]=-1; // FIXME - remove?
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
    if(_n_transfer >= 248)
    {
        Log("[VFE_adapter::Init] Event size too big! Please reduce number of samples per frame.", 1);
        Log("[VFE_adapter::Init] Max frame size : 28670", 1);
    }
    int reset = StartDAQ() && StopDAQ();
    Print();

    // init event header
    //
    // if different FW versions across devices, the version of the last device is written
    _header=0;
    setHeadFwVersion(_fw_version); 
    setHeadNSamples(_nsamples);
    setHeadNDevices(_dv.size());
    setHeadFrequency(3); // FIXME: if FW changed to handle different frequency, implement a function to read it from the FW directly

    Log(Form("[VFE_adapter::Init] Header: %x", _header), 1);
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


int VFE_adapter::Reset()
{
    if (_debug) Log("[VFE_adapter::Reset] entering...", 3);
    for (auto & hw : _dv) {
        hw.getNode("VICE_CTRL").write(VFE_adapter_RESET * 1);
        hw.dispatch();
    }
    usleep(5000000);
    if (_debug) Log("[VFE_adapter::Reset] ...returning.", 3);
    return 0;
}


int VFE_adapter::ProgramDAC()
{
    int soft_reset, full_reset, command;
    // Try to program DAC
    // First : Put DAC in powerup state
    full_reset = VFE_adapter_DAC_WRITE | (0xf<<16);
    soft_reset = VFE_adapter_DAC_WRITE | (0x7<<16);
    // Set the control register
    command = VFE_adapter_DAC_WRITE | VFE_adapter_DAC_CTRL_REG |
        (0x1<<9) | // midscale upon clear*
        (0x0<<8) | // No overrange
        (0x0<<7) | // No bipolar range
        (0x0<<6) | // No thermal shutdown
        (0x1<<5) | // Use internal reference voltage
        (0x1<<3) | // midscale at power-up
        (0x3<<0);  // 0-5V range
    Log(Form("[VFE_adapter::ProgramDAC] Ctrl register loading command : 0x%x", command), 3);
    for(auto & hw : _dv)
    {
        Log("[VFE_adapter::ProgramDAC] Do a full reset", 3);
        hw.getNode("VFE_CTRL").write(full_reset);
        hw.dispatch();

        Log("[VFE_adapter::ProgramDAC] Write in conrol reg", 3);
        hw.getNode("VFE_CTRL").write(command); // First write to reconfigure output range
        hw.dispatch();

        Log("[VFE_adapter::ProgramDAC] Do a full reset", 3);
        hw.getNode("VFE_CTRL").write(full_reset); // Full reset after output range reconfig
        hw.dispatch();

        Log("[VFE_adapter::ProgramDAC] Write in control reg", 3);
        hw.getNode("VFE_CTRL").write(command); // Rewrite default values and switch on DAC output stage
        hw.dispatch();

        Log("[VFE_adapter::ProgramDAC] do a soft reset", 3);
        hw.getNode("VFE_CTRL").write(soft_reset); // soft reset
        hw.dispatch();

        command = VFE_adapter_DAC_WRITE | VFE_adapter_DAC_VAL_REG | (_calib_level&0xffff);
        Log(Form("[VFE_adapter::ProgramDAC] Put %d in DAC register : 0x%x", _calib_level, command), 3);
        hw.getNode("VFE_CTRL").write(command);
        hw.dispatch();

        // Put ADC in two's complement mode (if no pedestal bias) and invert de conversion result
        _negate_data = (_negate_data & 1)<<2;
        _signed_data &= 3;
        command = VFE_adapter_ADC_WRITE |  VFE_adapter_ADC_OMODE_REG | _negate_data | _signed_data;
        Log(Form("[VFE_adapter::ProgramDAC] Put ADC coding : 0x%x", command), 3);
        hw.getNode("VFE_CTRL").write(command);
        hw.dispatch();

        // Set ADC input range (default=1.75V)
        _input_span &= 0x1F;
        command = VFE_adapter_ADC_WRITE | VFE_adapter_ADC_ISPAN_REG | _input_span;
        Log(Form("[VFE_adapter::ProgramDAC] Set ADC input span range : 0x%x", command), 3);
        hw.getNode("VFE_CTRL").write(command);
        hw.dispatch();
    }
    return 0;
}


int VFE_adapter::CalibrationTriggerSetting()
{
    int command;
    for(auto & hw : _dv)
    {
        command = (_calib_delay<<16) | (_calib_width&0xffff);
        Log(Form("[VFE_adapter::CalibrationTriggerSetting] Calibration trigger with %d clocks width and %d clocks delay : %x", _calib_width, _calib_delay, command), 3);
        hw.getNode("CALIB_CTRL").write(command);
        hw.dispatch();
    }
    return 0;
}


int VFE_adapter::SetLEDStatus(int status)
{
    if (_debug) Log("[VFE_adapter::SetLEDStatus] entering...", 3);
    for (auto & hw : _dv) {
        int command = VFE_adapter_LED_ON * 0 + VFE_adapter_GEN_100HZ * 0 + VFE_adapter_GEN_TRIGGER * 0;
        hw.getNode("FW_VER").write(command);
        hw.dispatch();
    }
    if (_debug) Log("[VFE_adapter::SetLEDStatus] ...returning.", 3);
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

        // uhal::ValWord<uint32_t> free_mem = hw.getNode("CAP_FREE").read();
        // hw.dispatch();
        // Log(Form("     Free mem before      : 0x%8.8x", free_mem.value()), 1);

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
        // free_mem = hw.getNode("CAP_FREE").read();
        hw.dispatch();
        // Log(Form("     Free mem after       : 0x%8.8x", free_mem.value()), 1);
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
    _trigger_self_mask       = Configurator::GetInt(bc->getElementContent("TriggerSelfMask").c_str());
    _trigger_soft            = Configurator::GetInt(bc->getElementContent("TriggerSoft").c_str());
    _trigger_type            = Configurator::GetInt(bc->getElementContent("TriggerType").c_str());
    _hw_daq_delay            = Configurator::GetInt(bc->getElementContent("HwDAQDelay").c_str());
    _sw_daq_delay            = Configurator::GetInt(bc->getElementContent("SwDAQDelay").c_str());
    _debug                   = Configurator::GetInt(bc->getElementContent("DebugLevel").c_str());
    _calib_level             = Configurator::GetInt(bc->getElementContent("CalibLevel").c_str());
    _calib_width             = Configurator::GetInt(bc->getElementContent("CalibWidth").c_str());
    _calib_delay             = Configurator::GetInt(bc->getElementContent("CalibDelay").c_str());
    _calib_n                 = Configurator::GetInt(bc->getElementContent("CalibN").c_str());
    _calib_step              = Configurator::GetInt(bc->getElementContent("CalibStep").c_str());
    _negate_data             = Configurator::GetInt(bc->getElementContent("NegateData").c_str());
    _signed_data             = Configurator::GetInt(bc->getElementContent("SignedData").c_str());
    _input_span              = Configurator::GetInt(bc->getElementContent("InputSpan").c_str());

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
        if(_signed_data == 1 && ((event[0]>>13)&1) == 1) event[0]|=0xc000;
        event[1] =  _mem[j+1]     &0xFFFF;
        if(_signed_data == 1 && ((event[1]>>13)&1) == 1) event[1]|=0xc000;
        event[2] = (_mem[j+1]>>16)&0xFFFF;
        if(_signed_data == 1 && ((event[2]>>13)&1) == 1) event[2]|=0xc000;
        event[3] =  _mem[j+2]     &0xFFFF;
        if(_signed_data == 1 && ((event[3]>>13)&1) == 1) event[3]|=0xc000;
        event[4] = (_mem[j+2]>>16)&0xFFFF;
        if(_signed_data == 1 && ((event[4]>>13)&1) == 1) event[4]|=0xc000;
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
    Log(Form("  **  Trigger loop           : %d", _trigger_soft          ), 1);
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
    if (_trigger_soft == 0) {
        int timeout=0;
        while (1 && timeout<2000) {
            //check free memory
            for (auto & hw : _dv) {
                uhal::ValWord<uint32_t> free_mem = hw.getNode("CAP_FREE").read();
                hw.dispatch();
                // Log(Form("     Free mem           : 0x%8.8x", free_mem.value()), 1);
                if (free_mem.value() != _buffer_size.value()) //for the moment just signaling a trigger using an event present in memory
                    return 1;
            }
            usleep(30);
	    ++timeout;
        }
    } else if(_trigger_soft == 1) { //--self trigger
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
