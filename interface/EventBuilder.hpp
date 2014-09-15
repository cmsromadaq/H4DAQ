#include "interface/StandardIncludes.hpp"


using namespace std;


#ifndef EVTBLD_H
#define EVTBLD_H

typedef unsigned int WORD;

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

class EventBuilder {
// ---binary stream of the event -- 1char = 1byte
dataType dataStream_; // dynamic array of char*
	
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
	const void* GetStream(){ return dataStream_.c_str();}
	int  GetSize(){return dataStream_.size();}

	// ---  this will be used by hwmanager to convert the output of a board in a stream
	// ---  appends a header and trailer
	static dataType WordToStream(WORD x);
	static dataType BoardHeader(WORD boardId);
	static dataType BoardTrailer(WORD boardId);
	static dataType EventHeader();
	static dataType EventTrailer();
	static dataType BoardToStream(WORD boardId,vector<WORD> &v);
	static dataType MergeEventStream(dataType &x,dataType &y){};
};

#endif
