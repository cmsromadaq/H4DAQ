// *** Original Author: Andrea Carlo Marini
// *** email: andrea.marini@cern.ch
// *** date: 10 Sep 2014


class Daemon;
#include "interface/Daemon.hpp"
#include "interface/FSM.hpp"
#include "interface/Handler.hpp"
#include "interface/EventBuilder.hpp"

#include <cstdio>

int main(int argc, char**argv)
{
/*
 *  this is the main loop of the controller.
 */
// define Handlers for sigint
printf("HANDLERS\n");
define_handlers();
//
//Daemon *d=new Daemon();
printf("RC\n");
RunControllerFSM *d=new RunControllerFSM();
printf("INIT\n");
//d->Init("data/configRC.xml");
d->Init(argv[1]);
try{
	printf("LOOP\n");
	d->Loop();
   }
   catch (sigint_exception &e) { printf("%s\n",e.what());}

printf("CLEAR\n");
d->Clear();
return 0;
}
