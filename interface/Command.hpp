#ifndef CMD_H
#define CMD_H

#include "interface/DataType.hpp" // dataTypeSize_t
#include "interface/StandardIncludes.hpp"

// this are the cmd that the finate state machine can receive
enum CMD_t {	NOP=0,
		WWE,     // Run Controller Commands 1-99
		WE,
		EE,
		WBE,
		BT,
		WBT,
		EBT,
		STARTRUN,
		SEND,
		RECV,
		DATA,
		STATUS,
		SPILLCOMPL,
		ENDRUN,
		DIE,
		ERRORCMD,
		// GUI Commands 100 - 199: Parsed only by the RunControl
		GUI_STARTRUN= 100, // run# type#(PEDESTAL,LED,PHYSICS) #trig/spill (int)
		GUI_RESTARTRUN,
		GUI_STOPRUN,
		GUI_PAUSERUN,
		GUI_DIE,
		// EB Commands
		EB_SPILLCOMPLETED= 200,
		DR_READY // DR are ready to take data
		};
// this are the status of the finate state machines
enum STATUS_t { START 		= 0, 
		INIT 		= 1, 
		INITIALIZED	= 2 ,  
		BEGINSPILL 	= 3, 
		CLEARED 	= 4,
		WAITFORREADY 	= 5,
		CLEARBUSY 	= 6,
		WAITTRIG 	= 7, 
		READ     	= 8, 
		ENDSPILL	= 9, 
		RECVBUFFER	= 10,
		SENTBUFFER	= 11,
		SPILLCOMPLETED	= 12,
		BYE		= 13,
		ERROR		= 14,
		};

/* Command description:
 * NOP : No Operation. Need for dummy messages that goes around
 * WWE : SPS command of Warning Warning Ejection: 1s 
 * WE  : SPS command of Warining Ejection: 10ms
 * WBE : SPS command of Warning Bad Ejection
 * EE  : SPS command of End Ejection
 * BT  : SPS command of Burst Test: Train of 10 khz pulses, duration 1s
 * WBT : SPS command of Warning Burst Test
 * EBT : SPS command of End of Burst Test
 * RECV: Check if mex is received
 * SEND: send cmd
 * DATA: received data
 * READ: check if board with sps status generated something
 */

class Command{ // TODO pass to dataType
public:
	Command(){cmd=NOP; data=NULL; N=0;}
	~Command(){ if (data != NULL) delete [] (char*)data;}
	void *data;
	dataTypeSize_t N;
	CMD_t cmd;
	string name() ;
	inline void release(){data=NULL;N=0;};
};

#endif
