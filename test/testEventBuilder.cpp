// *** Original Author: Andrea Carlo Marini
// *** email: andrea.marini@cern.ch
// *** date: 10 Sep 2014


#include "interface/EventBuilder.hpp"
#include "interface/Logger.hpp"
#include "interface/Utility.hpp"
#include <cstdio>
#include <cstdlib>
#include <time.h>
#include <iostream>
using namespace std;
using namespace Utility;



void PrintTest( )
{
printf("[%c[01;31mX%c[00m]",033,033);
}
void PrintOk( )
{
printf("%c%c%c[%c[01;32mX%c[00m]\n",8,8,8,033,033);
}


int main(int argc,char**argv)
{
Logger l;
printf("-> Configure Logger" ) ; PrintTest();
	l.SetLogLevel(3);
	l.SetFileName("/tmp/log.txt");
	l.Init();
PrintOk();


printf("-> Constructing Event Builder" ) ; PrintTest();
EventBuilder *e=new EventBuilder();
PrintOk();

printf("->Init Logger") ; PrintTest();
e->LogInit(&l);
PrintOk();

printf("-> Configuring event Builder"); PrintTest();
e->ResetSpillNumber();
e->SetDumpEvent();
e->SetRecvEvent(0);
e->SetSendEvent(false);
e->SetDirName("/tmp/");
e->SetDumpCompress(false);
//e->SetDumpCompress();
PrintOk();

printf("-> Init"); PrintTest();
e->Init();
PrintOk();

printf("-> Set Run Number"); PrintTest();
e->SetRunNum(111);
PrintOk();

printf("-> Open Spill"); PrintTest();
e->OpenSpill();
PrintOk();

string myEvent;
//  SPILL HAS A RESERVE 1024 // HWManager::ReadALl
for( int i=0;i< 30000;i++){
	printf("-> Create Event i=%d ",i); 
	dataType event; 
	EventBuilder::EventHeader(event);
	
	vector<WORD> board; board.push_back(1); board.push_back(2);
	WORD nB=2; // nBoards == 1Board
	WORD zero=0;
	WORD uno=1;
	WORD due=1;
	EventBuilder::WordToStream(event,nB);
	EventBuilder::BoardToStream(event,uno,board);
	EventBuilder::BoardToStream(event,due,board);

	EventBuilder::EventTrailer(event);
	
	// Add To Spill And update Counters	
	e->AddEventToSpill(event);
	
	//myEvent=Utility::AsciiDataReadable(event.data(),event.size());	
	cout<< "MY SIZE IS: "<<e->GetSize()<<endl;
	}

printf("-> Close Spill"); PrintTest();
e->CloseSpill();
PrintOk();

sleep(1);

printf("DEBUG 1 EVENT:\n%s\n----\n",myEvent.c_str());


//FILE *fr=fopen("/tmp/111/101.txt","rb"); //  try to read it
//char c;
//dataType mySpill;
//while ( fread(&c,1,1,fr) >0 )mySpill.append( (void*)&c,1);
//fclose(fr);
//printf("MySpill Readable: %d\n%s\n----\n",int(mySpill.size()),Utility::AsciiDataReadable(  mySpill.data(),mySpill.size()).c_str()   );
//
//PrintTest();
//printf("-> Calling Destructors and exit");
//PrintOk();
}
