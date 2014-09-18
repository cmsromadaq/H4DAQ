#include "interface/Daemon.hpp"
// --- Constructor 
Daemon::Daemon(){
	// get the pid of the current process
	pid_=getpid();
}


int Daemon::Init(string configFileName){
	try{
		//  Construct objects
		configurator_=new Configurator();
		eventBuilder_=new EventBuilder();
		controlManager_=new ControlManager();
		connectionManager_=new ConnectionManager();
		
		// Set Configurator ; and Init it
		configurator_->xmlFileName=configFileName;
		configurator_->Init();

		// Configure Everything else
		eventBuilder_		->Config(*configurator_);
		controlManager_		->Config(*configurator_);
		connectionManager_	->Config(*configurator_);
		// Init Everything
		eventBuilder_->Init();
		controlManager_->Init();
		connectionManager_->Init();

		return 0;
	} catch( std::exception &e) 
		{
		printf("Exception: %s\n",e.what());
		return 1;
		};
}


void Daemon::Clear()
{
	if (configurator_) delete configurator_;
	if (eventBuilder_) delete eventBuilder_;
	if (controlManager_) delete controlManager_;
	if (connectionManager_) delete connectionManager_;
}


void Daemon::Loop(){
while (true) {
	try{
		// check source that can populate the commands
		// check Connection Manager
		// if cmds not empty do something
		if (todo_.empty() ) { usleep(500);continue;}
		Command myCmd=todo_.front();	todo_.pop();
		switch (myCmd.cmd){
		case NOP: break;
		case SEND: connectionManager_->Send(); 
			   break;
		case RECV: connectionManager_->Recv(); 
			   break;
		case DATA: connectionManager_->Recv(); 
			   //eventBuilder_->...
			   break;
		case WWE: break; // ClearAllBuffer ;todo_->push();
		case WE: break; // Activate triggers
		case EE: break; // end triggers; start sending datas
		case WBT: break;
		case BT: break;
		case EBT: break;
		case WBE: break;

		}
	}
	catch(std::exception &e){ printf("%s\n",e.what()); }
}
return;
}
