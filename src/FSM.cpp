#include "interface/Daemon.hpp"
#include "interface/FSM.hpp"
#include "interface/Utility.hpp"

// --- Constructor: C++11 inherits automatically. C++03 no
DataReadoutFSM::DataReadoutFSM(): Daemon() {

}

bool DataReadoutFSM::IsOk(){
	if ( eventBuilder_->GetSendEvent() != true ) return false;
	return true;
}

void DataReadoutFSM::Loop(){
// not constructed in the constructor -- should take this from the configuration

if ( !IsOk() ) throw config_exception();

while (true) {
	try{
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
					 hwManager_->BufferClearAll();
					 MoveToStatus(CLEARBUSY);
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
			//-----
                        dataType event;
			hwManager_->ReadAll(event);                                 /// DEBUG ME
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
				connectionManager_->Send(myMex);
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
	case BYE:
		    {
		    return;
		    }

	} // end switch 
	} // end try
	catch(std::exception &e){ printf("--- EXCEPTION ---\n%s\n-------------\n",e.what()); throw e; }
} // while-true
return;
} // end Loop

// ------------------------- RUN CONTROLLER
// --- Constructor
RunControllerFSM::RunControllerFSM() : Daemon() {};

bool RunControllerFSM::IsOk(){
	//if( hwManager_->GetRunControl() != true) return false;
	return true;
}

void RunControllerFSM::Loop(){

if ( !IsOk() ) throw config_exception();

while (true) {
	try{
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
			    connectionManager_->Send(myFufMex);
			    dataType myMex;
			    myMex.append((void*)"STARTRUN\0",9);
			    myMex.append((void*)&runNum,WORDSIZE);
			    printf("RUN MEX: %d\n",myMex.size());
				printf("------------------\n");
				//fwrite(myMex.data(),1,myMex.size(),stdout);
				printf("%s",Utility::AsciiData(myMex.data(),myMex.size()).c_str() );
				printf("------------------\n");
			    connectionManager_->Send(myMex);
			    MoveToStatus(READ);
			    break;
		    }
	case BEGINSPILL:
		    {
		    // send WWE
			    printf("WWE\n");
			    dataType myMex;
			    myMex.append((void*)"WWE\0",4);
			    connectionManager_->Send(myMex);
			    MoveToStatus(READ);
			    break;

		    }
	case CLEARED:
		    {
		    // send WE
			    printf("WE\n");
			    dataType myMex;
			    myMex.append((void*)"WE\0",3);
			    connectionManager_->Send(myMex);
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
			    connectionManager_->Send(myMex);
			    MoveToStatus(READ);
			    break;
		    }

	case SENTBUFFER:
		    {//send End Of Run
			    printf("End Of Run\n");
			    dataType myMex;
			    myMex.append((void*)"ENDRUN\0",7);
			    connectionManager_->Send(myMex);
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
			    connectionManager_->Send(myMex);
			    MoveToStatus(READ);
			break;
			}
	case BYE:
		    {
			    printf("BYE BYE DAQ!\n");
			    dataType myMex;
			    myMex.append((void*)"DIE\0",4);
			    connectionManager_->Send(myMex);
			    MoveToStatus(START);
			    break; // or return ?
		    }

	} // end switch
	} //end try
	catch(std::exception &e){ printf("--- EXCEPTION ---\n%s\n-------------\n",e.what()); throw e; }
}//end while
return;
}//end LOOP

// ------------------- EVENT BUILDER
bool EventBuilderFSM::IsOk(){
	if( eventBuilder_->GetRecvEvent() != true) return false;
	return true;
}

void EventBuilderFSM::Loop(){

if ( !IsOk() ) throw config_exception();
}
