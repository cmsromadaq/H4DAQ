#include "interface/EventBuilder.hpp"

// --- Implemantation of class dataType: a stream of data of size N
const bool operator==(dataType &x, dataType &y)
	{
	if (x.size() != y.size() ) return false;
	for(int i=0;i<x.size();i++)
		{
		if ( ((char*)x.data())[i] != ((char*)y.data())[i] ) return false;
		}
	return true;
	}

	
	// --- Constructor
dataType::dataType(){
	clear();
}

dataType::dataType(int N,void*v){
	// this constructor will not do malloc and take ownership of the stream
	dataStream_=v;
	size_=N;
	reserved_=N;
}

	// --- Destructor
dataType::~dataType(){clear();}
	// --- reserve N bytes in memory
void dataType::reserve(int N)
	{
	if (reserved_ >= N) return; // nothing to be done
	// allocate new space of size N
	void *newStream=new char[N];
	if(newStream == NULL ) throw std::bad_alloc(); //mem fail
	//copy content to new residence
	memcpy(newStream,dataStream_,size_);
	// release old memory
	delete [] (char*)dataStream_;
	//update pointers
	dataStream_=newStream;
	reserved_=N;
	return;
	}
void dataType::shrink(int N){
	if(size_>=N) return; // bad call
	if(reserved_<=N) return; //nothing to be done
	void *newStream=new char[N];
	if(newStream == NULL ) throw std::bad_alloc(); //mem fail
	memcpy(newStream,dataStream_,size_);
	delete [] (char*)dataStream_;
	//update pointers
	dataStream_=newStream;
	reserved_=N;
	return;
	}
// --- append
void dataType::append(void* data, int N){
	int tot=N+size_;
	if(tot>0 && reserved_==0) reserve(tot);
	while (reserved_<tot){
		reserve(2*reserved_); // this avoids the +1, +1 ...
		}
	char * ptr=(char*) dataStream_;
	ptr += size_;
	memcpy(ptr,data,N);
	size_=tot;
	}
	// --- remove
void dataType::erase(int A,int B){
	if( B > size_) B=size_;
	if( A >= B ) return;
	// 0....A---B....N-1
	for(int i=A ; B+i-A<size_ ; ++i )	
		{
		((char*)dataStream_)[i] =((char*)dataStream_)[B+i-A];
		}
	size_ -= B-A;
	while(reserved_> 2*size_ && reserved_>2) shrink( reserved_/2); // shrikn to half of the reseserved. this avoid -1, -1...
	return;
	}
	// --- clear
void dataType::clear(){ 
	size_=0;
	reserved_=0; 
	if(dataStream_!=NULL)delete [] (char*)dataStream_; 
	dataStream_=NULL;
	}

void dataType::copy(void* data,int N){
	if(reserved_<N) reserve(N);
	memcpy(dataStream_,data,N);
	size_=N;
	return;
}
	// --- release
void dataType::release(){
	dataStream_=NULL; // don't destroy!
	clear();
}


// ---------- Event Builder
EventBuilder::EventBuilder()
{
	myRun_.reserve(1024); //reserve 1K for each stream
	dumpEvent_=true;
	sendEvent_=false;
	recvEvent_=0;
	isRunOpen_=false;
	lastRun_=100;
	// Init dumper
	dump_=new Logger();
	dump_->SetFileName("/tmp/dump.gz"); // rewritten by config
	dump_->SetCompress();
	dump_->SetBinary();
	dump_->SetAsync(); // asyncronous dumping
}
EventBuilder::~EventBuilder()
{
	Clear();
	delete dump_;
}


// ---- STATIC EVENT BUILDER
//

#define WORDSIZE 4

dataType EventBuilder::WordToStream(WORD x)
{
	dataType R; 
	R.reserve(WORDSIZE);
	if (sizeof(x) == WORDSIZE)
	{
		R.append(&x,WORDSIZE);
		return R;
	}	
	//if( sizeof(x) < 4 ) // this is not C if word is at least int
	if (sizeof(x) > WORDSIZE)  // take the less 4 bit significant digits
	{ // hope never arrived here
		unsigned int i=1;
		char *c=(char*)&i;
		if(*c)// little endian
			{ // we don't want to reverse the content probably the number is already a cast to something
			R.append(&x,WORDSIZE);
			}
		else// big endian
			{
			c=(char*)&x;
			c+=sizeof(x)-WORDSIZE;
			R.append(&c,WORDSIZE);
			}
	}
	return R;
}

