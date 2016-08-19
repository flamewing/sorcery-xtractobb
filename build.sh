#!/bin/bash

CXXFLAGS="-g3 -Wall -Wextra -std=c++14"
DEBUG_FLAGS=
LIBS="-lboost_system -lboost_filesystem -lboost_iostreams -lboost_regex"

if [[ "$1" == "debug" ]]; then
	DEBUG_FLAGS="-O0 -g3"
elif [[ "$1" == "gprof" ]]; then
	DEBUG_FLAGS="-O3 -g"
else
	DEBUG_FLAGS="-O3 -s"
fi

rm *~
g++ $CXXFLAGS $DEBUG_FLAGS -o xtractobb xtractobb.cc jsont.cc $LIBS
g++ $CXXFLAGS $DEBUG_FLAGS -o pretty-print-json pretty-print-json.cc jsont.cc $LIBS

