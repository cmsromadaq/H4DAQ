#include "interface/Handler.hpp"

void sigint_handler(int s){
     throw  sigint_exception();
}

void define_handlers() 
{
	signal (SIGINT,sigint_handler);
}

