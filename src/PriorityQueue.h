#ifndef PQ_H
#define PQ_H

#include <atomic>
#include <tuple>

using namespace std;

template <int D, long N, int R, typename TKey, typename TVal>
class PriorityQueue {
private:
    struct AdoptDesc;
    struct Node {
        int ver;
        int key;
        int k[D] = {0};
        atomic<TVal> val;
        atomic<Node*>* child;
        AdoptDesc* adesc;
        Node(): Node(0, 0) {}
        Node(int _key): Node(_key, 0) {}
        Node(int _key, TVal _val): key(_key), val(_val << 1), ver(0), adesc(nullptr) {
            child = new atomic<Node*>[D];
            for (int i = 0; i < D; i++) {
                child[i].store(nullptr);
            }
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
    void purge(Node* hd, Node* prg);
    void printHelper(Node* node, int dim, string prefix);

public:
    int basis;
    Node* head;
    atomic<Stack*> stack;
    atomic<int> markedNode;
    atomic<bool> notPurging;
    PriorityQueue();
    // TODO: Not Implement yet!
    ~PriorityQueue();
    void insert(int key, TVal val);
    void printHelper();
    void printStack();
    // TODO: Change the return type
    tuple<int, TVal, bool> deleteMin();
};

#endif