// *** Original Author: Andrea Carlo Marini
// *** email: andrea.marini@cern.ch
// *** date: 10 Sep 2014


#include "interface/ConnectionManager.hpp"
#include "interface/EventBuilder.hpp"
#include "interface/Utility.hpp"
#include <cstdio>
#include <cstdlib>
#include <time.h>

int main(int argc,char**argv)
{
 int port=5566;
 string address="127.0.0.1";
  
 printf("This program is going to test:\n");
 printf("   * the dataType consistency\n");
 printf("   * Port used for the communication is %d.\n",port);
 printf("   * Addressed used for the communication is '%s'.\n",address.c_str());
 printf("   * Logger.\n");
 printf("\n");

 Logger l;

 printf("-> Constructing Subscriber\n");
	 Subscriber sub;
 //printf("-> Constructing Context\n");
	// zmq::context_t context(2);

 // ActivateLog
 printf("-> Config Log\n");
	l.SetLogLevel(3);
 	//l.SetAsync();
 	l.SetFileName("/tmp/log_C.txt");
 	l.Init();
 printf("-> Activate Logs\n");
 	sub.LogInit(&l);
 //Configure
 printf("-> Setting port/address\n");
 	sub.SetAddress(address,port);
 //printf("-> Setting Context\n");
 	//sub.SetContext(&context);

 //pub.SetAsync();
 //Init Sockets
 printf("-> Init subscriber\n");
	sub.Init();

 //Init Mex
 printf("-> Init Mex\n");
	 dataType mexSend, mexRecv;
	 char mex[]="Ciao\0da\0Andrea";
 printf("-> Append to Send\n");
	 mexSend.append((void*)mex,14);

 printf("-> Recv Dummy\n");
 	int rc=1;
	while (rc) { rc=sub.RecvMessage(mexRecv); printf("Cycle Dummy\n"); sleep(1);}

 printf("-> Recv\n");
 	rc=1;
	while (rc) { rc=sub.RecvMessage(mexRecv); printf("Cycle\n"); sleep(2);}


 string toprint=Utility::AsciiData(mexRecv.data(),mexSend.size() );
 printf("%s",toprint.c_str());

 sleep(1);
 printf("-> Calling Destructors and exit\n");
 //sub.~Subscriber(); // destroy everything that use the context
}
