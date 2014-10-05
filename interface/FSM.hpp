#ifndef FSM_H
#define FSM_H
#include "interface/StandardIncludes.hpp"
#include "interface/Daemon.hpp"


class DataReadoutFSM : public Daemon {
public:
	// constructor
	DataReadoutFSM() ;
	void Loop();
	inline void Clear(){Daemon::Clear();};
	inline int Init(string configFileName="data/config.xml"){return Daemon::Init(configFileName);};
	bool IsOk();
};

class EventBuilderFSM : public Daemon{
bool eventStarted;
timeval transrate_stopwatch_start;
timeval transrate_stopwatch_stop;
dataTypeSize_t transrate_size;
public:
	// constructor
	EventBuilderFSM();
	void Loop();
	inline void Clear(){Daemon::Clear();}
	inline int Init(string configFileName="data/config.xml"){return Daemon::Init(configFileName);};
	bool IsOk();
};

class DummyRunControlFSM : public Daemon{

public:
	DummyRunControlFSM();
	void Loop();
	inline void Clear(){Daemon::Clear();}
	inline int Init(string configFileName="data/config.xml"){return Daemon::Init(configFileName);};
	bool IsOk();
};

class RunControlFSM : public Daemon{

protected:
	TRG_t trgType_;
	TRG_STATUS_t trgStatus_;
	long trgNevents_;
	long trgRead_;
	int readyDR_; // DR ready to take data

public:
	RunControlFSM();
	void Loop();
	inline void Clear(){Daemon::Clear();}
	inline int Init(string configFileName="data/config.xml"){return Daemon::Init(configFileName);};
	bool IsOk();
};

#endif
