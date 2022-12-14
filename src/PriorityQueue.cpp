#ifndef PQ_CPP
#define PQ_CPP

#include <iostream>
#include <cstring>
#include <bitset>
#include <atomic>
#include <cmath>
#include <vector>
#include <tuple>
#include "PriorityQueue.h"

using namespace std;

static int Fdel = 0x1, Fadp = 0x1, Fprg = 0x2;

#define SetMark(p, m) ({p |= m; p;})
#define ClearMark(p, m) ({p &= ~m; p;})
#define IsMarked(p, m) ({p & m;})

#define ClearFlags(ptr, flags) ((Node*)(((uintptr_t)ptr) & ~(flags)))
#define SetFlags(ptr, flags) ((Node*)(((uintptr_t)ptr) | (flags)))
#define HasFlags(ptr, flags) (((uintptr_t)ptr) & (flags))

// Creating and retrieving Val
#define CreateDeletedVal(v) (v << 1)
#define CreateVal(v) (v << 1)
#define GetVal(val) (val.load() >> 1)
// Val flags
#define SetDel(val) ({val |= 1; val.load();})
#define ClearDel(val) ({val &= ~1; val.load();})
#define IsDel(val) (val & 1)

template <int D, long N, int R, int IDBits, typename TKey, typename TVal>
PriorityQueue<D, N, R, IDBits, TKey, TVal>::PriorityQueue():
    Basis(ceil(pow(N, 1.0/D))),
    IDLimit((1 << IDBits) - 1),
    PriorityLimit((1L << (32-IDBits)) - 1),
    ids(PriorityLimit+1)
{
    ids[0].fetch_add(1);
    this->head = new Node(0);
    SetDel(head->val);
    Stack* sNew = new Stack();
    // TODO: filled with the dummy head?
    sNew->head = this->head;
    for (int i = 0; i < D; i++) {
        sNew->node[i].store(head);
    }
    this->stack.store(sNew);
}

template <int D, long N, int R, int IDBits, typename TKey, typename TVal>
PriorityQueue<D, N, R, IDBits, TKey, TVal>::~PriorityQueue() { }

// Helper function to map priority to key vector
template <int D, long N, int R, int IDBits, typename TKey, typename TVal>
void PriorityQueue<D, N, R, IDBits, TKey, TVal>::keyToCoord(int key, int* k) {
    int quotient = key;
    for (int i = D-1; quotient && i >= 0 ; i--) {
        k[i] = quotient % Basis;
        quotient = quotient / Basis;
    }
}

