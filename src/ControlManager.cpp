#include "interface/ControlManager.hpp"

// --- Constructor
ControlManager::ControlManager(){
	Clear();
}

void ControlManager::Init( )
	{
	//
//	 char buffer[512];
//	 context=new zmq::context_t(1);
//	 if (mode== CTRLMNG_SERVER ){
//		 recvSockets	=new zmq::socket_t(*context, ZMQ_REP)[clientPorts_.size()];
//		 for( int iPort=0;iPort<clientPorts_.size() ;++iPort)
//		 	{
//			sprintf(buffer,"%d",clientPort[iPort]);
//			socket->bind ( (string("tcp://*:" + buffer  ) .c_str());	 
//			}
//	}
//	else //initialize only communication with server
	}

void ControlManager::Config( Configurator&c)
	{
	mode_=c.GetInt( CTRLMNG_CONTEXT + string("mode"));
	serverAddress_=c.GetValue( CTRLMNG_CONTEXT + string("serverAddress"));
	serverPort_=c.GetValue( CTRLMNG_CONTEXT + string("serverPort"));
	c.GetVecInt(CTRLMNG_CONTEXT + string("clientPorts"),clientPorts_);
	}

void ControlManager::Clear( )
	{
//	if (context_ != NULL ) delete [] context_;
//	if (sendSocket_ != NULL ) delete [] sendSocket_;
//	if (recvSocket_ != NULL ) delete [] recvSocket_;
//	context=NULL;
//	sendSockets=NULL;
//	recvSockets=NULL;
	daemon_=NULL;
	mode_=CTRLMNG_CLIENT;
	
	}
