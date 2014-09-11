#include "interface/StandardIncludes.hpp"


#ifndef CONFIG_H
#define CONFIG_H

class Configurator
{
private:
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

};

#endif
