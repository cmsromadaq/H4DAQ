#include "interface/Daemon.hpp"
#include "interface/Utility.hpp"

//#define DAEMON_DEBUG

// --- Constructor 
Daemon::Daemon(){
	// get the pid of the current process
	pid_=getpid();
	//  Construct objects
	configurator_=new Configurator();
	eventBuilder_=new EventBuilder();
	hwManager_=new HwManager();
	connectionManager_=new ConnectionManager();
	myStatus_=START;
	myPausedFlag_=false;
	gettimeofday(&lastSentStatusMessageTime_,NULL);
	gettimeofday(&start_time,NULL);
	iLoop=0;
	waitForDR_=0;
	srand((unsigned)time(NULL));
}


int Daemon::Init(string configFileName){
	try{
		// Set Configurator ; and Init it
		configurator_->xmlFileName=configFileName;
		configurator_->Init();

		waitForDR_=Configurator::GetInt(Configurable::getElementContent(*configurator_,"waitForDR",configurator_->root_element) ); // move to Config
		ostringstream s; s<<"[Daemon]::[Init] Wait For DR "<< waitForDR_;
		Log(s.str(),1);
		printf("%s\n",s.str().c_str());

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


Command Daemon::ParseData(dataType &mex)
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
		//mex.erase(dataTypeSize_t(0),dataTypeSize_t(5));
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
	//	else if (N >=7  and !strcmp( (char*) mex.data(), "STATUS")  ) // move in the not null terminated part TODO
	//		{
	//		mex.erase(0,7);
	//		myCmd.cmd=STATUS;
	//		myCmd.data = mex.data();
	//		myCmd.N    = mex.size();
	//		mex.release();
	//		}
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
	else if (N >=14  and !strcmp( (char*) mex.data(), "EB_SPILLCOMPL")  )
		myCmd.cmd=EB_SPILLCOMPLETED;
	else if (N >=9  and !strcmp( (char*) mex.data(), "DR_READY")  )
		myCmd.cmd=DR_READY;
	else if (N >=7  and !strcmp( (char*) mex.data(), "ENDRUN")  )
		myCmd.cmd=ENDRUN;
	else if (N >=4  and !strcmp( (char*) mex.data(), "DIE")  )
		myCmd.cmd=DIE;
	// GUI CMD are not NULL Terminated
	else 	{ // GUI --- I'M changing the mex

		dataType mex2;
		mex2.copy(mex.data(),mex.size()) ;
		mex2.append((void*)"\0",1);
		Utility::SpaceToNull(mex2.size(),mex2.data(),false); // true=only the first
#ifdef DAEMON_DEBUG
		ostringstream s; s<<"[Daemon]::[ParseCommand]::[DEBUG] GUI Mex: '"<< (char*)mex2.data()<<"'";
		Log(s.str(),3);
#endif
		ostringstream s; s<<"[Daemon]::[ParseCommand]::[DEBUG] GUI Mex: '"<< (char*)mex2.data()<<"'";
		Log(s.str(),3);

		if (N >=4  and !strcmp( (char*) mex2.data(), "NOP")  )
			myCmd.cmd=NOP;
		else if (N >=4  and !strcmp( (char*) mex2.data(), "WWE")  )
			myCmd.cmd=WWE;
		else if (N >=3  and !strcmp( (char*) mex2.data(), "WE")  )
			myCmd.cmd=WE;
		else if (N >=3  and !strcmp( (char*) mex2.data(), "EE")  )
			myCmd.cmd=EE;
		else if (N >=9  and !strcmp( (char*) mex2.data(), "STARTRUN")  )
			{
			mex2.erase(0,9);
			myCmd.cmd=STARTRUN;
			myCmd.data = mex.data();
			myCmd.N    = mex.size();
			mex2.release();
			}
		else if (N >=11  and !strcmp( (char*) mex2.data(), "SPILLCOMPL")  )
			myCmd.cmd=SPILLCOMPL;
		else if (N >=14  and !strcmp( (char*) mex2.data(), "EB_SPILLCOMPL")  )
			myCmd.cmd=EB_SPILLCOMPLETED;
		else if (N >=10  and !strcmp( (char*) mex2.data(), "DR_READY")  )
			myCmd.cmd=DR_READY;
		else if (N >=7  and !strcmp( (char*) mex2.data(), "ENDRUN")  )
			myCmd.cmd=ENDRUN;
		else if (N >=4  and !strcmp( (char*) mex2.data(), "DIE")  )
			myCmd.cmd=DIE;
		// GUI CMD are not NULL Terminated
		else if (N >=12  and !strcmp( (char*) mex2.data(), "GUI_STARTRUN")  )
		   {
		   mex2.erase(0,13);
		   myCmd.cmd=GUI_STARTRUN;
		   //Utility::SpaceToNull(mex2.size(),mex2.data() ) ;
		   myCmd.data = mex2.data();
		   myCmd.N    = mex2.size();
		   mex2.release();
		   }
		else if (N >=12  and !strcmp( (char*) mex2.data(), "GUI_PAUSERUN")  )
		   {
		   myCmd.cmd=GUI_PAUSERUN;
		   }
		else if (N >=11  and !strcmp( (char*) mex2.data(), "GUI_STOPRUN")  )
		   {
		   myCmd.cmd=GUI_STOPRUN;
		   }
		else if (N >=14  and !strcmp( (char*) mex2.data(), "GUI_RESTARTRUN")  )
		   {
		   myCmd.cmd=GUI_RESTARTRUN;
		   }
		else if (N >=7  and !strcmp( (char*) mex2.data(), "GUI_DIE")  )
		   {
		   myCmd.cmd=GUI_DIE;
		   }
		if (myCmd.data != NULL)mex2.release();
		} // ENDGUI

	if(myCmd.data !=NULL ) mex.release();
	return myCmd;
}



void Daemon::MoveToStatus(STATUS_t newStatus){
	// -- dataType myMex;
	// -- myMex.append((void*)"STATUS ",7);
	// -- WORD myStatus=(WORD)newStatus;
	// -- myMex.append((void*)&myStatus,WORDSIZE);
	// -- connectionManager_->Send(myMex,StatusSck);
	ostringstream s;
	s << "[Daemon]::[DEBUG]::Moving to status " << newStatus;
	Log(s.str(),3);
	std::cout << s.str() << std::endl;
	STATUS_t oldStatus = myStatus_;
	myStatus_=newStatus;
	myPausedFlag_=false;
	if (!((oldStatus==CLEARBUSY && newStatus==WAITTRIG) ||	\
	      (oldStatus==WAITTRIG && newStatus==READ) ||	\
	      (oldStatus==READ && newStatus==CLEARBUSY) )) {
	  SendStatus(oldStatus,newStatus); //Send status to GUI (formatted correctly)
	  }
}

void Daemon::SendStatus(STATUS_t oldStatus, STATUS_t newStatus){
	dataType myMex;
	myMex.append((void*)"STATUS ",7);
	char mybuffer[255];
	int n=0;
	n = snprintf(mybuffer,255,"%u ",newStatus);
	myMex.append((void*)mybuffer,n);
	WORD runnr = 0;
	WORD spillnr = 0;
	WORD evinspill = 0;
	if (eventBuilder_){
	  runnr = eventBuilder_->GetEventId().runNum_;
	  spillnr = eventBuilder_->GetEventId().spillNum_;
	  evinspill = eventBuilder_->GetEventId().eventInSpill_;
	}
	n = snprintf(mybuffer,255,"%u ",runnr); //runnr
	myMex.append((void*)mybuffer,n);
	n = snprintf(mybuffer,255,"%u ",spillnr); //spillnr
	myMex.append((void*)mybuffer,n);
	n = snprintf(mybuffer,255,"%u ",evinspill); //evinspill
	myMex.append((void*)mybuffer,n);
	if (myPausedFlag_) myMex.append((void*)"PAUSED",6);
	connectionManager_->Send(myMex,StatusSck);
	gettimeofday(&lastSentStatusMessageTime_,NULL);
}

void Daemon::PublishStatusWithTimeInterval(){
  timeval tv;
  gettimeofday(&tv,NULL);
  long timediff = Utility::timevaldiff(&lastSentStatusMessageTime_,&tv); // in usec
  if (timediff>200000) SendStatus(myStatus_,myStatus_);
}

bool Daemon::IsOk(){return true;}
void Daemon::LogInit(Logger*l){
				LogUtility::LogInit(l);
				eventBuilder_->LogInit(l);
				hwManager_->LogInit(l);
				connectionManager_->LogInit(l);
				//configurator_->LogInit(l);
				};

int Daemon::Daetach(){

	pid_t pid=fork();
	if (pid >0 ){ // parent
		printf("[EventBuilderDaemon] Detaching process %d\n",pid);
		exit(0); //
		} 
	else if (pid== 0 ) { // child
		setsid(); // obtain a new group process
		// close all descriptors
		fflush(NULL);
		int i;
		for (i=getdtablesize();i>=0;--i) close(i); /* close all descriptors */
		i=open("/dev/null",O_RDWR); /* open stdin */
		dup(i); /* stdout */
		dup(i); /* stderr */
		}
	else {
		printf("[EventBuilderDaemon] Cannot Daemonize");
		return 1;
		}

	return 0;
}
