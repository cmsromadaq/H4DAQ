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
 printf("   * the publisher and subscriber ability to handle it\n");
 printf("   * Port used for the communication is %d.\n",port);
 printf("   * Addressed used for the communication is '%s'.\n",address.c_str());
 printf("   * Logger.\n");
 printf("\n");

 Logger l;

 printf("-> Constructing Publisher\n");
	 Publisher pub;
 printf("-> Constructing Subscriber\n");
	 Subscriber sub;
 printf("-> Constructing Context\n");
	 zmq::context_t context(2);

 // ActivateLog
 printf("-> Config Log\n");
	l.SetLogLevel(3);
 	//l.SetAsync();
 	l.SetFileName("/tmp/log.txt");
 	l.Init();
 printf("-> Activate Logs\n");
 	sub.LogInit(&l);
 	pub.LogInit(&l);
 //Configure
 printf("-> Setting port\n");
 	pub.SetPort(port);
 printf("-> Setting port/address\n");
 	sub.SetAddress(address,port);
 printf("-> Setting Context\n");
 	pub.SetContext(&context);
 	sub.SetContext(&context);

 //pub.SetAsync();
 //Init Sockets
 printf("-> Init Publisher\n");
 	pub.Init();
 printf("-> Init subscriber\n");
	sub.Init();

 //Init Mex
 printf("-> Init Mex\n");
	 dataType mexSend, mexRecv;
	 char mex[]="Ciao\0da\0Andrea";
 printf("-> Append to Send\n");
	 mexSend.append((void*)mex,14);

 printf("-> Send Dummy\n");
	int rc=1;
	rc=sub.RecvMessage(mexRecv);
	pub.SendMessage(mexSend);
 printf("-> Recv Dummy\n");
	//while (rc) { rc=sub.RecvMessage(mexRecv); printf("Cycle\n"); sleep(1);}
	rc=sub.RecvMessage(mexRecv);

 printf("-> Send\n");
	 pub.SendMessage(mexSend);
 printf("-> Recv\n");
 	rc=1;
	while (rc) { rc=sub.RecvMessage(mexRecv); printf("Cycle\n"); sleep(2);}


 printf("-> Verify content of size %d == %d\n",mexSend.size(),mexRecv.size());
 if (mexSend == mexRecv)printf(" ------ SUCCESS -------\n");
 else printf(" ------ FAIL -------\n");

 // for(int i=0;i<mexSend.size();i++)
 // {
 //        char c1='\0';
 //        if(i<mexSend.size())c1=((char*)mexSend.data())[i];
 //        char c2='\0';
 //        if(i<mexRecv.size())c2=((char*)mexRecv.data())[i];
 //        printf(" %02X %02X\n",c1,c2);	
 // }
 string toprint=Utility::AsciiData(mexSend.data(),mexRecv.data(),mexSend.size() );
 printf("%s",toprint.c_str());

 sleep(1);
 printf("-> Calling Destructors and exit\n");
}
