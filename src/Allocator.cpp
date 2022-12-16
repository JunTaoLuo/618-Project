#include "Allocator.h"

using namespace std;

Allocator::Allocator(int _cap, int _sizeOfNode, int _sizeOfDesc, int _sizeOfStack) {
    capacity = _cap;
    sizeOfNode = _sizeOfNode;
    sizeOfDesc = _sizeOfDesc;
    sizeOfStack = _sizeOfStack;
    posix_memalign(&nodeAllocator, sizeOfNode, sizeOfNode * capacity);
    posix_memalign(&descAllocator, sizeOfDesc, sizeOfDesc * capacity);
    posix_memalign(&stackAllocator, sizeOfStack, sizeOfStack * capacity);
    currNodeIdx = 0;
    currDescIdx = 0;
    currStackIdx = 0;
}

Allocator::~Allocator() {
    free(nodeAllocator);
    free(descAllocator);
    free(stackAllocator);
}

Node* Allocator::newNode() {
    int currIdx = currNodeIdx.fetch_add(1);
    return nodeAllocator[currIdx];
}

AdoptDesc* Allocator::newDesc() {
    int currIdx = currDescIdx.fetch_add(1);
    return descAllocator[currIdx];
}

Stack* Allocator::newStack() {
    int currIdx = currStackIdx.fetch_add(1);
    return stackAllocator[currIdx];
}
