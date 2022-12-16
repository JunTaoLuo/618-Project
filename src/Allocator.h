#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include "PriorityQueue.h"
#include <atomic>

class Allocator {
public:
    Allocator(int _cap, int _sizeOfNode, int _sizeOfDesc, int _sizeOfStack);
    ~Allocator();
    Node* newNode();
    AdoptDesc* newDesc();
    Stack* newStack();

private:
    int capacity;
    int sizeOfNode;
    int sizeOfDesc;
    int sizeOfStack;
    Node* nodeAllocator;
    AdoptDesc* descAllocator;
    Stack* stackAllocator;
    atomic<int> currNodeIdx;
    atomic<int> currDescIdx;
    atomic<int> currStackIdx;
    
};

#endif