#include "interface/Daemon.hpp"
#include "interface/FSM.hpp"
#include "interface/Utility.hpp"

//#define FSM_DEBUG
//#define SYNC_DEBUG

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
	  PublishStatusWithTimeInterval();
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
		        	   hwManager_->SetBusyOff();
				   hwManager_->ClearBusy(); //just for "safety"
			           hwManager_->TriggerAck();
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
		        hwManager_->SetBusyOff();
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
		        	    hwManager_->SetBusyOff();
			            hwManager_->ClearBusy();
				    hwManager_->TriggerAck();
				    MoveToStatus(ENDSPILL);
				    break;
			    }
		    }
		     /// check trigger
		    if( hwManager_->TriggerReceived() ){ 
			cout<<"TRIGGER RECEIVED"<<endl;
			hwManager_->SetBusyOn();
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
			int sleeptime=rand()%5000 +1000;
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
		    hwManager_->Close();  //Clean Exit
		    exit(0); // return is not working correctly
		    return;
		    }

	} // end switch 
	} // end try
	catch(sigint_exception &sigint) { printf("\n%s\n",sigint.what());  hwManager_->Close(); exit(0);return ; } // grace exit . return doesn't work. 
	catch(std::exception &e){ printf("--- EXCEPTION ---\n%s\n-------------\n",e.what()); MoveToStatus(ERROR); }
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
	  PublishStatusWithTimeInterval();
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
	catch(std::exception &e){ printf("--- EXCEPTION ---\n%s\n-------------\n",e.what()); MoveToStatus(ERROR); }
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
	  PublishStatusWithTimeInterval();
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
					 if (! eventBuilder_->AreSpillsMerged() ) Log("[EventBuilder]::[FSM] ERROR, Unmerged spill in new run",3); // TODO Exception
					 eventBuilder_->ResetLastBadSpill();
					 eventBuilder_->ResetMerged();
					 MoveToStatus(CLEARED);
					 // Reset Bad Spill
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
			eventStarted=false;
			gettimeofday(&transrate_stopwatch_start,NULL);
			transrate_size=0;
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
				    eventStarted= true;
				    // 
				    //DEBUG
				    //   string buf=Utility::AsciiDataReadable( myData.data(), myData.size());
				    //   if (buf.size() >100)
				    //       buf.erase(101,string::npos);
				    //   Log(buf,3);
				    transrate_size+=myData.size();
				    eventBuilder_->MergeSpills(myData);

			    }
		    }
		    if ( eventBuilder_->AreSpillsMerged() && eventStarted) { // enter here only if you merged something
			// SENT STATUS BUFFER COMPLETED
			 dataType myMex;
			 myMex.append((void*)"STATUS SPILLCOMPL\0\0\0",18);
			 connectionManager_->Send(myMex,StatusSck);
			 myMex.clear();

		   	// Transfer
			 gettimeofday(&transrate_stopwatch_stop,NULL);
			 ostringstream rate;
			 long transferTime=Utility::timevaldiff(&transrate_stopwatch_start,&transrate_stopwatch_stop);
			 rate<<"[FSM]::[SENTBUFFER]::[TransferTime]="<<
				 transferTime<<"usec "<<
				 "Size="<<transrate_size<<"bytes "<<
				 "Rate="<< (transrate_size>>20)/(double(transferTime)/1.e6) <<"MB/s" ;
			 ReportTransferPerformance(transferTime,transrate_size);
			 Log(rate.str(),1);

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
	catch(std::exception &e){ printf("--- EXCEPTION ---\n%s\n-------------\n",e.what()); MoveToStatus(ERROR); }

} // end while
} // end Loop

