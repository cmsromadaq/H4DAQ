#include "interface/StandardIncludes.hpp"

#ifndef DAEMON_H
#define DAEMON_H

//class ControlManager;
//class Daemon;
//class EventBuilder;
//class Configurator;

#include "interface/EventBuilder.hpp"
#include "interface/ControlManager.hpp"
#include "interface/ConnectionManager.hpp"
#include "interface/Configurator.hpp"
#include "interface/HWManager.hpp"


enum CMD_t {NOP=0,WWE,WE,EE,WBE,BT,WBT,EBT,SEND,RECV};
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
ControlManager 		*controlManager_;
Configurator 		*configurator_	;
ConnectionManager 	*connectionManager_;

pid_t pid_;

// by default uses deque, it can also use list in case
queue< Command > todo_; // front/pop/push/empty/size

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
