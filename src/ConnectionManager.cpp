#include "interface/ConnectionManager.hpp"

// ---------------- PUBLISHER ------------------
//
Publisher::Publisher(){
	socket=NULL;
	Port="";
	//async_=false;
}

Publisher::~Publisher(){ 
	Log("[3] Publisher:: Destructor",3);
	sleep(1); Clear();
} // give 0mq the time to flush

void Publisher::Init(){
	Log("[3] Publisher::Init",3);
	char buffer[256];
	sprintf(buffer,"tcp://*:%s",Port.c_str());
	socket=new zmq::socket_t(*context,ZMQ_PUB);
	//bind
	Log(string("[3] Publisher::Init buffer='")+buffer+"'",3);
	socket->bind(buffer);
}

void Publisher::Clear(){
	Log("[3] Publisher::Clear",3);
	// context is taken and should not be destroyed by this class
	if (socket != NULL) delete socket;
	socket=NULL;
	Port="";
}

int  Publisher::SendMessage(dataType &mex) //  return 0 if correct
{
	Log("[3] Publisher::SendMessage",3);
	pid_t childpid;
	char buffer[16];
	//if (async_){  ZMQ is already async
	//	childpid=fork();
	//	sprintf(buffer,"%d",childpid);
	//	Log( string("[3] Publisher::SendMessage Async: pid=")+buffer,3);
	//	if (childpid==0) //CHILD
	//		{} //nope
	//	else if (childpid>0) //PARENT
	//		return 0; //asyncronous: don't know if message delivered or not
	//	else throw fork_exception() ;
	//	}
	//create the message and put it in the send
	//zmq::message_t message( mex.data() ,mex.size() ,myzmq_nofree) ;
	zmq::message_t message( mex.data() ,mex.size() , NULL ) ;
	//   void*myMsg=mex.data();
	//   int mySize=mex.size();
	//   mex.release();
	//   zmq::message_t message( myMsg ,mySize , NULL ) ; // should take ownership
	socket->send(message);
	//
	//if(async_ && childpid==0) _exit(0); //kill child
	//Log("[3] Publisher::SendMessager: if Async you dont see me",3);
	return 0;
}

void Publisher::SetPort(int p){
	Log("[1] Publisher::SetPort",1);
	char buffer[8];
	sprintf(buffer,"%d",p);
	Port=buffer;
	Log("[3] Publisher::SetPort Port=" + Port,3);
	return;
}
void Publisher::Config(Configurator &){}; //TODO

// ---------------- SUBSCRIBER ------------------
Subscriber::Subscriber(){
	socket=NULL;
	Clear();
};
Subscriber::~Subscriber(){
	Log("[3] Subscriber::Destructor",3);
	sleep(1);
	Clear();
};
int  Subscriber::RecvMessage(dataType &mex) // return 0 if yes
{
	Log("[3] Subscriber::RecvMessage",3);
	zmq::message_t zmq_mex;	
	bool status=socket->recv(&zmq_mex,ZMQ_DONTWAIT);
	// bool status=socket->recv(&zmq_mex);

       	if ( !status ){
			Log("[3] Subscriber::RecvMessage Fail Read",3);
		 	mex.clear();
			return -1;
	}
	Log("[3] Subscriber::RecvMessage ReadSuccess",3);
	mex.copy(zmq_mex.data(), zmq_mex.size()) ;
	return 0;
}

void Subscriber::SetAddress(string ip,int port)
{
	Log("[3] Subscriber::SetAddress",3);
	char buffer[256];
	sprintf(buffer,"tcp://%s:%d",ip.c_str(),port);
	Address=buffer;
	Log("[3] Subscriber::SetAddress "+Address,3);
	return;

}

void Subscriber::Clear(){
	// context should not be destroyed
	if(socket != NULL) delete socket;
	socket=NULL;
	Address="";
} 
void Subscriber::Init(){
	socket=new zmq::socket_t(*context,ZMQ_SUB);
	socket->connect(Address.c_str() );
	socket->setsockopt(ZMQ_SUBSCRIBE,"",0);
} 
void Subscriber::Config(Configurator &){} //TODO
// ------------ CONTEXT ------------
//  MADE STATIC TO BE CORRECTLY DESTROYED
//void HasContext::SetContext(zmq::context_t *c){
//	context=c;
//} // 1 context per program
zmq::context_t* HasContext::context=new zmq::context_t( CONTEXT_THREAD );

// ---------------- CONNECTION MANAGER
//
//

void ConnectionManager::Clear(){}
void ConnectionManager::Init(){}
void ConnectionManager::Config(Configurator &){}
