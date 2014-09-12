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

 printf("-> Constructing Publisher\n");
	 Publisher pub;
 printf("-> Constructing Context\n");
	 zmq::context_t context(2);

 // ActivateLog
 printf("-> Config Log\n");
	l.SetLogLevel(3);
 	//l.SetAsync();
 	l.SetFileName("/tmp/log.txt");
 	l.Init();
 printf("-> Activate Logs\n");
 	pub.LogInit(&l);
 //Configure
 printf("-> Setting port\n");
 	pub.SetPort(port);
 printf("-> Setting Context\n");
 	pub.SetContext(&context);

 //pub.SetAsync();
 //Init Sockets
 printf("-> Init Publisher\n");
 	pub.Init();

 printf("-> Sleep\n");
 sleep(10);
 //Init Mex
 printf("-> Init Mex\n");
	 dataType mexSend, mexRecv;
	 char mex[]="Ciao\0da\0Andrea";
 printf("-> Append to Send\n");
	 mexSend.append((void*)mex,14);

 printf("-> Send Dummy\n");
	pub.SendMessage(mexSend);

 printf("-> Send\n");
	 pub.SendMessage(mexSend);

 printf("-> Sleep\n");
 sleep(10);
 string toprint=Utility::AsciiData(mexSend.data(),mexSend.size() );
 printf("%s",toprint.c_str());

 sleep(1);
 printf("-> Calling Destructors and exit\n");
 return 0;
}
