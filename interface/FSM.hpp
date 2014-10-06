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
        void ReportTransferPerformance(long transferTime, dataTypeSize_t transrate_size);
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
		    bool gui_pauserun 	;
		    bool gui_restartrun ;
		    bool gui_stoprun 	;
		    bool gui_die 	;
		    bool eb_endspill 	;

public:
	RunControlFSM();
	void Loop();
	inline void Clear(){Daemon::Clear();}
	inline int Init(string configFileName="data/config.xml"){return Daemon::Init(configFileName);};
	bool IsOk();
	void inline ResetMex(){gui_pauserun=false; gui_stoprun=false; gui_restartrun=false; gui_die=false; eb_endspill=false;};
	void UpdateMex();
};

#endif
