#include "interface/Logger.hpp"

Logger::Logger()
{
	fileName_="log.txt"
	compress_=false;
	counter_=0;
	maxlines_=-1;
	gw_=NULL;
	fw_=NULL;
	logNumber_=-1;
	async_=false;
}

Logger::Init(){
	string fileName= fileName_;
	if (maxlines_ >0)
		{
		++logNumber_ ;
		fileName += "_" + string(logNumber_);
		}
	
	if (compress){
		#ifndef NO_ZLIB
			gw_=gzopen(fileName.c_str(),"w");
		#else
			throw no_zlib;
		#endif
		}
	else {fw_=fopen(fileName.c_str(),"w"); }
}

Logger::SetCompress(bool compress)
{
	#ifdef NO_ZLIB
		if (compress==true)
		{
			throw no_zlib; 
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
	if(compress)
		{
		#ifndef NO_ZLIB
			if(!dryrun)gzwrite(gw_,line.c_str(),line.size());
		#else
			throw no_zlib;
		#endif
		}
	else {
		if(!dryrun)fprintf(fw_,line.c_str());
		}
	counter_++;
	if (maxlines_ >0 && counter_ > maxlines_)
		{
		Close();
		Init();
		}
	return;
}

Logger::Close(){
	if(compress)
		{
		#ifndef NO_ZLIB
			gzclose(gw_);
			gz_=NULL;
		#else
			throw no_zlib;
		#endif
		}
	else 
		{
		fclose(fw_);
		fw_=NULL;
		}
	return;
}


void Logger::Log(string line){
// implement the high level logging for async operation
    if (async_) 
    {
	//fork copies the memories you write;
   	pid_t mypid=fork(); 
	if (mypid==0) //child
		{
		Write(line);
		_exit(0); // kill the child process, w/o flushing anything != Exit(); Exit is C++11 , exit is unistd
		}
	else if(mypid > 0 ) //parent
		{
		return;
		}
	else // mypid <0 //parent but didn't fork
		{
		Write( line );
		throw forkError;
		}
    }
    else Write(line);
    return;
}
