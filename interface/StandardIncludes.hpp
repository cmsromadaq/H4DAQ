// GLOBAL OBJECTS 
// NO_ROOT, NO_ZMQ, NO_ZLIB

// STL
#include <string>
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

#ifdef __linux__
#define LINUX
#endif

#include "CAENVMElib.h"
#include "CAENVMEtypes.h" 
#include "CAENVMEoslib.h"

#include <CAENDigitizer.h>

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
#endif

#ifndef NO_ZLIB
	#include <zlib.h>
#endif
