#include "interface/Configurator.hpp"

Configurator::Configurator(){
	//---TXT 
	//context_="";
	//--- XML 
	xmlFileName="";
	doc=NULL;
	root_element=NULL;
#ifndef NO_XML
	LIBXML_TEST_VERSION
#endif
	}

Configurator::~Configurator(){
	Clear();
}


void Configurator::Clear(){
	if(doc != NULL)     xmlFreeDoc (doc) ;
	doc=NULL;
	if(root_element!=NULL)delete root_element;
	root_element=NULL;
	xmlFileName="";
#ifndef NO_XML
    	xmlCleanupParser () ;
#endif
}

void Configurator::Init(){
#ifndef NO_XML
	if(xmlFileName!="") {
		doc = xmlReadFile(xmlFileName.c_str(), NULL, 0);
		if (doc==NULL) throw configfile_exception(); 
		root_element = xmlDocGetRootElement(doc);
	}
#else
	printf("[Configurator]::[Init] NO_XML: action Forbid\n");
	throw config_exception();
#endif
	return; 
}

void Configurator::ReadFromStream(string stream){
	// ugly for the moment -- this does not need to be very fast
	Clear();
	char *name=tmpnam(NULL);
	xmlFileName=name;
	FILE *fw=fopen(xmlFileName.c_str(),"w");
	fprintf(fw,"%s",stream.c_str());
	fclose(fw);
	delete name;
	Init();
}


void   Configurator::GetVecInt(string value,vector<int> &v) {
	size_t comma_pos;
	size_t dash_pos;
	// split againt ,
	do {
		comma_pos=value.find(",");
		string str=value.substr(0,comma_pos);
		value.erase(0,comma_pos+1);
		// split againt -
		dash_pos=str.find("-");
		if(dash_pos == string::npos) v.push_back(atoi(str.c_str()) );
		else {
			string begin=str.substr(0,dash_pos);
			string end= str.substr(dash_pos+1,string::npos);
			for(int k=atoi(begin.c_str()); k<= atoi(end.c_str()) ;k++)
				v.push_back(k);
			}
	} while ( comma_pos != string::npos);
}

/* assuming there's no sub-structure in the searched note */
string Configurable::getElementContent (xmlDocPtr doc, const char * key, const xmlNode * node)
  {
#ifndef NO_XML
    char * content = "NULL" ;
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
#else
	printf("[Configurator]::[getElementContent] NO_XML: action Forbid\n");
	throw config_exception();
	return string("");
#endif
  }

string Configurable::getElementContent(Configurator&c, const char * key, const xmlNode * node)
{
	return getElementContent(c.doc, key,node);
}

vector<string> Configurable::getElementVector (xmlDocPtr doc, const char * key, const xmlNode * node)
  {
    vector<string> R;
#ifndef NO_XML
    char * content ;
    xmlNode *cur_node = NULL;
    for (cur_node = node->children; cur_node ; cur_node = cur_node->next)
      {
        if ((!xmlStrcmp (cur_node->name, (const xmlChar *) key)))
          {
            content = (char *) xmlNodeListGetString (doc, cur_node->xmlChildrenNode, 1);
	    R.push_back(content);
          }
       }
#else
	printf("[Configurator]::[getElementVector] NO_XML: action Forbid\n");
	throw config_exception();
#endif
    return R ;
  }

vector<string> Configurable::getElementVector(Configurator&c, const char * key, const xmlNode * node)
{
	return getElementVector(c.doc, key,node);
}

int    Configurator::GetInt   (string value) {// { return atoi(value.c_str());}
	if ( value.find("0x") != string::npos ) {
	int mynum;
	sscanf(value.c_str(),"%x",&mynum);
	return mynum;
	}
	else return atoi(value.c_str());

}

/** assuming there's only one sub-structure in the searched node, 
composed by the replication of the same pattern */
vector<pair<string, string> >
Configurable::getNodeContentList (xmlDocPtr doc, const xmlNode * node)
  {
    string name ;
    string content ;
    vector<pair<string, string> > output ;
#ifndef NO_XML
    xmlNode *cur_node = NULL;
    for (cur_node = node->children; cur_node ; cur_node = cur_node->next)
      {
        name    = (char *) cur_node->name ;
        content = (char *) xmlNodeListGetString (doc, cur_node->xmlChildrenNode, 1) ;
        output.push_back (pair<string, string> (name, content)) ;
       }
#else
	printf("[Configurator]::[getNodeContentList] NO_XML: action Forbid\n");
	throw config_exception();
#endif
    return output ;
  }

vector<pair<string, string> >
Configurable::getNodeContentList (Configurator & c, const xmlNode * node)
{
  return getNodeContentList (c.doc, node) ;
 }
