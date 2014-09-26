#ifndef BOARDCONFIG_H
#define BOARDCONFIG_H


#include "interface/StandardIncludes.hpp"
#include "interface/Configurator.hpp"

class BoardConfig : public Configurator {

private:
	bool ownConfig;	

protected:
	xmlNode *board_node;

public:
	// --- Constructor
	BoardConfig();
	// --- Construct from a Configurator.
	BoardConfig(Configurator&c);
	// --- Destructor
	~BoardConfig();
	void Clear();
	// --- Init
	void Init();
	// --- Init From a Configurator - this will set the xml tree, without taking ownership
	void Init(Configurator&c);
	// --- Set XML Node for Board
	void inline SetBoardNode(xmlNode *mynode){board_node=mynode;}
	// --- Get XML NODE PTR
	xmlNode* GetBoardNodePtr(){return board_node;}
	// --- overload member to read the xml 
	string inline getElementContent(const char*key){return Configurable::getElementContent(*this,key,board_node) ;};

};

#endif
