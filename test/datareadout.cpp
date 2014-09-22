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
//
//Daemon *d=new Daemon();
DataReadoutFSM *d=new DataReadoutFSM();
d->Init("data/config.xml");
try{
	d->Loop();
   }
   catch (sigint_exception &e) { printf("%s\n",e.what());}

d->Clear();
return 0;
}
