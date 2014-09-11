#include "interface/StandardIncludes.hpp"

#ifndef LOGGER_H
#define LOGGER_H

class Logger{

// --- fileName where to log to
string fileName_;

// --- File Containers: I use the stdio in order to easily switch between gzip and normal mode
FILE *fw_;
#ifndef NO_ZLIB
	gzFile *gw_;
#else
	void *gw_;
#endif

bool compress_;
bool async_;
long int counter_;
long int maxlines_;
long int logNumber_;

public:
	// --- Constructor
	Logger();
	// --- Destructor
	~Logger();
	// --- Configure from a configurator
	void Config();
	// --- Init Function
	void Init();
	// --- Set Compression of the log
	void SetCompress(bool compress=true);
	// --- Set Asyncronous logging
	void SetAsync(bool async=true) { async_=async};
	// --- Write a line to the log file
	void Write(string line, bool dryrun=false);
	// --- Log 
	void Log(string line);
	// --- Close files
	void Close();
};

#endif
