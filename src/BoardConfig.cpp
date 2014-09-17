#include "interface/BoardConfig.hpp"


// --- Constructor
BoardConfig::BoardConfig(){
	board_node=NULL;
	ownConfig=true;
	Clear();
}
// --- Destructor ; nothing to destruct
BoardConfig::~BoardConfig(){
	Clear();
}
// --- 
//
void BoardConfig::Clear(){
	board_node=NULL;
	if (!ownConfig)
		{ // don't destructo configurator
			doc=NULL;
			root_element=NULL;
		}
	Configurator::Clear();
	ownConfig=true;
	return;
}

void BoardConfig::Init(){
	if(ownConfig) Configurator::Init();
}
void BoardConfig::Init(Configurator&c){
	ownConfig=false;
	doc=c.doc;
	root_element=c.root_element;
}
//
