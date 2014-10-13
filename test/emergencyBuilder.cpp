#include "interface/StandardIncludes.hpp"
#include "interface/Utility.hpp"
#include "interface/EventBuilder.hpp"
#include "interface/FindMatch.hpp"
#include "interface/dataType.hpp"

int main() 
{
 // This is an emerency program for merging two spills 
//
printf("Usage ./bin/... file1 file2 outfile. Please make me configurable.\n");
 
FindMatch A;
A.SetMaxWindow(1); 

vector<uint64_t> time1,time2,time3;

string fileName1=argv[1];
string fileName2=argv[2];
string outFileName=argv[3];

dataType file1;
dataType file2;

dataType mySpill;

// Read Files and move them into datatype
//h
FILE *fr1= fopen(fileName1.c_str(),"rb");
FILE *fr2= fopen(fileName2.c_str(),"rb");

char c;
while ( fscanf(fr1,"%c",&c) != EOF ) file1.append((void*)&c,1);
while ( fscanf(fr2,"%c",&c) != EOF ) file2.append((void*)&c,1);

fclose(fr1);
fclose(fr2);

// get list of times
// go trough
	WORD runNum1 = ReadRunNumFromSpill(spill1);
	WORD runNum2 = ReadRunNumFromSpill(spill2);

	WORD spillNum1 = ReadSpillNum(spill1);
	WORD spillNum2 = ReadSpillNum(spill2);

	WORD spillNevents1 = ReadSpillNevents(spill1);
	WORD spillNevents2 = ReadSpillNevents(spill2);

	WORD zero=0; //spillSize

	if (runNum1 != runNum2) { printf("Different Run Number: I will proceed using the first.\n"); return 1; } // 
	if (spillNum1 != spillNum2){ printf("Different SpillN: I will proceed using the first.\n");return 1;} 
	if (spillNevents1 != spillNevents2){printf("Different n. event in spill. I will try to match.\n"); return 1; }



	// navigate through to find the numbers and times
	char *ptr1 = (char*)file1.data();
	char *ptr2 = (char*)file2.data();
	long long left1 = file1.size() - WORDSIZE*SpillHeaderWords();
	long long left2 = file2.size() - WORDSIZE*SpillHeaderWords();

	// save here the positions of the events
	vector<char*> myEvents1;
	vector<long>  myEventSize1;

	vector<char*> myEvents2;
	vector<long>  myEventSize2;

	for(unsigned long long iEvent=0;iEvent< spillNevents1 ;iEvent++)
	{
		long eventSize1= IsEventOk(ptr1,left1);
		if (eventSize1 == 0 ) return 2;
		WORD tB=(WORD)_TIME_;
		WORD bId1 = * (WORD*)(ptr1+ (EventTimePos()-2)*WORDSIZE );
		if ( (bId1 &GetBoardTypeIdBitMask () ) >> GetBoardTypeShift() != tB )  return 3;
		uint64_t mytime1 = *( (uint64_t*) (ptr1 + EventTimePos()*WORDSIZE)   );  // carefull to parenthesis
		time1.push_back(mytime1);
		myEvents1.push_back(ptr1);
		myEventSize1.push_back(eventSize1);
		ptr1+= eventSize1;
		left1-=eventSize1;
	}
	for(unsigned long long iEvent=0;iEvent< spillNevents2 ;iEvent++)
	{
		long eventSize2= IsEventOk(ptr2,left2);
		if (eventSize2 == 0 ) return 2;
		WORD tB=(WORD)_TIME_;
		WORD bId2 = * (WORD*)(ptr2+ (EventTimePos()-2)*WORDSIZE );
		if ( (bId2 &GetBoardTypeIdBitMask () ) >> GetBoardTypeShift() != tB )  return 3;
		uint64_t mytime2 = *( (uint64_t*) (ptr2 + EventTimePos()*WORDSIZE)   );  // carefull to parenthesis
		time2.push_back(mytime2);
		myEvents2.push_back(ptr2);
		myEventSize2.push_back(eventSize2);
		ptr2+= eventSize2;
		left2-=eventSize2;
	}

	// Run Matching Algo
	A.SetTimes(time1,time2);
	timeval tv_start; 
	gettimeofday(&tv_start,NULL);
	int status=A.Run();
	if (status >0 ) return status+100;
	timeval tv_stop;
	gettimeofday(&tv_stop,NULL);

	printf("     Converged=%d\n", int(A.Converged()) ) ;
	if( !A.Converged() ) {printf("Not Converged\n");return 200;}

	vector<pair<uint_t,uint_t> > matched=A.GetMatch(); // positions in time1/time2

	

	dataType H;EventBuilder::SpillHeader(H);
	dataType T;EventBuilder::SpillTrailer(T);

	mySpill.append(H);
	mySpill.append((void*)&runNum1,WORDSIZE); // 
	mySpill.append((void*)&spillNum1,WORDSIZE); // 
	mySpill.append( (void*)&zero, WORDSIZE); // spillSize
	mySpill.append((void*)&spillNevents1,WORDSIZE);
	for(unsigned int iEvent=0; iEvent< matched.size() ;++iEvent)
	{
		dataType event1(myEventSize1[matched[iEvent].first],myEvent1[matched[iEvent].first]);
		dataType event2(myEventSize2[matched[iEvent].second],myEvent2[matched[iEvent].scond]);
		dataType myEvent;
		EventBuilder::MergeEventStream(myEvent,event1,event2);
		event1.release();
		event2.release();
		mySpill.append(myEvent);
	}
	// trailer
	mySpill.append(T);
	/// update size
	WORD *spillSizePtr= ((WORD*) mySpill.data() )+ EventBuilder::SpillSizePos();
	(*spillSizePtr)=(WORD)mySpill.size();

	// write on disk
	FILE *fw=fopen(outFile.c_str(),"wb");
	fwrite(mySpill.data(),1,mySpill.size(),fw);
	fflush(fw);
	fclose(fw);
}
