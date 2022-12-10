#include <iostream>
#include <bitset>
#include <atomic>
#include <cmath>
#include <vector>
// #include <boost/algorithm/string/join.hpp>

using namespace std;

int Fdel = 0x1, Fadp = 0x1, Fprg = 0x2;

#define SetMark(p, m) ({p |= m; p;})
#define ClearMark(p, m) ({p &= ~m; p;})
#define IsMarked(p, m) ({p & m;})

// Creating and retrieving Val
#define CreateDeletedVal(v) (v << 1)
#define CreateVal(v) (v << 1)
#define GetVal(val) (val.load() >> 1)
// Val flags
#define SetDel(val) ({val |= 1; val.load();})
#define ClearDel(val) ({val &= ~1; val.load();})
#define IsDel(val) (val & 1)


template <int D, long N, int R, typename TKey, typename TVal>
class PriorityQueue {
private:
// public:
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
    void keyToCoord(int key, int* k) {
        int quotient = key;
        for (int i = D-1; quotient && i >= 0 ; i--) {
            k[i] = quotient % basis;
            quotient = quotient / basis;
        }
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
        // *prgCopy = *prg;
        prgCopy->adesc = prg->adesc;
        copy(begin(prg->k), end(prg->k), begin(prgCopy->k));
        prgCopy->key = prg->key;
        prgCopy->ver = prg->ver;
        prgCopy->val.store(prgCopy->val);
        for (int i = 0; i < D; i++) {
            prgCopy->child[i].store(nullptr);
        }
        hdNew->val = Fdel;
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
                if (d == 0 || prgCopy->child[d - 1].load(memory_order_seq_cst) == reinterpret_cast<Node*>(Fdel)) {
                    hdNew->child[d].store(prgCopy);
                }
            }
            d = d + 1;
        }
        SetDel(hd->val);
        SetDel(prg->val);
        head = hdNew;
    }

public:
    int basis;
    Node* head;
    atomic<Stack*> stack;
    atomic<int> markedNode;
    atomic<bool> notPurging;
    PriorityQueue() {
        basis = ceil(pow(N, 1.0/D));
        this->head = new Node(0);
        Stack* sNew = new Stack();
        // TODO: filled with the dummy head?
        sNew->head = this->head;
        for (int i = 0; i < D; i++) {
            sNew->node[i].store(head);
        }
        this->stack.store(sNew);
        markedNode.store(0);
        notPurging.store(true);
    }
    // TODO: Not Implement yet!
    ~PriorityQueue();
    void insert(int key, TVal val) {
        Stack* s = new Stack();
        Node* node = new Node(key, val);
        keyToCoord(key, node->k);
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
            // testing: 
            // cout << "testing stack -------------------------------------" << key << " " << s->head->key << " " << dp << " " << dc << endl;
            // for (int i = 0; i < D; i++) {
            //     Node* curNode = s->node[i].load();
            //     if (curNode) {
            //         cout << curNode->key << " ";
            //     }
            // }
            // cout << endl;
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
                    sNew = stack;
                    if (s->head->ver == sOld->head->ver) {
                        if (node->key <= sNew->node[D-1].load(memory_order_seq_cst)->key) {
                            // MOdified: There is a bug at line 254 if we follow the proof 4 on page 10
                            // Test case will be displayed on PR
                            // for (int i = dp + 1; i < D; i++) {
                            for (int i = dp; i < D; i++) {
                                s->node[i].store(pred);
                            }
                        } else if (first_iteration) {
                            s->head = sNew->head;
                            for (int i = 0; i < D; i++) {
                                s->node[i].store(sNew->node[i].load(memory_order_seq_cst));
                            }
                            // *s = *sNew;
                            first_iteration = false;
                        } else {
                            break;
                        }
                    } else if (s->head->ver > sOld->head->ver) {
                        uintptr_t uintPtr = reinterpret_cast<uintptr_t>(sOld->head);
                        Node* prg = reinterpret_cast<Node*>(ClearMark(uintPtr, Fadp|Fprg));
                        if (prg->key <= sOld->node[D-1].load(memory_order_seq_cst)->key) {
                            // TODO: check correctness(what about Fdel flag);
                            ClearDel(prg->val);
                            s->head = prg;
                            for (int i = 0; i < D; i++) {
                                s->node[i].store(s->head);
                            }
                        } else if (first_iteration) {
                            s->head = sOld->head;
                            for (int i = 0; i < D; i++) {
                                s->node[i].store(sOld->node[i].load(memory_order_seq_cst));
                            }
                            // *s = *sOld;
                            first_iteration = false;
                        } else {
                            break;
                        }
                    } else {
                        uintptr_t uintPtr = reinterpret_cast<uintptr_t>(s->head);
                        Node* prg = reinterpret_cast<Node*>(ClearMark(uintPtr, Fadp|Fprg));
                        if (prg->key <= node->key) {
                            ClearDel(prg->val);
                            s->head = prg;
                            for (int i = 0; i < D; i++) {
                                s->node[i].store(s->head);
                            }
                        } else {
                            // MOdified: There is a bug if we follow the proof 4 on page 10
                            // Test case will be displayed on PR
                            // for (int i = dp + 1; i < D; i++) {
                            for (int i = dp; i < D; i++) {
                                s->node[i].store(pred);
                            }
                        }
                    }
                } while(stack.compare_exchange_strong(sNew, s) || IsDel(node->val));
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
        //     s->node[i].store(sOld->node[i]);
        // }
        int d = D - 1;
        // Modified: change the codition from d > 0 --> d >= 0 
        while (d >= 0) {
            Node* last = s->node[d].load(memory_order_seq_cst);
            finishInserting(last, d, d);
            Node* child = last->child[d].load(memory_order_seq_cst);
            uintptr_t uintPtr = reinterpret_cast<uintptr_t>(child);
            child = reinterpret_cast<Node*>(ClearMark(uintPtr, Fadp|Fprg));
            // cout << "child is:" << d << " " << child << endl;
            if (!child) {
                d = d - 1;
                continue;
            }
            auto val = child->val.load();
            if (IsDel(val)) {
                if (!(val & ~Fdel)) {
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
                // uintptr_t uintPtr = reinterpret_cast<uintptr_t>(val);
                if (child->val.compare_exchange_strong(val, val | Fdel)) {
                    for (int i = d; i < D; i++) {
                        s->node[i].store(child);
                    }
                    min = child;
                    stack.compare_exchange_strong(sOld, s);
                    int ori = markedNode.fetch_add(1);
                    bool expected = true;
                    bool target = false;
                    if (ori > R - 1 && notPurging.compare_exchange_strong(expected, target)) {
                        purge(s->head, s->node[D- 1].load(memory_order_seq_cst));
                        markedNode.store(R - ori - 1);
                        notPurging.compare_exchange_strong(target, expected);
                    }
                }
                break;
            }
        }
        // return min->key;
        // cout << "min address is " << min << endl;
        return min == nullptr? -1: min->key;
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
        cout << node->k[D-1] << "]: " << GetVal(node->val) << "(" << IsDel(node->val) << ")" << endl;

        for (int i = D-1; i >= dim; i--) {
            string childPrefix = prefix;
            for (int j = dim; j < i; j++) {
                childPrefix += "│";
            }
            printHelper(node->child[i], i, childPrefix);
        }

    }
};

