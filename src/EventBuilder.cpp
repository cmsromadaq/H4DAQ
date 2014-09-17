#include "interface/EventBuilder.hpp"

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

dataType::dataType(int N,void*v){
	// this constructor will not do malloc and take ownership of the stream
	dataStream_=v;
	size_=N;
	reserved_=N;
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


// ---------- Event Builder
EventBuilder::EventBuilder()
{
	dataStream_.reserve(1024); //reserve 1K for each stream
}


// ---- STATIC EVENT BUILDER
//

#define WORDSIZE 4

dataType EventBuilder::WordToStream(WORD x)
{
	dataType R; 
	R.reserve(WORDSIZE);
	//if( sizeof(x) < 4 ) // this is not C if word is at least int
	if (sizeof(x) > WORDSIZE)  // take the less 4 bit significant digits
	{ // hope never arrived here
		unsigned int i=1;
		char *c=(char*)&i;
		if(*c)// little endian
			{ // we don't want to reverse the content probably the number is already a cast to something
			R.append(&x,WORDSIZE);
			}
		else// big endian
			{
			c=(char*)&x;
			c+=sizeof(x)-WORDSIZE;
			R.append(&c,WORDSIZE);
			}
	}
	if (sizeof(x) == WORDSIZE)
	{
		R.append(&x,WORDSIZE);
	}	
	return R;
}

dataType EventBuilder::BoardHeader(WORD boardId)
{
	dataType R; R.reserve(2*WORDSIZE);
	R.append((void*)"BRDH\0\0\0\0\0\0\0\0",WORDSIZE); //WORDSIZE<12
	R.append(WordToStream(boardId) ); 
	return R;
}

dataType EventBuilder::BoardTrailer(WORD boardId)
{
	dataType R;R.reserve(WORDSIZE);
	R.append((void*)"BRDT\0\0\0\0\0\0\0\0",WORDSIZE); // WORDSIZE<12
	return R;
}


// [HEAD][Nbytes][ ----- ][TRAILER]
// [HEAD]="BRDH"+"BRDID" - WORD-WORD
dataType EventBuilder::BoardToStream(WORD boardId,vector<WORD> &v)
{
	dataType R; R.reserve(v.size()*4+12);// not important the reserve, just to avoid N malloc operations
	R.append( BoardHeader(boardId)   );
	WORD N= v.size()*WORDSIZE;
	R.append( WordToStream(N)  ); 
	for(int i=0;i<v.size();i++) R.append( WordToStream(v[i])  );
	R.append( BoardTrailer(boardId)   );

	return R;
}

dataType EventBuilder::EventHeader()
{
	dataType R; R.reserve(WORDSIZE);
	R.append((void*)"EVTH\0\0\0\0\0\0\0\0",WORDSIZE);
	return R;
}

dataType EventBuilder::EventTrailer()
{
	dataType R;R.reserve(WORDSIZE);
	R.append((void*)"EVNT\0\0\0\0\0\0\0\0",WORDSIZE);
	return R;
}

dataType EventBuilder::MergeEventStream(dataType &x,dataType &y){ //TODO
	// takes two streams and merge them independently from the content
}

vector<WORD>	EventBuilder::StreamToWord(void*v,int N){
	dataType myStream(N,v);
	vector<WORD> R = StreamToWord( myStream);
	myStream.release();
	return R;
}
vector<WORD>	EventBuilder::StreamToWord(dataType &x){
	vector<WORD> R;
	for(int n=0; n< x.size() ; n++)
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

long long EventBuilder::IsBoardOk(dataType &x,WORD boardId=0){

	dataType H=BoardHeader(boardId);
	dataType T=BoardTrailer(boardId);
	
	// Look in the Header for the 
	vector<WORD> Header  = StreamToWord( H );
	vector<WORD> Trailer = StreamToWord( T );
	if(x.size() < 3*WORDSIZE ) return 0;
	vector<WORD> myHead  = StreamToWord( x.data(), 3*WORDSIZE ); // read the first three
	
	if (myHead[0]  != Header[0] ) return 0;

	if( boardId !=0 )
	{
		if ( myHead.size()< 2 || boardId != myHead[1] ) return 0;
	}

	// the the N of bytes of the stream
	if (myHead.size() <3) return 0;
	WORD NBytes = myHead[2];
	WORD NWords  = NBytes / WORDSIZE;
	if( boardId !=0 )
	{
		// TODO Add a check on NWORDS for the specific BoardId
		// return 0;
	}

	if ( x.size() < (NWords+3+1)*WORDSIZE )  return 0;
	vector<WORD> myWords = StreamToWord( x.data(), (NWords+3+1)*WORDSIZE  ); //
	// ------------------|TRAILER POS| +1 
	if (myWords.size() < NWords + 3 + 1 ) return 0; // useless now
	//check trailer
	if (myWords[NWords+3] != Trailer[0] ) return 0;

	return (NWords+3+1)*WORDSIZE; // interesting size

}

long long EventBuilder::IsBoardOk(void *v,int MaxN,WORD boardId=0)
	{
	// take ownership of myStream (*v)
	dataType myStream(MaxN,v);
	long long R= IsBoardOk(myStream,boardId);
	// release ownership of myStream
	myStream.release();
	return R;
	}

long long EventBuilder::IsEventOk(dataType &x){
	char *ptr=(char*)x.data();
	vector<WORD> myHead=StreamToWord(x.data(),WORDSIZE*2); // read the first two WORDS
	dataType H=EventHeader();
	dataType T=EventTrailer();
	
	vector<WORD> Header=StreamToWord( H );
	vector<WORD> Trailer=StreamToWord( T );

	// check header
	if( myHead.size() <2 ) return 0;
	if( myHead[0] != Header[0] ) return 0;
	// header is fine
	WORD nBoard=myHead[1];

	long long leftsize=x.size() - WORDSIZE*2;
	ptr += WORDSIZE*2 ;
	for(WORD iBoard = 0 ; iBoard < nBoard ;iBoard++)
		{
		long long readByte=IsBoardOk(ptr, leftsize);
		if (readByte==0) return 0;
		leftsize -= readByte;
		ptr += readByte;
		}
	vector<WORD> myTrail=StreamToWord( ptr , WORDSIZE ) ;
	ptr += WORDSIZE;
	if ( myTrail[0] != Trailer[0] )  return 0;
	return ptr - (char*)x.data();
} 
