#include <atomic>
#include <cmath>
#include <vector>

using namespace std;

const int Fdel = 0x1, Fadp = 0x1, Fprg = 0x2;

typedef struct AdoptDesc DESC;

template <int D>
class PriorityQueue {
private:
    struct Val {
        void *val;
        int Fdel;
        Val(): val(nullptr), Fdel(0) {}
        Val(void* _val): val(_val), Fdel(0) {}
    };
    struct Child {
        Node* node;
        int Flag;
        Child(): node(nullptr), Flag(0) {}
        Child(Node* _node): node(_node), Flag(0) {}
    }
    struct Node {
        int key;
        vector<int> k;
        atomic<Val> val;
        atomic<Child*>* child;
        DESC* adesc;
        Node(int _key, void* _val): key(_key), adesc(nullptr) {
            val.store(new Val(_val));
            k = vector<int>(D, 0);
            child = new atomic<Child*>[D];
            for (int i = 0; i < D; i++) {
                child[i].store(new Child());
            }
        }
    };
    struct HeadNode: public Child {
        int ver;
        // TODO: need to change to Child?
        HeadNode(int _key): Child(Node(_key)), ver(0) {}
    };
    struct AdoptDesc {
        atomic<Child*> curr;
        int dp, dc;
    };
    struct Stack {
        atomic<Child*>* node;
        HeadNode* head;
        Stack() {
            node = new atomic<Child*>[D];
            head = nullptr;
            for (int i = 0; i < D; i++) {
                node[i].store(new Child());
            }
        }
    };

    // Helper function to map priority to key vector
    vector<int> keyToCoord(int key) {
        // TODO: move basis to be global variable
        int basis = ceil(power(N, double(1/D)));
        int quotient = key;
        vector<int> k(D);
        for (int i = D - 1; i >= 0; i--) {
            k[i] = quotient % basis;
            quotient = quotient / basis;
        }
        return k;
    }

public:
    const int N, R;
    HeadNode* head;
    Stack* stack;
    PriorityQueue(int _N, int  _R): N(_N), R(_R) {
        this->head = new HeadNode(0);
        Stack* sNew = new Stack();
        // TODO: filled with the dummy head?
        sNew->node[0].store(head);
        this->stack.store(sNew);
    }
    void insert(int key, void* val) {
        Stack* s = new Stack();
        Node* node = new Node(key, val);
        node->k = keyToCoord(key);
        Child* currNode = new Child(node);
        while (true) {
            Child* pred = nullptr, *curr = head;
            int dp = 0, dc = 0;
            s->head = curr;
            // locatePred;
            while (dc < D) {
                while (curr && currNode->node->k[dc] > curr->node->k[dc]) {
                    pred = curr;
                    dp = dc;
                    finishInserting(curr, dc, dc);
                    curr = curr->node->child[dc].load(memory_order_seq_cst);
                }
                if (curr == nullptr || currNode->node->k[dc] < curr->node->k[dc]) {
                    break;
                } else {
                    s->node[dc].store(curr);
                    dc++;
                }
            }

            if dc == D {
                break;
            }
            finishInserting(curr, dp, dc);
            // fillNewNode();
            node->adesc = nullptr;
            if dp < dc {
                node->adesc = new AdoptDesc();
                node->adesc->curr = curr;
                node->adesc->dp = dp;
                node->adesc->dc = dc;
            }
            for (int i = 0; i < dp; i++) {
                Child* currChild = currNode->node->child[i].load(memory_order_seq_cst);
                currChild->Flag |= Fadp;
                currNode->node->child[i].store(currChild);
            }
            for (int i = dp; i < D; i++) {
                currNode->node->child[i].store(nullptr);
            }
            currNode->node->child[dc].store(curr);
            if (atomic_compare_exchange_strong(&pred->node->child[dp],curr, currNode)) {
                finishInserting(currNode, dp, dc);
//                 rewindStack();
                Stack* sOld = stack;
                bool first_iteration = true;
                do {
                    if (s->head->ver == sOld->head->ver) {
                        if (currNode->node->key <= sOld->node[D-1].load(memory_order_relaxed)->node->key) {
                            for (int i = dp; i < D; i++) {
                                s->node[i].store(pred);
                            }
                        } else if (first_iteration) {
                            s->head = sOld->head;
                            for (int i = 0; i < D; i++) {
                                s->node[i].store(sOld->node[i].load(memory_order_seq_cst));
                            }
                            first_iteration = false;
                        } else {
                            break;
                        }
                    } else if (s->head->ver > sOld->head->ver) {
                        // Node* prg = ClearMark(sOld->head, fdel, Fdel);
                        Val* currVal = sold->head->node->val.load(memory_order_seq_cst);
                        currVal->Fdel &= ~Fdel;
                        sold->head->node->val.store(currVal);
                        Child* prg = (Child*)currVal;
                        if (prg->node->key <= sOld->node[D-1].load(memory_order_relaxed)->node->key) {
                            Val* prgVal = prg->node->Val.load(memory_order_seq_cst);
                            prgVal->Fdel &= ~Fdel;
                            prg->node->Val.store(prgVal);
                            // TODO: check correctness(what about Fdel flag);
                            s->head = (HeadNode*) prg->node->Val.load(memory_order_seq_cst);
                            for (int i = 0; i < D; i++) {
                                s->node[i].store(s->head);
                            }
                        } else if (first_iteration) {
                            s->head = sOld->head;
                            for (int i = 0; i < D; i++) {
                                s->node[i].store(sOld->node[i].load(memory_order_seq_cst));
                            }
                            first_iteration = false;
                        } else {
                            break;
                        }
                    } else {
                        // TODO: modify to correctness
                        // Node* prg = ClearMark(s->head, fdel, Fdel);
                        Val* headVal = s->head->node->Val.load(memory_order_seq_cst);
                        headVal->Fdel &= ~Fdel;
                        s->head->node->Val.store(headVal);
                        Child* prg = (Child*)headVal;
                        if prg->key <= currNode->node->key {
                            Val* prgVal = prg->node->Val.load(memory_order_seq_cst);
                            prgVal->Fdel &= ~Fdel;
                            prg->node->Val.store(prgVal);
                            s->head = (HeadNode*) prgVal;
                            for (int i = 0; i < D; i++) {
                                s->node[i].store(s->head);
                            }
                        } else {
                            for (int i = dp; i < D; i++) {
                                s->node[i].store(pred);
                            }
                        }
                    }
                } while(atomic_compare_exchange_strong(&stack, sNew, s) || IsMarked(prg, fdel, Fdel));
                break;
             }
        }
    }
};