#ifndef HANDLER_H
#define HANDLER_H


#include "interface/StandardIncludes.hpp"



// ----------------  will define the following exceptions
class sigint_exception ;
class counter_exception ;
class no_zlib_exception ;
class fork_exception ;
class logfile_open_exception ;
class config_exception ;
class configfile_exception;
class hw_exception;
class logic_exception;
class building_exception;
//std::bad_alloc = almost sure memory is full

//handle of the sigint signal
void sigint_handler(int s);

//this functions calls the definition in signal in order to deal with SIGINT ...
void define_handlers() ;

// ---------------------- EXCEPTIONS

//define here all type of execptions that can go across the all program
class sigint_exception: public exception
{
public:
  virtual const char* what() const throw()
  {
    return "Caught signal CTRL+C";
  }
} ;

class hw_exception: public exception
{
public:
  virtual const char* what() const throw()
  {
    return "Hardware Exception";
  }
} ;


class counter_exception:public exception
{
public:
 virtual const char*what() const throw()
 {
 	return "Counters Errors";
 }
} ;

class fork_exception:public exception
{
public:
 virtual const char*what() const throw()
 {
 	return "Unable to run asyncronous process";
 }
} ;

class config_exception:public exception
{
public:
 virtual const char*what() const throw()
 {
 	return "Configuration Error";
 }
} ;
class logfile_open_exception:public exception
{
public:
 virtual const char*what() const throw()
 {
 	return "Cannot open log file";
 }
} ;

class configfile_exception:public exception
{
public:
 virtual const char*what() const throw()
 {
 	return "No config file";
 }
} ;

class no_zlib_exception : public exception
{
public:
 virtual const char*what() const throw()
 {
 	return "ZLib Support not included";
 }
};
class logic_exception: public exception
{
public:
  virtual const char* what() const throw()
  {
    return "Error in the logic";
  }
} ;
class building_exception: public exception
{
public:
  virtual const char* what() const throw()
  {
    return "Spill is not OK";
  }
} ;


#endif
