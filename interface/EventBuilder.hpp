#include "interface/StandardIncludes.hpp"


using namespace std;


#ifndef EVTBLD_H
#define EVTBLD_H
#include "interface/Logger.hpp"

class Command; // fwd decl

//typedef unsigned int WORD;
typedef uint32_t WORD;
#define WORDSIZE 4

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
	unsigned int size_; // store the used size of dataStream
	unsigned int reserved_; //store the actual dimension in memory of dataStream
public:
	// --- Constructor
	dataType();
	//dataType(const char*);//init with a null terminated string
	dataType(int N, void* );//init with a dataType
	// --- Destructor
	~dataType();
	// --- Get size
	unsigned int size(){return size_;}
	// --- reserve N bytes in memory
	void reserve(int N);
	// --- shrink N bytes in memory
	void shrink(int N);
	// --- return stream pointer
	const void* c_str(){return (const void*)dataStream_;}
	void* data(){
		return ( void*)dataStream_;
		}
	// --- append
	void append(void* data, int N);
	// --- append using dataType
	void append(dataType x){return append( x.data(), int(x.size()) ) ;};
	// --- remove [A,B)
	void erase(int A,int B);
	// --- clear
	void clear();
	// --- copy
	void copy(void * data, int N);
	// --- release ownership of data. This will lost control of data and location
	void release();
	//--- Operators
	//dataType& operator+=(dataType &x) { this->append(x); return this;}

};
// --- bool operator -- mv inside the class
const bool operator==(dataType &x, dataType &y);

class EventBuilder : public LogUtility, public Configurable{
// ---binary stream of the event -- 1char = 1byte
dataType mySpill_; // dynamic array of char*

bool dumpEvent_; // default true
bool sendEvent_; // default false
int recvEvent_; // if set to true will set also dumpEvent
Logger *dump_; // this is not the Logger. This will be used to dump the event, and will be set in binary mode.
string dirName_;
bool isSpillOpen_;
WORD lastSpill_;
WORD runNum_;

map<WORD,pair<int,dataType> > spills_; //store incomplete spills if in recv mode. SPILLNUM -> NMerged, SpillStream

	void MergeSpills(dataType &spill1,dataType &spill2 ); //TODO
	
public:
	/* this class contains the raw event.
	 * It performs simple operations, like merging all the event informations, with base checks on the counters
	 * In case of errors returns an exception that will trigger a reset of the counters
	 */
	// --- Constructor
	EventBuilder();
	// --- Destructor
	~EventBuilder();
	// 
	void AppendToStream();
	const void* GetStream(){ return mySpill_.c_str();}
	int  GetSize(){return mySpill_.size();}
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
	void SetRunNum(WORD x);//{ runNum_=x;}
	// ---  this will be used by hwmanager to convert the output of a board in a stream
	// ---  appends a header and trailer
	static dataType WordToStream(WORD x);
	static vector<WORD> StreamToWord(dataType &x);
	static vector<WORD> StreamToWord(void*v,int N);
	static WORD 	ReadSpillNum(dataType &x);
	static WORD 	ReadRunNumFromSpill(dataType &x);
	static WORD 	ReadSpillNevents(dataType &x);
	static WORD 	ReadEventNboards(dataType &x);
	static dataType BoardHeader(WORD boardId);
	static dataType BoardTrailer(WORD boardId);
	static dataType EventHeader();
	static dataType EventTrailer();
	static dataType SpillHeader();
	static dataType SpillTrailer();
	static dataType BoardToStream(WORD boardId,vector<WORD> &v);
	static dataType MergeEventStream(dataType &x,dataType &y);
	static long long IsBoardOk(dataType &x,WORD boardId); // return 0 if NOT, otherwise the NBytes of the TOTAL BOARD STREAM 
	static long long IsBoardOk(void *v,int MaxN,WORD boardId); // if id==NULL don't check consistency for that id; MaxN is the space where it is safe to look into (allocated)
	static long long IsEventOk(dataType &x); // return 0 if NOT, otherwise the NBytes of the TOTAL Event STREAM
	static inline long long IsEventOk(void *v, int N){ dataType tmp(N,v);long long R= IsEventOk(tmp); tmp.release(); return R;};
};

#include "interface/Daemon.hpp" // Command -- fwd decl

#endif
