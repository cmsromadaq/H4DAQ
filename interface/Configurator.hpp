#include "interface/StandardIncludes.hpp"
#include "interface/Handler.hpp"


#ifndef CONFIG_H
#define CONFIG_H

class Configurator;

class Configurable{ // base class - abstract
public:
	virtual void Clear()=0;
	virtual void Init()=0;
	virtual void Config(Configurator &)=0;
	static string getElementContent (xmlDocPtr doc, const char * key, const xmlNode * node);
	static string getElementContent (Configurator&c, const char * key, const xmlNode * node);
	static vector<string> getElementVector (Configurator&c, const char * key, const xmlNode * node);
	static vector<string> getElementVector (xmlDocPtr doc, const char * key, const xmlNode * node);
};

class Configurator
{
//friend class Configurable;
private:
	//---- TXT ----
	//map<string,string> config; //contains the configuration key->value
	//string context_; // txt
	// --- XML ---
public:
	xmlDoc *doc;// = NULL;
	xmlNode *root_element;// = NULL;
	string xmlFileName;
	// --- constructor
	Configurator();
	// --- destructor
	~Configurator();
	// --- Initialization 
	virtual void Init();
	virtual void Clear();
	void ReadFromStream(string stream);
	// --- Read from a dat file
	//void ReadFromDat(string fileName);	
	// --- Write to a file/stream
	//string GetDatConfiguration();
	// --- Read from a strem
	//void Read(string stream);	
	// -- Read a Single line
	//void ReadLine(string stream);	
	// -- Read Configuratio
	//string GetValue(string key) { 
	//			if ( config.find(key) ==config.end() )
	//				throw config_exception();
	//			return config[key]; 
	//			}
	static inline double GetDouble(string value) { return atof(value.c_str());}
	static inline int    GetInt   (string value) { return atoi(value.c_str());}
	static inline long   GetLong  (string value) { return atol(value.c_str());}
	static void  GetVecInt(string value,vector<int> &v) ;
	
	// -------------XML
};

#endif
