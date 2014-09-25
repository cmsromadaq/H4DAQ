

using namespace std;

#ifndef CTRLMNG_H
#define CTRLMNG_H
#include "interface/StandardIncludes.hpp"
#include "interface/Configurator.hpp"
#include "interface/Logger.hpp"


class ControlManager : public Configurable, public LogUtility
{
/* This cals will parse the hw results and produce cmd, 
 * to performe the right actions at the right moment
 */

public:
	// --- constructor
	ControlManager();
	// --- destructor
	~ControlManager();
	// --- Initialization 
	void Config(Configurator &c);
	void Init(); 
	void Clear();

};

#endif
