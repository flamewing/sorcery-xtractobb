EXTRACTOBB_BIN := xtractobb
PRETTYJSON_BIN := pretty-print-json
BIN := $(EXTRACTOBB_BIN) $(PRETTYJSON_BIN)

SRCDIRS := .

CC  ?= gcc
CXX ?= g++

EXTRACTOBB_SRCSC := xtractobb.cc jsont.cc 
PRETTYJSON_SRCSC := pretty-print-json.cc jsont.cc
SRCSC            := $(EXTRACTOBB_SRCSC) $(PRETTYJSON_SRCSC)
SRCSH            := $(foreach SRCDIR,$(SRCDIRS),$(wildcard $(SRCDIR)/*.h))

EXTRACTOBB_OBJECTS := $(EXTRACTOBB_SRCSC:%.cc=%.o)
PRETTYJSON_OBJECTS := $(PRETTYJSON_SRCSC:%.cc=%.o)
OBJECTS       := $(EXTRACTOBB_OBJECTS) $(PRETTYJSON_OBJECTS)
DEPENDENCIES  := $(OBJECTS:%.o=%.d)

DEBUG ?= 0

ifeq ($(DEBUG),1)
	DEBUGFLAGS := -O0 -g3
else
	DEBUGFLAGS := -O3 -s
endif

CFLAGS   := -std=c11 ${DEBUGFLAGS} -MMD -Wall -Wextra -pedantic
CXXFLAGS := -std=c++17 ${DEBUGFLAGS} -MMD -Wall -Wextra -pedantic -Wcast-qual -Wwrite-strings -Wredundant-decls -Wdisabled-optimization -fcheck-new -Wctor-dtor-privacy -Wnon-virtual-dtor -Wold-style-cast -Woverloaded-virtual -Wuseless-cast -Wno-long-long
CPPFLAGS :=
INCFLAGS :=
LDFLAGS  := -Wl,-rpath,/usr/local/lib
LIBS     := -lboost_system -lboost_filesystem -lboost_iostreams
EXTRACTOBB_LIBS :=
PRETTYJSON_LIBS :=

# Targets
all: $(BIN)

count:
	wc *.c *.cc *.C *.cpp *.h *.hpp *.hh *.H *.yy *.ll

clean:
	rm -f *.o *~ $(BIN) *.d

.PHONY: all count clean

.SUFFIXES:
.SUFFIXES:	.c .cc .C .cpp .o .yy .ll .h .hh

$(EXTRACTOBB_BIN): $(EXTRACTOBB_OBJECTS)
	$(CXX) -o $(EXTRACTOBB_BIN) $(EXTRACTOBB_OBJECTS) $(LDFLAGS) $(LIBS) $(EXTRACTOBB_LIBS)

$(PRETTYJSON_BIN): $(PRETTYJSON_OBJECTS)
	$(CXX) -o $(PRETTYJSON_BIN) $(PRETTYJSON_OBJECTS) $(LDFLAGS) $(LIBS) $(PRETTYJSON_LIBS)

%.o: %.cc
	$(CXX) -o $@ -c $(CXXFLAGS) $(CPPFLAGS) $< $(INCFLAGS)

# Dependencies
-include $(DEPENDENCIES)


