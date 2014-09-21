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
		hwManager_=new HwManager();
		connectionManager_=new ConnectionManager();
		
		// Set Configurator ; and Init it
		configurator_->xmlFileName=configFileName;
		configurator_->Init();

		// Configure Everything else
		eventBuilder_		->Config(*configurator_);
		hwManager_		->Config(*configurator_);
		connectionManager_	->Config(*configurator_);
		// Init Everything
		eventBuilder_->Init();
		hwManager_->Init();
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
	if (hwManager_) delete hwManager_;
	if (connectionManager_) delete connectionManager_;
}


void Daemon::Loop(){
while (true) {
	try{
		// check source that can populate the commands
		// check Connection Manager
		// if cmds not empty do something
	switch (myStatus_ ) {
	case OUT_OF_RUN: 
		{
			if (todo_.empty() ) { usleep(500);continue;}
			Command myCmd=todo_.front();	todo_.pop();
			switch (myCmd.cmd){
				case NOP: break;
				case SEND: 
					  {
					   connectionManager_->Send(); 
					   break;
					  }
				case RECV: 
					  {
					   dataType myMex;
					   connectionManager_->Recv(myMex); 
					   break;
					  }
				case DATA: 
					  {
					   dataType myMex;
					   connectionManager_->Recv(myMex); 
					   //eventBuilder_->...
					   break;
					  }
				case WWE: break; // ClearAllBuffer ;todo_->push(); ENTER WAITING FOR TRIGGER
				case WE: break; // Activate triggers
				case EE: break; // end triggers; start sending datas
				case WBT: break;
				case BT: break;
				case EBT: break;
				case WBE: break;

			}
			break;
		} // OUT_OF RUN
	//eventBuilder->myRun is open
	default:
	case WAIT_TRIG:
		{
		// read if I have a trigger
		if( hwManager_->TriggerReceived() ){
			//read everything 	
			dataType event=hwManager_->ReadAll();
			eventBuilder_->AddEventToRun(event);
			hwManager_->BufferClearAll();
			}
		// read if I have something on the network
		dataType myMex;
		if (connectionManager_->Recv(myMex) ==0 )
			{
			//case EE: break; // end triggers; close runs;start sending data
			}
	       	break;
		} // WAIT_TRIG

	}//end of status switch
	}
	catch(std::exception &e){ printf("%s\n",e.what()); }
}
return;
}
