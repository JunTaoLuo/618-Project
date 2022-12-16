#ifndef PQ_H
#define PQ_H

#include <atomic>
#include <tuple>
#include <cstring>
#include <string>
#include <iostream>
#include <sstream>
#include <stdlib.h>

using namespace std;

template <int D, long N, int R, int IDBits, int CAP, typename TKey, typename TVal>
class PriorityQueue {
private:
    struct AdoptDesc;
    struct Node {
        int ver;
        TKey key;
        atomic<TVal> val;
        atomic<Node*>* child;
        AdoptDesc* adesc;
        int k[D] = {0};
        Node(): Node(0, 0) {}
        Node(TKey _key): Node(_key, 0) {}
        Node(TKey _key, TVal _val): key(_key), val(_val << 1), ver(0), adesc(nullptr) {
            child = new atomic<Node*>[D];
            for (int i = 0; i < D; i++) {
                child[i].store(nullptr);
            }
        }
        void initNode(TKey _key, TVal _val) {
            key = _key;
            val = (_val << 1);
            ver = 0;
            child = new atomic<Node*>[D];
            for (int i = 0; i < D; i++) {
                child[i].store(nullptr);
            }
        }

        string toString() {
            stringstream buffer;
            buffer << "Key: " << (key >> IDBits) << "-" << (key & ((1<<IDBits)-1)) << " (" << std::bitset<2>(key & 3) << ")"  << " Value: " << (val >> 1) << " (deleted: " << (val & 1) << ")" ;
            return buffer.str();
        }
    };
    struct AdoptDesc {
        Node* curr;
        int dp, dc;
        AdoptDesc(): curr(nullptr), dp(0), dc(0) {}
    };
    struct Stack {
        atomic<Node*> node[D];
        Node* head;
        Stack() {
            head = nullptr;
            for (int i = 0; i < D; i++) {
                node[i].store(nullptr);
            }
        }

        void copyFrom(Stack* other) {
            head = other->head;
            memcpy(node, other->node, sizeof(Node*)*D);
        }
    };

    class Allocator {
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
    public:
        Allocator(int _cap, int _sizeOfNode, int _sizeOfDesc, int _sizeOfStack) {
            capacity = _cap;
            sizeOfNode = _sizeOfNode;
            sizeOfDesc = _sizeOfDesc;
            sizeOfStack = _sizeOfStack;
            nodeAllocator = new Node[capacity];
            descAllocator = new AdoptDesc[capacity];
            stackAllocator = new Stack[capacity];
            currNodeIdx = 0;
            currDescIdx = 0;
            currStackIdx = 0;
        }
        ~Allocator() {
            free(nodeAllocator);
            free(descAllocator);
            free(stackAllocator);
        }
        Node* newNode() {
            int currIdx = currNodeIdx.fetch_add(1);
            if (currIdx < capacity) {
                return &nodeAllocator[currIdx];
            }
            return new Node();
        }
        AdoptDesc* newDesc() {
            int currIdx = currDescIdx.fetch_add(1);
            if (currIdx < capacity) {
                return &descAllocator[currIdx];
            }
            return new AdoptDesc();
        }
        Stack* newStack() {
            int currIdx = currStackIdx.fetch_add(1);
            if (currIdx < capacity) {
                // return (Stack*)stackAllocator + currIdx * sizeOfStack);
                return &stackAllocator[currIdx];
            }
            return new Stack();
        }
    };

    // Helper function to map priority to key vector
    void keyToCoord(int key, int* k);
    void finishInserting(Node* n, int dp, int dc);
    void printPQ(Node* node, int dim, string prefix);
    string formatStack(Stack* stack);

    // Private constants
    const int Basis;
    const int IDLimit;
    const long PriorityLimit;

    // Private fields
    Node* head;
    atomic<Stack*> stack;
    vector<atomic<unsigned short>> ids;

    // Allocator
    Allocator* allocator;

public:
    // Constructor, destructor
    PriorityQueue();
    ~PriorityQueue();

    // Main API
    void insert(TKey key, TVal val);
    tuple<TKey, TVal, bool> deleteMin();

    // Debugging
    void printPQ();
    void printStack();
};

#endif