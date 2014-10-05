#include "interface/StandardIncludes.hpp"
// Daemon detach
int Daemonize(){

	pid_t pid=fork();
	if (pid >0 ){ // parent
		printf("[EventBuilderDaemon] Detaching process %d\n",pid);
		exit(0); //
		} 
	else if (pid== 0 ) { // child
		setsid(); // obtain a new group process
		//int fd = open("/dev/tty", O_RDWR);
		//ioctl(fd, TIOCNOTTY, NULL);
		// close all descriptors
		fflush(NULL);
		int i;
		for (i=getdtablesize();i>=0;--i) close(i); /* close all descriptors */
		i=open("/dev/null",O_RDWR); /* open stdin */
		dup(i); /* stdout */
		dup(i); /* stderr */
		}
	else {
		printf("[EventBuilderDaemon] Cannot Daemonize");
		return 1;
		}

	return 0;
}
