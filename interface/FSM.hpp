#include "interface/Daemon.hpp"

#ifndef FSM_H
#define FSM_H


class DataReadoutFSM : public Daemon {
public:
	void Loop();
	inline void Clear(){Daemon::Clear();};
	inline int Init(string configFileName="data/config.xml"){return Daemon::Init(configFileName);};
};

class EventBuilderFSM : public Daemon{
public:
	void Loop();
	inline void Clear(){Daemon::Clear();}
	inline int Init(string configFileName="data/config.xml"){return Daemon::Init(configFileName);};
};

class RunControllerFSM : public Daemon{

public:
	void Loop();
	inline void Clear(){Daemon::Clear();}
	inline int Init(string configFileName="data/config.xml"){return Daemon::Init(configFileName);};
};

#endif
