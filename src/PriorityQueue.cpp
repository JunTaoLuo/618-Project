#include <iostream>
#include <bitset>
#include <atomic>
#include <cmath>
#include <vector>
#include <boost/algorithm/string/join.hpp>

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
        atomic<void*> val;
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
    void purge(Node* hd, Node* prg) {
        if (hd != head) {
            return;
        }
        Node* hdNew = new Node(0), *prgCopy = new Node(0);
        *prgCopy = *prg;
        for (int i = 0; i < D; i++) {
            prgCopy->child[i].store(nullptr);
        }
        hdNew->val = reinterpret_cast<void *>(Fdel);
        hdNew->ver = hd->ver + 1;
        Node* child;
        Node* pvt = hd;
        for (int d = 0; d < D;) {
            bool locatePivot = true;
            // LocatePivot
            uintptr_t unitPtr;
            while(prg->k[d] > pvt->k[d]) {
                finishInserting(pvt, d, d);
                unitPtr = reinterpret_cast<uintptr_t>(pvt->child[d].load(memory_order_seq_cst));
                pvt = reinterpret_cast<Node*>(ClearMark(unitPtr, Fadp|Fprg));
            }
            do {
                child = pvt->child[d].load(memory_order_seq_cst);
                unitPtr = reinterpret_cast<uintptr_t>(child);
            } while(pvt->child[d].compare_exchange_strong(child, reinterpret_cast<Node*>(SetMark(unitPtr, Fprg))) || IsMarked(unitPtr, Fadp|Fprg));
            if (IsMarked(unitPtr, Fprg)) {
                child = reinterpret_cast<Node*>(unitPtr, Fprg);
            } else {
                locatePivot = false;
            }
            if (!locatePivot) {
                pvt = hd;
                d = 0;
                continue;
            }
            if (pvt == hd) {
                hdNew->child[d].store(child);
                prgCopy->child[d].store(reinterpret_cast<Node*>(Fdel));
            } else {
                prgCopy->child[d].store(child);
                if (d == 0 || prgCopy.child[d - 1].load(memory_order_seq_cst) == reinterpret_cast<Node*>(Fdel)) {
                    hdNew->child[d].store(prgCopy);
                }
            }
            d = d + 1;
        }
        uintptr_t uintPtr = reinterpret_cast<uintptr_t>(prg);
        uintptr_t uintPtrForHdNew = reinterpret_cast<uintptr_t>(hdNew);
        hd->val.store(reinterpret_cast<void*>(uintPtr, Fdel));
        prg->val.store(reinterpret_cast<void*>(uintPtrForHdNew, Fdel));
        head = hdNew;
    }

