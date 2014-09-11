#include "interface/Daemon.hpp"
// --- Constructor 
Daemon::Daemon(){
	// get the pid of the current process
	pid_=getpid();
}
