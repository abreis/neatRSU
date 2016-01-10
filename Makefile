# Makefile: build neatRSU
# Expects libboost on PATH or /usr/local/include

CC=g++
CXXFLAGS=-c -O2 -std=c++11 -Wall --pedantic
LDFLAGS=-O2

SOURCES=neatRSU.cpp
EXECUTABLE=neatRSU
OBJECTS=$(SOURCES:.cpp=.o)

INCLUDEDIRS=-I/usr/local/include
EXTRALIBS=

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(OBJECTS) $(EXTRALIBS) $(LDFLAGS) -o $@

.cpp.o:
	$(CC) $< -o $@ $(INCLUDEDIRS) $(CXXFLAGS)

clean:
	rm -rf $(OBJECTS) $(EXECUTABLE)
