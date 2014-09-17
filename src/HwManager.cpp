#include "interface/HwManager.hpp"


// --- Board
Board::Board(){id_=0;bC_=NULL;};
Board::~Board(){};
int Board::Config(BoardConfig *bC){
	bC_=bC;
};
//unsigned int Board::GetId(){return id_;};

// -------------------  HW Manager ---------------
//
// --- Constructor
HwManager::HwManager(){}
// --- Destructor
HwManager::~HwManager(){}
// --- Configure the HwManager
void HwManager::Config(Configurator &c){
	// take the configurator. Translate it into a Board config. Configure the board
	xmlNode *hw_node = NULL;	
	//locate Hardware Node
	for (hw_node = c.root_element->children; hw_node ; hw_node = hw_node->next)
	{
		if (hw_node->type == XML_ELEMENT_NODE &&
				xmlStrEqual (hw_node->name, xmlCharStrdup ("Hardware"))  )
			break;
	}
	if ( hw_node== NULL ) throw  config_exception();

	// locate each board node and extract info
	xmlNode *board_node = NULL;	
	for ( board_node=hw_node->children; board_node ; board_node = board_node->next)
	{
		if (  board_node->type == XML_ELEMENT_NODE 	
				&& xmlStrEqual (hw_node->name, xmlCharStrdup ("board")) )
		{
		int ID=Configurator::GetInt(getElementContent(c, "ID" , board_node));
		//TODO
		}
	}

	return;	
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
