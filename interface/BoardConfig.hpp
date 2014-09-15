#include "interface/StandardIncludes.hpp"


#ifndef BOARDCONFIG_H
#define BOARDCONFIG_H
class BoardConfig{
private:

protected:
public:
	// --- Constructor
	BoardConfig();
	// --- Destructor
	~BoardConfig();
	// 
	virtual void Read(vector<unsigned int> &v) ;


};

#endif
