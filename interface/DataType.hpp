#ifndef DATATYPE_H
#define DATATYPE_H

#define WORDSIZE 4
typedef unsigned long long dataTypeSize_t;

#include "interface/StandardIncludes.hpp"

typedef uint32_t WORD;


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

#endif
