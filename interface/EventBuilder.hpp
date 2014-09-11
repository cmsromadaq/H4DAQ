#include <vector>
#include <map>
#include <exceptions>

using namespace std;

#include "TTree.h"

#ifndef EVTBLD_H
#define EVTBLD_H

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
	char *dataStream_;
	unsigned int size_; // store the used size of dataStream
	unsigned int reserved_; //store the actual dimension in memory of dataStream
public:
	// --- Constructor
	dataType();
	dataType(const char*);//init with a null terminated string
	dataType(int N, const char*);//init with a dataType
	// --- Destructor
	~dataType();
	// --- Get size
	unsigned int size(){return size_;}
	// --- reserve N bytes in memory
	void reserve(int N);
	// --- return stream pointer
	const char* c_str()(return (const char*)dataStream_;}
	// --- append
	// --- remove
	// --- clear
	void clear(){ delete dataStream_; size_=0;reserved_=0; dataStream_=NULL;}

};

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
	const char* GetStream(){ return dataStream_.c_str();}
	int  GetSize(){return dataStream_.size();}

};

#endif
