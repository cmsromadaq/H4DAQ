#include "interface/Daemon.hpp"

#ifndef FSM_H
#define FSM_H


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
public:
	// constructor
	EventBuilderFSM();
	void Loop();
	inline void Clear(){Daemon::Clear();}
	inline int Init(string configFileName="data/config.xml"){return Daemon::Init(configFileName);};
	bool IsOk();
};

class RunControllerFSM : public Daemon{

public:
	RunControllerFSM();
	void Loop();
	inline void Clear(){Daemon::Clear();}
	inline int Init(string configFileName="data/config.xml"){return Daemon::Init(configFileName);};
	bool IsOk();
};

#endif