void EventBuilderFSM::ReportTransferPerformance(long transferTime, dataTypeSize_t transrate_size){
  dataType myMex;
  myMex.append((void*)"TRANSFER ",9);
  char mybuffer[255];
  int n=0;
  WORD runnr=0;
  WORD spillnr=0;
  WORD goodevinrun=0;
  WORD badspills=0;
  if (eventBuilder_){
    runnr = eventBuilder_->GetEventId().runNum_;
    spillnr = eventBuilder_->GetEventId().spillNum_;
    goodevinrun = eventBuilder_->GetGoodEvents();
    badspills = eventBuilder_->GetBadSpills();
  }
  myMex.append((void*)"runnumber=",10);
  n = snprintf(mybuffer,255,"%u ",runnr); //runnr
  myMex.append((void*)mybuffer,n);
  myMex.append((void*)"spillnumber=",12);
  n = snprintf(mybuffer,255,"%u ",spillnr); //spillnr
  myMex.append((void*)mybuffer,n);
  myMex.append((void*)"evinrun=",8);
  n = snprintf(mybuffer,255,"%u ",goodevinrun); //evinrun
  myMex.append((void*)mybuffer,n);
  myMex.append((void*)"badspills=",10);
  n = snprintf(mybuffer,255,"%u ",badspills); //badspills
  myMex.append((void*)mybuffer,n);
  myMex.append((void*)"transferTime=",13);
  n = snprintf(mybuffer,255,"%li ",transferTime);
  myMex.append((void*)mybuffer,n);
  myMex.append((void*)"transrate_size=",15);
  n = snprintf(mybuffer,255,"%lli",transrate_size);
  myMex.append((void*)mybuffer,n);
  connectionManager_->Send(myMex,StatusSck);
}

