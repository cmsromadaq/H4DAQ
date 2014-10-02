#include "interface/Daemon.hpp"
#include "interface/FSM.hpp"
#include "interface/Utility.hpp"

//#define FSM_DEBUG
#define SYNC_DEBUG

// --- Constructor: C++11 inherits automatically. C++03 no
DataReadoutFSM::DataReadoutFSM(): Daemon() {

}

bool DataReadoutFSM::IsOk(){
	if ( eventBuilder_->GetSendEvent() != true ) 
		{	
		Log("[DataReadoutFSM]::[IsOk] FSM not configured to send event",1);
		return false;
		}
	return true;
}

void DataReadoutFSM::Loop(){
// not constructed in the constructor -- should take this from the configuration

if ( !IsOk() ) throw config_exception();

while (true) {
	try{
	 SendStatus();
		// check source that can populate the commands -- Connection and HW
		// check Connection Manager
		// if cmds not empty do something
	switch (myStatus_ ) {
	case START: {// not in the LOOP
			    MoveToStatus(INIT);
			    break;
		    } 
	case INIT:  {   // not in the LOOP
			    MoveToStatus(INITIALIZED);
			    break;
		    } 
	case INITIALIZED:
		    {
		    // wait for start run
		    dataType myMex;
		    if (connectionManager_->Recv(myMex) ==0 )
			    {
			    Log("[DataReadoutFSM]::[Loop]::[INITIALIZED] Received a message on the incoming connection",3);
			    Command myCmd=ParseData(myMex);
			    if( myCmd.cmd ==  STARTRUN ) 
			   	 {
				   hwManager_->BufferClearAll();
				   eventBuilder_->ResetSpillNumber();
				   WORD myRunNum=*(WORD*)myCmd.data;
				   ostringstream s;
				   s << "[DataReadoutFSM]::[INFO]::Starting a new run " <<  myRunNum;
				   Log(s.str(),1);
				   // init RunNum in eventBuilder
				   eventBuilder_->SetRunNum(myRunNum);
				   MoveToStatus(BEGINSPILL);
				 }
			    }
		    break;
		    }
	case BEGINSPILL:
		    {
		    // wait for wwe
		    dataType myMex;
		    if (connectionManager_->Recv(myMex) ==0 )
			    {
			    Command myCmd=ParseData(myMex);
			    if( myCmd.cmd ==  WWE ) 
			   	 {
				   hwManager_->BufferClearAll();
				   hwManager_->ClearBusy(); //just for "safety"
				   eventBuilder_->OpenSpill();
				   MoveToStatus(CLEARED);
				 }
			    }
		    break;
		    }
	case CLEARED:
		    {
		    // wait for we
		    dataType myMex;
		    if (connectionManager_->Recv(myMex) ==0 )
			    {
			    Command myCmd=ParseData(myMex);
			    if( myCmd.cmd ==  WE ) 
			   	 {

				   //					 hwManager_->BufferClearAll();
				         // Send Ready to the RC
					dataType myMex;
					myMex.append((void*)"DR_READY\0",9);
					connectionManager_->Send(myMex,CmdSck);
					MoveToStatus(WAITTRIG);
				 }
			    }
		    break;
		    }
	case CLEARBUSY: {
		        hwManager_->ClearBusy();
			MoveToStatus(WAITTRIG);
			}
	case WAITTRIG:
		    {
		     // check network
		    dataType myMex;
		    if (connectionManager_->Recv(myMex) ==0 )    
		    {                                                                     
			    Command myNewCmd = ParseData( myMex); 
			    if (myNewCmd.cmd == EE )  {
			      hwManager_->ClearBusy();
				    MoveToStatus(ENDSPILL);
				    break;
			    }
		    }
		     /// check trigger
		    if( hwManager_->TriggerReceived() ){ 
			cout<<"TRIGGER RECEIVED"<<endl;
			hwManager_->TriggerAck();
			MoveToStatus(READ);
                        }  

		    break;
		    }
	case READ:
		    {
			static int READED=0;
			cout<<"Counter "<<std::dec<<++READED<<endl;
			static int Counter=0; 
			if (Counter==0 )  gettimeofday(&stopwatch_start_time,NULL);
			++Counter;
			if (Counter >= 500) {
				gettimeofday(&stopwatch_stop_time,NULL);
				long elapsed=  Utility::timevaldiff(&stopwatch_start_time,&stopwatch_stop_time);
				ostringstream s; s <<"[DataReadOutFSM]::[READ] "<<"Triggers in spill "<< READED<<" Rate "<< double(Counter)/elapsed* 1e6 <<" Hz";
				cout <<s.str()<<endl;
				Log(s.str(),1);
				Counter=0;
			}
			//----- Costruct Events
                        dataType event;
			eventBuilder_->OpenEvent(event,hwManager_->GetNboards());
			hwManager_->ReadAll(event);                                 /// DEBUG ME
#ifdef SYNC_DEBUG
			int sleeptime=rand()%400 +100;
			usleep(sleeptime);
#endif

			eventBuilder_->CloseEvent(event);
			// ----- Add Event To Spill
                        eventBuilder_->AddEventToSpill(event);                                
			MoveToStatus(CLEARBUSY);
		    break;
		    }
	case ENDSPILL:
		    {
			    // received EE
			Command myCmd=eventBuilder_->CloseSpill(); // eventBuilder know if the mex is to be sent on the network
			if (myCmd.cmd == SEND)
			{
				dataType myMex;
				myMex.append(myCmd.data,myCmd.N);
				connectionManager_->Send(myMex,DataSck);
			}
			MoveToStatus(SENTBUFFER);
		    break;
		    }
	case SENTBUFFER:
		    { // wait for SPILLCOMPLETED
		    dataType myMex;
		    if (connectionManager_->Recv(myMex) ==0 )    
		    {                                                                     
			    Command myNewCmd = ParseData( myMex); 
			    if (myNewCmd.cmd == SPILLCOMPL )  {
				    MoveToStatus(BEGINSPILL);
			    }
			    else if (myNewCmd.cmd == ENDRUN){
				    MoveToStatus(INITIALIZED);
			    }
			    else if (myNewCmd.cmd == DIE){
				    MoveToStatus(BYE);
			    }
		    }
			
		    break;
		    }
	case ERROR: {
		    ErrorStatus();
		    break;
		    }
	case BYE:
		    {
		    exit(0); // return is not working correctly
		    return;
		    }

	} // end switch 
	} // end try
	catch(sigint_exception &sigint) { printf("\n%s\n",sigint.what()); exit(0);return ; } // grace exit . return doesn't work. 
	catch(std::exception &e){ printf("--- EXCEPTION ---\n%s\n-------------\n",e.what()); throw e; }
} // while-true
return;
} // end Loop