dataType EventBuilder::BoardHeader(WORD boardId)
{
	dataType R; R.reserve(2*WORDSIZE);
	R.append((void*)"BRDH\0\0\0\0\0\0\0\0",WORDSIZE); //WORDSIZE<12
	R.append(WordToStream(boardId) ); 
	return R;
}

dataType EventBuilder::BoardTrailer(WORD boardId)
{
	dataType R;R.reserve(WORDSIZE);
	R.append((void*)"BRDT\0\0\0\0\0\0\0\0",WORDSIZE); // WORDSIZE<12
	return R;
}


// [HEAD][Nbytes][ ----- ][TRAILER]
// [HEAD]="BRDH"+"BRDID" - WORD-WORD
dataType EventBuilder::BoardToStream(WORD boardId,vector<WORD> &v)
{
	dataType R; R.reserve(v.size()*4+12);// not important the reserve, just to avoid N malloc operations
	R.append( BoardHeader(boardId)   );
	WORD N= v.size()*WORDSIZE;
	R.append( WordToStream(N)  ); 
	for(int i=0;i<v.size();i++) R.append( WordToStream(v[i])  );
	R.append( BoardTrailer(boardId)   );

	return R;
}

dataType EventBuilder::EventHeader()
{
	dataType R; R.reserve(WORDSIZE);
	R.append((void*)"EVTH\0\0\0\0\0\0\0\0",WORDSIZE);
	return R;
}

dataType EventBuilder::EventTrailer()
{
	dataType R;R.reserve(WORDSIZE);
	R.append((void*)"EVNT\0\0\0\0\0\0\0\0",WORDSIZE);
	return R;
}

dataType EventBuilder::RunHeader()
{
	dataType R; R.reserve(WORDSIZE);
	R.append((void*)"RUNH\0\0\0\0\0\0\0\0",WORDSIZE);
	return R;
}

dataType EventBuilder::RunTrailer()
{
	dataType R;R.reserve(WORDSIZE);
	R.append((void*)"RUNT\0\0\0\0\0\0\0\0",WORDSIZE);
	return R;
}

dataType EventBuilder::MergeEventStream(dataType &x,dataType &y){ 
	// takes two streams and merge them independently from the content
	dataType R;
	R.append(x);

	WORD nboards1=ReadEventNboards(x);
	WORD nboards2=ReadEventNboards(y);
	long size1=IsEventOk(x);
	long size2=IsEventOk(y);
	WORD *ptrNboards=(WORD*)R.data() +1;
	*ptrNboards=nboards1+nboards2;
	R.reserve(size1+size2); // minus hedrs but it is just a reserve
	R.erase(size1-WORDSIZE, size1); // delete trailer

	WORD*ptr2= (WORD*)y.data() +2 ; // content

	R.append( (void*)ptr2, size2 -WORDSIZE*2 ) ; // remove H, copy T
	return R;
}


vector<WORD>	EventBuilder::StreamToWord(void*v,int N){
	dataType myStream(N,v);
	vector<WORD> R = StreamToWord( myStream);
	myStream.release();
	return R;
}
vector<WORD>	EventBuilder::StreamToWord(dataType &x){
	vector<WORD> R;
	for(int n=0; n< x.size() ; n++)
		{
		R.push_back( *(WORD*)(  ((char*)x.data()) + n*WORDSIZE) );
		}
	// check rounding
	if( x.size() % WORDSIZE  != 0 ) 
		{
		//Log("[2] EventBuilder::StreamToWord: Error in rounding, last bytes ignored",2); -- i'm in a static member function, can't call it
		}

	return R;
}

long long EventBuilder::IsBoardOk(dataType &x,WORD boardId=0){

	dataType H=BoardHeader(boardId);
	dataType T=BoardTrailer(boardId);
	
	// Look in the Header for the 
	vector<WORD> Header  = StreamToWord( H );
	vector<WORD> Trailer = StreamToWord( T );
	if(x.size() < 3*WORDSIZE ) return 0;
	vector<WORD> myHead  = StreamToWord( x.data(), 3*WORDSIZE ); // read the first three
	
	if (myHead[0]  != Header[0] ) return 0;

	if( boardId !=0 )
	{
		if ( myHead.size()< 2 || boardId != myHead[1] ) return 0;
	}

	// the the N of bytes of the stream
	if (myHead.size() <3) return 0;
	WORD NBytes = myHead[2];
	WORD NWords  = NBytes / WORDSIZE;
	if( boardId !=0 )
	{
		// TODO Add a check on NWORDS for the specific BoardId
		// return 0;
	}

	if ( x.size() < (NWords+3+1)*WORDSIZE )  return 0;
	vector<WORD> myWords = StreamToWord( x.data(), (NWords+3+1)*WORDSIZE  ); //
	// ------------------|TRAILER POS| +1 
	if (myWords.size() < NWords + 3 + 1 ) return 0; // useless now
	//check trailer
	if (myWords[NWords+3] != Trailer[0] ) return 0;

	return (NWords+3+1)*WORDSIZE; // interesting size

}

