#include "interface/StandardIncludes.hpp"
#include "interface/Configurator.hpp"

#ifndef LOGGER_H
#define LOGGER_H


class LogUtility;

class Logger: public Configurable{
friend class LogUtility;

// --- fileName where to log to
string fileName_;

// --- File Containers: I use the stdio in order to easily switch between gzip and normal mode
FILE *fw_;
#ifndef NO_ZLIB
	gzFile gw_; // this is actually a void*
#else
	void* gw_;
#endif

bool compress_;
bool async_;
long int counter_;
long int maxlines_;
long int logNumber_;
short logLevel_;

vector<LogUtility*> registered_;
// --- Register a LogUtility
void Register(LogUtility*u); 
// --- Release a LogUtility
void Release(LogUtility *u); 

public:
	// --- Constructor
	Logger();
	// --- Destructor
	~Logger();
	// --- Configure from a configurator
	void Config(Configurator &){};
	// --- Init Function
	void Init();
	// --- Set Compression of the log
	void SetCompress(bool compress=true);
	// --- Set Asyncronous logging
	void SetAsync(bool async=true) { async_=async;};
	// --- Set Max n. of lines in a log file
	void SetMaxLines(long n) { maxlines_=n;};
	// --- Set Log Level 0-> quite 1-> normal 3->verbose
	void SetLogLevel(short l) { logLevel_ = l ;}
	// --- Set FileName 
	void SetFileName(string name) {fileName_=name;}
	// --- Write a line to the log file
	void Write(string line, bool dryrun=false);
	// --- Log 
	void Log(string line,short level);
	// --- Close files
	void Close();
	// --- Clear from configurator
	void Clear(){Close();}
};

class LogUtility
{
	friend class Logger;
	// class that each stuff that should log inheriths from
	Logger *log;
	public:
	LogUtility(){log=NULL;}
	~LogUtility();//{log=NULL;} // dont delete it - disable logging
	void Log(string line,short level){ if(log!=NULL) log->Log(line,level);}
	void LogInit(Logger *l);//{log=l;}
	// --- Disable logging
	void LogClear();
	void LogClearDirty(){log=NULL;}
};

#endif
