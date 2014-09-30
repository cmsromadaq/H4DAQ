#include "StandardIncludes.hpp"
#include "interface/Utility.hpp"
#include "interface/ConnectionManager.hpp"




int main(int argc,char**argv){

string logFileName="/tmp/log.txt";
 int port=5566;

 Logger l;

 printf("-> Constructing Publisher\n");
 Publisher pub;

 printf("-> Config Log\n");
	l.SetLogLevel(3);
 	l.SetAsync();
 	l.SetFileName(logFileName);
 	l.Init();
 printf("-> Activate Logs\n");
 	pub.LogInit(&l);
 printf("-> Setting port\n");
 	pub.SetPort(port);

 printf("-> Init Publisher\n");
 	pub.Init();
while (true){
char buf[1024];
 printf("-> Write your message\n:");
 scanf("%s",buf);
 printf("-> Init Mex\n");
 string mexStr=buf;
 dataType mex;
 mex.append( (void*)mexStr.c_str(),mexStr.size() +1 ); // NULL TERMINATED
 printf("-> Send Dummy\n");
	pub.SendMessage(mex);
}



}
