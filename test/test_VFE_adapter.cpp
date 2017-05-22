#include "interface/VFE_adapter.hpp"

#include <chrono>

int main(int argc, char ** argv)
{
        Board * b = new VFE_adapter(); 

	Configurator * cfg = new Configurator();
	cfg->xmlFileName = "data/VFE_adapter.xml";
	//cfg->xmlFileName = argv[1];
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

        b->Clear();

        std::vector<WORD> v;
        int cnt = 0;
        auto begin = std::chrono::high_resolution_clock::now();
        // can be commented if a fake trigger is generated
        //while (cnt++ < 10000) {
        while (1) {
                // can be commented if a fake trigger is generated
                usleep(5000);
                // uncomment to generate fake trigger
                //(static_cast<VFE_adapter *>(b))->Trigger();
                b->Read(v);
                //getchar();
        }
        auto end = std::chrono::high_resolution_clock::now();
        auto dt = std::chrono::duration_cast<std::chrono::nanoseconds>(end-begin).count();
        printf("Elapsed time for %d events: %ld ns (%.2f Hz)\n", cnt - 1, dt, (cnt - 1) / (dt * 1e-9));

        return 0;
}
