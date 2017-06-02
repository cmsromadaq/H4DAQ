#ifndef STANDARD_INCLUDES_H
#define STANDARD_INCLUDES_H
// GLOBAL OBJECTS 
// NO_ROOT, NO_ZMQ, NO_ZLIB

// STL
#include <string>
#include <sstream>
#include <vector> //LIFO -> use stack
#include <queue> //FIFO
#include <map>
#include <algorithm>
#include <exception>
#include <iostream>
using namespace std;

//C
#include <csignal>
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <ctime>
#include <unistd.h> //fork exit
#include <sys/types.h> //pid_t
#include <sys/time.h> //pid_t
#include <fcntl.h> // for O_...

#ifdef __linux__
#define LINUX
#endif

#ifndef NO_CAEN
	#include "CAENVMElib.h"
	#include "CAENVMEtypes.h" 
	#include "CAENVMEoslib.h"
	#include "CAENComm.h"
	#include <CAENDigitizer.h>
#endif

/// XML
#ifndef NO_XML
	#include <libxml/parser.h>
	#include <libxml/tree.h>
#endif

#ifndef NO_ZMQ
//ZQM
	#include <zmq.hpp>
	//#include <zmq_utils.h>
#endif

#ifndef NO_ROOT
//ROOT
#include "TTree.h"
#include "TFile.h"
#include "TString.h"
#endif

#ifndef NO_ZLIB
	#include <zlib.h>
#endif


// --------------- MY LIB ------------
// Fwd declarations
class dataType;
class Command;
//
#include "interface/DataType.hpp"
#include "interface/Utility.hpp"
#include "interface/Handler.hpp" // class fork_exception ;
#include "interface/Command.hpp"
#include "interface/CommonTypes.hpp"

#endif
