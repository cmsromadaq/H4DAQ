#include "interface/VFE_adapter.hpp"

int main(int argc, char ** argv)
{
        Board * b = new VFE_adapter(); 

	Configurator * cfg = new Configurator();
	cfg->xmlFileName = "data/VFE_adapter.xml";
	cfg->Init();

	// find the board in the cfg
	xmlNode * hw_node = NULL;
	for (hw_node = cfg->root_element->children; hw_node; hw_node = hw_node->next) {
		if (hw_node->type == XML_ELEMENT_NODE &&
		    xmlStrEqual (hw_node->name, xmlCharStrdup ("Hardware"))) {
			break;
                }
	}
	xmlNode * board_node = NULL;
	for ( board_node = hw_node->children; board_node; board_node = board_node->next) {
		if (board_node->type == XML_ELEMENT_NODE
		    && xmlStrEqual (board_node->name, xmlCharStrdup ("board")) ) {
			int id = Configurator::GetInt(Configurable::getElementContent (*cfg, "ID" , board_node));
                        std::string type = Configurable::getElementContent (*cfg, "type" , board_node);
                        printf("--> Configuring board id = %d   type = %s\n", id, type.c_str());
			if (type == "VFE_adapter") break;
		}
	}

	BoardConfig bc;
	bc.Init (*cfg);
	bc.SetBoardNode(board_node);

        b->Config(&bc);
        b->Init();

        std::vector<WORD> v;
        b->Read(v);

        return 0;
}
