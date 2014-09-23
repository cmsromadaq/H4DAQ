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
	dataStream_=NULL;
	size_=0;
	reserved_=0;
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
	//printf("Clearing dataType\n"); //DEBUG
	size_=0;
	reserved_=0; 
	if(dataStream_!=NULL)delete [] (char*)dataStream_; 
	dataStream_=NULL;
	//printf("Clearing dataType:DONE\n"); //DEBUG
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
	mySpill_.reserve(1024); //reserve 1K for each stream
	dumpEvent_=true;
	sendEvent_=false;
	recvEvent_=0;
	isSpillOpen_=false;
	//lastSpill_=100;
	ResetSpillNumber();
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


void EventBuilder::WordToStream(dataType&R, WORD x)
{
	R.append( (void*)&x,WORDSIZE);
	return ;
}

void EventBuilder::BoardHeader(dataType &R,WORD boardId)
{
	//R.reserve(2*WORDSIZE);
	R.append((void*)"BRDH\0\0\0\0\0\0\0\0",WORDSIZE); //WORDSIZE<12
	//dataType 
	//R.append(WordToStream(boardId) ); 
	WordToStream(R,boardId);	
	return ;
}

void EventBuilder::BoardTrailer(dataType&R,WORD boardId)
{
	//R.reserve(WORDSIZE);
	R.append((void*)"BRDT\0\0\0\0\0\0\0\0",WORDSIZE); // WORDSIZE<12
	return ;
}


// [HEAD][Nbytes][ ----- ][TRAILER]
// [HEAD]="BRDH"+"BRDID" - WORD-WORD
void EventBuilder::BoardToStream(dataType &R ,WORD boardId,vector<WORD> &v)
{
	//R.reserve(v.size()*4+12);// not important the reserve, just to avoid N malloc operations
	BoardHeader(R,boardId   );
	WORD N= v.size()*WORDSIZE;
	WordToStream(R,N)  ; 
	for(int i=0;i<v.size();i++) WordToStream(R,v[i])  ;
	BoardTrailer(R,boardId) ;

	return ;
}

void EventBuilder::EventHeader( dataType &R)
{
	//R.reserve(WORDSIZE);
	R.append((void*)"EVTH\0\0\0\0\0\0\0\0",WORDSIZE);
	return ;
}

void EventBuilder::EventTrailer(dataType &R)
{
	//R.reserve(WORDSIZE);
	R.append((void*)"EVNT\0\0\0\0\0\0\0\0",WORDSIZE);
	return ;
}

void EventBuilder::SpillHeader(dataType &R)
{
	//R.reserve(WORDSIZE);
	R.append((void*)"SPLH\0\0\0\0\0\0\0\0",WORDSIZE);
	return ;
}

void EventBuilder::SpillTrailer(dataType &R)
{
	//R.reserve(WORDSIZE);
	R.append((void*)"SPLT\0\0\0\0\0\0\0\0",WORDSIZE);
	return ;
}

