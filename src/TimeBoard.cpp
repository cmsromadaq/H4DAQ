#include "interface/TimeBoard.hpp"


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
	return 0;
}
int TimeBoard::Read(vector<WORD> &v)
{
	v.clear();
	WORD x=(WORD) ((unsigned)time(NULL) );
	v.push_back(x);
	return 0;
}

int TimeBoard::SetHandle(WORD x) {
	//nothing to set
	return 0;
}
