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
	gettimeofday(&start_time,NULL);
	iLoop=0;
}


int Daemon::Init(string configFileName){
	try{
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
		if (N >=12  and !strcmp( (char*) mex2.data(), "GUI_STARTRUN")  )
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
		else if (N >=8  and !strcmp( (char*) mex2.data(), "GUI_DIE")  )
		   {
		   myCmd.cmd=GUI_DIE;
		   }
		else if (N >=7  and !strcmp( (char*) mex2.data(), "STATUS")  ) // here because this is spaced and not null/null terminated
			{
			mex2.erase(0,7);
			myCmd.cmd=STATUS;
			myCmd.data = mex2.data();
			myCmd.N    = mex2.size();
			mex2.release();
			}
		if (myCmd.data != NULL)mex2.release();
		} // ENDGUI

	if(myCmd.data !=NULL ) mex.release();
	return myCmd;
}



void Daemon::MoveToStatus(STATUS_t newStatus){
	dataType myMex;
	myMex.append((void*)"STATUS ",7);
	WORD myStatus=(WORD)newStatus;
	myMex.append((void*)&myStatus,WORDSIZE);
	connectionManager_->Send(myMex,StatusSck);
	ostringstream s;
	s << "[Daemon]::[DEBUG]::Moving to status " << newStatus;
	Log(s.str(),3);
	std::cout << s.str() << std::endl;
	myStatus_=newStatus;
}

void Daemon::SendStatus(){
	if (iLoop > 10000) {
	iLoop=0;
	dataType myMex;
	myMex.append((void*)"STATUS ",7);
	char mybuffer[255];
	int n = snprintf(mybuffer,255,"%u",myStatus_);
	myMex.append((void*)mybuffer,n);
	connectionManager_->Send(myMex,StatusSck);
	}
	++iLoop;
	return;
}

bool Daemon::IsOk(){return true;}
void Daemon::LogInit(Logger*l){
				LogUtility::LogInit(l);
				eventBuilder_->LogInit(l);
				hwManager_->LogInit(l);
				connectionManager_->LogInit(l);
				//configurator_->LogInit(l);
				};
