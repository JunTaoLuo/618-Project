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

all: dijk-$(CONFIGURATION) pardijk-$(CONFIGURATION) PriorityQueueTests pardijkMdlist

PriorityQueueTests: $(HEADERS) src/PriorityQueueTests.cpp src/PriorityQueue.cpp
	$(CXX) -o $@ $(CFLAGS) src/PriorityQueueTests.cpp

pardijkMdlist: $(HEADERS) src/pardijkMdlist.cpp src/PriorityQueue.cpp
	$(CXX) -o $@ $(CFLAGS) src/pardijkMdlist.cpp

pardijk-$(CONFIGURATION): $(HEADERS) src/pardijk.cpp
	$(CXX) -o $@ $(CFLAGS) src/pardijk.cpp

dijk-$(CONFIGURATION): $(HEADERS) src/dijk.cpp
	$(CXX) -o $@ $(CFLAGS) src/dijk.cpp

clean:
	rm -rf ./pardijk-$(CONFIGURATION)* ./dijk-$(CONFIGURATION)* PriorityQueue.o pardijkMdlist PriorityQueueTests

check:	default
	./checker.py