#include "interface/DataType.hpp"

// --- Implemantation of class dataType: a stream of data of size N
const bool operator==(dataType &x, dataType &y)
	{
	if (x.size() != y.size() ) return false;
	for(dataTypeSize_t i=0;i<x.size();i++)
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

dataType::dataType(dataTypeSize_t N,void*v){
	// this constructor will not do malloc and take ownership of the stream
	dataStream_=v;
	size_=N;
	reserved_=N;
}

	// --- Destructor
dataType::~dataType(){clear();}
	// --- reserve N bytes in memory
void dataType::reserve(dataTypeSize_t N)
	{
	if (reserved_ >= N) return; // nothing to be done
	// allocate new space of size N
	void *newStream=new char[N];
	//void *newStream=malloc(N);
	if(newStream == NULL ) {
		cout <<" BAD ALLOC !!! "<<endl;
		throw std::bad_alloc(); //mem fail
		}
	//copy content to new residence
	memcpy(newStream,dataStream_,size_);
	// release old memory
	//free (dataStream_);
	delete [] (char*)dataStream_;
	//update pointers
	dataStream_=newStream;
	reserved_=N;
	return;
	}
void dataType::shrink(dataTypeSize_t N){
	if(size_>=N) return; // bad call
	if(reserved_<=N) return; //nothing to be done
	void *newStream=new char[N];
	//void*newStream=malloc(N);
	if(newStream == NULL ) throw std::bad_alloc(); //mem fail
	memcpy(newStream,dataStream_,size_);
	delete [] (char*)dataStream_;
	//free(dataStream_);
	//update pointers
	dataStream_=newStream;
	reserved_=N;
	return;
	}
// --- append
void dataType::append(void* data, dataTypeSize_t N){
	dataTypeSize_t tot=N+size_;
	if(tot>0 && reserved_==0) reserve(tot);
	if(tot == 0 ) cout<<"[DataType]::[append] TOT IS 0 "<<endl; // TODO something
	while (reserved_<tot){
		reserve(2*reserved_); // this avoids the +1, +1 ...
		}
	char * ptr=(char*) dataStream_;
	ptr += size_;
	memcpy(ptr,data,N); 
	size_=tot;
	}
	// --- remove
void dataType::erase(dataTypeSize_t A,dataTypeSize_t B){
	if( B > size_) B=size_;
	if( A >= B ) return;
	// 0....A---B....N-1
	for(dataTypeSize_t i=A ; B+i-A<size_ ; ++i )	
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
	//if(dataStream_!=NULL)free(dataStream_); 
	dataStream_=NULL;
	//printf("Clearing dataType:DONE\n"); //DEBUG
	}

void dataType::copy(void* data,dataTypeSize_t N){
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
