#ifndef VFE_adapter_H
#define VFE_adapter_H

#include "interface/StandardIncludes.hpp"
#include "interface/Board.hpp"
#include "interface/BoardConfig.hpp"

#include "uhal/uhal.hpp"

#define VFE_adapter_CAPTURE_START   1
#define VFE_adapter_CAPTURE_STOP    2

#define VFE_adapter_RESET           1

#define VFE_adapter_GEN_TRIGGER     1
#define VFE_adapter_GEN_100HZ       2
#define VFE_adapter_LED_ON          4

// for reference (and debugging messages)
#define VFE_adapter_DV              1750./16384.; // 14 bits on 1.75V

//#define VFE_adapter_NSAMPLE_MAX 28670
// Max ethernet packet = 1536 bytes, max user payload = 1500 bytes
#define VFE_adapter_MAX_PAYLOAD 1380

/*  Data format
 *  -----------
 *   First 3 words: header, containing the timestamp
 *     information (i.e. the number of the 160 MHz clocks
 *     from the StartDAQ() command):
 *     timestamp = (t5<<56) + (t4<<42) + (t3<<28) + (t2<<14) + t1;
 *   Subsequent words: _nsamples for each of the 5 VFE channels
 *   Then concatenate header and samples for each VFE adapter
 *   read by the board.
 *
 *           32 ... 16 ...  0 -> bits
 *   words
 *           VFE_adapter_1
 *     1     [10...0][  t1  ] \
 *     2     [  t3  ][  t2  ]  |-> header
 *     3     [  t5  ][  t4  ] /
 *     4     [  --  ][ch0_s0] \
 *     5     [ch2_s0][ch1_s0]  |
 *     6     [ch4_s0][ch3_s0]  |
 *     7     [  --  ][ch0_s1]  |-> samples
 *     8     [ch2_s1][ch1_s1]  |
 *     9     [ch4_s1][ch3_s1]  |
 *    ...       ...    ...    ...
 *           VFE_adapter_N
 *    M+1    [10...0][  t1  ] \
 *    M+2    [  t3  ][  t2  ]  |-> header
 *    ...       ...    ...    ...
 */

class VFE_adapter : public TriggerBoard, IOControlBoard
{
    public:

        VFE_adapter() : TriggerBoard(), IOControlBoard() 
        {
            type_     = "VFE_ADAPTER";
	    _header = 0;
        }

        virtual int Init();
        virtual int Clear(); // stop + start
        virtual int BufferClear(); // stop + start
        virtual int ClearBusy();
        virtual int Config(BoardConfig *bc);
        virtual int Read(std::vector<WORD> &v);
        virtual int SetHandle(int) { return 0; }

        void Trigger(); // for standalone acquisition (e.g. pedestals)

        int Print();

        void ConfigOptions(std::string & s);

        // accessors for config parameters
        size_t NDevices()             { return _devices.size();         }
        int    NSamples()             { return _nsamples;               }
        int    SelfTrigger()          { return _trigger_self;           }
        int    SelfTriggerThreshold() { return _trigger_self_threshold; }
        int    TriggerLoop()          { return _trigger_loop;           }
        int    TriggerType()          { return _trigger_type;           }
        int    HwDAQDelay()           { return _hw_daq_delay;           }
        int    SwDAQDelay()           { return _sw_daq_delay;           }

        // header encoding/decoding functions
        // header structure: 32 bits; from the LSB to the MSB:
        // 14 bits [ 1-14] -> number of samples
        //  2 bits [15-16] -> sampling frequency: 0 = 40 MHz; 1 = 80 MHz; 2 = 120 MHz; 3 = 160 MHz
        //  3 bits [17-19] -> number of devices (adapters) read by the VFE_adapter instance
        //  5 bits [20-24] -> firmware version
        //  8 bits [25-32] -> unreserved
        void     setHeadNSamples(int nsamples)    { _header |= (nsamples & 0x3FFF); }
        void     setHeadFrequency(int ifreq)      { _header |= ((ifreq & 0x3)<<14); }
        void     setHeadNDevices(int ndev)        { _header |= ((ndev & 0x7)<<16);  }
        void     setHeadFwVersion(int ver)        { _header |= ((ver & 0x1F)<<19);  }
        // to be used e.g. by DQM or any RAW data decoder
        static int headNSamples(uint32_t header)  { return  header & 0x3FFF;    }
        static int headFrequency(uint32_t header) { return (header>>14) & 0x3;  }
        static int headNDevices(uint32_t header)  { return (header>>16) & 0x7;  }
        static int headFwVersion(uint32_t header) { return (header>>19) & 0x1F; }

        int headNSamples()  { return headNSamples(_header);  }
        int headFrequency() { return headFrequency(_header); }
        int headNDevices()  { return headNDevices(_header);  }
        int headFwVersion() { return headFwVersion(_header); }

        //Main functions to handle the event trigger
        virtual int SetBusyOn();
        virtual int SetBusyOff();
        virtual bool TriggerReceived();
        virtual int TriggerAck();
        virtual inline bool  SignalReceived(CMD_t signal) { return true; };
        virtual int SetTriggerStatus(TRG_t triggerType, TRG_STATUS_t triggerStatus);

    protected:

        void Decode();
        int StartDAQ();
        int StopDAQ();
        int Reset();
        int SetLEDStatus(int status);
        /* reads params from cfg file */  
        int ParseConfiguration(BoardConfig * bc);

        // main hardware interface, can read multiple VFE adapter (devices)
        std::vector<uhal::HwInterface> _dv;

        // to host data
        uhal::ValVector<uint32_t> _block;
        uhal::ValVector<uint32_t> _mem; // just for debugging

        // working variables
        uhal::ValWord<uint32_t> _address;
        uhal::ValWord<uint32_t> _fw_version;
        uhal::ValWord<uint32_t> _buffer_size;
        uint32_t _header;

        // how many ethernet packets are needed to transfer the VFE data
        // (depends on the number of samples)
        int _n_transfer;
        int _n_last;

        // configuration parameters
        std::string _manager_cfg;
        std::vector<std::string> _devices;
        int _nsamples;                // number of samples to acquire
        int _trigger_self;            // generate self trigger from data (1) or not (0)
        int _trigger_self_threshold;  // signal threshold [ADC count] to generate self trigger
        int _trigger_loop;            // use internal software trigger (1) or Laser with external trigger (0)
        int _trigger_type;            // continuous DAQ (0) or triggered DAQ (1)
        int _hw_daq_delay;            // readout waiting time on external trigger arrival [# clocks @ 160 MHz]
        // greater values moves the signal *left*
        int _sw_daq_delay;            // as _hw_daq_delay but for internally generated trigger (e.g. when using the internal trigger to trigger external HW, like a laser)
        int _debug;                   // debug level: 0: none, 1: functions, 2: detailed
};

#endif
