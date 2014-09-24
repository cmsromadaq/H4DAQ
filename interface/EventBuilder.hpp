#include "interface/StandardIncludes.hpp"


using namespace std;


#ifndef EVTBLD_H
#define EVTBLD_H
#include "interface/Logger.hpp"

class Command; // fwd decl

//typedef unsigned int WORD;
typedef uint32_t WORD;
#define WORDSIZE 4
typedef unsigned long long dataTypeSize_t;

class EventParser{
public:
	/* this class converts the streams events in ROOT TTree files
	 */
	// --- Constructor
	EventParser();
	// --- Destructor
	~EventParser();

};

class dataType{
	//this is a dynamic array of char*. 
	//string is null terminated = bad!!! 
	//the syntax is the same of string
private:
	void *dataStream_;
	dataTypeSize_t size_; // store the used size of dataStream
	dataTypeSize_t reserved_; //store the actual dimension in memory of dataStream
public:
	// --- Constructor
	dataType();
	//dataType(const char*);//init with a null terminated string
	dataType(dataTypeSize_t N, void* );//init with a dataType
	// --- Destructor
	~dataType();
	// --- Get size
	dataTypeSize_t size(){return size_;}
	// --- reserve N bytes in memory
	void reserve(dataTypeSize_t N);
	// --- shrink N bytes in memory
	void shrink(dataTypeSize_t N);
	// --- return stream pointer
	const void* c_str(){return (const void*)dataStream_;}
	void* data(){
		return ( void*)dataStream_;
		}
	// --- append
	void append(void* data, dataTypeSize_t N);
	// --- append using dataType
	void append(dataType &x){return append( x.data(), dataTypeSize_t(x.size()) ) ;};
	// --- remove [A,B)
	void erase(dataTypeSize_t A,dataTypeSize_t B);
	// --- clear
	void clear();
	// --- copy
	void copy(void * data, dataTypeSize_t N);
	// --- release ownership of data. This will lost control of data and location
	void release();
	//--- Operators
	//dataType& operator+=(dataType &x) { this->append(x); return this;}

};
// --- bool operator -- mv inside the class
const bool operator==(dataType &x, dataType &y);

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
WORD lastSpill_;
WORD runNum_;

map<WORD,pair<int,dataType> > spills_; //store incomplete spills if in recv mode. SPILLNUM -> NMerged, SpillStream

	void MergeSpills(dataType &spill1,dataType &spill2 ); 
	
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
	inline string 	GetDirName()const {return dirName_;}
	// --- Set  Info
	inline void SetDumpCompress(bool x=true){dump_->SetCompress(x);};
	inline void SetDumpEvent(bool x=true){dumpEvent_=x;};
	inline void SetSendEvent(bool x=true){sendEvent_=x;};
	inline void SetRecvEvent(int x=0) {recvEvent_=x;};
	void 	SetRunNum(WORD x);// set also the filename ...
	inline void  	SetDirName(string mydir="/tmp/") {dirName_=mydir;}
	//void AppendToStream();
	// Configurable
	void Config(Configurator&); // TODO --check that all is complete
	void Init();//TODO -- check
	void Clear(); //TODO -- check
	inline void Dump(dataType&spill) {dump_->Dump(spill);};
	void OpenSpill(WORD spillNum);
	inline void OpenSpill() { OpenSpill( lastSpill_ + 1 ) ; }; 
	inline void ResetSpillNumber(){lastSpill_=100;}
	// -- close the spill. Return the command to be parsed by the daemon
	Command CloseSpill( );
	void AddEventToSpill(dataType &event ); 
	// merge in Spill1, Spill2. check i spillNum is to be dumped
	void MergeSpills(dataType &spill2 ) ;//{WORD spillNum=ReadSpillNum(spill2); MergeSpills(spills_[spillNum].second,spill2); spills_[spillNum].first++; return;} ;
	//
	// ---  this will be used by hwmanager to convert the output of a board in a stream
	// ---  appends a header and trailer
	static void WordToStream(dataType&R,WORD x);
	static vector<WORD> StreamToWord(dataType &x);
	static vector<WORD> StreamToWord(void*v,int N);
	static WORD 	ReadSpillNum(dataType &x);
	static WORD 	ReadRunNumFromSpill(dataType &x);
	static WORD 	ReadSpillNevents(dataType &x);
	static WORD 	ReadEventNboards(dataType &x);
	static void BoardHeader(dataType &R,WORD boardId);
	static void BoardTrailer(dataType &R,WORD boardId);
	static void EventHeader(dataType&R);
	static void EventTrailer(dataType&R);
	static void SpillHeader(dataType&R);
	static void SpillTrailer(dataType&);
	static void BoardToStream(dataType &R,WORD boardId,vector<WORD> &v);
	static void MergeEventStream(dataType&R,dataType &x,dataType &y);
	static dataTypeSize_t IsBoardOk(dataType &x,WORD boardId); // return 0 if NOT, otherwise the NBytes of the TOTAL BOARD STREAM 
	static dataTypeSize_t IsBoardOk(void *v,int MaxN,WORD boardId); // if id==NULL don't check consistency for that id; MaxN is the space where it is safe to look into (allocated)
	static dataTypeSize_t IsEventOk(dataType &x); // return 0 if NOT, otherwise the NBytes of the TOTAL Event STREAM
	static inline dataTypeSize_t IsEventOk(void *v, int N){ dataType tmp(N,v);dataTypeSize_t R= IsEventOk(tmp); tmp.release(); return R;};
};

#include "interface/Daemon.hpp" // Command -- fwd decl

#endif
