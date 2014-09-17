#include "interface/StandardIncludes.hpp"
#include "interface/Handler.hpp" // class fork_exception ;

#ifndef ASYNC_H
#define ASYNC_H

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>

class AsyncUtils{
private:
protected:
	// --- take track of children processes
	static vector<pid_t> children;
	static int maxN;
	bool async_;
public:
	// --- Constructor
	AsyncUtils();
	// --- Destructor
	~AsyncUtils();
	// --- Get N of child process
	int GetN();
	// --- call the fork
	int Fork();
        // --- Set Asyncronous logging                                 
	void inline SetAsync(bool async=true) { async_=async;};      
};

#endif