public:
    const int N, R;
    int markedNode;
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
        markedNode = 0;
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
                            uintPtr = reinterpret_cast<uintptr_t>(prg->val.load(memory_order_seq_cst));
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
                        uintptr_t uintPtr = reinterpret_cast<uintptr_t>(s->head->val.load(memory_order_seq_cst));
                        Node* prg = reinterpret_cast<Node *>(ClearMark(uintPtr, Fdel));
                        if (prg->key <= node->key) {
                            uintPtr = reinterpret_cast<uintptr_t>(prg->val.load(memory_order_seq_cst));
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
                } while(stack.compare_exchange_strong(sNew, s) || IsMarked(reinterpret_cast<uintptr_t>(node->val.load(memory_order_seq_cst)), Fdel));
                break;
             }
        }
    }
    void printHelper() {
        printHelper(this->head, 0, "│");
    }
    void printStack() {
        cout << "Print the deletion stack" << endl;
        Stack* curStack = this->stack.load();
        cout << "The head of the stack is: " << curStack->head->key << endl;
        cout << "The nodes of the stack are: " << endl;
        for (int i = 0; i < D; i++) {
            Node* curNode = curStack->node[i].load();
            cout << curNode->key << endl;
        }
    }
    // TODO: Change the return type
    int deleteMin() {
        Node* min = nullptr;
        Stack* sOld = stack, *s = new Stack();
        *s = *sOld;
        // s->head = sOld->head;
        // for (int i = 0; i < D; i++) {
        //     s->node[i].store(sOld->node[i].load(memory_order_seq_cst));
        // }
        int d = D - 1;
        while (d > 0) {
            Node* last = s->node[d].load(memory_order_seq_cst);
            finishInserting(last, d, d);
            Node* child = last->child[d].load(memory_order_seq_cst);
            uintptr_t uintPtr = reinterpret_cast<uintptr_t>(child);
            child = reinterpret_cast<Node*>(ClearMark(uintPtr, Fadp|Fprg));
            if (!child) {
                d = d - 1;
                continue;
            }
            void* val = child->val.load();
            if (IsMarked(reinterpret_cast<uintptr_t>(val), Fdel)) {
                uintPtr = reinterpret_cast<uintptr_t>(val);
                if (!ClearMark(uintPtr, Fdel)) {
                    for (int i = d; i < D; i++) {
                        s->node[i].store(child);
                    }
                    d = D - 1;
                } else {
                    s->head = reinterpret_cast<Node*>(ClearMark(uintPtr, Fdel));
                    for (int i = 0; i < D; i++) {
                        s->node[i].store(s->head);
                    }
                    d = D - 1;
                }
            } else {
                uintptr_t uintPtr = reinterpret_cast<uintptr_t>(val);
                if (child->val.compare_exchange_strong(val, reinterpret_cast<void *>(SetMark(uintPtr, Fdel)))) {
                    for (int i = d; i < D; i++) {
                        s->node[i].store(child);
                    }
                    min = child;
                    stack.compare_exchange_strong(sOld, s);
                    markedNode++;
                    if (markedNode > R) {
                        purge(s->head.load(memory_order_seq_cst), s->node[D- 1].load(memory_order_seq_cst));
                    }
                    /**
                     * if (marked_node > R && not_purging) {
                     *  purge(s->head, s.node[D - 1])
                     * }
                     */
                }
                break;
            }
        }
        return min->key;
    }
private:
    void printHelper(Node* node, int dim, string prefix) {
        if (node == nullptr) {
            return;
        }

        uintptr_t uintptr = reinterpret_cast<uintptr_t>(node);
        uintptr_t flags = IsMarked(uintptr, Fadp|Fprg);
        node = reinterpret_cast<Node*>(ClearMark(uintptr, Fadp|Fprg));

        bool lastChild = node->child[dim] == nullptr;

        // cout << prefix.length() << endl;
        string newPrefix = prefix;
        newPrefix.pop_back();
        newPrefix.pop_back();
        newPrefix.pop_back();
        if (lastChild) {
            newPrefix += "└";
            prefix.pop_back();
            prefix.pop_back();
            prefix.pop_back();
            prefix += " ";
        } else {
            newPrefix += "├";
        }

        cout << newPrefix << node->key << " (" << std::bitset<2>(flags) << ") [";
        for (int i = 0; i < D-1; i++) {
            cout << node->k[i] << ", ";
        }
        cout << node->k[D-1] << "]: " << endl;

        for (int i = D-1; i >= dim; i--) {
            string childPrefix = prefix;
            for (int j = dim; j < i; j++) {
                childPrefix += "│";
            }
            printHelper(node->child[i], i, childPrefix);
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

    cout << "After the insertion -----------------------------" << endl;
    pq->printHelper();
    cout << "Delete the min value ----------------------------" << endl;
    int minVal = pq->deleteMin();
    cout << minVal << endl;
    minVal = pq->deleteMin();
    cout << minVal << endl;
    minVal = pq->deleteMin();
    cout << minVal << endl;
    cout << "After the deletion -----------------------------" << endl;
    pq->printHelper();
    pq->printStack();
    cout << "Insert some smaller nodes ----------------------" << endl;
    // pq->insert(1, nullptr);
    // pq->insert(2, nullptr);
    pq->insert(3, nullptr);
    pq->insert(4, nullptr);
    cout << "After the insertin ------------------------------" << endl;
    pq->printHelper();
    pq->printStack();
    return 0;
}