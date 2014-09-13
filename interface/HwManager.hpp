#include "interface/StandardIncludes.hpp"
#include "interface/Configurator.hpp"
#include "interface/Logger.hpp"

class Board: public Configurable {
public:
	// --- Constructor 
	Board();
	// --- Destructor
	~Board();
	// --- Configurable
	void Init();
	void Clear();
	void Config(Configurator&c);

};

class HwManager: public Configurable, public LogUtility
{
/* this class take care of the hardware configuration
 * read the boards and send back the results to the 
 * control manager
 */
private:
	vector<Board> hw_;

public:
	// --- Constructor
	HwManager();
	// --- Destructor
	~HwManager();
	// --- Configurable
	void Init();
	void Clear();
	void Config(Configurator&c);
};
