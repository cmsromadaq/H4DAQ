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
	BoardConfig(Configurator&c);
	// --- Destructor
	~BoardConfig();
	void Clear();
	void Init();
	void Init(Configurator&c);
	// 
	void inline SetBoardNode(xmlNode *mynode){board_node=mynode;}
	xmlNode* GetBoardNodePtr(){return board_node;}
	// overload of operator
	string inline getElementContent(const char*key){return Configurable::getElementContent(*this,key,board_node) ;};

};

#endif
