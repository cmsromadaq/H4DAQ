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
  	ref_date.tm_mon = 8; // start from 0 ?
  	ref_date.tm_mday = 29;
  	ref=mktime(&ref_date);

	return 0;
}
int TimeBoard::Read(vector<WORD> &v)
{
	v.clear();
	//WORD x=(WORD) ((unsigned)time(NULL) );
	timeval tv;
	gettimeofday(&tv,NULL);
	unsigned long x	= Utility::timestamp(&tv,&ref);
	unsigned long x_prime= x>>32;
	WORD x_msb = (( x_prime ) & 0xFFFFFFFF ); 
	WORD x_lsb = x & (0xFFFFFFFF);
	//printf("[TimeBoard]::[Read] %lu xprime=%lu %X %X\n",x,x_prime,x_lsb,x_msb); //DEBUG
	v.push_back(x_lsb);
	v.push_back(x_msb);
	return 0;
}

int TimeBoard::SetHandle(int x) {
	//nothing to set
	return 0;
}