// ------------------------- RUN CONTROLLER
// --- Constructor
DummyRunControlFSM::DummyRunControlFSM() : Daemon() {};

bool DummyRunControlFSM::IsOk(){
	//if( hwManager_->GetRunControl() != true) return false;
	return true;
}

void DummyRunControlFSM::Loop(){

if ( !IsOk() ) throw config_exception();

while (true) {
	try{
	SendStatus();
		// check source that can populate the commands -- Connection and HW
		// check Connection Manager
		// if cmds not empty do something
	switch (myStatus_ ) {
	case START: {// not in the LOOP
			    MoveToStatus(INIT);
			    break;
		    } 
	case INIT:  {   // not in the LOOP
			    MoveToStatus(INITIALIZED);
			    break;
		    } 
	case INITIALIZED:
		    {
		    // start RUN
			    printf ("waiting for RunNum\n:");
			    WORD runNum;
			    scanf("%u",&runNum);
			    printf("Starting Run %u\n",runNum);
			    dataType myFufMex;
			    myFufMex.append((void*)"NOP\0",4);
			    connectionManager_->Send(myFufMex,CmdSck);
			    dataType myMex;
			    myMex.append((void*)"STARTRUN\0",9);
			    myMex.append((void*)&runNum,WORDSIZE);
			    printf("RUN MEX: %d\n",myMex.size());
				printf("------------------\n");
				//fwrite(myMex.data(),1,myMex.size(),stdout);
				printf("%s",Utility::AsciiData(myMex.data(),myMex.size()).c_str() );
				printf("------------------\n");
			    connectionManager_->Send(myMex,CmdSck);
			    MoveToStatus(READ);
			    break;
		    }
	case BEGINSPILL:
		    {
		    // send WWE
			    printf("WWE\n");
			    dataType myMex;
			    myMex.append((void*)"WWE\0",4);
			    connectionManager_->Send(myMex,CmdSck);
			    MoveToStatus(READ);
			    break;

		    }
	case CLEARED:
		    {
		    // send WE
			    printf("WE\n");
			    dataType myMex;
			    myMex.append((void*)"WE\0",3);
			    connectionManager_->Send(myMex,CmdSck);
			    MoveToStatus(READ);
			    break;
		    }
	case WAITTRIG:
		    {
			    // Read Network Buffer
			    dataType myMex;
			    if (connectionManager_->Recv(myMex) == 0 )
			    {
				printf("Reiceved Message:\n");
				printf("------------------\n");
				printf("size = %d\n",myMex.size() ) ;
				printf("------------------\n");
				//fwrite(myMex.data(),1,myMex.size(),stdout);
				printf("%s",Utility::AsciiData(myMex.data(),myMex.size()).c_str()  );
				printf("\n------------------\n");
				printf("\n%s\n",(char*)myMex.data());
				printf("\n------------------\n");
			    }
			    MoveToStatus(READ);
			    break;
		    }

	case ENDSPILL:
		    {
		    // send EE
			    printf("EE\n");
			    dataType myMex;
			    myMex.append((void*)"EE\0",3);
			    connectionManager_->Send(myMex,CmdSck);
			    MoveToStatus(READ);
			    break;
		    }

	case SENTBUFFER:
		    {//send End Of Run
			    printf("End Of Run\n");
			    dataType myMex;
			    myMex.append((void*)"ENDRUN\0",7);
			    connectionManager_->Send(myMex,CmdSck);
			    MoveToStatus(READ);
			    break;
		    }
	case READ:
		    {
			int cmd;
			printf("waiting for command:\n ");
			printf("\t0->WWE\n",WWE);
			printf("\t1->WE\n",WE);
			printf("\t2->EE\n",EE);
			printf("\t3->STATUS\n",STATUS);
			printf("\t4->ENDRUN\n",ENDRUN);
			printf("\t5->SPILLCOMPL\n",DIE);
			printf("\t6->DIE\n",DIE);
			printf("\n: ");
			scanf("%d",&cmd);
			switch ( cmd ){
				case  0: 	{MoveToStatus(BEGINSPILL); break;}
				case  1 : 	{MoveToStatus(CLEARED);break;}
				case  2:  	{MoveToStatus(ENDSPILL); break;}
				case  3: 	{MoveToStatus(WAITTRIG); break;}
				case  4: 	{MoveToStatus(SENTBUFFER); break;}
				case  5: 	{MoveToStatus(SPILLCOMPLETED);break;}
				case  6: 	{MoveToStatus(BYE);break;}
				default: break;
			}

		    break;
		    }
	case SPILLCOMPLETED: {
			    printf("SPILLCOMPL!\n");
			    dataType myMex;
			    myMex.append((void*)"SPILLCOMPL\0",11);
			    connectionManager_->Send(myMex,CmdSck);
			    MoveToStatus(READ);
			break;
			}
	case BYE:
		    {
			    printf("BYE BYE DAQ!\n");
			    dataType myMex;
			    myMex.append((void*)"DIE\0",4);
			    connectionManager_->Send(myMex,CmdSck);
			    MoveToStatus(START);
			    break; // or return ?
		    }

	} // end switch
	} //end try
	catch(sigint_exception &sigint) { printf("\n%s\n",sigint.what()); exit(0);return ; } // grace exit . return doesn't work. 
	catch(std::exception &e){ printf("--- EXCEPTION ---\n%s\n-------------\n",e.what()); throw e; }
}//end while
return;
}//end LOOP

