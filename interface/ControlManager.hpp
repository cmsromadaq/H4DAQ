#include "interface/StandardInclude.hpp"
#include "interface/Daemon.hpp"

using namespace std;

#ifndef CTRLMNG_H
#define CTRLMNG_H

#define CTRLMNG_CLIENT 0
#define CTRLMNG_SERVER 1

class ControlManager
{
zmq::context_t  *context;
zmq::socket_t   *socket;

Daemon *daemon;


public:
	// --- constructor
	ControlManager();
	// --- destructor
	~ControlManager();
	// --- Initialization 
	//     mode: 0, client; 1, server-> it is used  to decide whatever this instance has to control other instances or being controlled (receive inputs
	void Init(unsigned short mode=0); //mode=0 client, mode=1 server
	void Reset();
	void Listen();
	void Send();

};

#endif