long long EventBuilder::IsBoardOk(void *v,int MaxN,WORD boardId=0)
	{
	// take ownership of myStream (*v)
	dataType myStream(MaxN,v);
	long long R= IsBoardOk(myStream,boardId);
	// release ownership of myStream
	myStream.release();
	return R;
	}

long long EventBuilder::IsEventOk(dataType &x){
	char *ptr=(char*)x.data();
	vector<WORD> myHead=StreamToWord(x.data(),WORDSIZE*2); // read the first two WORDS
	dataType H=EventHeader();
	dataType T=EventTrailer();
	
	vector<WORD> Header=StreamToWord( H );
	vector<WORD> Trailer=StreamToWord( T );

	// check header
	if( myHead.size() <2 ) return 0;
	if( myHead[0] != Header[0] ) return 0;
	// header is fine
	WORD nBoard=myHead[1];

	long long leftsize=x.size() - WORDSIZE*2;
	ptr += WORDSIZE*2 ;
	for(WORD iBoard = 0 ; iBoard < nBoard ;iBoard++)
		{
		long long readByte=IsBoardOk(ptr, leftsize);
		if (readByte==0) return 0;
		leftsize -= readByte;
		ptr += readByte;
		}
	vector<WORD> myTrail=StreamToWord( ptr , WORDSIZE ) ;
	ptr += WORDSIZE;
	if ( myTrail[0] != Trailer[0] )  return 0;
	return ptr - (char*)x.data();
} 


WORD 	EventBuilder::ReadRunNum(dataType &x)
{
	WORD *runNumPtr=(WORD*)x.data() + 1;
	return *runNumPtr;
}
WORD 	EventBuilder::ReadEventNboards(dataType &x)
{
	WORD *runNumPtr=(WORD*)x.data() + 1;
	return *runNumPtr;
}
WORD 	EventBuilder::ReadRunNevents(dataType &x)
{
	WORD *runNeventsPtr=(WORD*)x.data() + 2;
	return *runNeventsPtr;
}

// ---- EVENT BUILDER NON STATIC -----
void EventBuilder::Config(Configurator &c){
        xmlNode *eb_node = NULL;
        //locate EventBuilder Node
        for (eb_node = c.root_element->children; eb_node ; eb_node = eb_node->next)
        {
                if (eb_node->type == XML_ELEMENT_NODE &&
                                xmlStrEqual (eb_node->name, xmlCharStrdup ("EventBuilder"))  )
                        break;
        }
        if ( eb_node== NULL ) throw  config_exception();
	dumpEvent_=Configurator::GetInt(getElementContent(c, "dumpEvent" , eb_node) );
	sendEvent_=Configurator::GetInt(getElementContent(c, "sendEvent" , eb_node) );
	recvEvent_=Configurator::GetInt(getElementContent(c, "recvEvent" , eb_node) );
	dump_->SetFileName(getElementContent(c, "dumpFileName" , eb_node) );
	dump_->SetBinary();
	if (dump_->GetFileName().find(".gz") != string::npos)
		{dump_->SetCompress();}
	else {dump_->SetCompress(false);}
}

void EventBuilder::Init(){
	if(dumpEvent_)dump_->Init();
}

void EventBuilder::Clear(){
	if(dumpEvent_)dump_->Clear();
}
//
// RUN 
void EventBuilder::OpenRun(WORD runNum)
{
	if (isRunOpen_) CloseRun(); 
	if (dumpEvent_ && !recvEvent_) // TODO: what happen if I open a run without other runs to be close
	{ // open dumping file
		string fileName=dump_->GetFileName();
		size_t dot= fileName.rfind(".");
		string newFileName=fileName.substr(0,dot) + "_" + to_string(runNum) + fileName.substr(dot,string::npos); // c++11 to string, othewsie use something like sprintf
		dump_->SetFileName( newFileName );	
		dump_->Init();
	}
	isRunOpen_=true;
	lastRun_= runNum;
	dataType runH=RunHeader();
	myRun_.append(runH);
	myRun_.append( (void *)&runNum,WORDSIZE);
	WORD zero=0;
	myRun_.append( (void*)&zero, WORDSIZE);
}

