#ifndef LOGGER_H
#define LOGGER_H

#include "interface/StandardIncludes.hpp"
#include "interface/Configurator.hpp"
#include "interface/AsyncUtils.hpp"

//class dataType;
class LogUtility;
class ConnectionManager;

class Logger: public Configurable, public AsyncUtils{
friend class LogUtility;
// this is actually a class that can be used to write on files. 
// 

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
bool binary_;
//bool async_;
long int counter_;
long int maxlines_;
long int logNumber_;
short logLevel_;

vector<LogUtility*> registered_;
// --- Register a LogUtility
void Register(LogUtility*u); 
// --- Release a LogUtility
void Release(LogUtility *u); 

ConnectionManager *logConnMan_;
int logStatusSck_;

public:
	// --- Constructor
	Logger();
	// --- Destructor
	~Logger();
	// --- Configure from a configurator
	void Config(Configurator &){};
	// --- Init Function
	void Init();
	// --- Binary
	inline void SetBinary(bool binary=true){binary_=binary;}
	// --- Set Compression of the log
	void SetCompress(bool compress=true);
	// --- Get Compression of the log
	inline bool GetCompress()const { return compress_;}
	// --- Set Asyncronous logging
	//void SetAsync(bool async=true) { async_=async;};
	// --- Set Max n. of lines in a log file
	inline void SetMaxLines(long n) { maxlines_=n;};
	// --- Set Log Level 0-> quite 1-> normal 3->verbose
	inline void SetLogLevel(short l) { logLevel_ = l ;}
	// --- Set FileName 
	inline void SetFileName(string name) {fileName_=name;}
	// --- Get FileName 
	inline string GetFileName() {return fileName_;}
	// --- Write a line to the log file -- low level
	void Write(string line, bool dryrun=false);
	// --- Write a DataType on file
	void Write(dataType &d, bool dryrun=false); // this requires Binary moed
	// --- Log 
	void Log(string& line,short level);
	// --- High level function for dumping
	void Dump(dataType &d);
	// --- Close files
	void Close();
	// --- Clear from configurator
	void Clear(){Close();}
        // --- Set network logging
        void ConfigLogConnManager(ConnectionManager *logConnMan, int logStatusSck){
	  logConnMan_=logConnMan;
	  logStatusSck_=logStatusSck;
	}
        // --- Write log on the network
        void NetworkWrite(string line);
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
	void Log(ostringstream s,short level){ Log(s.str(),level);}
	void LogInit(Logger *l);//{log=l;}
	// --- Disable logging
	void LogClear();
        inline Logger * GetLogger(){return log;} 
	inline void LogClearDirty(){log=NULL;}
	inline short int GetLogLevel(){ return log->logLevel_ ;}
        void ConfigLogConnManager(ConnectionManager *logConnMan, int logStatusSck){log->ConfigLogConnManager(logConnMan,logStatusSck);}
};

// put this here in order to define dataType. Cannot put before because in it it requires logger. Therefore we use a fwd declaration of dataType
//#include "interface/EventBuilder.hpp"
#include "interface/ConnectionManager.hpp"

#endif
