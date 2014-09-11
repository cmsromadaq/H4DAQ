#include "interface/Handler.hpp"


void define_handlers() 
{
	signal (SIGINT,sigint_handler);
}
