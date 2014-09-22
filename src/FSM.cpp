#include "interface/Daemon.hpp"
#include "interface/FSM.hpp"



void DataReadoutFSM::Loop(){

hwManager_->SetRunControl(false);

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
			    while (connectionManager_->Recv(myMex) !=0 )
			    {
			    Command myCmd=ParseData(myMex);
			    if( myCmd.cmd ==  STARTRUN ) 
			   	 {
					 hwManager_->BufferClearAll();
					 eventBuilder_->ResetSpillNumber();
					 WORD myRunNum=*(WORD*)myCmd.data;
					 // init RunNum in eventBuilder
					 eventBuilder_->SetRunNum(myRunNum);
					 MoveToStatus(BEGINSPILL);
					 break; //while
				 }
			    usleep(10); // nothing on the net
			    }
		    break;
		    }
	case BEGINSPILL:
		    {
		    // wait for wwe
		    dataType myMex;
			    while (connectionManager_->Recv(myMex) !=0 )
			    {
			    Command myCmd=ParseData(myMex);
			    if( myCmd.cmd ==  WWE ) 
			   	 {
					 hwManager_->BufferClearAll();
					 eventBuilder_->OpenSpill();
					 MoveToStatus(CLEARED);
					 break; //while
				 }
			    usleep(10); // nothing on the net
			    }
		    break;
		    }
	case CLEARED:
		    {
		    // wait for we
		    dataType myMex;
			    while (connectionManager_->Recv(myMex) !=0 )
			    {
			    Command myCmd=ParseData(myMex);
			    if( myCmd.cmd ==  WE ) 
			   	 {
					 hwManager_->BufferClearAll();
					 MoveToStatus(WAITTRIG);
					 break; //while
				 }
			    usleep(2); // nothing on the net
			    }
		    break;
		    }
	case WAITTRIG:
		    {
		     /// check trigger
		    if( hwManager_->TriggerReceived() ){ 
			MoveToStatus(READ);
                        }  
		     // check network
		    dataType myMex;
		    if (connectionManager_->Recv(myMex) ==0 )    
		    {                                                                     
			    Command myNewCmd = ParseData( myMex); 
			    if (myNewCmd.cmd == EE )  {
				    MoveToStatus(ENDSPILL);
			    }
		    }

		    break;
		    }
	case READ:
		    {
                        dataType event=hwManager_->ReadAll();                                 
                        eventBuilder_->AddEventToSpill(event);                                
                        hwManager_->BufferClearAll();                                         
			MoveToStatus(WAITTRIG);
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
void RunControllerFSM::Loop(){
hwManager_->SetRunControl(true);
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
			    dataType myMex;
			    printf("myMex.size()=%d\n",myMex.size() );
			    myMex.append((void*)"STARTRUN\0",9);
			    printf("myMex.size()=%d\n",myMex.size() );
			    myMex.append((void*)&runNum,WORDSIZE);
			    printf("myMex.size()=%d\n",myMex.size() );
			    printf("Sending Mex\n"); //DEBUG
			    //connectionManager_->Send(myMex);
			    printf("MexSent\n"); // DEBUG
			    MoveToStatus(READ);
			    printf("myMex.size()=%d\n",myMex.size() );
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
				fwrite(myMex.data(),1,myMex.size(),stdout);
				printf("------------------\n");
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
			printf("\t5->DIE\n",DIE);
			printf("\n: ");
			scanf("%d",&cmd);
			switch ( cmd ){
				case  0: 	{MoveToStatus(BEGINSPILL); break;}
				case  1 : 	{MoveToStatus(CLEARED);break;}
				case  2:  	{MoveToStatus(ENDSPILL); break;}
				case  3: 	{MoveToStatus(WAITTRIG); break;}
				case  4: 	{MoveToStatus(SENTBUFFER); break;}
				case  5: 	{MoveToStatus(BYE);break;}
				default: break;
			}

		    break;
		    }
	case BYE:
		    {
			    printf("BYE BYE DAQ!\n");
			    dataType myMex;
			    myMex.append((void*)"DIE\0",4);
			    connectionManager_->Send(myMex);
			    MoveToStatus(READ);
			    break; // or return ?
		    }

	} // end switch
	} //end try
	catch(std::exception &e){ printf("--- EXCEPTION ---\n%s\n-------------\n",e.what()); throw e; }
}//end while
return;
}//end LOOP