// ------------------- EVENT BUILDER
EventBuilderFSM::EventBuilderFSM(): Daemon() {

}
bool EventBuilderFSM::IsOk(){
	if( eventBuilder_->GetRecvEvent() <= 0) {
		Log("[EventBuilderFSM]::[IsOk] Machine is not configured to receive events",1);
		return false;
		}
	return true;
}

void EventBuilderFSM::Loop(){

if ( !IsOk() ) throw config_exception();

while (true) {
	try{
	SendStatus();
		// check source that can populate the commands -- Connection and HW
		// check Connection Manager
		// if cmds not empty do something
	switch (myStatus_ ) {
	case START: {// not in the LOOP
			    MoveToStatus(INIT);
			    break;
		    } 
	case INIT:  {   // not in the LOOP
			    MoveToStatus(INITIALIZED);
			    break;
		    } 
	case INITIALIZED:
		    {
		    // wait for start run
		    dataType myMex;
		    if (connectionManager_->Recv(myMex) ==0 )
			    {
			    Command myCmd=ParseData(myMex);
			    if( myCmd.cmd ==  STARTRUN ) 
			   	 {
				   //hwManager_->BufferClearAll();
				   eventBuilder_->ResetSpillNumber();
				   WORD myRunNum=*(WORD*)myCmd.data;
				   ostringstream s;
				   s << "[EventBuilderFSM]::[INFO]::Starting a new run " <<  myRunNum;
				   Log(s.str(),1);
				   // init RunNum in eventBuilder
				   eventBuilder_->SetRunNum(myRunNum);
				   MoveToStatus(BEGINSPILL);
				 }
			    }
		    break;
		    }
	case BEGINSPILL:
		    {
		    // wait for wwe
		    dataType myMex;
		    if (connectionManager_->Recv(myMex) ==0 )
			    {
			    Command myCmd=ParseData(myMex);
			    if( myCmd.cmd ==  WWE ) 
			   	 {
					 //hwManager_->BufferClearAll();
					 //hwManager_->ClearBusy(); //just for "safety"
					 eventBuilder_->OpenSpill();
					 MoveToStatus(CLEARED);
				 }
			    }
		    break;
		    }
	case CLEARED:
		    {
		    // wait for we
		    dataType myMex;
		    if (connectionManager_->Recv(myMex) ==0 )
			    {
			    Command myCmd=ParseData(myMex);
			    if( myCmd.cmd ==  WE ) 
			   	 {
				   //					 hwManager_->BufferClearAll();
					 MoveToStatus(CLEARBUSY);
				 }
			    }
		    break;
		    }
	case CLEARBUSY: {
		        //hwManager_->ClearBusy();
			MoveToStatus(WAITTRIG);
			}
	case WAITTRIG:
		    {
		     // check network  -- wait for EE
		    dataType myMex;
		    if (connectionManager_->Recv(myMex) ==0 )    
		    {                                                                     
			    Command myNewCmd = ParseData( myMex); 
			    if (myNewCmd.cmd == EE )  {
				    MoveToStatus(ENDSPILL);
				    break;
			    }
		    }
		

		    break;
		    }
	case READ:
		    {
			// Do Nothing for the EventBuilder FSm
			MoveToStatus(CLEARBUSY);
		    break;
		    }
	case ENDSPILL:
		    {
			    // received EE NOT Open Close Spill
			MoveToStatus(RECVBUFFER);
		    break;
		    }
	case RECVBUFFER:
		    { // wait for ALL the BUFFERS
		    dataType myMex;
		    if (connectionManager_->Recv(myMex) ==0 )    
		    {                                                                     
			    Command myNewCmd = ParseData( myMex); 
			    if ( myNewCmd.cmd == DATA ) {
				    // pass the structure to a dataType
				    dataType myData(myNewCmd.N,myNewCmd.data);
				    // release the destruction from Command
				    myNewCmd.release();
				    //Merge Spills
				    eventBuilder_->MergeSpills(myData);

			    }
		    }
		    if ( eventBuilder_->AreSpillsMerged() ) 		   	 {
			// SENT STATUS BUFFER COMPLETED
			 dataType myMex;
			 myMex.append((void*)"STATUS SPILLCOMPL\0\0\0",18);
			 connectionManager_->Send(myMex,StatusSck);
			 myMex.clear();

			 //inform run controller that spill has been completed
			 myMex.append((void*)"EB_SPILLCOMPL\0\0\0",14);
			 connectionManager_->Send(myMex,CmdSck);
		    
			 MoveToStatus(SENTBUFFER);
		   	 }
		    break;
		    }
	case SENTBUFFER:
		    { // wait for SPILLCOMPLETED -- instructions from the Run Control
		    dataType myMex;
		    if (connectionManager_->Recv(myMex) ==0 )    
		    {                                                                     
			    Command myNewCmd = ParseData( myMex ); 
			    if (myNewCmd.cmd == SPILLCOMPL )  {
			            MoveToStatus(BEGINSPILL);
			    }
			    else if (myNewCmd.cmd == ENDRUN){
				    MoveToStatus(INITIALIZED);
			    }
			    else if (myNewCmd.cmd == DIE){
				    MoveToStatus(BYE);
			    }
		    }
		    break;
		    }
	case ERROR: {
		    ErrorStatus();
		    break;
		    }
	case BYE:
		    {
		    exit(0); // return is not working correctly
		    return;
		    }

	} // end switch
	} //end try
	catch(sigint_exception &sigint) { printf("\n%s\n",sigint.what()); exit(0);return ; } // grace exit . return doesn't work. 
	catch(std::exception &e){ printf("--- EXCEPTION ---\n%s\n-------------\n",e.what()); throw e; }

} // end while
} // end Loop



