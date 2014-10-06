#ifndef DAEMON_H
#define DAEMON_H

#include "interface/StandardIncludes.hpp"
#include "interface/Logger.hpp"

// fwd decl -- avoid loop
//class ControlManager;
class ConnectionManager;
class Daemon;
class EventBuilder;
class Configurator;
class HwManager;
class dataType;


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
bool myPausedFlag_;
timeval lastSentStatusMessageTime_;

timeval start_time;
timeval stopwatch_start_time;
timeval stopwatch_stop_time;

const static int DataSck = 0;
const static int StatusSck = 1;
const static int CmdSck = 2;

int iLoop;
int waitForDR_;

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
	//
	virtual void ErrorStatus(){};
        virtual void PublishStatusWithTimeInterval();
        virtual void SendStatus(STATUS_t oldStatus, STATUS_t newStatus);
	
	//
	int Daemonize();
			
};


// fwd decl
#include "interface/EventBuilder.hpp"
//#include "interface/ControlManager.hpp"
#include "interface/HwManager.hpp"
#include "interface/ConnectionManager.hpp"
#include "interface/Configurator.hpp"

#endif
