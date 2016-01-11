# Makefile: build neatRSU
# Expects libboost on PATH or /usr/local/include, /usr/local/lib

CC=g++
CXXFLAGS=-c -O2 -std=c++11 -Wall --pedantic
LDFLAGS=-O2

SOURCES=neatRSU.cpp genetic.cpp neural.cpp
EXECUTABLE=neatRSU
EXTRALIBS=-lboost_program_options

INCLUDEDIRS=-I/usr/local/include
LIBDIRS=-L/usr/local/lib

OBJECTS=$(SOURCES:.cpp=.o)

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(OBJECTS) $(LIBDIRS) $(EXTRALIBS) $(LDFLAGS) -o $@

.cpp.o:
	$(CC) $< -o $@ $(INCLUDEDIRS) $(CXXFLAGS)

clean:
	rm -rf $(OBJECTS) $(EXECUTABLE)