void EventBuilder::MergeEventStream(dataType &R,dataType &x,dataType &y){ 
	// takes two streams and merge them independently from the content
	//dataType R;
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
	return ;
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

	dataType H;BoardHeader(H,boardId);
	dataType T;BoardTrailer(T,boardId);
	
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
	dataType H;EventHeader(H);
	dataType T;EventTrailer(T);
	
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

WORD 	EventBuilder::ReadRunNumFromSpill(dataType &x)
{
	WORD *spillNumPtr=(WORD*)x.data() + 1;
	return *spillNumPtr;
}

WORD 	EventBuilder::ReadSpillNum(dataType &x)
{
	WORD *spillNumPtr=(WORD*)x.data() + 2;
	return *spillNumPtr;
}
WORD 	EventBuilder::ReadEventNboards(dataType &x)
{
	WORD *spillNumPtr=(WORD*)x.data() + 1;
	return *spillNumPtr;
}
WORD 	EventBuilder::ReadSpillNevents(dataType &x)
{
	WORD *spillNeventsPtr=(WORD*)x.data() + 2;
	return *spillNeventsPtr;
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
	//dump_->SetFileName(getElementContent(c, "dumpFileName" , eb_node) );
	dirName_=getElementContent(c, "dumpDirName" , eb_node) ;
	bool dumpCompress=Configurator::GetInt(getElementContent(c, "dumpCompress" , eb_node) );
	dump_->SetCompress(dumpCompress);
	dump_->SetBinary();
}

void EventBuilder::Init(){
	if(dumpEvent_)dump_->Init();
}

void EventBuilder::Clear(){
	if(dumpEvent_)dump_->Clear();
}
//
// SPILL 
void EventBuilder::OpenSpill(WORD spillNum)
{
	if (isSpillOpen_) CloseSpill(); 
	if (dumpEvent_ && !recvEvent_) 
	{ // open dumping file
		dump_->Close();
		string newFileName= dirName_ + "/" + to_string((unsigned long long) runNum_) + "/" + to_string((unsigned long long)spillNum);
		if (dump_->GetCompress() )  newFileName += ".gz";
		else newFileName +=".txt";
		dump_->SetFileName( newFileName );	
		dump_->Init();
	}
	isSpillOpen_=true;
	lastSpill_= spillNum;
	SpillHeader(mySpill_);
	mySpill_.append( (void *)&runNum_,WORDSIZE);
	mySpill_.append( (void *)&spillNum,WORDSIZE);
	WORD zero=0;
	mySpill_.append( (void*)&zero, WORDSIZE);
	
}

Command EventBuilder::CloseSpill()
{
	Command myCmd; myCmd.cmd=NOP;
	isSpillOpen_=false;	
	dataType  spillT;SpillTrailer(spillT);
	mySpill_.append(spillT);
	if( dumpEvent_ && !recvEvent_) 
	{
		Dump(mySpill_);
		dump_->Close();
	}
	if (recvEvent_) { 
		WORD spillNum=ReadSpillNum(mySpill_);
		if ( spills_.find(spillNum) != spills_.end() ) 
			{
			spills_[spillNum] = pair<int,dataType>(1,dataType( mySpill_.size(),mySpill_.data())   );
			mySpill_.release();
			}
		else MergeSpills(spills_[spillNum].second,mySpill_);
	} 
	if (sendEvent_) {//TODO -- also do the merging if recv
		// --- Instruct Daemon to send them through the connection manager
		myCmd.cmd=SEND;
		dataType myMex;
		myMex.append((void*)"DATA\0",5);
		myMex.append(mySpill_);
		myCmd.data=myMex.data();
		myCmd.N=myMex.size();
		myMex.release();
		} 
	mySpill_.clear();
	return myCmd;
}

void EventBuilder::AddEventToSpill(dataType &event){
	if (!isSpillOpen_) return; // throw exception TODO
	// find the N.Of.Event in the actual RUn
	if (mySpill_.size() < WORDSIZE*4)  return; //throw exception TODO
	WORD *nEventsPtr=((WORD*)mySpill_.data() +3 );
	WORD nEvents= *nEventsPtr;
	nEvents+=1;
	(*nEventsPtr)=nEvents;
	mySpill_.append(event);
	return;
}
void EventBuilder::MergeSpills(dataType &spill1,dataType &spill2 ){
	// --- can't merge inplace two events
	WORD runNum1 = ReadRunNumFromSpill(spill1);
	WORD runNum2 = ReadRunNumFromSpill(spill2);

	WORD spillNum1 = ReadSpillNum(spill1);
	WORD spillNum2 = ReadSpillNum(spill2);

	WORD spillNevents1 = ReadSpillNevents(spill1);
	WORD spillNevents2 = ReadSpillNevents(spill2);

	if (runNum1 != runNum2) return ; // TODO throw exception
	if (spillNum1 != spillNum2) return ; // TODO throw exception
	if (spillNevents1 != spillNevents2) return ; // TODO throw exception

	dataType oldSpill(spill1.size(),spill1.data());	
	spill1.release();spill1.clear();


	dataType H;SpillHeader(H);
	dataType T;SpillTrailer(T);
	spill1.append(H);
	spill1.append((void*)&runNum1,WORDSIZE); // TODO new runN- spillN
	spill1.append((void*)&spillNum1,WORDSIZE); // TODO new runN- spillN
	spill1.append((void*)&spillNevents1,WORDSIZE);

	char* ptr1=(char*)oldSpill.data();
	char* ptr2=(char*)spill2  .data();
	long left1=oldSpill.size() - WORDSIZE*4;
	long left2=spill2.size() - WORDSIZE*4;
	ptr1+= WORDSIZE*4;
	ptr2+= WORDSIZE*4;
	for(int iEvent=0;iEvent< spillNevents1 ;iEvent++)
	       {
		long eventSize1= IsEventOk(ptr1,left1);
		long eventSize2= IsEventOk(ptr2,left2);
		//---- Unire i due eventi
		dataType event1(eventSize1,ptr1);
		dataType event2(eventSize2,ptr2);
		dataType myEvent;MergeEventStream(myEvent,event1,event2);
		event1.release();
		event2.release();
		spill1.append(myEvent);
		//----
		ptr1+= eventSize1;
		ptr2+= eventSize2;
		left1-=eventSize1;
		left2-=eventSize2;
	       }	
	
	spill1.append(T); 
}

void EventBuilder::MergeSpills(dataType &spill2 ) {
	WORD spillNum=ReadSpillNum(spill2); 
	MergeSpills(spills_[spillNum].second,spill2); 
	spills_[spillNum].first++; 
	if (!recvEvent_ ) return ;

	if ( spills_[spillNum].first > recvEvent_)  // dump for recvEvent
		{
		WORD myRunNum=ReadRunNumFromSpill(spill2);
		string newFileName= dirName_ + "/" + to_string((unsigned long long) myRunNum) + "/" + to_string((unsigned long long)spillNum);
		if (dump_->GetCompress() )  newFileName += ".gz";
		else newFileName +=".txt";
		dump_->SetFileName( newFileName );	
		dump_->Init();
		Dump(spills_[spillNum].second);
		dump_->Close();
		spills_.erase(spillNum);
		}
	return;
} 

void EventBuilder::SetRunNum(WORD x)
{
runNum_=x;
if (dumpEvent_ || recvEvent_)  // POSIX
  system(  ("mkdir -p "+dirName_+ to_string((unsigned long long)runNum_) ).c_str() );
}


