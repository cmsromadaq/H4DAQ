#include "interface/Logger.hpp"
#include "interface/Handler.hpp"

Logger::Logger()
{
	fileName_="log.txt";
	compress_=false;
	counter_=0;
	maxlines_=-1;
	gw_=NULL;
	fw_=NULL;
	logNumber_=-1;
	async_=false;
	logLevel_=1; // 0 -> no log, 1-> reasonable 3->everything
}
// Destructor
Logger::~Logger(){ 
	Log("[3] Logger::Destructor",3);
	Close();
}

void Logger::Init(){
	string fileName= fileName_;
	if (maxlines_ >0)
		{
		++logNumber_ ;
		char numberStr[10];sprintf(numberStr,"_%ld",logNumber_);
		fileName +=  numberStr;
		}
	
	if (compress_){
		#ifndef NO_ZLIB
			gw_=gzopen(fileName.c_str(),"w");
		#else
			throw no_zlib_exception();
		#endif
		}
	else {fw_=fopen(fileName.c_str(),"w"); }
}

void Logger::SetCompress(bool compress)
{
	#ifdef NO_ZLIB
		if (compress==true)
		{
			throw no_zlib_exception(); 
		} 
	#endif
	compress_=compress;
return;
}

void Logger::Write(string line, bool dryrun)
{
	// implement the low level logging
	if (maxlines_==0 ) return;
	
	line += "\n";
	if(compress_)
		{
		#ifndef NO_ZLIB
			if(!dryrun){
				gzwrite(gw_,line.c_str(),line.size());
				gzflush(gw_,Z_SYNC_FLUSH);
			}
		#else
			throw no_zlib_exception();
		#endif
		}
	else {
		if(!dryrun){
			fprintf(fw_,"%s",line.c_str());
			fflush(fw_);
			}
		}
	counter_++;
	if (maxlines_ >0 && counter_ > maxlines_)
		{
		Close();
		Init();
		}
	return;
}

void Logger::Close(){
	if(compress_)
		{
		#ifndef NO_ZLIB
			gzclose(gw_);
			gw_=NULL;
		#else
			throw no_zlib_exception();
		#endif
		}
	else 
		{
		fclose(fw_);
		fw_=NULL;
		}
	return;
}


void Logger::Log(string line,short level){
// implement the high level logging for async operation
    if (level>logLevel_) return; // don't log unrequired ones
    if (async_) 
    {
	//fork copies the memories you write;
   	pid_t childpid=fork(); 
	if (childpid==0) //child
		{
		printf("DEBUG Child in logger: mypid=%d\n",getpid());
		Write(line);
		_exit(0); // kill the child process, w/o flushing anything != Exit(); Exit is C++11 , exit is unistd
		//_Exit(0); // kill the child process, w/o flushing anything != Exit(); Exit is C++11 , exit is unistd
		//abort();
		}
	else if(childpid > 0 ) //parent
		{
		printf("DEBUG Parent in logger %d parentpid=%d\n",childpid,getpid());
		return;
		}
	else // childpid <0 //parent but didn't fork
		{
		Write( line );
		throw fork_exception();
		}
    }
    else Write(line);
    return;
}
