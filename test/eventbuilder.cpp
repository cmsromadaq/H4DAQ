// *** Original Author: Andrea Carlo Marini
// *** email: andrea.marini@cern.ch
// *** date: 10 Sep 2014


class Daemon;
#include "interface/Daemon.hpp"
#include "interface/FSM.hpp"
#include "interface/Handler.hpp"
#include "interface/EventBuilder.hpp"
#include "interface/Daemonize.hpp"

#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <getopt.h>


void print_usage() {
  printf("Usage: datareadout -v log_verbosity -l log_file -c config_file [-d]\n");
}

int main(int argc, char**argv)
{
/*
 *  this is the main loop of the controller.
 */
// define Handlers for sigint
define_handlers();

 int opt= 0;

 int verbosity=2;
 string logFileName="";
 string configFileName="";
 bool daemon=false;

 static struct option long_options[] = {
   {"verbosity", required_argument,       0,  'v' },
   {"log", required_argument,       0,  'l' },
   {"config",    required_argument, 0,  'c' },
   {"daemon",    optional_argument, 0,  'd' },
   {0,           0,                 0,  0   }
 };
 
 int long_index =0;
 while ((opt = getopt_long(argc, argv,"c:l:v:d", 
			   long_options, &long_index )) != -1) {
   switch (opt) {
   case 'v' : verbosity = atoi(optarg);
     break;
   case 'l' : logFileName=string(optarg);
     break;
   case 'c' : configFileName=string(optarg);
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

 if (logFileName == "" || configFileName == "" ) 
   {
     print_usage(); 
     exit(EXIT_FAILURE);
   }

/// --- 

if (daemon) Daemonize();

Logger l;
l.SetLogLevel(verbosity);
printf("[EventBuilderDaemon]::Init Logfile => %s\n",logFileName.c_str());
l.SetFileName(logFileName);
try
  {
    l.Init();
  }
 catch (logfile_open_exception &l)
   {
     printf("Cannot Open Log File: %s\n",logFileName.c_str());
     exit(EXIT_FAILURE);
   }
//
//Daemon *d=new Daemon();
//printf("Construct\n");
EventBuilderFSM *d=new EventBuilderFSM();
//printf("LogInit\n");
d->LogInit(&l);
//printf("Init\n");
printf("[EventBuilderDaemon]::Init Configfile => %s\n",configFileName.c_str());
try{
	d->Init(configFileName);
} catch ( std::exception &e ) { printf("%s\n",e.what()); exit(1); }
try{
  //	printf("Loop\n");
	d->Loop();
   }
   catch (sigint_exception &e) { printf("%s\n",e.what());}

d->Clear();
return 0;
}
