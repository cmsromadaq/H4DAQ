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
};

class Configurator
{
private:
map<string,string> config; //contains the configuration key->value
string context_;
public:
	// --- constructor
	Configurator();
	// --- destructor
	~Configurator();
	// --- Initialization 
	void Init();
	// --- Read from a dat file
	void ReadFromDat(string fileName);	
	// --- Write to a file/stream
	string GetDatConfiguration();
	// --- Read from a strem
	void Read(string stream);	
	// -- Read a Single line
	void ReadLine(string stream);	
	// -- Read Configuratio
	string GetValue(string key) { 
				if ( config.find(key) ==config.end() )
					throw config_exception();
				return config[key]; 
				}
	double GetDouble(string key) { return atof(GetValue(key).c_str());}
	int    GetInt   (string key) { return atoi(GetValue(key).c_str());}
	long   GetLong  (string key) { return atol(GetValue(key).c_str());}
	void   GetVecInt(string key,vector<int> &v) ;

};

#endif
