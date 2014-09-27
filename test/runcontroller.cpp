// *** Original Author: Andrea Carlo Marini
// *** email: andrea.marini@cern.ch
// *** date: 10 Sep 2014


class Daemon;
#include "interface/Daemon.hpp"
#include "interface/FSM.hpp"
#include "interface/Handler.hpp"
#include "interface/EventBuilder.hpp"

#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <getopt.h>

void print_usage() {
  printf("Usage: runcontroller -c config_file\n");
}

int main(int argc, char**argv)
{
/*
 *  this is the main loop of the controller.
 */
// define Handlers for sigint
define_handlers();

 int opt= 0;

 string configFileName="";

 static struct option long_options[] = {
   {"config",    required_argument, 0,  'c' },
   {0,           0,                 0,  0   }
 };
 
 int long_index =0;
 while ((opt = getopt_long(argc, argv,"c:", 
			   long_options, &long_index )) != -1) {
   switch (opt) {
   case 'c' : configFileName=string(optarg);
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

//
//Daemon *d=new Daemon();
printf("RC\n");
RunControllerFSM *d=new RunControllerFSM();
printf("INIT\n");
//d->Init("data/configRC.xml");
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
