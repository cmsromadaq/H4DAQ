#include "interface/TimeBoard.hpp"
#include "interface/Utility.hpp"

TimeBoard::TimeBoard(): Board() {
	type_ = string("TIME") ;
}

int TimeBoard::Init(){
	// nothing to init
	return 0;
}

int TimeBoard::Clear(){
	//nothing to Clear
	return 0;
}

int TimeBoard::Print(){
	cout<<"Time Board: Current time is"<<(unsigned)time(NULL)<<endl;
	return 0;

}

int TimeBoard::Config(BoardConfig *bC){
	Board::Config(bC);	
	//to be configured
	tm ref_date;
  	ref_date.tm_hour = 0;
  	ref_date.tm_min = 0;
  	ref_date.tm_sec = 0;
  	ref_date.tm_year = 114;
  	ref_date.tm_mon = 9;
  	ref_date.tm_mday = 1;
  	ref=mktime(&ref_date);

	return 0;
}
int TimeBoard::Read(vector<WORD> &v)
{
	v.clear();
	//WORD x=(WORD) ((unsigned)time(NULL) );
	timeval tv;
	gettimeofday(&tv,NULL);
	long x	= Utility::timestamp(&tv,&ref);
	WORD x_msb = x>>32; 
	WORD x_lsb = x & (0xFFFFFFFF);
	v.push_back(x_lsb);
	v.push_back(x_msb);
	return 0;
}

int TimeBoard::SetHandle(int x) {
	//nothing to set
	return 0;
}
