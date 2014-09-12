#include "interface/Profiler.hpp"

Profiler::Profiler(){
	begin_=0;
	end_=0;
	clockBegin_=0;
	clockEnd_=0;
}


void Profiler::Start(){
	end_=0;
	clockEnd_=0;
	begin_=time(NULL);
	clockBegin_=clock();
}

void Profiler::Stop(){
	end_=time(NULL);
	clockEnd_=clock();
}

int Profiler::GetRealTime(){
	if (end_==0) return -1;
	return int( end_ - begin_ );
}
float Profiler::GetCPUTime(){
	if(clockEnd_==0) return -1.;
	return ((float)(clockEnd_-clockBegin_))/CLOCKS_PER_SEC;
}