// ----------------------- RUN CONTROL FSM -----------
RunControlFSM::RunControlFSM(): Daemon() {

}

bool RunControlFSM::IsOk(){
	if ( !hwManager_->HaveControlBoard() ){
		Log("[RunControlFSM]::[IsOk] does not have a control board",1);
		return false;
		}
	return true;
}

void RunControlFSM::Loop(){
// not constructed in the constructor -- should take this from the configuration

if ( !IsOk() ) throw config_exception();

while (true) {
	try{
	SendStatus();
		// check source that can populate the commands -- Connection and HW
		// check Connection Manager
		// if cmds not empty do something
	switch (myStatus_ ) {
	case START: {// not in the LOOP
			    MoveToStatus(INIT);
			    break;
		    } 
	case INIT:  {   // not in the LOOP
			    MoveToStatus(INITIALIZED);
			    break;
		    } 
	case INITIALIZED:
		    {
		    // wait for gui start run
		    dataType myMex;
		    if (connectionManager_->Recv(myMex) ==0 )
			    {
			    Command myCmd=ParseData(myMex);
			    if( myCmd.cmd ==  GUI_STARTRUN ) 
			   	 {
#ifdef FSM_DEBUG
				   Log("[RunControlFSM]::[Loop]::[DEBUG] Is GUI START RUN 2",3);
#endif
				   hwManager_->BufferClearAll();
				   eventBuilder_->ResetSpillNumber();

				   // read runNum
				   WORD myRunNum;
				   sscanf( (char*)myCmd.data,"%u",&myRunNum);
				   // find out the type of RUN and the Trigger rate (if exists)
				   int shift=Utility::FindNull(myCmd.N,myCmd.data,1);
#ifdef FSM_DEBUG
				   ostringstream s2; s2<<"[RunControlFSM]::[Loop] Enter GUI_STARTRUN Routine. shift1="<<shift ;
				   Log(s2.str(),3);
#endif
				   if (shift<0) {
					   Log("[RunControlFSM]::[Loop] GUI command has wrong spelling. Ignored",1);
					   break;
					   }
				   char *ptr= (char*)myCmd.data + shift;
				   if (!strcmp(ptr,"PED")) // pedestal run
						   {
				   		   shift=Utility::FindNull(myCmd.N,myCmd.data,2);
						   if (shift <0 ) {
					   		Log("[RunControlFSM]::[Loop] GUI command has wrong spelling. Ignored.",1);
							break;
						  	}
				   		   char*ptr2= (char*)myCmd.data + shift;
						   sscanf(ptr2,"%ld",trgNevents_);
						   trgType_=PED_TRIG;
						   }
				   else if (!strcmp(ptr,"PHYSICS"))
						   {
						   trgType_=BEAM_TRIG;
						   }
				   else trgType_=UNK_TRIG; // LED_TRIG not impl

			    	   dataType myFufMex;
			    	   myFufMex.append((void*)"NOP\0",4);
			    	   connectionManager_->Send(myFufMex,CmdSck);
			    	   dataType myMex;
			    	   myMex.append((void*)"STARTRUN\0",9);
			    	   myMex.append((void*)&myRunNum,WORDSIZE);
			    	   connectionManager_->Send(myMex,CmdSck);

				   ostringstream s;
				   s << "[RunControlFSM]::[INFO]::Starting a new run " <<  myRunNum;
				   Log(s.str(),1);
				   // init RunNum in eventBuilder
				   eventBuilder_->SetRunNum(myRunNum);
				   MoveToStatus(BEGINSPILL);
				 }
			    }
		    break;
		    }
	case BEGINSPILL: 
		    {
		    // wait for wwe
		    dataType wweMex;
		    wweMex.append((void*)"WWE\0",4);
		    if (trgType_==PED_TRIG ) 
		    {
			    connectionManager_->Send(wweMex,CmdSck);
			    hwManager_->BufferClearAll();
			    hwManager_->ClearBusy(); //just for "safety"
			    eventBuilder_->OpenSpill();
			    MoveToStatus(CLEARED);
		    }
		    else if (trgType_==BEAM_TRIG)
		    {
		   	 // read the boards for WWE
			 if (
#define RC_DEBUG
#ifndef RC_DEBUG
					 hwManager_->SignalReceived(WWE)
#else
					 true
#endif
					 )
			 {
			   hwManager_->ClearSignalStatus();
			    connectionManager_->Send(wweMex,CmdSck);
			    hwManager_->BufferClearAll();
			    eventBuilder_->OpenSpill();
			    MoveToStatus(CLEARED);
			 }

		    }

		    break;
		    }
	case CLEARED:
		    {
		    // wait for we
		    dataType weMex;
		    weMex.append((void*)"WE\0",3);
		    if (trgType_==PED_TRIG ) 
		    {
		      connectionManager_->Send(weMex,CmdSck);
		      trgRead_=0;
		      //usleep(100000); //Wait acknowledge from DR
		      hwManager_->BufferClearAll();
		      hwManager_->ClearBusy();
		      readyDR_=0;
		      MoveToStatus(WAITFORREADY);
		    }
		    else if (trgType_==BEAM_TRIG)
		    {
		   	 // read the boards for WWE
			 if (hwManager_->SignalReceived(WE))
			 {
			   connectionManager_->Send(weMex,CmdSck);
			   //usleep(100000); //Wait acknowledge from DR
			   hwManager_->ClearSignalStatus(); //Acknowledge receive of WE
			   hwManager_->BufferClearAll();
		           hwManager_->ClearBusy();
			   readyDR_=0;
			   MoveToStatus(WAITFORREADY);
			 }

		    }
		    break;
		    }
	case WAITFORREADY:
		    {
		    dataType myMex;
		    //usleep(1000);
		    if (connectionManager_->Recv(myMex) ==0 )
			    {
			    Command myCmd=ParseData(myMex);
			    if( myCmd.cmd ==  DR_READY ) 
			   	 {
				 ++readyDR_;
				 }
			    }
		    if (readyDR_ >= waitForDR_)
		    {
		         hwManager_->SetTriggerStatus(trgType_,TRIG_ON );
		   	 MoveToStatus(WAITTRIG);
		    }
		    break;
		    }
	case CLEARBUSY: {
		        hwManager_->ClearBusy();
			MoveToStatus(WAITTRIG);
			}
	case WAITTRIG:
		    {
		    // check for END OF SPILL
		    dataType eeMex;
		    eeMex.append((void*)"EE\0",3);
		    // check end of spill conditions
		    if (trgType_== BEAM_TRIG ) 
		   	{
			if (hwManager_->SignalReceived(EE) )
				{
				  hwManager_->SetTriggerStatus(trgType_,TRIG_OFF );
				  //				  usleep(10000);
				  connectionManager_->Send(eeMex,CmdSck);
				  hwManager_->ClearSignalStatus();
				  hwManager_->ClearBusy();
				MoveToStatus(ENDSPILL);
				break;
				}
		    	}
		    else if (trgType_ == PED_TRIG ) 
		    	{
				if (trgRead_ >= trgNevents_)
				{
				  hwManager_->SetTriggerStatus(trgType_,TRIG_OFF );
				  //usleep(10000);
				  connectionManager_->Send(eeMex,CmdSck);
				  hwManager_->ClearBusy();
				MoveToStatus(ENDSPILL);
				break;
				}
		    	}
		     /// check trigger
		    if( hwManager_->TriggerReceived() ){ 
			cout<<"TRIGGER RECEIVED"<<endl;
			hwManager_->TriggerAck();
			MoveToStatus(READ);
                        }  

		    break;
		    }
	case READ:
		    {
			trgRead_++;
			static int READED=0;
			cout<<"Counter "<<std::dec<<++READED<<endl;
			static int Counter=0; 
			if (Counter==0 )  gettimeofday(&stopwatch_start_time,NULL);
			++Counter;
			if (Counter >= 500) {
				gettimeofday(&stopwatch_stop_time,NULL);
				long elapsed=  Utility::timevaldiff(&stopwatch_start_time,&stopwatch_stop_time);
				ostringstream s; s <<"[DataReadOutFSM]::[READ] "<<"Triggers in spill "<< READED<<" Rate "<< double(Counter)/elapsed* 1e6 <<" Hz";
				cout <<s.str()<<endl;
				Log(s.str(),1);
				Counter=0;
			}
			//----- Costruct Events
                        dataType event;
			eventBuilder_->OpenEvent(event,hwManager_->GetNboards());
			hwManager_->ReadAll(event); 
#ifdef SYNC_DEBUG
			int sleeptime=rand()%400 +100;
			usleep(sleeptime);
#endif
			eventBuilder_->CloseEvent(event);
			// ----- Add Event To Spill
                        eventBuilder_->AddEventToSpill(event);                                
			MoveToStatus(CLEARBUSY);
		    break;
		    }
	case ENDSPILL: 
		    {
			    // received EE
			Command myCmd=eventBuilder_->CloseSpill(); // eventBuilder know if the mex is to be sent on the network
			if (myCmd.cmd == SEND)
			{
				dataType myMex;
				myMex.append(myCmd.data,myCmd.N);
				// TODO check if myCmd slhould destruct data/N or not
				connectionManager_->Send(myMex,DataSck);
			}

			MoveToStatus(SENTBUFFER);
		    break;
		    }
	case SENTBUFFER:// TODO ---> I'm HERE,// Loop over the whole queue of messages
		    { // wait for EB_SPILLCOMPLETED
		    dataType myMex;
		    bool gui_pauserun 	= false;
		    bool gui_restartrun = false;
		    bool gui_stoprun 	= false;
		    bool gui_die 	= false;
		    bool eb_endspill 	= false;
		    while (connectionManager_->Recv(myMex) ==0 )     // while I have messages on the network buffer
		    {                                                                     
			    Command myNewCmd = ParseData( myMex); 
			    //todo_.push_back(myNewCmd);
			    if (myNewCmd.cmd == EB_SPILLCOMPLETED )  
				    eb_endspill=true;
			    else if (myNewCmd.cmd == GUI_STOPRUN)
				    gui_stoprun = true;
			    else if (myNewCmd.cmd == GUI_PAUSERUN)
				    gui_pauserun=true;
			    else if (myNewCmd.cmd == GUI_DIE)
				    gui_die=true;
			    else if (myNewCmd.cmd == GUI_RESTARTRUN)
				    gui_restartrun=true;
			    
		    }
		    // ORDER MATTERS!!! FIRST GUI, THEN EB_SPILL
		    if (gui_stoprun)
		   	 { 
				dataType myMex;
				myMex.append((void*)"ENDRUN\0",7);
				connectionManager_->Send(myMex,CmdSck);
				MoveToStatus(INITIALIZED);
		    	 }
		    else if (gui_pauserun)
				break;
		    else if( gui_restartrun ) 
		   	{
				dataType myMex;
				myMex.append((void*)"SPILLCOMPL\0",11);
				connectionManager_->Send(myMex,CmdSck);
			    	//SEND beginSPILL
				MoveToStatus(BEGINSPILL);
			}
		    else if( gui_die )
			    	//SEND DIE
			{
				dataType myMex;
				myMex.append((void*)"DIE\0",4);
				connectionManager_->Send(myMex,CmdSck);
			        MoveToStatus(BYE);
			}
		    else if ( eb_endspill )
			    // SEND BEGINSPILL
			    {
				dataType myMex;
				myMex.append((void*)"SPILLCOMPL\0",11);
				connectionManager_->Send(myMex,CmdSck);
				MoveToStatus(BEGINSPILL);
			    }
			    
		    break;
		    }
	case ERROR: {
		    ErrorStatus();
		    break;
		    }
	case BYE:
		    {
		    exit(0); // return is not working correctly
		    return;
		    }

	} // end switch 
	} // end try
	catch(sigint_exception &sigint) { printf("\n%s\n",sigint.what()); exit(0);return ; } // grace exit . return doesn't work. 
	catch(std::exception &e){ printf("--- EXCEPTION ---\n%s\n-------------\n",e.what()); throw e; }
} // while-true
return;
} // end Loop