template <int D, long N, int R, int IDBits, typename TKey, typename TVal>
void PriorityQueue<D, N, R, IDBits, TKey, TVal>::finishInserting(Node* n, int dp, int dc) {
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

template <int D, long N, int R, int IDBits, typename TKey, typename TVal>
void PriorityQueue<D, N, R, IDBits, TKey, TVal>::insert(TKey key, TVal val) {
        auto id = ids[key].fetch_add(1);
        auto newKey = (key << IDBits) | id;
        // cout << "The D is: " << D << endl;
        if (IDBits > 0 && key > PriorityLimit) {
            cout << "Warning: Priority limit exceeded: " << key << " > " << PriorityLimit << endl;
        }
        if (IDBits > 0 && id > IDLimit) {
            cout << "Warning: ID limit exceeded: " << id << " > " << IDLimit << endl;
        }


        Stack s;
        Node* node = new Node(newKey, val);
        keyToCoord(newKey, node->k);
        Node* pred = nullptr, *curr = head;
        int dp = 0, dc = 0;
        s.head = curr;

        while (true) {
            // locatePred;
            while (dc < D) {
                while (curr && node->k[dc] > curr->k[dc]) {
                    pred = curr;
                    dp = dc;
                    // finishInserting(curr, dc, dc);
                    // curr = curr->child[dc].load(memory_order_seq_cst);
                    // uintptr_t uintPrt = reinterpret_cast<uintptr_t>(curr->child[dc].load(memory_order_seq_cst));
                    // curr = reinterpret_cast<Node*>(ClearMark(uintPrt, Fadp|Fprg));
                    curr = ClearFlags(curr->child[dc].load(), Fadp|Fprg);
                }
                if (curr == nullptr || node->k[dc] < curr->k[dc]) {
                    break;
                } else {
                    s.node[dc].store(curr);
                    dc++;
                }
            }
            if (dc == D) {
                break;
            }
            AdoptDesc* ad = pred->adesc;
            if (ad && ad->dp <= dp && dp <= ad->dc) {
                finishInserting(pred, ad->dp, ad->dc);
                curr = pred;
                dc = dp;
                continue;
            }
            ad = curr? curr->adesc: nullptr;
            if (ad && dp != dc) {
                finishInserting(curr, ad->dp, ad->dc);
            }
            // Node* predChild = pred->child[dp].load(memory_order_seq_cst);
            if (pred->child[dp].load(memory_order_seq_cst) == curr) {
                AdoptDesc* adesc = nullptr;
                if (dp != dc) {
                    adesc = new AdoptDesc();
                    adesc->curr = curr;
                    adesc->dp = dp;
                    adesc->dc = dc;
                }
                for (int i = 0; i < dp; i++) {
                    node->child[i].store(SetFlags(nullptr, Fadp));
                }
                for (int i = dp; i < D; i++) {
                    node->child[i].store(nullptr);
                }
                node->child[dc].store(curr);
                node->adesc = adesc;
                if (pred->child[dp].compare_exchange_strong(curr, node)) {
                    cout << "change pred node and prepare for the rewind: " << newKey << " "  << s.head->ver << " " << stack.load()->head->ver << endl;
                    cout << "the Fdel flag " << IsDel(pred->val) << " " << IsDel(node->val) << " " << (pred->key >> IDBits) << " " << (node->key >> IDBits) << endl;
                    if (adesc) {
                        finishInserting(node, dp, dc);
                    }
                    if (IsDel(pred->val) && !IsDel(node->val)) {
                        cout << "insert key: " << newKey << "and rewind the stack" << endl;
                        Stack* sOld = nullptr, *sNew = new Stack();
                        Stack* curStack;
                        do {
                            curStack = stack.load(memory_order_seq_cst);
                            if (s.head->ver == curStack->head->ver) {
                                cout << "comparison: " << newKey << " " << (curStack->node[D-1].load(memory_order_seq_cst)->key) << endl;
                                if (newKey <= curStack->node[D-1].load(memory_order_seq_cst)->key) {
                                    cout << "branch 1 " << newKey << " " << ((curStack->node[D-1].load(memory_order_seq_cst)->key) >> IDBits) << endl;
                                    sNew->head = s.head;
                                    for (int i = 0; i < dp; i++) {
                                        sNew->node[i].store(s.node[i]);
                                    }
                                    // MOdified: There is a bug at line 254 if we follow the proof 4 on page 10
                                    // Test case will be displayed on PR
                                    // for (int i = dp + 1; i < D; i++) {
                                    for (int i = dp; i < D; i++) {
                                        sNew->node[i].store(pred);
                                    }
                                } else if (sOld == nullptr) {
                                    cout << "branch 2" << endl;
                                    // s->head = sNew->head;
                                    // for (int i = 0; i < D; i++) {
                                    //     s->node[i].store(sNew->node[i].load(memory_order_seq_cst));
                                    // }
                                    // *s = *sNew;
                                    *sNew = *curStack;
                                    // first_iteration = false;
                                } else {
                                    cout << "branch 3" << endl;
                                    break;
                                }
                            } else {
                                cout << "The current stack is outdated" << endl;
                                // break;
                            }
                            sOld = curStack;
                        } while(!stack.compare_exchange_strong(sOld, sNew) && !IsDel(node->val));
                    }
                break;
            }
        }
        // uintptr_t pcVal = reinterpret_cast<uintptr_t>(predChild);
        if (HasFlags(pred->child[dp].load(), Fadp|Fprg)) {
            curr = head;
            dc = 0;
            pred = nullptr;
            dp = 0;
            s.head = curr;
        } else {
            curr = pred;
            dc = dp;
        }
    }
    // testing:
    Stack* tmp = stack.load();
    cout << "testing stack -------------------------------------" << key << " " << (tmp->head->key >> IDBits) << " " << dp << " " << dc << endl;
    for (int i = 0; i < D; i++) {
        Node* curNode = tmp->node[i].load();
        if (curNode) {
            cout << (curNode->key >> IDBits) << " ";
        } else {
            cout << "no ptr" << " ";
        }
    }
    cout << "finish testing the stack --------------------------" << endl;
}

template <int D, long N, int R, int IDBits, typename TKey, typename TVal>
void PriorityQueue<D, N, R, IDBits, TKey, TVal>::purge(Node* hd, Node* prg) {
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

template <int D, long N, int R, int IDBits, typename TKey, typename TVal>
tuple<TKey, TVal, bool> PriorityQueue<D, N, R, IDBits, TKey, TVal>::deleteMin() {
    Node* min = nullptr;
    Stack* sOld = stack, *s = new Stack();
    // *s = *sOld;
    s->head = sOld->head;
    memcpy(s->node, sOld->node, sizeof(Node*)*D);
    // s->head = sOld->head;
    // for (int i = 0; i < D; i++) {
    //     s->node[i].store(sOld->node[i]);
    // }
    int d = D - 1;
    // Modified: change the codition from d > 0 --> d >= 0
    while (d >= 0) {
        Node* last = s->node[d].load(memory_order_seq_cst);
        AdoptDesc* adesc = last->adesc;
        if (adesc && adesc->dp <= d && d < adesc->dc) {
            finishInserting(last, adesc->dp, adesc->dc);
        }
        Node* child = last->child[d].load(memory_order_seq_cst);
        uintptr_t childPtr = reinterpret_cast<uintptr_t>(child);
        child = reinterpret_cast<Node*>(ClearMark(childPtr, 3));
        // cout << "child is:" << d << " " << child << endl;
        if (!child) {
            d = d - 1;
            continue;
        }
        // uintptr_t valPtr = reinterpret_cast<uintptr_t>(child->val.load(memory_order_seq_cst));
        auto val = child->val.load();
        if (IsDel(val)) {
            for (int i = d; i < D; i++) {
                s->node[i].store(child);
            }
            d = D - 1;
            // if (!ClearDel(child->val)) {
            //     for (int i = d; i < D; i++) {
            //         s->node[i].store(child);
            //     }
            //     d = D - 1;
            // } else {
            //     // TODO: Purge
            //     cout << "Purge hasn't been implemented yet" << endl;
            // }
        } else {
            // SetDel(child->val);
            if (child->val.compare_exchange_strong(val, val|Fdel)) {
                for (int i = d; i < D; i++) {
                    s->node[i].store(child);
                }
                min = child;
                stack.compare_exchange_strong(sOld, s);
                break;
            }
        }
    }
    // return min->key;
    // cout << "min address is " << min << endl;
    return min == nullptr? make_tuple<TKey, TVal, bool>(0, 0, false): make_tuple((min->key >> IDBits), GetVal(min->val), true);
}

template <int D, long N, int R, int IDBits, typename TKey, typename TVal>
void PriorityQueue<D, N, R, IDBits, TKey, TVal>::printHelper(Node* node, int dim, string prefix) {
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

    cout << newPrefix << (node->key >> IDBits) << "-" << (node->key & ((1<<IDBits)-1)) << " (" << std::bitset<2>(flags) << ") [";
    for (int i = 0; i < D-1; i++) {
        cout << node->k[i] << ", ";
    }
    cout << node->k[D-1] << "]: " << GetVal(node->val) << "(deleted: " << IsDel(node->val) << ")" << endl;

    for (int i = D-1; i >= dim; i--) {
        string childPrefix = prefix;
        for (int j = dim; j < i; j++) {
            childPrefix += "│";
        }
        printHelper(node->child[i], i, childPrefix);
    }
}

template <int D, long N, int R, int IDBits, typename TKey, typename TVal>
void PriorityQueue<D, N, R, IDBits, TKey, TVal>::printHelper() {
    printHelper(this->head, 0, "│");
}

template <int D, long N, int R, int IDBits, typename TKey, typename TVal>
void PriorityQueue<D, N, R, IDBits, TKey, TVal>::printStack() {
    cout << "Print the deletion stack" << endl;
    Stack* curStack = this->stack.load();
    cout << "The head of the stack is: " << (curStack->head->key >> IDBits) << "-" << (curStack->head->key & ((1<<IDBits)-1)) << endl;
    cout << "The nodes of the stack are: " << endl;
    for (int i = 0; i < D; i++) {
        Node* curNode = curStack->node[i].load();
        cout << (curNode->key >> IDBits) << "-" << (curNode->key & ((1<<IDBits)-1)) << endl;
    }
}

#endif