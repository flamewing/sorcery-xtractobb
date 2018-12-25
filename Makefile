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
CXXFLAGS := -std=c++17 ${DEBUGFLAGS} -MMD -Wall -Wextra -pedantic -Walloc-zero -Walloca -Wcatch-value=1 -Wcast-align -Wcast-qual -Wconditionally-supported -Wctor-dtor-privacy -Wdisabled-optimization -Wduplicated-branches -Wduplicated-cond -Wextra-semi -Wformat-nonliteral -Wformat-security -Wlogical-not-parentheses -Wlogical-op -Wmissing-include-dirs -Wnon-virtual-dtor -Wnull-dereference -Wold-style-cast -Woverloaded-virtual -Wplacement-new -Wredundant-decls -Wshift-negative-value -Wshift-overflow -Wtrigraphs -Wundef -Wuninitialized -Wuseless-cast -Wwrite-strings -Wformat-signedness -Wcast-align=strict -Wshadow -Wsign-conversion -Wsuggest-attribute=cold -Wsuggest-attribute=const -Wsuggest-attribute=format -Wsuggest-attribute=malloc -Wsuggest-attribute=noreturn -Wsuggest-attribute=pure -Wsuggest-final-methods -Wsuggest-final-types
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


