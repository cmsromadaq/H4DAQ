#include "interface/EventBuilder.hpp"

EventBuilder::EventBuilder()
{
	dataStream_.reserve(1024); //reserve 1K for each stream
}



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