int main() {

    // auto pq = new PriorityQueue<8, 4294967295, 100, long, long>();
    auto pq = new PriorityQueue<3, 64, 100, long, long>();

    pq->insert(1, 1);
    pq->insert(2, 2);
    pq->insert(3, 3);
    pq->insert(4, 4);
    pq->insert(6, 6);
    pq->insert(10, 10);
    pq->insert(12, 12);
    pq->insert(13, 13);
    pq->insert(18, 18);
    pq->insert(19, 19);
    pq->insert(22, 22);
    pq->insert(23, 23);
    pq->insert(25, 25);
    pq->insert(33, 33);
    pq->insert(34, 34);
    pq->insert(36, 36);
    pq->insert(40, 40);
    pq->insert(48, 48);
    pq->insert(49, 49);
    pq->insert(50, 50);
    pq->insert(51, 51);
    pq->insert(52, 52);
    pq->insert(56, 56);
    pq->insert(60, 60);
    pq->insert(61, 61);
    pq->insert(62, 62);
    pq->insert(63, 63);

    cout << "After the insertion -----------------------------" << endl;
    pq->printHelper();
    cout << "Delete the min value ----------------------------" << endl;
    for (int i = 0; i < 8; i++) {
        auto minVal = pq->deleteMin();
        cout << minVal << endl;
    }
    cout << "After the deletion1 -----------------------------" << endl;
    pq->printHelper();
    pq->printStack();
    cout << "After the deletion2 -----------------------------" << endl;
    // pq->printHelper();
    auto minVal = pq->deleteMin();
    cout << minVal << endl;
    pq->printStack();
    cout << "Insert some smaller nodes ----------------------" << endl;
    pq->insert(1, 1);
    pq->insert(2, 2);
    pq->insert(3, 3);
    // pq->insert(4, nullptr);
    // pq->insert(32, nullptr);
    cout << "After the insertion ------------------------------" << endl;
    pq->printHelper();
    pq->printStack();
    return 0;
}