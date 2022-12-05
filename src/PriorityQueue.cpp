#include <iostream>
#include <atomic>
#include <cmath>
#include <vector>

using namespace std;

int Fdel = 0x1, Fadp = 0x1, Fprg = 0x2;

#define SetMark(p, m) ({p |= m; p;})
#define ClearMark(p, m) ({p &= ~m; p;})
#define IsMarked(p, m) ({p & m;})

template <int D>
class PriorityQueue {
private:
// public:
    struct AdoptDesc;
    struct Node {
        int ver;
        int key;
        vector<int> k;
        void* val;
        atomic<Node*>* child;
        AdoptDesc* adesc;
        Node(int _key): key(_key), val(nullptr), ver(0), adesc(nullptr) {
            k = vector<int>(D, 0);
            child = new atomic<Node*>[D];
            for (int i = 0; i < D; i++) {
                child[i].store(nullptr);
            }
        }
        Node(int _key, void* _val): key(_key), val(_val), ver(0), adesc(nullptr) {
            k = vector<int>(D, 0);
            child = new atomic<Node*>[D];
            for (int i = 0; i < D; i++) {
                child[i].store(nullptr);
            }
        }
    };
    struct AdoptDesc {
        Node* curr;
        int dp, dc;
        AdoptDesc(): curr(nullptr), dp(0), dc(0) {

        }
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

    // Helper function to map priority to key vector
    vector<int> keyToCoord(int key) {
        // TODO: move basis to be global variable
        int basis = ceil(pow(N, 1.0/D));
        int quotient = key;
        vector<int> k(D);
        for (int i = D - 1; i >= 0; i--) {
            k[i] = quotient % basis;
            quotient = quotient / basis;
        }
        return k;
    }