// ----------------------- RUN CONTROL FSM -----------
RunControlFSM::RunControlFSM(): Daemon() {
  gettimeofday(&spillduration_stopwatch_start_time,NULL);
  gettimeofday(&spillduration_stopwatch_stop_time,NULL);
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
	  PublishStatusWithTimeInterval();
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
				   ostringstream s2; s2<<"[RunControlFSM]::[Loop]::[DEBUG] Enter GUI_STARTRUN Routine. shift1="<<shift ;
				   Log(s2.str(),3);
#endif
				   if (shift<0) {
					   Log("[RunControlFSM]::[Loop] GUI command has wrong spelling. Ignored",1);
					   break;
					   }
				   char *ptr= (char*)myCmd.data + shift;
				   if (!strcmp(ptr,"PED")) // pedestal run
						   {
#ifdef FSM_DEBUG
				   {
				   ostringstream s2; s2<<"[RunControlFSM]::[Loop]::[DEBUG] Enter PED Trigger"<<shift ;
				   Log(s2.str(),3);
				   }
#endif
				   		   shift=Utility::FindNull(myCmd.N,myCmd.data,2);
						   if (shift <0 ) {
					   		Log("[RunControlFSM]::[Loop] GUI command has wrong spelling. Ignored.",1);
							break;
						  	}
				   		   char*ptr2= (char*)myCmd.data + shift;
#ifdef FSM_DEBUG
				   {
				   ostringstream s2; s2<<"[RunControlFSM]::[Loop]::[DEBUG] Starting Scanf nEvents | "<<shift ;
				   Log(s2.str(),3);
				   s2.str() = ""; s2 <<"[RunControlFSM]::[Loop]::[DEBUG] myCmd.N="<<myCmd.N<<"| shift1="<<Utility::FindNull(myCmd.N,myCmd.data,1)
							<<" | shift2="<<Utility::FindNull(myCmd.N,myCmd.data,2)
							<<" | shift3="<<Utility::FindNull(myCmd.N,myCmd.data,3);
				   Log(s2.str(),3);
					
				   }
#endif
						   if ( sscanf(ptr2,"%ld",&trgNevents_) < 1) {
					   		Log("[RunControlFSM]::[Loop] GUI command has wrong spelling. Ignored.",1);
							break;
						  	}
#ifdef FSM_DEBUG
				   {
				   ostringstream s2; s2<<"[RunControlFSM]::[Loop]::[DEBUG] End Scanf trigger set"<<shift ;
				   Log(s2.str(),3);
				   }
#endif
						   trgType_=PED_TRIG;
						   }
				   else if (!strcmp(ptr,"LED")) // pedestal run
						   {
#ifdef FSM_DEBUG
				   {
				   ostringstream s2; s2<<"[RunControlFSM]::[Loop]::[DEBUG] Enter LED Trigger"<<shift ;
				   Log(s2.str(),3);
				   }
#endif
				   		   shift=Utility::FindNull(myCmd.N,myCmd.data,2);
						   if (shift <0 ) {
					   		Log("[RunControlFSM]::[Loop] GUI command has wrong spelling. Ignored.",1);
							break;
						  	}
				   		   char*ptr2= (char*)myCmd.data + shift;
						   if ( sscanf(ptr2,"%ld",&trgNevents_) <1 ){
					   		Log("[RunControlFSM]::[Loop] GUI command has wrong spelling. Ignored.",1);
							break;
						  	}
						   trgType_=LED_TRIG;
						   }
				   else if (!strcmp(ptr,"PHYSICS"))
						   {
						   trgType_=BEAM_TRIG;
						   }
				   else {
					trgType_=UNK_TRIG; // LED_TRIG not impl
					Log("[RunControlFSM]::[Loop] GUI command has wrong spelling. Ignored.",1);
					break;
					}

#ifdef FSM_DEBUG
				   {
				   ostringstream s2; s2<<"[RunControlFSM]::[Loop]::[DEBUG] Going Send STARTRUN MEX"<<shift ;
				   Log(s2.str(),3);
				   }
#endif
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
		    if (trgType_==PED_TRIG || trgType_==LED_TRIG ) 
		    {
			    connectionManager_->Send(wweMex,CmdSck);
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
			    hwManager_->ClearSignalStatus(); // acknowledge receival of status
			    connectionManager_->Send(wweMex,CmdSck);
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
		    if (trgType_==PED_TRIG || trgType_==LED_TRIG ) 
		    {
		      connectionManager_->Send(weMex,CmdSck);
		      trgRead_=0;
		      //usleep(100000); //Wait acknowledge from DR
		      hwManager_->BufferClearAll();
		      hwManager_->SetBusyOff();
		      hwManager_->ClearBusy();
		      hwManager_->TriggerAck();
		      readyDR_=0;
		      MoveToStatus(WAITFORREADY);
		    }
		    else if (trgType_==BEAM_TRIG)
		    {

#ifdef RC_DEBUG
		      usleep(1000);
		      ostringstream s; s<< "Waiting for WE" << hwManager_->SignalReceived(WE);
		      Log(s.str(),3);
#endif

		   	 // read the boards for WE
			 if (hwManager_->SignalReceived(WE))
			 {
			   connectionManager_->Send(weMex,CmdSck);
			   //usleep(100000); //Wait acknowledge from DR
			   hwManager_->ClearSignalStatus(); //Acknowledge receive of WE
			   hwManager_->BufferClearAll();
		           hwManager_->SetBusyOff();
		           hwManager_->ClearBusy();
			   hwManager_->TriggerAck();
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
		      
		      	 gettimeofday(&spillduration_stopwatch_start_time,NULL);
		         hwManager_->SetTriggerStatus(trgType_,TRIG_ON );
		   	 MoveToStatus(WAITTRIG);
		    }
		    break;
		    }
	case CLEARBUSY: {
		        hwManager_->SetBusyOff();
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
				  hwManager_->SetBusyOff();
				  hwManager_->ClearBusy();
			          hwManager_->TriggerAck();
				  gettimeofday(&spillduration_stopwatch_stop_time,NULL);
				  SendSpillDuration();
				MoveToStatus(ENDSPILL);
				break;
				}
		    	}
		    else if (trgType_ == PED_TRIG || trgType_==LED_TRIG) 
		    	{
				if (trgRead_ >= trgNevents_)
				{
				  hwManager_->SetTriggerStatus(trgType_,TRIG_OFF );
				  //usleep(10000);
				  connectionManager_->Send(eeMex,CmdSck);
				  hwManager_->SetBusyOff();
				  hwManager_->ClearBusy();
			          hwManager_->TriggerAck();
				  gettimeofday(&spillduration_stopwatch_stop_time,NULL);
				  SendSpillDuration();
				MoveToStatus(ENDSPILL);
				break;
				}
		    	}
		     /// check trigger
		    if( hwManager_->TriggerReceived() ){ 
			cout<<"TRIGGER RECEIVED"<<endl;
			hwManager_->SetBusyOn();
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
			int sleeptime=rand()%5000 +1000;
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
			ResetMex();
			MoveToStatus(RECVBUFFER);
		    break;
		    }
	case RECVBUFFER:{
			
			// this will pause the rc here, while the eb is receiving data, and will set the correct action (also gui) in the next step (SENTBUFFER)
		    	UpdateMex();
		    	if ( eb_endspill ) MoveToStatus( SENTBUFFER );
			break;
			}
	case SENTBUFFER:// Loop over the whole queue of messages
		    { // wait for EB_SPILLCOMPLETED
		    UpdateMex();
		    // ORDER MATTERS!!! FIRST GUI, THEN EB_SPILL
		    if (gui_stoprun)
		   	 { 
				dataType myMex;
				myMex.append((void*)"ENDRUN\0",7);
				connectionManager_->Send(myMex,CmdSck);
				MoveToStatus(INITIALIZED);
		    	 }
		    else if( gui_restartrun ) 
		   	{
				dataType myMex;
				gui_pauserun=false;
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
		    else if (gui_pauserun)
		        {
				//gui_pauserun=false;
				if (! myPausedFlag_)SendStatus(myStatus_,myStatus_); // just for sending the paused information to the GUI --
			        myPausedFlag_=true;
			        break;
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
		    hwManager_->Close();  //Clean Exit
		    exit(0); // return is not working correctly
		    return;
		    }

	} // end switch 
	} // end try
	catch(sigint_exception &sigint) { printf("\n%s\n",sigint.what());  hwManager_->Close(); exit(0);return ; } // grace exit . return doesn't work. 
	catch(std::exception &e){ printf("--- EXCEPTION ---\n%s\n-------------\n",e.what()); MoveToStatus(ERROR); }
} // while-true
return;
} // end Loop


