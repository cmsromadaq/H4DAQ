#include "interface/BoardConfig.hpp"

using namespace std ;

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

vector<pair<string, string> > 
BoardConfig::getNodeContentList (const char * key)
{
  xmlNode * cur_node = NULL;
  for (cur_node = board_node->children; cur_node ; cur_node = cur_node->next)
    {
      if ((!xmlStrcmp (cur_node->name, (const xmlChar *) key)))
        {
          return Configurable::getNodeContentList (*this, cur_node) ;
        }
     }
  return vector<pair<string, string> > () ;
}
