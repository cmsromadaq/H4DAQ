#include "interface/Board.hpp"

//unsigned int Board::GetId(){return id_;};
// --- Board
Board::Board() : LogUtility() {id_=0;bC_=NULL;};
Board::~Board(){};
int Board::Config(BoardConfig *bC){
	bC_=bC;
	return 0;
};
