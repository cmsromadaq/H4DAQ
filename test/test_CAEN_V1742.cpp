#include "interface/ConnectionManager.hpp"
#include "interface/StandardIncludes.hpp"
#include "interface/EventBuilder.hpp"
#include "interface/Utility.hpp"
#include "interface/CAEN_V1742.hpp"
#include "interface/BoardConfig.hpp"
#include "interface/Configurator.hpp"
#include "interface/Board.hpp"
#include "interface/EventBuilder.hpp" // boardId
#include "interface/Logger.hpp"
#include <cstdio>
#include <cstdlib>
#include <time.h>

int main(int argc, char**argv)
{ 
  
  Board * CAEN_V1742_instance = new CAEN_V1742 () ;
  Configurator * config = new Configurator () ;
  config->xmlFileName = argv[1] ;
  config->Init () ; // load configurator

  // find the board in the cfg
  // ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
  
  xmlNode * hw_node = NULL;    
  for (hw_node = config->root_element->children ; hw_node ; hw_node = hw_node->next)
    {
        if (hw_node->type == XML_ELEMENT_NODE &&
            xmlStrEqual (hw_node->name, xmlCharStrdup ("Hardware"))  )
            break;
    }
  xmlNode * board_node = NULL;    
  for ( board_node = hw_node->children ; board_node ; board_node = board_node->next)
    {
      if (board_node->type == XML_ELEMENT_NODE     
          && xmlStrEqual (board_node->name, xmlCharStrdup ("board")) )
        {  
          int ID = Configurator::GetInt (Configurable::getElementContent (*config, "ID" , board_node)) ;
          cout << "[2] Configuring Board ID = " 
               << ID
               << " type= " 
               << Configurable::getElementContent (*config, "type" , board_node) 
               << endl ;

          if (Configurable::getElementContent (*config, "type", board_node) == "CAEN_V1742") break ;
        }
    }
  
  
  BoardConfig bC ;
  bC.Init (*config) ;
  bC.SetBoardNode (board_node) ;

  CAEN_V1742_instance->Config (&bC) ;
  CAEN_V1742_instance->Print () ;
   
  delete CAEN_V1742_instance ;
  return 0 ;
}