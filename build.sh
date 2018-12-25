#!/usr/bin/env bash

if [ -z "${CXX}" ]; then
	CXX="g++"
fi

CXXFLAGS="-std=c++17 -Wall -Wextra -pedantic -Wcast-qual -Wwrite-strings -Wredundant-decls -Wdisabled-optimization -fcheck-new -Wctor-dtor-privacy -Wnon-virtual-dtor -Wold-style-cast -Woverloaded-virtual -Wuseless-cast -Wno-long-long"
DEBUG_FLAGS=
LIBS="-lboost_system -lboost_filesystem -lboost_iostreams"

if [[ "$1" == "debug" ]]; then
	DEBUG_FLAGS="-O0 -ggdb3"
elif [[ "$1" == "gprof" ]]; then
	DEBUG_FLAGS="-O3 -g3"
else
	DEBUG_FLAGS="-O3 -s"
fi

#echo $DEBUG_FLAGS
rm -f ./*~

print_and_run() {
	echo "$1"
	eval "$1"
}

print_and_run "${CXX} ${CXXFLAGS} ${DEBUG_FLAGS} -o xtractobb xtractobb.cc jsont.cc $LIBS"
print_and_run "${CXX} ${CXXFLAGS} ${DEBUG_FLAGS} -o pretty-print-json pretty-print-json.cc jsont.cc $LIBS"
