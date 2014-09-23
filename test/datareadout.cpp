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
define_handlers();

Logger l;
l.SetLogLevel(3);
l.SetFileName("/tmp/dataro.log");
l.Init();
//
//Daemon *d=new Daemon();
printf("Construct\n");
DataReadoutFSM *d=new DataReadoutFSM();
printf("LogInit\n");
d->LogInit(&l);
printf("Init\n");
d->Init("data/config.xml");
try{
	printf("Loop\n");
	d->Loop();
   }
   catch (sigint_exception &e) { printf("%s\n",e.what());}

d->Clear();
return 0;
}
