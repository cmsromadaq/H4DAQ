#include "interface/StandardIncludes.h"


#ifndef HANDLER_H
#define HANDLER_H
class sigint_exception ;
class counter_exception ;
class no_zlib_exception ;
class fork_exception ;
//std::bad_alloc = almost sure memory is full


//define here all type of execptions that can go across the all program
class sigint_exception: public exception
{
  virtual const char* what() const throw()
  {
    return "Caught signal CTRL+C";
  }
} sigint;


class counter_exception:public exception
{
 virtual const char*what() const throw()
 {
 	return "Counters Errors";
 }
} counterError;

class fork_exception:public exception
{
 virtual const char*what() const throw()
 {
 	return "Unable to run asyncronous process";
 }
} forkError;

class no_zlib_exception : public exception
{
 virtual const char*what() const throw()
 {
 	return "ZLib Support not included";
 }
}no_zlib;

void sigint_handler(int s){
     throw  myex;
}

//this functions calls the definition in signal in order to deal with SIGINT ...
void define_handlers() ;

#endif
