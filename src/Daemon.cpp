#include "interface/Daemon.hpp"

string Command::name()
{
switch (cmd) {
	case WWE  : { return "WWE" ; }
	case WE   : { return "WE"  ; }
	case EE   : { return "EE"  ; }
	case WBE  : { return "WBE" ; }
	case BT   : { return "BT"  ; }
	case WBT  : { return "WBT" ; }
	case EBT  : { return "EBT" ; }
	case SEND : { return "SEND"; }
	case RECV : { return "RECV"; }
	case DATA : { return "DATA"; }
	default:
	case NOP  : { return "NOP"; }
	}
}

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
	if (configurator_) { configurator_->Clear(); delete configurator_; }
	if (eventBuilder_) {eventBuilder_->Clear(); delete eventBuilder_; }
	if (hwManager_) { hwManager_->Clear(); delete hwManager_; }
	if (connectionManager_) { connectionManager_->Clear(); delete connectionManager_;}
}


void Daemon::Loop(){
while (true) {
	try{
		// check source that can populate the commands -- Connection and HW
		// check Connection Manager
		// if cmds not empty do something
	switch (myStatus_ ) {
	case OUT_OF_RUN: 
		{
			if (todo_.empty() ) { Command myCmd; myCmd.cmd=RECV; todo_.push(myCmd);  usleep(500);continue;}
			Command myCmd=todo_.front();	todo_.pop();
			switch (myCmd.cmd){
				case NOP: break;
				case SEND: 
					  {
					   dataType myMex;
					   myMex.append( (void*)"SEND\0" , 5 );
					   myMex.append( myCmd.data , myCmd.N );
					   connectionManager_->Send(myMex); 
					   break;
					  }
				case RECV: 
					  {
					   dataType myMex;
					   connectionManager_->Recv(myMex);  // parse mex to generate cmds
					   Command myNewCmd=ParseData(myMex);
					   todo_.push(myNewCmd);
					   break;
					  }
				case DATA: 
					  {
					   dataType myMex;
					   myMex.append(myCmd.data,myCmd.N);
					   eventBuilder_->MergeRuns(myMex);
					   break;
					  }
				case WWE: break; // ClearAllBuffer ;todo_->push(); ENTER WAITING FOR TRIGGER
				case WE:  // Activate triggers - start run
					  {
					  eventBuilder_->OpenRun();
					  myStatus_=WAIT_TRIG;
					  break;
					  }
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
			int N=myMex.size();
			Command myNewCmd = ParseData( myMex);
			if (myNewCmd.cmd == EE ) 
				{ // EE
				myStatus_=OUT_OF_RUN;
				//case EE: break; // end triggers; close runs;start sending data
				Command closeRunCmd=eventBuilder_->CloseRun(); // this will send the run or dump it
				if (closeRunCmd.cmd == DATA)
					todo_.push(closeRunCmd);
				}
			else // other mex ??
				{ 
				todo_.push(myNewCmd );
				}
			}
	       	break;
		} // WAIT_TRIG

	}//end of status switch
	}
	catch(std::exception &e){ printf("--- EXCEPTION ---\n%s\n-------------\n",e.what()); throw e; }
} // while-true
return;
}



Command Daemon::ParseData(dataType mex)
{
	Command myCmd;
	int N=mex.size();
	// SEND
	if (N >=5  and !strcmp( (char*) mex.data(), "SEND")  )
		//	((char*)mex.data() ) [0]== 'S' and
		//	((char*)mex.data() ) [1]== 'E' and
		//	((char*)mex.data() ) [2]== 'N' and
		//	((char*)mex.data() ) [3]== 'D' and
		//	((char*)mex.data() ) [4]== '\0'  
		{
		mex.erase(0,5);
		myCmd.cmd=SEND;
		myCmd.data = mex.data();
		myCmd.N    = mex.size();
		mex.release();
		}
	else if (N >=5  and !strcmp( (char*) mex.data(), "RECV")  )
		{
		mex.erase(0,5);
		myCmd.cmd=RECV;
		myCmd.data = mex.data();
		myCmd.N    = mex.size();
		mex.release();
		}
	else if (N >=5  and !strcmp( (char*) mex.data(), "DATA")  )
		{
		mex.erase(0,5);
		myCmd.cmd=DATA;
		myCmd.data = mex.data();
		myCmd.N    = mex.size();
		mex.release();
		}
	else if (N >=4  and !strcmp( (char*) mex.data(), "NOP")  )
		myCmd.cmd=NOP;
	else if (N >=4  and !strcmp( (char*) mex.data(), "WWE")  )
		myCmd.cmd=WWE;
	else if (N >=3  and !strcmp( (char*) mex.data(), "WE")  )
		myCmd.cmd=WE;
	else if (N >=3  and !strcmp( (char*) mex.data(), "EE")  )
		myCmd.cmd=EE;
	else if (N >=4  and !strcmp( (char*) mex.data(), "WBT")  )
		myCmd.cmd=WBT;
	else if (N >=4  and !strcmp( (char*) mex.data(), "WBE")  )
		myCmd.cmd=WBE;
	else if (N >=4  and !strcmp( (char*) mex.data(), "EBT")  )
		myCmd.cmd=EBT;
	return myCmd;
}
