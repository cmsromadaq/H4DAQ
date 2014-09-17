#include <cstdio>
#include <iostream>
#include <vector>
#include <string>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include "interface/StandardIncludes.hpp"
#include "interface/Logger.hpp"
#include "interface/EventBuilder.hpp"

using namespace std ;


/* assuming there's no sub-structure in the searched note */
string getElementContent (xmlDocPtr doc, const char * key, const xmlNode * node)
  {
    char * content ;
    xmlNode *cur_node = NULL;
    for (cur_node = node->children; cur_node ; cur_node = cur_node->next) 
      {
        if ((!xmlStrcmp (cur_node->name, (const xmlChar *) key))) 
          {
            content = (char *) xmlNodeListGetString (doc, cur_node->xmlChildrenNode, 1);
            break ;
          }
       }    
    return string (content) ;
  }


// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----


class Board {
    // this is the BaseClass. Each other class needs to implement this
protected:
    unsigned int id_ ;
public:
    // --- Constructor 
    Board () ;
    // --- Destructor
    ~Board () ;
    // -- Get Id
    unsigned int GetId () ;//{return id;};
    // --- Configurable
    virtual void Init () ;
    virtual void Clear () ;
    virtual void Config (xmlDocPtr doc, const xmlNode * boardNode) ;
    // --- Actually the size in bit of int is 16/32 on 32 bit and 64 on 64bit machines
    virtual void Read (vector<WORD> & v) ;
    virtual void Print () ;

} ;


// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----


class ExampleBoard1 : public Board {
public:
    ExampleBoard1 (xmlDocPtr doc, const xmlNode * boardNode) :
        Board ()
        {
          Config (doc, boardNode) ;
          Init () ; 
        }
    void Config (xmlDocPtr doc, const xmlNode * boardNode)
        {
           id_  = atoi (getElementContent (doc, "ID", boardNode).c_str ()) ;
           var1 = atoi (getElementContent (doc, "var1", boardNode).c_str ()) ;
           var2 = atof (getElementContent (doc, "var2", boardNode).c_str ()) ;
           var3 = getElementContent (doc, "var3", boardNode) ;
        }
    void Print () 
        {
          cout << "obj. ExampleBoard1, ID : " << id_ << "\n" ;
          cout << "  var1 : " << var1 << "\n" ;
          cout << "  var2 : " << var2 << "\n" ;
          cout << "  var3 : " << var3 << "\n" ;
          return ;
        }
    void Init () {} ;
    void Clear () {} ;
    void Read (vector<WORD> & v) {} ;
private:
    int var1 ;
    float var2 ;
    string var3 ;
} ;


// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----


class ExampleBoard2 : public Board {
public:
    ExampleBoard2 (xmlDocPtr doc, const xmlNode * boardNode) :
        Board ()
        {
          Config (doc, boardNode) ;        
          Init () ; 
        }
    void Config (xmlDocPtr doc, const xmlNode * boardNode)
        {
           id_  = atoi (getElementContent (doc, "ID", boardNode).c_str ()) ;
           variable1 = atoi (getElementContent (doc, "variable1", boardNode).c_str ()) ;
           variable2 = atof (getElementContent (doc, "variable2", boardNode).c_str ()) ;
           variable3 = atof (getElementContent (doc, "variable3", boardNode).c_str ()) ;
           variable4 = getElementContent (doc, "variable4", boardNode) ;        
        }
    void Print () 
        {
          cout << "obj. ExampleBoard2, ID : " << id_ << "\n" ;
          cout << "  variable1 : " << variable1 << "\n" ;
          cout << "  variable2 : " << variable2 << "\n" ;
          cout << "  variable3 : " << variable3 << "\n" ;
          cout << "  variable4 : " << variable4 << "\n" ;
          return ;
        }
    void Init () {} ;
    void Clear () {} ;
    void Read (vector<WORD> & v) {} ;
private:
    int variable1 ;
    float variable2 ;
    float variable3 ;
    string variable4 ;
} ;


// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----


int main (int argc, char **argv)
{
    xmlDoc *doc = NULL;
    xmlNode *root_element = NULL;

    if (argc != 2) return(1);
    LIBXML_TEST_VERSION

    /*parse the file and get the DOM */
    doc = xmlReadFile(argv[1], NULL, 0);

    if (doc == NULL) {
        printf("error: could not parse file %s\n", argv[1]);
    }
 
    vector <Board *> electronics ;

    /*Get the root element node */
    root_element = xmlDocGetRootElement(doc);

    xmlNode *cur_node = NULL;
    for (cur_node = root_element->children; cur_node ; cur_node = cur_node->next) 
      {
        if (cur_node->type == XML_ELEMENT_NODE &&
            xmlStrEqual (cur_node->name, xmlCharStrdup ("board"))) 
          {
            cout << "found a board\n" ;
            cout << "  ID   : "  << getElementContent (doc, "ID", cur_node) << endl ;
            string type = getElementContent (doc, "type", cur_node) ;
            cout << "  type : " << type << endl ;
            
            if (type == "ExampleBoard1") {
		    		cout<<"Pushing back EB1"<<endl;
		    		electronics.push_back (new ExampleBoard1 (doc, cur_node)) ;  }
            else if (type == "ExampleBoard2")
		    		{
		    		cout<<"Pushing back EB2"<<endl;
				electronics.push_back (new ExampleBoard2 (doc, cur_node)) ; 
				}
            // decide which board is it 
//            electronics.push_back (new )
          }
      }
    xmlFreeDoc (doc) ;
    xmlCleanupParser () ;

    for (int i = 0 ; i < electronics.size () ; i++)
      {
	  cout<<"considering element i:"<<i<<endl;
        electronics.at (i)->Print () ;
      }

    return 0;

}
