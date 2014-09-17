#include "interface/HwManager.hpp"


// --- Board
Board::Board(){id_=0;};
Board::~Board(){};
void Board::Init(){};
void Board::Clear(){};
void Board::Config(Configurator&c){};
void Board::Read(vector<WORD> &v){};
unsigned int Board::GetId(){return id_;};

// -------------------  HW Manager ---------------
//
// --- Constructor
HwManager::HwManager(){}
// --- Destructor
HwManager::~HwManager(){}
// --- Configure the HwManager
void HwManager::Config(Configurator &c){
	// take the configurator. Translate it into a Board config. Configure the board
}
// --- Init
void HwManager::Init(){
}
// --- Clear
void HwManager::Clear(){
	// --- reset to un-initialized/ un-config state	
}

void HwManager::Read(int i,vector<WORD> &v)
{
	v.clear();
	hw_[i]->Read(v);
	return;
}

dataType HwManager::ReadAll(){
	dataType R;
	R.append(EventBuilder::EventHeader());
	WORD M=hw_.size();
	R.append(EventBuilder::WordToStream(M));
	vector<WORD> v; 
	for(int i=0;i< hw_.size();i++)
	{
		hw_[i]->Read(v);
		R.append(EventBuilder::BoardToStream( hw_[i]->GetId(), v ) ) ;
	}
	R.append(EventBuilder::EventTrailer());
	return R;
}
