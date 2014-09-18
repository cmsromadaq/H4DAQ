#include "interface/HwManager.hpp"


// --- Board
Board::Board(){id_=0;bC_=NULL;};
Board::~Board(){};
int Board::Config(BoardConfig *bC){
	bC_=bC;
	return 0;
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
				&& xmlStrEqual (board_node->name, xmlCharStrdup ("board")) )
		{
		int ID=Configurator::GetInt(getElementContent(c, "ID" , board_node));
		Log("[2] Configuring Board ID="+ getElementContent(c, "ID" , board_node)+"type=" + getElementContent(c, "type" , board_node),2);
		//TODO -- construct the board
		}
	}

	return;	
}
// --- Init
void HwManager::Init(){
	for(unsigned int i=0;i<hw_.size();i++)
	{
		int R=hw_[i]->Init();
		if ( !(R&~1) )  throw hw_exception();
	}

}
// --- Clear
void HwManager::Clear(){
	// --- reset to un-initialized/ un-config state	
}

void HwManager::Print(){
	Log("[2] Printing configuratio",2);
	for(vector<Board*>::iterator iBoard=hw_.begin();iBoard!=hw_.end();iBoard++)
		{
		int r = (*iBoard)->Print();// 0-1 are ok status
		if(r)Log( string("[2] Error on Board")+(*iBoard)->GetType(),2);
		}
	return ; 
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