Command EventBuilder::CloseRun()
{
	Command myCmd; myCmd.cmd=NOP;
	isRunOpen_=false;	
	dataType  runT=RunTrailer();
	myRun_.append(runT);
	if( dumpEvent_ && !recvEvent_) 
	{
		Dump(myRun_);
		dump_->Close();
	}
	if (recvEvent_) { 
		WORD runNum=ReadRunNum(myRun_);
		if ( runs_.find(runNum) != runs_.end() ) 
			{
			runs_[runNum] = pair<int,dataType>(1,dataType( myRun_.size(),myRun_.data())   );
			myRun_.release();
			}
		else MergeRuns(runs_[runNum].second,myRun_);
	} 
	if (sendEvent_) {//TODO -- also do the merging if recv
		// --- Instruct Daemon to send them through the connection manager
		myCmd.cmd=SEND;
		dataType myMex;
		myMex.append((void*)"DATA\0",5);
		myMex.append(myRun_);
		myCmd.data=myMex.data();
		myCmd.N=myMex.size();
		myMex.release();
		} 
	myRun_.clear();
	return myCmd;
}

void EventBuilder::AddEventToRun(dataType &event){
	if (!isRunOpen_) return; // throw exception TODO
	// find the N.Of.Event in the actual RUn
	if (myRun_.size() < WORDSIZE*3)  return; //throw exception TODO
	WORD *nEventsPtr=((WORD*)myRun_.data() +2 );
	WORD nEvents= *nEventsPtr;
	nEvents+=1;
	(*nEventsPtr)=nEvents;
	myRun_.append(event);
	return;
}
void EventBuilder::MergeRuns(dataType &run1,dataType &run2 ){
	// --- can't merge inplace two events
	WORD runNum1 = ReadRunNum(run1);
	WORD runNum2 = ReadRunNum(run2);

	WORD runNevents1 = ReadRunNevents(run1);
	WORD runNevents2 = ReadRunNevents(run2);

	if (runNum1 != runNum2) return ; // TODO throw exception
	if (runNevents1 != runNevents2) return ; // TODO throw exception

	dataType oldRun(run1.size(),run1.data());	
	run1.release();run1.clear();


	dataType H=RunHeader();
	dataType T=RunTrailer();
	run1.append(H);
	run1.append((void*)&runNum1,WORDSIZE);
	run1.append((void*)&runNevents1,WORDSIZE);

	char* ptr1=(char*)oldRun.data();
	char* ptr2=(char*)run2  .data();
	long left1=oldRun.size() - WORDSIZE*3;
	long left2=run2.size() - WORDSIZE*3;
	ptr1+= WORDSIZE*3;
	ptr2+= WORDSIZE*3;
	for(int iEvent=0;iEvent< runNevents1 ;iEvent++)
	       {
		long eventSize1= IsEventOk(ptr1,left1);
		long eventSize2= IsEventOk(ptr2,left2);
		//---- Unire i due eventi
		dataType event1(eventSize1,ptr1);
		dataType event2(eventSize2,ptr2);
		dataType myEvent=MergeEventStream(event1,event2);
		event1.release();
		event2.release();
		run1.append(myEvent);
		//----
		ptr1+= eventSize1;
		ptr2+= eventSize2;
		left1-=eventSize1;
		left2-=eventSize2;
	       }	
	
	run1.append(T); 
}

void EventBuilder::MergeRuns(dataType &run2 ) {
	WORD runNum=ReadRunNum(run2); 
	MergeRuns(runs_[runNum].second,run2); 
	runs_[runNum].first++; 
	if (!recvEvent_ ) return ;

	if ( runs_[runNum].first > recvEvent_)  // dump for recvEvent
		{
		string fileName=dump_->GetFileName();
		size_t dot= fileName.rfind(".");
		string newFileName=fileName.substr(0,dot) + "_" + to_string(runNum) + fileName.substr(dot,string::npos); // c++11 to string, othewsie use something like sprintf
		dump_->SetFileName( newFileName );	
		dump_->Init();
		Dump(runs_[runNum].second);
		dump_->Close();
		runs_.erase(runNum);
		}
	return;
} 