    void finishInserting(Node* n, int dp, int dc) {
        if (!n) {
            return;
        }
        AdoptDesc* ad = n->adesc;
        if (!ad || dc < ad->dp || dp > ad->dc) {
            return;
        }
        Node* child, *cur = ad->curr;
        dp = ad->dp;
        dc = ad->dc;
        for (int i = dp; i < dc; i++) {
            // TODO: Only change local data?
            Node* child, *desired;
            child = cur->child[i].load();
            do {
                desired = reinterpret_cast<Node*>(reinterpret_cast<uintptr_t>(child) | Fadp);
            } while(!cur->child[i].compare_exchange_weak(child, desired));
            uintptr_t uintPtr = reinterpret_cast<uintptr_t>(child);
            child = reinterpret_cast<Node*>(ClearMark(uintPtr, Fadp));
            Node* tmp = nullptr;
            n->child[i].compare_exchange_strong(tmp, child);
        }
        n->adesc = nullptr;
    }

public:
    const int N, R;
    Node* head;
    atomic<Stack*> stack;
    PriorityQueue(int _N, int  _R):N(_N), R(_R) {
        this->head = new Node(0);
        Stack* sNew = new Stack();
        // TODO: filled with the dummy head?
        sNew->head = this->head;
        for (int i = 0; i < D; i++) {
            sNew->node[i].store(head);
        }
        this->stack.store(sNew);
    }
    // TODO: Not Implement yet!
    ~PriorityQueue();
    void insert(int key, void* val) {
        Stack* s = new Stack();
        Node* node = new Node(key, val);
        node->k = keyToCoord(key);
        while (true) {
            Node* pred = nullptr, *curr = head;
            int dp = 0, dc = 0;
            s->head = curr;
            // locatePred;
            while (dc < D) {
                while (curr && node->k[dc] > curr->k[dc]) {
                    pred = curr;
                    dp = dc;
                    finishInserting(curr, dc, dc);
                    curr = curr->child[dc].load(memory_order_seq_cst);
                }
                if (curr == nullptr || node->k[dc] < curr->k[dc]) {
                    break;
                } else {
                    s->node[dc].store(curr);
                    dc++;
                }
            }

            if (dc == D) {
                break;
            }
            finishInserting(curr, dp, dc);
            // fillNewNode();
            node->adesc = nullptr;
            if (dp < dc) {
                node->adesc = new AdoptDesc();
                node->adesc->curr = curr;
                node->adesc->dp = dp;
                node->adesc->dc = dc;
            }
            for (int i = 0; i < dp; i++) {
                // TODO: what if child is nullptr
                Node* child = node->child[i].load(memory_order_seq_cst);
                // node->child[i].store(reinterpret_cast<Node*>(SetMark(reinterpret_cast<uintptr_t>(child), Fadp)));
                uintptr_t uintPtr = reinterpret_cast<uintptr_t>(child);
                node->child[i].store(reinterpret_cast<Node*>(SetMark(uintPtr, Fadp)));
            }
            for (int i = dp; i < D; i++) {
                node->child[i].store(nullptr);
            }
            node->child[dc].store(curr);
            if (pred->child[dp].compare_exchange_strong(curr, node)) {
                finishInserting(node, dp, dc);
//                 rewindStack();
                Stack* sOld = stack.load(memory_order_seq_cst), * sNew;
                bool first_iteration = true;
                do {
                    sNew = stack.load(memory_order_seq_cst);
                    if (s->head->ver == sOld->head->ver) {
                        if (node->key <= sNew->node[D-1].load(memory_order_seq_cst)->key) {
                            for (int i = dp; i < D; i++) {
                                s->node[i].store(pred);
                            }
                        } else if (first_iteration) {
                            s->head = sNew->head;
                            for (int i = 0; i < D; i++) {
                                s->node[i].store(sNew->node[i].load(memory_order_seq_cst));
                            }
                            first_iteration = false;
                        } else {
                            break;
                        }
                    } else if (s->head->ver > sOld->head->ver) {
                        uintptr_t uintPtr = reinterpret_cast<uintptr_t>(sOld->head);
                        Node* prg = reinterpret_cast<Node*>(ClearMark(uintPtr, Fadp|Fprg));
                        if (prg->key <= sOld->node[D-1].load(memory_order_seq_cst)->key) {
                            // TODO: check correctness(what about Fdel flag);
                            uintPtr = reinterpret_cast<uintptr_t>(prg->val);
                            s->head = reinterpret_cast<Node*>(ClearMark(uintPtr, Fdel));
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
                        uintptr_t uintPtr = reinterpret_cast<uintptr_t>(s->head->val);
                        Node* prg = reinterpret_cast<Node *>(ClearMark(uintPtr, Fdel));
                        if (prg->key <= node->key) {
                            uintPtr = reinterpret_cast<uintptr_t>(prg->val);
                            s->head = reinterpret_cast<Node *>(ClearMark(uintPtr, Fdel));
                            for (int i = 0; i < D; i++) {
                                s->node[i].store(s->head);
                            }
                        } else {
                            for (int i = dp; i < D; i++) {
                                s->node[i].store(pred);
                            }
                        }
                    }
                } while(stack.compare_exchange_strong(sNew, s) || IsMarked(reinterpret_cast<uintptr_t>(node->val), Fdel));
                break;
             }
        }
    }
    void printHelper() {
        printHelper(this->head, "0");
    }
private:
    void printHelper(Node* node, string dimension) {
        uintptr_t uintPtr = IsMarked(reinterpret_cast<uintptr_t>(node), Fadp);
        // cout << uintPtr << endl;
        if (node == nullptr || uintPtr) {
            // cout << "is null or marked" << endl;
            return;
        }

        cout << node->key << " " << dimension << endl;
        for (int i = 0; i < D; i++) {
            dimension.push_back('0'+i);
            printHelper(node->child[i].load(memory_order_seq_cst), dimension);
            dimension.pop_back();
        }
    }
};

vector<int> keyToCoord(int key) {
    // TODO: move basis to be global variable
    int basis = ceil(pow(64, 1.0/3));
    // cout << key << " basis is " << basis << endl;
    int quotient = key;
    vector<int> k(3);
    for (int i = 2; i >= 0; i--) {
        k[i] = quotient % basis;
        quotient = quotient / basis;
        // cout << i << k[i] << " ";
    }
    // cout << endl;
    return k;
}

int main() {
    auto pq = new PriorityQueue<3>(64, 100);
    // for (int i = 0; i < 64; i++) {
    //     vector<int> k = keyToCoord(i);
    //     cout << i << " ";
    //     for (int j = 0; j < 3; j++) {
    //         cout << k[j] << " ";
    //     }
    //     cout << endl;
    // }
    pq->insert(1, nullptr);
    pq->insert(2, nullptr);
    pq->insert(3, nullptr);
    pq->insert(4, nullptr);
    pq->insert(6, nullptr);
    pq->insert(10, nullptr);
    pq->insert(12, nullptr);
    pq->insert(13, nullptr);
    pq->insert(18, nullptr);
    pq->insert(19, nullptr);
    pq->insert(22, nullptr);
    pq->insert(25, nullptr);
    pq->insert(33, nullptr);
    pq->insert(34, nullptr);
    pq->insert(36, nullptr);
    pq->insert(40, nullptr);
    pq->insert(48, nullptr);
    pq->insert(49, nullptr);
    pq->insert(50, nullptr);
    pq->insert(51, nullptr);
    pq->insert(52, nullptr);
    pq->insert(56, nullptr);
    pq->insert(60, nullptr);
    pq->insert(61, nullptr);
    pq->insert(62, nullptr);
    pq->insert(63, nullptr);

    pq->insert(32, nullptr);

    pq->printHelper();
    return 0;
}