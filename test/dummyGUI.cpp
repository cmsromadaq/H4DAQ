#include "StandardIncludes.hpp"
#include "interface/Utility.hpp"
#include "interface/ConnectionManager.hpp"




int main(int argc,char**argv){

string logFileName="/dev/stdout";
 int portPub=5566;
 int portSub=6002;
string ConnectTo="pcethtb2";

printf("usage: PubPort SubAddress SubPort\n    @->'0'\n    !XXXX->NUM\n");

if(argc>1)
	portPub=atoi(argv[1]);
if(argc>2)
	ConnectTo=argv[2];
if(argc>3)
	portSub=atoi(argv[3]);


 Logger l;

 printf("-> Constructing Publisher\n");
 Publisher pub;
 printf("-> Constructing Subscriber\n");
 Subscriber sub;

 printf("-> Config Log\n");
	l.SetLogLevel(3);
 	l.SetAsync();
 	l.SetFileName(logFileName);
 printf("-> Activate Logs\n");
try{
 	l.Init();
	pub.LogInit(&l);
 	sub.LogInit(&l);
} catch ( std::exception &e) { printf("%s\n Trying to continue\n",e.what());};
 printf("-> Setting port\n");
 	pub.SetPort(portPub);
 printf("-> Setting Addr\n");
 	sub.SetAddress(ConnectTo,portSub);

 printf("-> Init Publisher\n");
 	pub.Init();
 printf("-> Init Subscriber\n");
 	sub.Init();
while (true){
 string mexStr;
 printf("-> Write your message\n:");
 getline(cin,mexStr);
 for(int i=0;i<mexStr.size();i++) if(mexStr[i]=='@') mexStr[i]='\0';
 for(int i=0;i<mexStr.size();i++) if(mexStr[i]=='!') {
	 string myNumStr=mexStr.substr(i+1,4);
	 WORD myNum=atoi(myNumStr.c_str());
	 mexStr.erase(i); // delete '!'
	 mexStr[i  ]= ((char*) (&myNum))[0];
	 mexStr[i+1]= ((char*) (&myNum))[1];
	 mexStr[i+2]= ((char*) (&myNum))[2];
	 mexStr[i+3]= ((char*) (&myNum))[3];
	 i=i+3; // fast-forward
 	}
// printf("-> Init Mex\n");
 dataType mex;
 mex.append( (void*)mexStr.c_str(),mexStr.size() +1 ); // NULL TERMINATED
 //printf("-> Send Dummy\n");
	pub.SendMessage(mex);
	mex.release();
dataType  mexRecv;
 	int rc=sub.RecvMessage(mexRecv);
	mexRecv.append( (void*)"\0", 1);
	if (rc==0 ) printf("Received Message %s\n",(char*)mexRecv.data());
	

}



}
