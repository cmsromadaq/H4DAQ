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
	binary_=0;
}
// Destructor
Logger::~Logger(){ 
	// Disconnect Utilities
	for(int i=0;i<registered_.size();i++)
		{
		//printf("~Logger %p\n",registered_[i]); //DEBUG
		if(registered_[i]!=NULL)registered_[i]->LogClearDirty(); //remove utility from logging -- don't call LogUtility->Release; it's the clear after
		}
	registered_.clear();
	Close();
}
// Destructor
LogUtility::~LogUtility(){
	LogClear();
} // dont delete it - disable loggin
void LogUtility::LogInit(Logger *l){
	//register utility
	log=l;
	log->Register(this);
}
void LogUtility::LogClear(){
		if(log!=NULL)log->Release(this);
		log=NULL;
} //disable logging

//Register Log Utilities
void Logger::Register(LogUtility*u) { 
	//printf("Register %p i=%d\n",u,registered_.size()); //DEBUG
	registered_.push_back(u);
}
void Logger::Release(LogUtility *u) { 
	//printf("Releasing %p\n",u); //DEBUG
	int i=0;
	for(i=0;i<registered_.size();i++)
		{
		if ( registered_[i] == u ) 
			{ 
			//printf("Releasing Utility i=%d\n",i); //DEBUG
			registered_[i]=NULL;
			}
		}
}

void Logger::Init(){
	string fileName= fileName_;
	if (maxlines_ >0 && binary_) { maxlines_=-1; } // binary mode don't split. In future may count the bytes written
	if (maxlines_ >0)
		{
		++logNumber_ ;
		char numberStr[10];sprintf(numberStr,"_%ld",logNumber_);
		fileName +=  numberStr;
		}
	
	if (compress_){
		#ifndef NO_ZLIB
			if (binary_)gw_=gzopen(fileName.c_str(),"wb");
			else gw_=gzopen(fileName.c_str(),"w");
		#else
			throw no_zlib_exception();
		#endif
		}
	else {
		if(binary_)fw_=fopen(fileName.c_str(),"wb"); 
		else fw_=fopen(fileName.c_str(),"w"); 
		}
	return ;
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

void Logger::Write(dataType &d, bool dryrun){
	// implement the low level Dump. intended for binary
	if (maxlines_>=0) return; // can't count lines
	if ( compress_)
		{
		#ifndef NO_ZLIB
			gzwrite(gw_,d.data(),d.size());
			gzflush(gw_,Z_SYNC_FLUSH);
		#else
			throw no_zlib_exception();
		#endif
		}
	else    {
		if(!dryrun)fwrite(d.data(),1,d.size(),fw_);
		}
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

void Logger::Dump(dataType &d)
{
	if (async_){
   	pid_t childpid=Fork(); 
	if (childpid==0) //child
		{
		Write(d);
		_exit(0);
		}
	else if (childpid >0 ) {
		return; 
		}
	else  { Write(d); return; } // async off -- Async utils takes care of limits
	
	}
	else { Write(d) ; return;}
}

void Logger::Log(string line,short level){
// implement the high level logging for async operation
    if (level>logLevel_) return; // don't log unrequired ones
    if (async_) 
    {
	//fork copies the memories you write;
   	//pid_t childpid=fork(); 
   	pid_t childpid=Fork(); 
	if (childpid==0) //child
		{
		//printf("DEBUG Child in logger: mypid=%d\n",getpid());
		Write(line);
		_exit(0); // kill the child process, w/o flushing anything != Exit(); Exit is C++11 , exit is unistd
		//_Exit(0); // kill the child process, w/o flushing anything != Exit(); Exit is C++11 , exit is unistd
		//abort();
		}
	else if(childpid > 0 ) //parent
		{
		//printf("DEBUG Parent in logger %d parentpid=%d\n",childpid,getpid());
		return;
		}
	else // childpid <0 //parent but didn't fork - changed behaviour in AsyncUtils, -1 is also more than max
		{
		Write( line );
		//throw fork_exception();
		}
    }
    else Write(line);
    return;
}
