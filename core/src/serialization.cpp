/*
Copyright 2015, Krauss-Maffei-Wegmann GmbH & Co. KG.
All rights reserved.

@author Tomas Prerovsky <tomas.prerovsky@gmail.com>

Description:
States.
*/


#include "core/include/serialization.hpp"
#include <string>
#include <math.h>
#include "core/include/base_defs.hpp"

/*Specialization for strings*/
size_t ceps::serialize_value(const std::string & v, char * buffer, size_t max_buffer_size, bool write_data, throw_exception_policy)
{
	auto n = v.length() + 1;
	if (!write_data) return n;
	if (n > max_buffer_size)
		throw state_serialization_error("Buffer overflow.");
	memcpy(buffer, v.c_str(), n);
	return n;
}

size_t ceps::serialize_value(const std::string & v, char * buffer, size_t max_buffer_size, bool write_data, nothrow_exception_policy)
{
	auto n = v.length() + 1;
	if (!write_data) return n;
	if (n > max_buffer_size) return 0;
	memcpy(buffer, v.c_str(), n);
	return n;
}

/*Deserialization: Specialization for strings*/

size_t ceps::deserialize_value(std::string & v, char * buffer, size_t max_buffer_size)
{
	auto n = strlen(buffer) + 1;

	if (n > max_buffer_size)
		throw state_serialization_error("Buffer overflow.");
	v = std::string(buffer);

	return n;
}


/*Specialization for doubles*/

size_t ceps::serialize_value(const double & v, char * buffer, size_t max_buffer_size, bool write_data, nothrow_exception_policy){
	char buffer_[64] = {0};
	sprintf_s(buffer_,"%la",v);
	auto s =  strlen(buffer_) + 1;
	if (write_data)
		strcpy_s(buffer,max_buffer_size, buffer_);
	return s;
}

size_t ceps::deserialize_value(double & v, char * buffer, size_t max_buffer_size){
 	size_t l = strlen(buffer)+1;
    sscanf(buffer,"%la",&v);
	return l;
}




