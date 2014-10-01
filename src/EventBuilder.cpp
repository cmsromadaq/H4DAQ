#include "interface/EventBuilder.hpp"
#include "interface/DataType.hpp"
#include "interface/Utility.hpp"
#include <sstream>

//#define EB_DEBUG



// ---------- Event Builder
EventBuilder::EventBuilder()
{
	//mySpill_.reserve(2048); //reserve 1K for each stream
	dumpEvent_=true;
	sendEvent_=false;
	recvEvent_=0;
	isSpillOpen_=false;
	//lastSpill_=100;
	ResetSpillNumber();
	// Init dumper
	dump_=new Logger();
	// dump_->SetFileName("/tmp/dump.gz"); // rewritten by config
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

void EventBuilder::BoardHeader(dataType &R, BoardId id)
{
	//R.reserve(2*WORDSIZE);
	R.append((void*)"BRDH\0\0\0\0\0\0\0\0",WORDSIZE); //WORDSIZE<12
	//dataType 
	//
	//compute shifts for BitMasks: 
	//crate id starts from 1->n independently from the bit mask
	int myCrateIdShift=GetCrateIdShift();
	int myBoardIdShift=GetBoardIdShift();
	int myBoardTypeShift=GetBoardTypeShift();

	WORD myId= 	( (id.crateId_<<myCrateIdShift)       & GetCrateIdBitMask()     ) | 
			( (id.boardId_<<myBoardIdShift)     & GetBoardIdBitMask()     ) |  
			( (id.boardType_<<myBoardTypeShift) & GetBoardTypeIdBitMask() )  ;
	WordToStream(R,myId);
#ifdef EB_DEBUG
	ostringstream s;
	s<<"[EventBuilder]::[BoardId]::[DEBUG] CrateId="<< id.crateId_
		<<" BID="<<id.boardId_
		<<" TYP="<<id.boardType_
		<<" WORD="<<std::hex<< myId<<std::dec;
	printf("%s\n",s.str().c_str());
#endif

	return ;
}

void EventBuilder::BoardTrailer(dataType&R)
{
	//R.reserve(WORDSIZE);
	R.append((void*)"BRDT\0\0\0\0\0\0\0\0",WORDSIZE); // WORDSIZE<12
	return ;
}


// [HEAD][Nbytes][ ----- ][TRAILER]
// [HEAD]="BRDH"+"BRDID"+[BOADSIZE] - WORD-WORD
void EventBuilder::BoardToStream(dataType &R ,BoardId id,vector<WORD> &v)
{
	//R.reserve(v.size()*4+12);// not important the reserve, just to avoid N malloc operations
	BoardHeader(R, id );
	WORD N= (v.size() + BoardHeaderWords() + BoardTrailerWords() )*WORDSIZE  ;
	WordToStream(R,N)  ; 
	for(unsigned long long i=0;i<v.size();i++) WordToStream(R,v[i])  ;
	BoardTrailer(R) ;

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
	// R should be empty
	if (R.size() >0 ) {
			//Log("[EventBuilder]::[MergeEventStream] Return Size is more than 0", 1); // cannot Log, private
			throw logic_exception();
			}
	//dataType R;
	R.append(x);

	WORD nboards1=ReadEventNboards(x);
	WORD nboards2=ReadEventNboards(y);
		
	WORD eventNum1=ReadEventNumber(x);
	WORD eventNum2=ReadEventNumber(y);
		
	if(eventNum1 != eventNum2) {
		R.clear();
		return;
		}

	long size1=IsEventOk(x);
	long size2=IsEventOk(y);

	WORD *ptrNboards=(WORD*)R.data() + EventNboardsPos();
	*ptrNboards=nboards1+nboards2;
	R.reserve(size1+size2); // minus hedrs but it is just a reserve
	R.erase(size1-WORDSIZE*EventTrailerWords(), size1); // delete trailer

	WORD*ptr2= (WORD*)y.data() + EventHeaderWords() ; // content

	R.append( (void*)ptr2, size2 - WORDSIZE*EventHeaderWords() ) ; // remove H, copy T
	// update size
	WORD *sizepos=(WORD*)R.data() + EventSizePos();
	*sizepos= (WORD)R.size();
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
	for(unsigned long long int n=0; n< x.size() ; n++)
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

dataTypeSize_t EventBuilder::IsBoardOk(dataType &x){

	BoardId empty;
	dataType H;BoardHeader(H,empty);
	dataType T;BoardTrailer(T);
	
	// Look in the Header for the 
	vector<WORD> Header  = StreamToWord( H );
	vector<WORD> Trailer = StreamToWord( T );
	if(x.size() < 4*WORDSIZE ) return 0;
	vector<WORD> myHead  = StreamToWord( x.data(), 4*WORDSIZE ); // read the first three
	
	if (myHead[0]  != Header[0] ) return 0;


	// the the N of bytes of the stream
	if (myHead.size() <4) return 0;
	WORD NBytes = myHead[3];
	WORD NWords  = NBytes / WORDSIZE;

	if ( x.size() < (NWords+4+1)*WORDSIZE )  return 0;
	vector<WORD> myWords = StreamToWord( x.data(), (NWords+4+1)*WORDSIZE  ); //
	// ------------------|TRAILER POS| +1 
	if (myWords.size() < NWords + 4 + 1 ) return 0; // useless now
	//check trailer
	if (myWords[NWords+4] != Trailer[0] ) return 0;

	return (NWords+4+1)*WORDSIZE; // interesting size

}

dataTypeSize_t EventBuilder::IsBoardOk(void *v,int MaxN)
	{
	// take ownership of myStream (*v)
	dataType myStream(MaxN,v);
	dataTypeSize_t R= IsBoardOk(myStream);
	// release ownership of myStream
	myStream.release();
	return R;
	}

dataTypeSize_t EventBuilder::IsEventOk(dataType &x){
	char *ptr=(char*)x.data();
	vector<WORD> myHead=StreamToWord(x.data(),WORDSIZE*4); // read the first two WORDS
	dataType H;EventHeader(H);
	dataType T;EventTrailer(T);
	
	vector<WORD> Header=StreamToWord( H );
	vector<WORD> Trailer=StreamToWord( T );

	// check header
	if( myHead.size() <4 ) return 0;
	if( myHead[0] != Header[0] ) return 0;
	// header is fine
	WORD nBoard   = myHead[3];
	WORD eventSize= myHead[2];
	WORD eventNum = myHead[1];

	dataTypeSize_t leftsize=x.size() - WORDSIZE*4;
	ptr += WORDSIZE*4 ;
	for(WORD iBoard = 0 ; iBoard < nBoard ;iBoard++)
		{
		dataTypeSize_t readByte=IsBoardOk(ptr, leftsize);
		if (readByte==0) return 0;
		leftsize -= readByte;
		ptr += readByte;
		}
	vector<WORD> myTrail=StreamToWord( ptr , WORDSIZE ) ;
	ptr += WORDSIZE;
	if ( myTrail[0] != Trailer[0] )  return 0;
	//mismatch in size
	if (eventSize != (WORD)(ptr -(char*)x.data()) ) return 0;
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
	WORD *spillNumPtr=(WORD*)x.data() + EventNboardsPos();
	return *spillNumPtr;
}
WORD 	EventBuilder::ReadEventNumber(dataType &x)
{
	WORD *eventNumPtr=(WORD*)x.data() + 1;
	return *eventNumPtr;
}
WORD 	EventBuilder::ReadSpillNevents(dataType &x)
{
	WORD *spillNeventsPtr=(WORD*)x.data() + 4;
	return *spillNeventsPtr;
}

// ---- EVENT BUILDER NON STATIC -----
void EventBuilder::Config(Configurator &c){
#ifndef NO_XML
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

	ostringstream s;
	s<<"[EventBuilder]::[Config]::[INFO] DumpEvent="<<dumpEvent_;
	Log(s.str(),1);
	s.str("");
	s<<"[EventBuilder]::[Config]::[INFO] RecvEvent="<<recvEvent_;
	Log(s.str(),1);
	s.str("");
	s<<"[EventBuilder]::[Config]::[INFO] SendEvent="<<sendEvent_;
	Log(s.str(),1);
#else
	printf("[EventBuilder]::[Config] NO_XML Action Forbid\n");
	throw config_exception();

#endif
}

void EventBuilder::Init(){
	if(dumpEvent_)dump_->Init();
}

void EventBuilder::Clear(){
	if(dumpEvent_)dump_->Clear();
}
//
// SPILL 
void EventBuilder::OpenSpill()
{
	if (isSpillOpen_) CloseSpill(); 
	lastEvent_.spillNum_ ++;
	lastEvent_.eventInSpill_=1;
	if (dumpEvent_ && !recvEvent_) 
	{ // open dumping file
		dump_->Close();
		string newFileName= dirName_ + "/" + to_string((unsigned long long) lastEvent_.runNum_) + "/" + to_string((unsigned long long)lastEvent_.spillNum_);
		if (dump_->GetCompress() )  newFileName += ".raw.gz";
		else newFileName +=".raw";
		dump_->SetFileName( newFileName );	
		dump_->Init();
		Log("[EventBuilder]::[OpenSpill] Open file name:" + newFileName,1) ;
	}
	isSpillOpen_=true;
	SpillHeader(mySpill_);
	mySpill_.append( (void *)& lastEvent_.runNum_,WORDSIZE);
	mySpill_.append( (void *)& lastEvent_.eventInSpill_,WORDSIZE);
	WORD zero=0;
	mySpill_.append( (void*)&zero, WORDSIZE);
	mySpill_.append( (void*)&zero, WORDSIZE);
	
}

Command EventBuilder::CloseSpill()
{
	Command myCmd; myCmd.cmd=NOP;
	isSpillOpen_=false;	
	dataType  spillT;  SpillTrailer(spillT);
	mySpill_.append(spillT);
	// ack the position and the n.of.events
	WORD *nEvents=((WORD*)mySpill_.data()) + SpillNeventPos();
	WORD *spillSize=((WORD*)mySpill_.data()) + SpillSizePos();
	(*spillSize)= (WORD)mySpill_.size();
	(*nEvents)  = (WORD)lastEvent_.eventInSpill_; // TODO chekc offset by 1


	if( dumpEvent_ && !recvEvent_) 
	{
		Log("[EventBuilder]::[CloseSpill] File Closed",2) ;
		Dump(mySpill_);
		dump_->Close();
	}
	if (recvEvent_) { 
		Log("[EventBuilder]::[CloseSpill] File In Recv Mode",2) ;
		WORD spillNum=ReadSpillNum(mySpill_);
		if ( spills_.find(spillNum) != spills_.end() ) 
			{
			spills_[spillNum] = pair<int,dataType>(1,dataType( mySpill_.size(),mySpill_.data())   ); // spills_[] take ownership of the structuer
			mySpill_.release();
			}
		else MergeSpills(spills_[spillNum].second,mySpill_);
	} 
	if (sendEvent_) {//TODO -- also do the merging if recv
		// --- Instruct Daemon to send them through the connection manager
		Log("[EventBuilder]::[CloseSpill] File In Send Mode",2) ;
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
	
	lastEvent_.eventInSpill_++;
	// this are updated in the CloseSpill -- should we keep them consistent
	//WORD *nEventsPtr=((WORD*)mySpill_.data() +3 );
	//WORD nEvents= *nEventsPtr;
	//nEvents+=1;
	//(*nEventsPtr)=nEvents;
	
	mySpill_.append(event);
	return;
}
void EventBuilder::MergeSpills(dataType &spill1,dataType &spill2 ){ 
	// TODO: update to the new dataformat
	// --- can't merge inplace two events
	WORD runNum1 = ReadRunNumFromSpill(spill1);
	WORD runNum2 = ReadRunNumFromSpill(spill2);

	WORD spillNum1 = ReadSpillNum(spill1);
	WORD spillNum2 = ReadSpillNum(spill2);

	WORD spillNevents1 = ReadSpillNevents(spill1);
	WORD spillNevents2 = ReadSpillNevents(spill2);

	WORD zero=0; //spillSize

	if (runNum1 != runNum2) return ; // TODO throw exception
	if (spillNum1 != spillNum2) return ; // TODO throw exception
	if (spillNevents1 != spillNevents2) return ; // TODO throw exception
	dataType oldSpill(spill1.size(),spill1.data());	
	spill1.release();spill1.clear();

	dataType H;SpillHeader(H);
	dataType T;SpillTrailer(T);

	spill1.append(H);
	spill1.append((void*)&runNum1,WORDSIZE); // 
	spill1.append((void*)&spillNum1,WORDSIZE); // 
	spill1.append( (void*)&zero, WORDSIZE); // spillSize
	spill1.append((void*)&spillNevents1,WORDSIZE);

	char* ptr1=(char*)oldSpill.data();
	char* ptr2=(char*)spill2  .data();
	long left1=oldSpill.size() - WORDSIZE*SpillHeaderWords();
	long left2=spill2.size() - WORDSIZE*SpillHeaderWords();
	ptr1+= WORDSIZE*SpillHeaderWords();
	ptr2+= WORDSIZE*SpillHeaderWords();
	for(unsigned long long iEvent=0;iEvent< spillNevents1 ;iEvent++)
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
	// update Spill Size
	WORD *spillSizePtr= ((WORD*) spill1.data() )+ SpillSizePos();
	(*spillSizePtr)=(WORD)spill1.size();
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
		if (dump_->GetCompress() )  newFileName += ".raw.gz";
		else newFileName +=".raw";
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
//runNum_=x;
lastEvent_.runNum_=x;
if (dumpEvent_ || recvEvent_)  // POSIX
  system(  ("mkdir -p "+dirName_+ to_string((unsigned long long)x) ).c_str() );
}


void EventBuilder::OpenEvent( dataType &R , WORD nBoard){
	if (R.size() >0 ) {
			Log("[EventBuilder]::[OpenEvent] Return Size is more than 0",1);
			throw logic_exception();
			}
        // Construt the event
        EventBuilder::EventHeader(R);

        EventBuilder::WordToStream(R, lastEvent_.eventInSpill_ );
        WORD zero=0;
        EventBuilder::WordToStream(R,zero); //eventSize in Byte
        EventBuilder::WordToStream(R,nBoard); // nBoard
	return;
}

void EventBuilder::CloseEvent( dataType &R){
	EventBuilder::EventTrailer(R);
        // N.Of Byte of the stream
        WORD* EventSizePtr = ((WORD*)R.data() ) + EventSizePos();
        (*EventSizePtr) = (WORD)R.size();
	return;

}

WORD EventBuilder::GetBoardTypeId(dataType &R){
	WORD myMergedId= *((WORD*)R.data() + BoardIdPos());
	WORD myResult= ( myMergedId & GetBoardTypeIdBitMask())>>GetBoardTypeShift();
	return myResult;
}

WORD EventBuilder::GetBoardBoardId(dataType &R){
	WORD myMergedId= *((WORD*)R.data() + BoardIdPos());
	WORD myResult= ( myMergedId & GetBoardIdBitMask())>>GetBoardIdShift();
	return myResult;
}

WORD EventBuilder::GetBoardCrateId(dataType &R){
	WORD myMergedId= *((WORD*)R.data() + BoardIdPos());
	WORD myResult= ( myMergedId & GetCrateIdBitMask())>>GetCrateIdShift();
	return myResult;
}
