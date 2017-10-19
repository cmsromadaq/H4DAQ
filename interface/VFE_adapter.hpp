#ifndef VFE_adapter_H
#define VFE_adapter_H

#include "interface/StandardIncludes.hpp"
#include "interface/Board.hpp"
#include "interface/BoardConfig.hpp"

#include "uhal/uhal.hpp"

#define VFE_adapter_CAPTURE_START   1
#define VFE_adapter_CAPTURE_STOP    2

#define VFE_adapter_GEN_TRIGGER     1
#define VFE_adapter_GEN_CALIB       2
#define VFE_adapter_GEN_100HZ       4
#define VFE_adapter_LED_ON          8


#define VFE_adapter_RESET                  (1<<0)
#define VFE_adapter_TRIGGER_MODE           (1<<1)
#define VFE_adapter_SOFT_TRIGGER           (1<<2)
#define VFE_adapter_SELF_TRIGGER           (1<<3)
#define VFE_adapter_CLOCK_PHASE            (1<<4)
#define VFE_adapter_CLOCK_RESET            (1<<7)
#define VFE_adapter_SELF_TRIGGER_THRESHOLD (1<<8)
#define VFE_adapter_SELF_TRIGGER_MASK      (1<<22)
#define VFE_adapter_BOARD_SN               (1<<28)

#define VFE_adapter_DAC_WRITE       (1<<25)
#define VFE_adapter_ADC_WRITE       (1<<24)
#define VFE_adapter_DAC_FULL_RESET  (0xF<<16)
#define VFE_adapter_DAC_SOFT_RESET  (0x7<<16)
#define VFE_adapter_DAC_VAL_REG     (0x3<<16)
#define VFE_adapter_DAC_CTRL_REG    (0x4<<16)

#define VFE_adapter_ADC_OMODE_REG   (0x14<<16) // ADC register to define Output mode of the ADC
#define VFE_adapter_ADC_ISPAN_REG   (0x18<<16) // ADC register to define input span of the ADC from 1.383V to 2.087V




// for reference (and debugging messages)
#define VFE_adapter_DV              1750./16384.; // 14 bits on 1.75V

#define VFE_adapter_NSAMPLE_MAX 28670
// Max ethernet packet = 1536 bytes, max user payload = 1500 bytes
#define VFE_adapter_MAX_PAYLOAD 1380
////////////////#define MAX_VFE     10
////////////////#define MAX_STEP    10000

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
        int    TriggerLoop()          { return _trigger_soft;           }
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
        int ProgramDAC();
        int CalibrationTriggerSetting();
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
        int _trigger_soft;            // 0: use external trigger (GPIO); 1: generate trigger from software (1 written in FW register)
        int _trigger_type;            // 0: pedestal;  1: calibration;  2: laser
        int _trigger_self;            // 0: don't generate trigger from data themselves; 1: generate trigger if any data > self_trigger_threshold
        int _trigger_self_threshold;  // threshold in ADC counts
        int _trigger_self_mask;       // channel mask to generate self trigger lsb=ch0 ... msb=ch4
        int _calib_level;             // DAC value 0 ... 65532
        int _calib_width;             // calib trigger width 0 ... 65532
        int _calib_delay;             // DAQ elay for calib triggers: 0 ... 65532
        int _calib_n;                 // number of calibration steps for linearity study
        int _calib_step;              // DAC step for linearity study
        int _negate_data;             // negate (1) or not (0) the converted data in the ADC
        int _signed_data;             // set ADC in normal binary mode (0) or 2's complement (1)
        int _input_span;              // set ADC input SPAN from 0x1f - 0 - 0x0f (0x10 = 1.383V, 0x1f=1.727V, 0=1.75V, 0x01=1.772V, 0x0f=2.087V)
        int _hw_daq_delay;            // readout waiting time on external trigger arrival [# clocks @ 160 MHz]
        // greater values moves the signal *left*
        int _sw_daq_delay;            // as _hw_daq_delay but for internally generated trigger (e.g. when using the internal trigger to trigger external HW, like a laser)

        int _nevent;

        int _debug;                   // debug level: 0: none, 1: functions, 2: detailed
};

#endif
