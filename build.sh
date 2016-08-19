#!/bin/sh
g++ -O3 -s -Wall -Wextra -std=c++11 -o xtractobb xtractobb.cc jsont.cc -lboost_system -lboost_filesystem -lboost_iostreams -lboost_regex
g++ -O3 -s -Wall -Wextra -std=c++11 -o pretty-print-json pretty-print-json.cc jsont.cc -lboost_system -lboost_filesystem -lboost_iostreams

