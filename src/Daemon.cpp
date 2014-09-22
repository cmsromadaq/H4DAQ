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


/*
void Daemon::Loop(){
while (true) {
	try{
		// check source that can populate the commands -- Connection and HW
		// check Connection Manager
		// if cmds not empty do something
	switch (myStatus_ ) {
	case OUT_OF_SPILL: 
		{
			if (todo_.empty() ) { 
				Command myCmd; 
				myCmd.cmd=RECV; 
				todo_.push(myCmd);  
				myCmd.cmd=READ;
				todo_.push(myCmd);  
				usleep(50); // avoid ultra-fast loop. Maybe remove it
				continue;
			}
			Command myCmd=todo_.front();	todo_.pop();
			switch (myCmd.cmd){
				case NOP: break;
				case SEND: 
					  {
					   dataType myMex; 
					   // the command is already formatted
					   //myMex.append( (void*)"SEND\0" , 5 );
					   myMex.append( myCmd.data , myCmd.N );
					   connectionManager_->Send(myMex); 
					   break;
					  }
				case RECV: 
					  {
					   dataType myMex;
					   connectionManager_->Recv(myMex);  // parse mex to generate cmds
					   Command myNewCmd=ParseData(myMex);

					   // don't push for the machine which is also SPILL CONTROL
					   if ( hwManager_->IsRunControl()  && 
							   ( myNewCmd.cmd == WWE || myNewCmd.cmd == WE  || myNewCmd.cmd == EE ||
							     myNewCmd.cmd == WBE || myNewCmd.cmd == BT  || myNewCmd.cmd == WBT || 
							     myNewCmd.cmd == EBT ) 
					      ) break;
					   todo_.push(myNewCmd);
					   break;
					  }
				case DATA: 
					  {
					   dataType myMex;
					   myMex.append(myCmd.data,myCmd.N);
					   eventBuilder_->MergeSpills(myMex);
					   break;
					  }
				case WWE: {
					// send WWE
				          if (hwManager_->IsRunControl()){ // SEND NOW, 'cause otherwise may not process until EE
					  	dataType myMex; myMex.append((void*)"WWE\0",4);
					   	connectionManager_->Send(myMex); 
					  	}
					   hwManager_->BufferClearAll(); 
					    break; // ClearAllBuffer ;todo_->push(); 
					  }
				case WE:  // Activate triggers - start run
					  {
				          if (hwManager_->IsRunControl()){ // SEND NOW, 'cause otherwise may not process until EE
					  	dataType myMex; myMex.append((void*)"WE\0",3);
					   	connectionManager_->Send(myMex); 
					  	}
					  eventBuilder_->OpenSpill();
					  myStatus_=WAIT_TRIG;
					  break;
					  }
				case EE:  {// this command is useless in this status (Out-of-run)
				          // if control manager, send to all EE - in the WAIT_FOR_TRIGGER, the SPILLCONTROL LOAD this cmd
					  if ( !hwManager_->IsRunControl()) break;
					  Command myNewCmd;
					  myNewCmd.cmd=SEND;
					  dataType myMex; myMex.append((void*)"EE\0",3);
					  myNewCmd.data= myMex.data();
					  myNewCmd.N=myMex.size();
					  myMex.release();
					  todo_.push(myNewCmd); 
					  break; // end triggers; start sending data; t 
					  }
				case WBT: {
				          if (hwManager_->IsRunControl()){ // SEND NOW, 'cause otherwise may not process until EE
					  	dataType myMex; myMex.append((void*)"WBT\0",4);
					   	connectionManager_->Send(myMex); 
					  	}
					  break;
					  }
				case BT: {
				          if (hwManager_->IsRunControl()){ // SEND NOW, 'cause otherwise may not process until EE
					  	dataType myMex; myMex.append((void*)"BT\0",3);
					   	connectionManager_->Send(myMex); 
					  	}
					  break;
					 }
				case EBT: {
				          if (hwManager_->IsRunControl()){ // SEND NOW, 'cause otherwise may not process until EE
					  	dataType myMex; myMex.append((void*)"EBT\0",4);
					   	connectionManager_->Send(myMex); 
					  	}
						  break;
					  }
				case WBE: {
				          if (hwManager_->IsRunControl()){ // SEND NOW, 'cause otherwise may not process until EE
					  	dataType myMex; myMex.append((void*)"WBT\0",4);
					   	connectionManager_->Send(myMex); 
					  	}
						  break;
					  }
				case READ: { // TODO
					   // Ask HwManager to read Boards with SPS Status.
					   if ( !hwManager_->IsRunControl()  ) break;
					   // Generate ad hoc commands
					   dataType myMex;
					   //---- .... ----
					   myMex.append((void*)"WWE\0",4);
					   Command myReadedCmd;
					   myReadedCmd.cmd=SEND;
					   myReadedCmd.data=myMex.data();
					   myReadedCmd.N=myMex.size();

					   myMex.release();
					   break;
					   }

			}
			break;
		} // OUT_OF SPILL
	//eventBuilder->mySpill is open
	default:
	case WAIT_TRIG:
		{
		// read if I have a trigger
		if( hwManager_->TriggerReceived() ){
			//read everything 	
			dataType event=hwManager_->ReadAll();
			eventBuilder_->AddEventToSpill(event);
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
				myStatus_=OUT_OF_SPILL;
				//case EE: break; // end triggers; close runs;start sending data
				Command closeSpillCmd=eventBuilder_->CloseSpill(); // this will send the run or dump it
				if (hwManager_->IsRunControl() ){ // append the EE to the list of commands
					todo_.push(myNewCmd);
					}
				if (closeSpillCmd.cmd == SEND) // this will send the DATA oft the run
					todo_.push(closeSpillCmd);
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
*/



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
	else if (N >=7  and !strcmp( (char*) mex.data(), "STATUS")  )
		{
		mex.erase(0,7);
		myCmd.cmd=STATUS;
		myCmd.data = mex.data();
		myCmd.N    = mex.size();
		mex.release();
		}
	else if (N >=9  and !strcmp( (char*) mex.data(), "STARTRUN")  )
		{
		mex.erase(0,9);
		myCmd.cmd=STARTRUN;
		myCmd.data = mex.data();
		myCmd.N    = mex.size();
		mex.release();
		}
	else if (N >=11  and !strcmp( (char*) mex.data(), "SPILLCOMPL")  )
		myCmd.cmd=SPILLCOMPL;
	else if (N >=7  and !strcmp( (char*) mex.data(), "ENDRUN")  )
		myCmd.cmd=ENDRUN;
	else if (N >=4  and !strcmp( (char*) mex.data(), "DIE")  )
		myCmd.cmd=DIE;
	return myCmd;
}



void Daemon::MoveToStatus(STATUS_t newStatus){
	dataType myMex;
	myMex.append((void*)"STATUS\0",7);
	WORD myStatus=(WORD)newStatus;
	myMex.append((void*)&myStatus,WORDSIZE);
	connectionManager_->Send(myMex);
	myStatus_=newStatus;
}
