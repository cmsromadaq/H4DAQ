#include "interface/StandardIncludes.hpp"
#include "interface/Configurator.hpp"
#include "interface/EventBuilder.hpp"
#include "interface/Logger.hpp"

#ifndef CONNMNG_H
#define CONNMNG_H
// 0MQ Implementation
class Configurator;
class Context{
protected:
	zmq::context_t *context; //common in the whole event
public:
	void SetContext(zmq::context_t *c);
	
};

class Publisher: 	public Configurable,
			public LogUtility,
			public Context {
private:
	//zmq::context_t *context; //common in the whole event
	zmq::socket_t *socket;
	string Port;
	bool async_;
public:
	Publisher();
	~Publisher();
	void SetAsync(bool async=true){async_=async;}
	int  SendMessage(dataType &mex); //  return 0 if correct
	void SetPort(int p);
	void Clear();
	void Init();
	void Config(Configurator &);
};

class Subscriber: 	public Configurable,
			public LogUtility, 
			public Context{
private:
	//zmq::context_t *context;
	zmq::socket_t *socket;
	string Address;
public:
	Subscriber();
	~Subscriber();
	int RecvMessage(dataType &mex); // return 0 if yes - NO STRING!
	void SetAddress(string ip,int port);
	void Clear();
	void Init();
	void Config(Configurator &);
	//void SetContext(zmq::context_t *c);

};

class ConnectionManager: public Configurable{
	/*this class implements the actual connections at low level
	 */
private:
vector<string> recvAddresses;
vector<string> sendPorts;
public:
	void Clear();
	void Init();
	void Config(Configurator &);

};


#endif
