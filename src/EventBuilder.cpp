#include "interface/EventBuilder.hpp"

EventBuilder::EventBuilder()
{
	dataStream_.reserve(1024); //reserve 1K for each stream
}
