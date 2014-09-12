#include "interface/StandardIncludes.hpp"


using namespace std;


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
	// --- remove [A,B)
	void erase(int A,int B);
	// --- clear
	void clear();
	// --- copy
	void copy(void * data, int N);
	// --- release ownership of data. This will lost control of data and location
	void release();

};

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

};

#endif
