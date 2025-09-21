
cmake_minimum_required(VERSION 3.10)
project(protobufish)

add_compile_options(-O  -Wall -MD  -std=c++2a  -fPIC -static -Wno-undef )

IF(NOT( DEFINED ENV{CEPSCORE}))
    MESSAGE(FATAL_ERROR "Could not find ceps core (Environment variable CEPSCORE not set).")
ENDIF()

IF(NOT( DEFINED ENV{MACHINES4CEPS}))
    MESSAGE(FATAL_ERROR "Could not find machines4ceps (Environment variable MACHINES4CEPS not set).")
ENDIF()

IF(NOT( DEFINED ENV{LOG4CEPS}))
    MESSAGE(FATAL_ERROR "Could not find log4ceps (Environment variable LOG4CEPS not set).")
ENDIF()

include_directories($ENV{CEPSCORE}/include)
include_directories($ENV{LOG4CEPS}/include)
include_directories($ENV{MACHINES4CEPS})
include_directories($ENV{MACHINES4CEPS}/core/src_gen/logging)
include_directories(include)
include_directories(../include)
#include_directories(include/tests)

link_directories($ENV{CEPSCORE}/bin)

add_library(protobufish SHARED 
           plugin-entrypoint.cpp 
           )

target_link_libraries(protobufish cepscore)					
