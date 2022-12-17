OUTPUTDIR := bin/

CFLAGS := -std=c++14 -fvisibility=hidden -lpthread -Isrc/

ifeq (,$(CONFIGURATION))
	CONFIGURATION := release
endif

ifeq (debug,$(CONFIGURATION))
CFLAGS += -g
else
CFLAGS += -O3 -fopenmp -pg
endif

SOURCES := src/*.cpp
HEADERS := src/*.h

.SUFFIXES:
.PHONY: all clean

all: dijk-$(CONFIGURATION) pardijk-$(CONFIGURATION) PriorityQueueTests pardijkMdlist micro

PriorityQueueTests: $(HEADERS) src/PriorityQueueTests.cpp src/PriorityQueue.cpp
	$(CXX) -o $@ $(CFLAGS) src/PriorityQueueTests.cpp

pardijkMdlist: $(HEADERS) src/pardijkMdlist.cpp src/PriorityQueue.cpp
	$(CXX) -o $@ $(CFLAGS) src/pardijkMdlist.cpp

pardijk-$(CONFIGURATION): $(HEADERS) src/pardijkTests.cpp src/pardijk.cpp
	$(CXX) -o $@ $(CFLAGS) src/pardijkTests.cpp

dijk-$(CONFIGURATION): $(HEADERS) src/dijk.cpp
	$(CXX) -o $@ $(CFLAGS) src/dijk.cpp

micro: $(HEADERS) src/microbenchmark.cpp
	 $(CXX) -o $@ $(CFLAGS) src/microbenchmark.cpp

clean:
	rm -rf ./pardijk-$(CONFIGURATION)* ./dijk-$(CONFIGURATION)* PriorityQueue.o pardijkMdlist PriorityQueueTests micro

check:	default
	./checker.py