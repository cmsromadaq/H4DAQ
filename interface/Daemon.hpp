#include "interface/StandardIncludes.hpp"
#include "interface/Logger.hpp"

#ifndef DAEMON_H
#define DAEMON_H


// fwd decl -- avoid loop
//class ControlManager;
class ConnectionManager;
class Daemon;
class EventBuilder;
class Configurator;
class HwManager;
class dataType;



// this are the cmd that the finate state machine can receive
enum CMD_t {NOP=0,WWE,WE,EE,WBE,BT,WBT,EBT,STARTRUN,SEND,RECV,DATA,STATUS,SPILLCOMPL,ENDRUN,DIE};
// this are the status of the finate state machines
enum STATUS_t { START=0, INIT, INITIALIZED ,  BEGINSPILL, CLEARED,CLEARBUSY,WAITTRIG, READ, ENDSPILL, SENTBUFFER,SPILLCOMPLETED,BYE  };

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

class Command{
public:
	Command(){cmd=NOP; data=NULL; N=0;}
	~Command(){ if (data != NULL) delete [] (char*)data;}
	void *data;
	dataTypeSize_t N;
	CMD_t cmd;
	string name() ;
};


class Daemon : public LogUtility
{
protected:
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

timeval start_time;
timeval stopwatch_start_time;
timeval stopwatch_stop_time;

public:
	// --- Constructor
	Daemon();
	// --- Destructor
	~Daemon();
	// --- Initialization
		//  Init itself and all the othe process
	virtual int Init(string configFileName="data/config.xml"); 
	// --- Loop
	//     Will loop on the things to do, and handle exceptions
	virtual void Loop()=0;
	// --- Reset
	virtual void Clear();
	// --- this is meant to parse the commands that navigate through the network
	Command ParseData(dataType &mex);
	// ---
	void MoveToStatus(STATUS_t newStatus);
	// -- check if configuration is consistent with the subclasses
	virtual bool IsOk();//{return true;};
	//
	void LogInit(Logger*l);
			
};


// fwd decl
#include "interface/EventBuilder.hpp"
//#include "interface/ControlManager.hpp"
#include "interface/HwManager.hpp"
#include "interface/ConnectionManager.hpp"
#include "interface/Configurator.hpp"

#endif