void RunControlFSM::UpdateMex(){
		    dataType myMex;
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
	return;
}

void RunControlFSM::SendSpillDuration(){
  long spilltime = Utility::timevaldiff(&spillduration_stopwatch_start_time,&spillduration_stopwatch_stop_time);
  dataType myMex;
  myMex.append((void*)"SPILLDURATION ",14);
  char mybuffer[255];
  int n=0;
  WORD runnr=0;
  WORD spillnr=0;
  if (eventBuilder_){
    runnr = eventBuilder_->GetEventId().runNum_;
    spillnr = eventBuilder_->GetEventId().spillNum_;
  }
  myMex.append((void*)"runnumber=",10);
  n = snprintf(mybuffer,255,"%u ",runnr); //runnr
  myMex.append((void*)mybuffer,n);
  myMex.append((void*)"spillnumber=",12);
  n = snprintf(mybuffer,255,"%u ",spillnr); //spillnr
  myMex.append((void*)mybuffer,n);
  myMex.append((void*)"spillduration=",14);
  n = snprintf(mybuffer,255,"%li",spilltime);
  myMex.append((void*)mybuffer,n);
  connectionManager_->Send(myMex,StatusSck);
}

void RunControlFSM::ErrorStatus(){
	Daemon::ErrorStatus();
	sleep(5); // wait for all machines to have completed the error cycle
	//
	//end the run
	dataType  endRun; endRun.append( (void*)"ENDRUN\0\0\0",7);
	connectionManager_->Send(endRun,CmdSck);
	// Go into wait for run num.
	error_=false;
	//wait a bit ~1s,
	sleep(1);
	//reset all the mex
	dataType myMex;
	while (connectionManager_->Recv(myMex) == 0 ); // nothing, queue of mex will be empty for the RC
	// repeat ER -- safety
	connectionManager_->Send(endRun,CmdSck);

	MoveToStatus(INITIALIZED);
}
