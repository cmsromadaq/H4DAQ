#include "interface/StandardIncludes.hpp"


using namespace std;

#ifndef CTRLMNG_H
#define CTRLMNG_H

class Daemon;
class ControlManager;
class Configurator;
//#include "interface/Daemon.hpp"
#include "interface/Configurator.hpp"

#define CTRLMNG_CLIENT 0
#define CTRLMNG_SERVER 1
#define CTRLMNG_CONTEXT "CTRLMNG_"

class ControlManager : public Configurable
{
//zmq::context_t  *context_;
//zmq::socket_t   *sendSocket_;
//zmq::socket_t   *recvSocket_;

Daemon *daemon_;
int mode_;//mode=0 client, mode=1 server
string serverAddress_;
string serverPort_; // server listen on more ports
vector<int> clientPorts_;

void InitClient();
void InitServer();

public:
	// --- constructor
	ControlManager();
	// --- destructor
	~ControlManager();
	// --- Initialization 
	//     mode: 0, client; 1, server-> it is used  to decide whatever this instance has to control other instances or being controlled (receive inputs
	void Config(Configurator &c);
	void Init(); 
	void Clear();
	void Listen();
	void Send();

};

#endif
