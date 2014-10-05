#ifndef EVTBLD_H
#define EVTBLD_H

#include "interface/StandardIncludes.hpp"
#include "interface/DataType.hpp"
#include "interface/Logger.hpp"

using namespace std;

//class Command; // Command is now in an ad hoc hpp

class EventParser{
public:
	/* this class converts the streams events in ROOT TTree files
	 */
	// --- Constructor
	EventParser();
	// --- Destructor
	~EventParser();

};

// --- ID
class EventId {
public:
	WORD runNum_;
	WORD spillNum_;
	WORD eventInSpill_;
};

class BoardId{
public:
	BoardId(){boardType_=boardId_=crateId_=0;};
	WORD boardType_;
	WORD boardId_;
	WORD crateId_;
};

class EventBuilder : public LogUtility, public Configurable{

friend class Daemon;
// ---binary stream of the event -- 1char = 1byte
dataType mySpill_; // dynamic array of char*

bool dumpEvent_; // default true
bool sendEvent_; // default false
int recvEvent_; //  wait for other classes. Change behaviour of dumpEvent
Logger *dump_; // this is not the Logger. This will be used to dump the event, and will be set in binary mode.
string dirName_;
bool isSpillOpen_;
//WORD lastSpill_;
//WORD runNum_;

EventId lastEvent_;


//map<WORD,pair<int,dataType> > spills_; //store incomplete spills if in recv mode. SPILLNUM -> NMerged, SpillStream
int merged_;
WORD lastBadSpill_;

	int MergeSpills(dataType &spill1,dataType &spill2 ); 
	
public:
	/* this class contains the raw event.
	 * It performs simple operations, like merging all the event informations, with base checks on the counters
	 * In case of errors returns an exception that will trigger a reset of the counters
	 */
	// --- Constructor
	EventBuilder();
	// --- Destructor
	~EventBuilder();
	// --- Get Info
	inline bool 	GetDumpEvent(){return dumpEvent_;};
	inline bool 	GetSendEvent(){return sendEvent_;};
	inline int 	GetRecvEvent() { return recvEvent_;};
	inline void*	GetStream(){ return mySpill_.data();};
	inline int  	GetSize(){return mySpill_.size();};
	inline bool 	AreSpillsMerged(){ return mySpill_.size() == 0 ; };
	inline string 	GetDirName()const {return dirName_;}
	inline EventId  GetEventId() const { return lastEvent_;}
	// --- Set  Info
	inline void SetDumpCompress(bool x=true){dump_->SetCompress(x);};
	inline void SetDumpEvent(bool x=true){dumpEvent_=x;};
	inline void SetSendEvent(bool x=true){sendEvent_=x;};
	inline void SetRecvEvent(int x=0) {recvEvent_=x;};
	void 	SetRunNum(WORD x);// set also the filename ...
	void  inline SetSpillNum(WORD x){ lastEvent_.spillNum_=x;};
	void  inline SetEventNum(WORD x){ lastEvent_.eventInSpill_=x;};
	inline void  SetDirName(string mydir="/tmp/") {dirName_=mydir;}
	// Configurable
	void Config(Configurator&); // TODO --check that all is complete
	void Init();//TODO -- check
	void Clear(); //TODO -- check
	inline void Dump(dataType&spill) {dump_->Dump(spill);};
	// -- Open Spill
	void OpenSpill(); 
	// -- close the spill. Return the command to be parsed by the daemon
	Command CloseSpill( ); 
	void OpenEvent(dataType &R,WORD nBoard);  
	void CloseEvent(dataType &R); 
	inline void ResetSpillNumber(){lastEvent_.spillNum_=0;}; 
	void AddEventToSpill(dataType &event );  
	// merge in Spill1, Spill2. check i spillNum is to be dumped
	void MergeSpills(dataType &spill2 ) ; 
	// ---  this will be used by hwmanager to convert the output of a board in a stream
	// ---  appends a header and trailer
	static void WordToStream(dataType&R,WORD x);
	static vector<WORD> StreamToWord(dataType &x);
	static vector<WORD> StreamToWord(void*v,int N);
	static WORD 	ReadSpillNum(dataType &x);
	static WORD 	ReadRunNumFromSpill(dataType &x);
	static WORD 	ReadSpillNevents(dataType &x);
	static WORD 	ReadEventNboards(dataType &x);
	static WORD 	ReadEventNumber(dataType &x);
	static inline WORD 	ReadBoardSize(dataType &x){ return *((WORD*)x.data() + BoardSizePos()); }
	static void BoardHeader(dataType &R,BoardId id);
	static void BoardTrailer(dataType &R);
	static void EventHeader(dataType&R);
	static void EventTrailer(dataType&R);
	static void SpillHeader(dataType&R);
	static void SpillTrailer(dataType&);
	static void BoardToStream(dataType &R,BoardId id,vector<WORD> &v);
	static void MergeEventStream(dataType&R,dataType &x,dataType &y); // R should be empty
	static dataTypeSize_t IsBoardOk(dataType &x); // return 0 if NOT, otherwise the NBytes of the TOTAL BOARD STREAM 
	static dataTypeSize_t IsBoardOk(void *v,int MaxN); // if id==NULL don't check consistency for that id; MaxN is the space where it is safe to look into (allocated)
	static dataTypeSize_t IsEventOk(dataType &x); // return 0 if NOT, otherwise the NBytes of the TOTAL Event STREAM
	static inline dataTypeSize_t IsEventOk(void *v, int N){ dataType tmp(N,v);dataTypeSize_t R= IsEventOk(tmp); tmp.release(); return R;};

	// ----- MASK for board id
	static inline const WORD GetBoardIdBitMask(){return 0x0000FFFF;};
	static inline const WORD GetCrateIdBitMask(){return 0x00FF0000;};
	static inline const WORD GetBoardTypeIdBitMask (){return 0xFF000000;};
	//static inline const int  GetCrateIdShift() {for (int i=0;i<32 ;i++) if ( (GetCrateIdBitMask() >> i )&1 ) { return i ; } ; return 0; }
	static inline const int  GetCrateIdShift() {return 16; };
	static inline const int  GetBoardIdShift() { return 0; };
	static inline const int  GetBoardTypeShift() { return 24; }
	// --- Method to get the BoardId from the masks
	static WORD GetBoardTypeId( dataType&);
	static WORD GetBoardBoardId( dataType&);
	static WORD GetBoardCrateId( dataType&);
	// ----- WORDS in event format
	static inline const int SpillHeaderWords(){return 5;};	
	static inline const int EventHeaderWords(){return 4;};	
	static inline const int BoardHeaderWords(){return 3;};
	static inline const int SpillTrailerWords(){return 1;};	
	static inline const int EventTrailerWords(){return 1;};	
	static inline const int BoardTrailerWords(){return 1;};
	static inline const int SpillSizePos(){return 3;};	
	static inline const int SpillNeventPos(){return 4;};	
	static inline const int EventSizePos(){return 2;};	
	static inline const int BoardSizePos(){return 2;};
	static inline const int BoardIdPos(){return 1;};
	static inline const int EventNboardsPos(){return 3;};
	static inline const int EventEnumPos(){return 1;};	
	static inline const int EventTimePos(){return 7;};	

};

//#include "interface/Daemon.hpp" // Command -- fwd decl

#endif
