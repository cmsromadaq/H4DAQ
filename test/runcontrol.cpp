// *** Original Author: Andrea Carlo Marini
// *** email: andrea.marini@cern.ch
// *** date: 10 Sep 2014


class Daemon;
#include "interface/Daemon.hpp"
#include "interface/FSM.hpp"
#include "interface/Handler.hpp"
#include "interface/EventBuilder.hpp"
#include "interface/Logger.hpp"

#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <getopt.h>

void print_usage() {
  printf("Usage: runcontroller -c config_file -l log_file -v verbosity [-d]\n");
}

int main(int argc, char**argv)
{
/*
 *  this is the main loop of the controller.
 */
// define Handlers for sigint
define_handlers();

 int opt= 0;

 int verbose=1;
 string configFileName="";
 string logFileName="";
 bool daemon=false;

 static struct option long_options[] = {
   {"config",    required_argument, 0,  'c' },
   {"log",    required_argument, 0,  'l' },
   {"verbose",    required_argument, 0,  'v' },
   {"daemon",    optional_argument, 0,  'd' },
   {0,           0,                 0,  0   }
 };
 
 int long_index =0;
 while ((opt = getopt_long(argc, argv,"c:l:v:", 
			   long_options, &long_index )) != -1) {
   switch (opt) {
   case 'c' : configFileName=string(optarg);
     break;
   case 'l' : logFileName=string(optarg);
     break;
   case 'v' : verbose=atoi(optarg);
     break;
   case 'd' : daemon=true;;
     break;
   case '?':
     /* getopt_long already printed an error message. */
     print_usage(); 
     exit(EXIT_FAILURE);
   default: print_usage(); 
     exit(EXIT_FAILURE);
   }
 }

 if ( configFileName == "" ) 
   {
     print_usage(); 
     exit(EXIT_FAILURE);
   }
// Daemon detach
if(daemon){
	pid_t pid=fork();
	if (pid >0 ){ // parent
		printf("[RunControlDaemon] Detaching process %d\n",pid);
		_exit(0);
		} 
	else if (pid== 0 ) { // child
		}
	else printf("[EventBuilderDaemon] Cannot Daemonize");
	}
// -----------------
Logger l;

try
  {
    printf("[RunControllerDaemon]::Init Logfile => %s\n",logFileName.c_str());
    l.SetLogLevel(verbose);
    l.SetFileName(logFileName);
    l.Init();
  }
 catch (logfile_open_exception &l)
   {
     printf("Cannot Open Log File: %s\n",logFileName.c_str());
     exit(EXIT_FAILURE);
   }

//
//Daemon *d=new Daemon();
printf("RC\n");
RunControlFSM *d=new RunControlFSM();
printf("INIT\n");
//d->Init("data/configRC.xml");
d->LogInit(&l);
printf("[RunControllerDaemon]::Init LogLevel=%d\n",d->GetLogLevel() ) ;
printf("[RunControllerDaemon]::Init Configfile => %s\n",configFileName.c_str());
d->Init(configFileName);
try{
	printf("LOOP\n");
	d->Loop();
   }
   catch (sigint_exception &e) { printf("%s\n",e.what());}

printf("CLEAR\n");
d->Clear();
return 0;
}
