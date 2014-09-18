#include "interface/Utility.hpp"
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <fstream>
using namespace std;


//
#include "interface/Logger.hpp"
#include "interface/HwManager.hpp"

int main(int argc, char**argv){

string fileName="/tmp/configXXX.xml";
// print a configuratio
ofstream myFile; myFile.open(fileName.c_str() ) ;
myFile<< "<general>"<<endl;
myFile<< "   <Network>"<<endl;
myFile<< "      <ListenPort>5555</ListenPort>"<<endl;
myFile<< "      <ConnectTo>localhost:5555</ConnectTo><!-- can be more than one-->"<<endl;
myFile<< "   </Network>"<<endl;
myFile<< "   <Hardware>"<<endl;
myFile<< "      <board>"<<endl;
myFile<< "         <ID>0</ID>"<<endl;
myFile<< "         <type>CAEN</type>"<<endl;
myFile<< "      </board>"<<endl;
myFile<< "      <board>"<<endl;
myFile<< "         <ID>1</ID>"<<endl;
myFile<< "         <type>CAEN2</type>"<<endl;
myFile<< "      </board>"<<endl;
myFile<< "   </Hardware>"<<endl;
myFile<< "</general>"<<endl;
myFile.close();

Logger l;
	l.SetLogLevel(3);
	printf("Logging on file /tmp/log.txt\n");
	l.SetFileName("/tmp/log.txt");
	l.SetAsync();
	l.Init();

Configurator *config= new Configurator();
	config->xmlFileName=fileName;
	config->Init(); // load configurator

HwManager *hw=new HwManager();
	hw->LogInit(&l);
	hw->Log("Start Logging",0);
	hw->Config(*config);
	hw->Init();
	hw->Print();

printf("-- END --\n");
printf("-- XML\n");
system( ("cat "+ fileName).c_str());
printf("-- LOG\n");
system( "cat /tmp/log.txt");
system( ("rm "+ fileName).c_str() );
}
