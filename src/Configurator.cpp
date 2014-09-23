#include "interface/Configurator.hpp"

Configurator::Configurator(){
	//---TXT 
	//context_="";
	//--- XML 
	xmlFileName="";
	doc=NULL;
	root_element=NULL;
	LIBXML_TEST_VERSION
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
    	xmlCleanupParser () ;
}

void Configurator::Init(){
	if(xmlFileName!="") {
		doc = xmlReadFile(xmlFileName.c_str(), NULL, 0);
		if (doc==NULL) throw configfile_exception(); 
		root_element = xmlDocGetRootElement(doc);
	}
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

// --- Read from a dat file
//    void Configurator::ReadFromDat(string fileName){
//    	// read one lines per times
//    	FILE *fr=fopen(fileName.c_str(),"r");
//    	char buffer[1024]; // max line length
//    
//    	while ( fgets ( buffer, 1024, fr ) != NULL )
//    	{
//    		string line=buffer;
//    		ReadLine(line);
//    	}
//    
//    	fclose(fr);
//    	return;
//    
//    }

// --- Read from a strem
//   void Configurator::Read(string stream){
//   	// split in lines
//   	while( !stream.empty() ){
//   		size_t pos=stream.find('\n');
//   		string line=stream.substr(0,pos+1);
//   		stream.erase(0,pos+1);
//   		ReadLine(line);
//   	}
//   	return;
//   }
//   
//   // -- Read a Single line
//   void Configurator::ReadLine(string line){
//   	if ( line.empty() ) return; // return on empty lines
//   	if ( line.c_str()[0] == '\n' ) return;
//   	if ( line.c_str()[0] == '#' ) return; //return on lines that start with #
//   	size_t pos = line.find('\n') ;
//   	line.erase(pos,string::npos); // erase everything from \n -> 
//   	if (line.c_str()[0] == '[' )
//   		{
//   		pos=line.find(']');
//   		context_=line.substr(1,pos);
//   		if (context_=="_" ) context_="";
//   		}
//   
//   	pos=line.find('=');
//   	string key=line.substr(0,pos);
//   	string value=line.substr(pos+1,string::npos);
//   	if ( key == "include" ) ReadFromDat(value);
//   	else if (context_ == "") config[key]=value;
//   	else config[ context_ + "_" + key ] =value;
//   	//log->Log("Configurator: key "+ key + " set to value: "+ value,1); // log is also configured. I'll write it with the getDat
//   }


// --- Write to a file/stream
//   string Configurator::GetDatConfiguration()
//   {
//   	string datStr="## Automatic Dat configuration dump##\n";
//   	map<string,string>::iterator it;
//   	for(it=config.begin();it!=config.end();it++)
//   	{
//   		datStr=it->first + "=" + it->second + "\n";
//   	}
//   	datStr += "## End of automatic configuration dump ##\n";
//   	return datStr;
//   }


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

vector<string> Configurable::getElementVector (xmlDocPtr doc, const char * key, const xmlNode * node)
  {
    vector<string> R;
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
    return R ;
  }

string Configurable::getElementContent(Configurator&c, const char * key, const xmlNode * node)
{
	return getElementContent(c.doc, key,node);
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
