SHELL := /bin/bash

EXTRACTOBB_BIN := xtractobb
REPACK_OBB_BIN := repackobb
PRETTYJSON_BIN := pretty-print-json
JSON2INK_BIN   := json2ink
BIN := $(EXTRACTOBB_BIN) $(REPACK_OBB_BIN) $(PRETTYJSON_BIN) $(JSON2INK_BIN)

SRCDIRS := .

CC  ?= gcc
CXX ?= g++
YACC := bison
LEXER := flex

EXTRACTOBB_SRCSCXX := xtractobb.cc jsont.cc
REPACK_OBB_SRCSCXX := repackobb.cc jsont.cc
PRETTYJSON_SRCSCXX := pretty-print-json.cc jsont.cc
JSON2INK_SRCSCXX   := parser.cc scanner.cc expression.cc statement.cc driver.cc json2ink.cc
SRCSCXX            := $(EXTRACTOBB_SRCSCXX) $(REPACK_OBB_SRCSCXX) $(PRETTYJSON_SRCSCXX) $(JSON2INK_SRCSCXX)
EXTRA_SRCSCXX      := parser.cc scanner.cc parser.hh location.hh position.hh stack.hh

EXTRACTOBB_OBJECTS := $(EXTRACTOBB_SRCSCXX:%.cc=%.o)
REPACK_OBB_OBJECTS := $(REPACK_OBB_SRCSCXX:%.cc=%.o)
PRETTYJSON_OBJECTS := $(PRETTYJSON_SRCSCXX:%.cc=%.o)
JSON2INK_OBJECTS   := $(JSON2INK_SRCSCXX:%.cc=%.o)
OBJECTS       := $(EXTRACTOBB_OBJECTS) $(REPACK_OBB_OBJECTS) $(PRETTYJSON_OBJECTS) $(JSON2INK_OBJECTS)
DEPENDENCIES  := $(OBJECTS:%.o=%.d)

DEBUG ?= 0

ifeq ($(DEBUG),1)
	DEBUGFLAGS :=-O0 -g3
else
	DEBUGFLAGS :=-O3 -s
endif

START_FLAGS:=-MMD -Wall -Wextra -pedantic -Walloc-zero -Walloca -Wcatch-value=1 -Wcast-align -Wcast-qual -Wconditionally-supported -Wctor-dtor-privacy -Wdisabled-optimization -Wduplicated-branches -Wduplicated-cond -Wextra-semi -Wformat-nonliteral -Wformat-security -Wlogical-not-parentheses -Wlogical-op -Wmissing-include-dirs -Wnon-virtual-dtor -Wnull-dereference -Wold-style-cast -Woverloaded-virtual -Wplacement-new -Wredundant-decls -Wshift-negative-value -Wshift-overflow -Wtrigraphs -Wundef -Wuninitialized -Wuseless-cast -Wwrite-strings -Wformat-signedness -Wcast-align=strict -Wshadow -Wsign-conversion -Wsuggest-attribute=cold -Wsuggest-attribute=const -Wsuggest-attribute=format -Wsuggest-attribute=malloc -Wsuggest-attribute=noreturn -Wsuggest-attribute=pure -Wsuggest-final-methods -Wsuggest-final-types

CXXFLAGS := -std=c++17 ${DEBUGFLAGS}
$(shell touch tmp.cc)
CXXFLAGS+=$(foreach flag,$(START_FLAGS),$(shell g++ -Werror $(flag) -c tmp.cc -o tmp.o &> /dev/null && echo "$(flag)"))
$(shell rm -f tmp.cc tmp.o tmp.d)
CPPFLAGS :=
INCFLAGS :=
ifndef MINGW_PREFIX
	LDFLAGS  := -Wl,-rpath,/usr/local/lib
	LIBS     := -lboost_system -lboost_filesystem -lboost_iostreams -lboost_serialization
else
	LDFLAGS  := -Wl,-rpath,$(MINGW_PREFIX)/lib
	LIBS     := -lboost_system-mt -lboost_filesystem-mt -lboost_iostreams-mt -lboost_serialization-mt
endif
EXTRACTOBB_LIBS :=
REPACK_OBB_LIBS :=
PRETTYJSON_LIBS :=
JSON2INK_LIBS   :=

.PHONY: all count clean test

# Targets
all: $(BIN)

count:
	wc *.c *.cc *.C *.cpp *.h *.hpp *.hh *.H *.yy *.ll

clean:
	rm -f *.o *~ $(BIN) $(EXTRA_SRCSCXX) *.d

test: all
	rm -rf tests/input
	mkdir -p tests/input
	cp tests/gold/*.json tests/input
	./pretty-print-json -w $$(ls -1 tests/input/*.json)
	diff -bru tests/gold tests/input || echo "Test failed"

.SUFFIXES:
.SUFFIXES:	.c .cc .C .cpp .o .yy .ll .h .hh

$(EXTRACTOBB_BIN): $(EXTRACTOBB_OBJECTS)
	$(CXX) -o $(EXTRACTOBB_BIN) $(EXTRACTOBB_OBJECTS) $(LDFLAGS) $(LIBS) $(EXTRACTOBB_LIBS)

$(REPACK_OBB_BIN): $(REPACK_OBB_OBJECTS)
	$(CXX) -o $(REPACK_OBB_BIN) $(REPACK_OBB_OBJECTS) $(LDFLAGS) $(LIBS) $(REPACK_OBB_LIBS)

$(PRETTYJSON_BIN): $(PRETTYJSON_OBJECTS)
	$(CXX) -o $(PRETTYJSON_BIN) $(PRETTYJSON_OBJECTS) $(LDFLAGS) $(LIBS) $(PRETTYJSON_LIBS)

$(JSON2INK_BIN): $(JSON2INK_OBJECTS)
	$(CXX) -o $(JSON2INK_BIN) $(JSON2INK_OBJECTS) $(LDFLAGS) $(LIBS) $(JSON2INK_LIBS)

%.o: %.cc
	$(CXX) -o $@ -c $(CXXFLAGS) $(CPPFLAGS) $< $(INCFLAGS)

driver.o: parser.cc parser.hh

scanner.o: parser.cc parser.hh

parser.hh: parser.cc

scanner.hh: scanner.cc

%.cc: %.yy
	$(YACC) -Wno-yacc -d -o parser.cc parser.yy
	for ff in parser.cc parser.hh location.hh ; do \
		sed -ri 's%(^.*[^\\*]$$)%\1// NOLINT%' $$ff; \
	done

%.cc: %.ll
	$(LEXER) --outfile=scanner.cc scanner.ll
	sed -ri 's%(^.*[^\\*]$$)%\1// NOLINT%' scanner.cc

# Dependencies
-include $(DEPENDENCIES)
