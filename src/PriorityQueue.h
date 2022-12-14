#ifndef PQ_H
#define PQ_H

#include <atomic>
#include <tuple>
#include <string>
#include <iostream>
#include <sstream>

using namespace std;

template <int D, long N, int R, int IDBits, typename TKey, typename TVal>
class PriorityQueue {
private:
    struct AdoptDesc;
    struct Node {
        int ver;
        TKey key;
        int k[D] = {0};
        atomic<TVal> val;
        atomic<Node*>* child;
        AdoptDesc* adesc;
        Node(): Node(0, 0) {}
        Node(TKey _key): Node(_key, 0) {}
        Node(TKey _key, TVal _val): key(_key), val(_val << 1), ver(0), adesc(nullptr) {
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
        atomic<Node*>* node;
        Node* head;
        Stack() {
            node = new atomic<Node*>[D];
            head = nullptr;
            for (int i = 0; i < D; i++) {
                node[i].store(nullptr);
            }
        }
    };
    struct PurgeFlag {
        int markedNode;
        bool notPurging;
        PurgeFlag(int _markedNode, bool _notPurging): markedNode(_markedNode), notPurging(_notPurging) {}
    };

    // Helper function to map priority to key vector
    void keyToCoord(int key, int* k);
    void finishInserting(Node* n, int dp, int dc);
    // void purge(Node* hd, Node* prg);
    void printPQ(Node* node, int dim, string prefix);
    void printStack(Stack* stack);

    // Private constants
    const int Basis;
    const int IDLimit;
    const long PriorityLimit;

    // Private fields
    Node* head;
    atomic<Stack*> stack;
    // atomic<int> markedNode;
    // atomic<bool> notPurging;
    vector<atomic<unsigned short>> ids;

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