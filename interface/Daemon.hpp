#include "interface/StandardIncludes.hpp"

#ifndef DAEMON_H
#define DAEMON_H

//class ControlManager;
//class Daemon;
//class EventBuilder;
//class Configurator;

#include "interface/EventBuilder.hpp"
//#include "interface/ControlManager.hpp"
#include "interface/HwManager.hpp"
#include "interface/ConnectionManager.hpp"
#include "interface/Configurator.hpp"
#include "interface/HwManager.hpp"


// this are the cmd that the finate state machine can receive
enum CMD_t {NOP=0,WWE,WE,EE,WBE,BT,WBT,EBT,SEND,RECV,DATA};
// this are the status of the finate state machines
enum STATUS_t {WAIT_TRIG=0,OUT_OF_RUN};

/* Command description:
 * NOP : No Operation. Need for dummy messages that goes around
 * WWE : SPS command of Warning Warning Ejection: 1s 
 * WE  : SPS command of Warining Ejection: 10ms
 * WBE : SPS command of Warning Bad Ejection
 * EE  : SPS command of End Ejection
 * BT  : SPS command of Burst Test: Train of 10 khz pulses, duration 1s
 * WBT : SPS command of Warning Burst Test
 * EBT : SPS command of End of Burst Test
 */

class Command{
public:
	Command(){cmd=NOP; data=NULL; N=0;}
	void *data;
	int N;
	CMD_t cmd;
};

class Daemon 
{
friend class ControlManager;

private:
	//private variable each word is capital execpt the first. Underscore at the end
	// This class should be think as a finite state machine that decides what to do
EventBuilder 		*eventBuilder_	; 
HwManager 		*hwManager_;
Configurator 		*configurator_	;
ConnectionManager 	*connectionManager_;

pid_t pid_;

// by default uses deque, it can also use list in case
queue< Command > todo_; // front/pop/push/empty/size
STATUS_t myStatus_;

public:
	// --- Constructor
	Daemon();
	// --- Destructor
	~Daemon();
	// --- Initialization
		//  Init itself and all the othe process
	int Init(string configFileName="data/config.xml"); 
	// --- Loop
	//     Will loop on the things to do, and handle exceptions
	void Loop();
	// --- Reset
	void Clear();


};


#endif
