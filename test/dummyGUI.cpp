#include "StandardIncludes.hpp"
#include "interface/Utility.hpp"
#include "interface/ConnectionManager.hpp"




int main(int argc,char**argv){

string logFileName="/tmp/log_meridian.txt";
int port=5566;
string ConnectTo=argv[1];

 Logger l;

 printf("-> Constructing Publisher\n");
 Publisher pub;
 printf("-> Constructing Subscriber\n");
 Subscriber sub;

 printf("-> Config Log\n");
	l.SetLogLevel(3);
 	l.SetAsync();
 	l.SetFileName(logFileName);
 	l.Init();
 printf("-> Activate Logs\n");
 	pub.LogInit(&l);
 	sub.LogInit(&l);
 printf("-> Setting port\n");
 	pub.SetPort(port);
 printf("-> Setting Addr\n");
 	sub.SetAddress(ConnectTo,port);

 printf("-> Init Publisher\n");
 	pub.Init();
 printf("-> Init Subscriber\n");
 	sub.Init();
while (true){
 string mexStr;
 printf("-> Write your message\n:");
 getline(cin,mexStr);
 for(int i=0;i<mexStr.size();i++) if(mexStr[i]=='@') mexStr[i]='\0';
 printf("-> Init Mex\n");
 dataType mex;
 mex.append( (void*)mexStr.c_str(),mexStr.size() +1 ); // NULL TERMINATED
 printf("-> Send Dummy\n");
	pub.SendMessage(mex);
	mex.release();
dataType  mexRecv;
 	int rc=sub.RecvMessage(mexRecv);
	mexRecv.append( (void*)"\0", 1);
	if (rc==0 ) printf("Received Message %s\n",(char*)mexRecv.data());
	

}



}
