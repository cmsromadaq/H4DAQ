#include "interface/Profiler.hpp"

Profiler::Profiler(){
begin_=0;
end_=0;
clockBegin_=0;
clockEnd_=0;
}


Profiler::Start(){
	end_=-1;
	clockEnd_=-1;
	begin_=time(NULL);
	clockBegin_=clock();
}

Profiler::Stop(){
	end_=time(NULL);
	clockEnd_=clock();
}

int Profile::GetRealTime(){
	if (end_<0) return -1;
	return int( end_ - begin_ );
}
float Profile::GetCPUTime(){
	if(clockEnd<0) return -1.;
	return (float)(clockEnd_-clockBegin_))/CLOCKS_PER_SEC;
}
