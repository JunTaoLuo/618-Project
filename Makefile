OUTPUTDIR := bin/

CFLAGS := -std=c++14 -fvisibility=hidden -lpthread -Isrc/

ifeq (,$(CONFIGURATION))
	CONFIGURATION := release
endif

ifeq (debug,$(CONFIGURATION))
CFLAGS += -g
else
CFLAGS += -O3 -fopenmp
endif

SOURCES := src/*.cpp
HEADERS := src/*.h

.SUFFIXES:
.PHONY: all clean

all: dijk-$(CONFIGURATION) pardijk-$(CONFIGURATION) PriorityQueueTests

PriorityQueue.o: $(HEADERS) src/PriorityQueue.cpp
	$(CXX) -c -o $@ $(CFLAGS) src/PriorityQueue.cpp

PriorityQueueTests: $(HEADERS) src/PriorityQueueTests.cpp PriorityQueue.o
	$(CXX) -o $@ $(CFLAGS) src/PriorityQueueTests.cpp PriorityQueue.o 

pardijk-$(CONFIGURATION): $(HEADERS) src/pardijk.cpp
	$(CXX) -o $@ $(CFLAGS) src/pardijk.cpp

dijk-$(CONFIGURATION): $(HEADERS) src/dijk.cpp
	$(CXX) -o $@ $(CFLAGS) src/dijk.cpp

clean:
	rm -rf ./pardijk-$(CONFIGURATION)* ./dijk-$(CONFIGURATION)*

check:	default
	./checker.py