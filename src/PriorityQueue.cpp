#ifndef PQ_CPP
#define PQ_CPP

#include <string>
#include <iostream>
#include <sstream>
#include <bitset>
#include <atomic>
#include <cmath>
#include <vector>
#include <tuple>
#include <omp.h>
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
    head = new Node(0);
    Stack* sNew = new Stack();
    sNew->head = head;
    for (int i = 0; i < D; i++) {
        sNew->node[i].store(head);
    }
    stack.store(sNew);
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
    stringstream buffer;
    buffer << "Worker " << omp_get_thread_num() << " Inserting " << key << ": " << val << endl;

    auto id = ids[key].fetch_add(1);
    auto newKey = (key << IDBits) | id;

    if (IDBits > 0 && key > PriorityLimit) {
        buffer << "Warning: Priority limit exceeded: " << key << " > " << PriorityLimit << endl;
    }
    if (IDBits > 0 && id > IDLimit) {
        buffer << "Warning: ID limit exceeded: " << id << " > " << IDLimit << endl;
    }

    Node* newNode = new Node(newKey, val);
    keyToCoord(newKey, newNode->k);

    Stack* nodeStack = new Stack();
    Node* predNode = nullptr;
    Node* currNode = head;
    nodeStack->head = currNode;
    int currDim = 0;
    int predDim = 0;

    while (true) {
        // locatePred;
        while (currDim < D) {
            while (currNode && newNode->k[currDim] > currNode->k[currDim]) {
                predNode = currNode;
                predDim = currDim;
                currNode = ClearFlags(currNode->child[currDim].load(), Fadp|Fprg);
            }

            if (currNode == nullptr || newNode->k[currDim] < currNode->k[currDim]) {
                break;
            } else {
                nodeStack->node[currDim].store(currNode);
                currDim++;
            }
        }
        // end locatePred

        // buffer << "Located Pred" << endl;
        // buffer << "Node stack: " << formatStack(nodeStack) < endl;

        if (currDim == D) {
            break;
        }

        // Finish adoption of previous node
        AdoptDesc* pendingAdesc = predNode->adesc;
        if (pendingAdesc && pendingAdesc->dp <= predDim && predDim <= pendingAdesc->dc) {
            finishInserting(predNode, pendingAdesc->dp, pendingAdesc->dc);
            currNode = predNode;
            currDim = predDim;
            continue;
        }

        // Finish adoption of current node
        pendingAdesc = currNode ? currNode->adesc : nullptr;
        if (pendingAdesc && predDim != currDim) {
            finishInserting(currNode, pendingAdesc->dp, pendingAdesc->dc);
        }

        // Insert new node
        if (predNode->child[predDim].load() == currNode) {
            // fillNewNode
            AdoptDesc* newAdesc = nullptr;

            if (predDim != currDim) {
                newAdesc = new AdoptDesc();
                newAdesc->curr = currNode;
                newAdesc->dp = predDim;
                newAdesc->dc = currDim;
            }

            for (int i = 0; i < predDim; i++) {
                newNode->child[i].store(SetFlags(nullptr, Fadp));
            }
            for (int i = predDim; i < D; i++) {
                newNode->child[i].store(nullptr);
            }

            newNode->child[currDim].store(currNode);
            newNode->adesc = newAdesc;

            // end fillNewNode

            if (predNode->child[predDim].compare_exchange_strong(currNode, newNode)) {
                if (newAdesc) {
                    finishInserting(newNode, newAdesc->dp, newAdesc->dc);
                }

                if (IsDel(predNode->val) && !IsDel(newNode->val)) {
                    buffer << "Rewinding stack" << endl;

                    // rewindStack

                    Stack* prevStack = nullptr;
                    Stack* currStack = stack.load();
                    Stack* newStack = new Stack();

                    do {
                        buffer << "PredNode: " << predNode->toString() << endl;
                        buffer << "CurrNode: " << (currNode ? currNode->toString() : "null") << endl;
                        buffer << "NewNode: " << newNode->toString() << endl;
                        currStack = stack.load();

                        buffer << "Current stack: " << formatStack(currStack) << endl;
                        buffer << "Node stack: " << formatStack(nodeStack) << endl;

                        if (currStack->head->ver == nodeStack->head->ver) {
                            if (newNode->key <= currStack->node[D-1].load()->key) {
                                newStack->head = nodeStack->head;
                                for (int i = 0; i < predDim; i++)
                                {
                                    newStack->node[i].store(nodeStack->node[i].load());
                                }
                                for (int i = predDim; i < D; i++)
                                {
                                    newStack->node[i].store(predNode);
                                }
                            }
                            else if (prevStack == nullptr) {
                                *newStack = *currStack;
                            }
                            else {
                                break;
                            }
                        }
                        else {
                            buffer << "Warning: purge not implemented" << endl;
                        }

                        prevStack = currStack;

                        buffer << "Candidate stack: " << formatStack(newStack) << endl;
                    } while (!stack.compare_exchange_strong(currStack, newStack) && !IsDel(newNode->val));

                    // end rewindStack

                    buffer << "New stack: " << formatStack(stack) << endl;
                }

                break;
            }
        }

        if (HasFlags(predNode->child[predDim].load(), Fadp | Fprg)) {
            currNode = head;
            predNode = nullptr;
            currDim = 0;
            predDim = 0;
            nodeStack->head = currNode;
        }
        else {
            buffer << "Warning: there should never be duplicate keys|id" << endl;
            currNode = predNode;
            currDim = predDim;
        }
    }

    cout << buffer.str();
}

// template <int D, long N, int R, int IDBits, typename TKey, typename TVal>
// void PriorityQueue<D, N, R, IDBits, TKey, TVal>::purge(Node* hd, Node* prg) {
//     if (hd != head) {
//         return;
//     }
//     Node* hdNew = new Node(0), *prgCopy = new Node(0);
//     // *prgCopy = *prg;
//     prgCopy->adesc = prg->adesc;
//     copy(begin(prg->k), end(prg->k), begin(prgCopy->k));
//     prgCopy->key = prg->key;
//     prgCopy->ver = prg->ver;
//     prgCopy->val.store(prgCopy->val);
//     for (int i = 0; i < D; i++) {
//         prgCopy->child[i].store(nullptr);
//     }
//     hdNew->val = Fdel;
//     hdNew->ver = hd->ver + 1;
//     Node* child;
//     Node* pvt = hd;
//     for (int d = 0; d < D;) {
//         bool locatePivot = true;
//         // LocatePivot
//         uintptr_t unitPtr;
//         while(prg->k[d] > pvt->k[d]) {
//             finishInserting(pvt, d, d);
//             unitPtr = reinterpret_cast<uintptr_t>(pvt->child[d].load(memory_order_seq_cst));
//             pvt = reinterpret_cast<Node*>(ClearMark(unitPtr, Fadp|Fprg));
//         }
//         do {
//             child = pvt->child[d].load(memory_order_seq_cst);
//             unitPtr = reinterpret_cast<uintptr_t>(child);
//         } while(pvt->child[d].compare_exchange_strong(child, reinterpret_cast<Node*>(SetMark(unitPtr, Fprg))) || IsMarked(unitPtr, Fadp|Fprg));
//         if (IsMarked(unitPtr, Fprg)) {
//             child = reinterpret_cast<Node*>(unitPtr, Fprg);
//         } else {
//             locatePivot = false;
//         }
//         if (!locatePivot) {
//             pvt = hd;
//             d = 0;
//             continue;
//         }
//         if (pvt == hd) {
//             hdNew->child[d].store(child);
//             prgCopy->child[d].store(reinterpret_cast<Node*>(Fdel));
//         } else {
//             prgCopy->child[d].store(child);
//             if (d == 0 || prgCopy->child[d - 1].load(memory_order_seq_cst) == reinterpret_cast<Node*>(Fdel)) {
//                 hdNew->child[d].store(prgCopy);
//             }
//         }
//         d = d + 1;
//     }
//     SetDel(hd->val);
//     SetDel(prg->val);
//     head = hdNew;
// }

template <int D, long N, int R, int IDBits, typename TKey, typename TVal>
tuple<TKey, TVal, bool> PriorityQueue<D, N, R, IDBits, TKey, TVal>::deleteMin() {
    stringstream buffer;
    buffer << "Worker " << omp_get_thread_num() << " deleteMin" << endl;

    Node* min = nullptr;
    Stack* prevStack = stack, *newStack = new Stack();
    *newStack = *prevStack;

    buffer << "Previous stack: " << formatStack(prevStack) << endl;

    int d = D - 1;
    // Modified: change the codition from d > 0 --> d >= 0
    while (d >= 0) {
        Node* last = newStack->node[d].load();
        AdoptDesc* pending = last->adesc;

        if (pending && pending->dp <= d && d < pending->dc) {
            finishInserting(last, pending->dp, pending->dc);
        }

        Node* child = last->child[d].load();
        uintptr_t uintPtr = reinterpret_cast<uintptr_t>(child);
        child = reinterpret_cast<Node*>(ClearMark(uintPtr, Fadp|Fprg));
        // cout << "child is:" << d << " " << child << endl;
        if (!child) {
            d = d - 1;
            continue;
        }
        auto val = child->val.load();
        if (IsDel(val)) {
            // if (!(val & ~Fdel)) {
                for (int i = d; i < D; i++) {
                    newStack->node[i].store(child);
                }
                d = D - 1;
            // } else {
            //     newStack->head = reinterpret_cast<Node*>(ClearMark(uintPtr, Fdel));
            //     for (int i = 0; i < D; i++) {
            //         newStack->node[i].store(newStack->head);
            //     }
            //     d = D - 1;
            // }
        } else {
            if (child->val.compare_exchange_strong(val, val | Fdel)) {
                for (int i = d; i < D; i++) {
                    newStack->node[i].store(child);
                }
                min = child;
                stack.compare_exchange_strong(prevStack, newStack);
                // int ori = markedNode.fetch_add(1);
                // bool expected = true;
                // bool target = false;
                // if (ori > R - 1 && notPurging.compare_exchange_strong(expected, target)) {
                //     purge(newStack->head, newStack->node[D- 1].load(memory_order_seq_cst));
                //     markedNode.store(R - ori - 1);
                //     notPurging.compare_exchange_strong(target, expected);
                // }
                break;
            }
        }
    }
    // return min->key;
    // cout << "min address is " << min << endl;

    buffer << "New stack: " << formatStack(stack) << endl;

    cout << buffer.str();
    return min == nullptr? make_tuple<TKey, TVal, bool>(0, 0, false): make_tuple((min->key >> IDBits), GetVal(min->val), true);
}

template <int D, long N, int R, int IDBits, typename TKey, typename TVal>
void PriorityQueue<D, N, R, IDBits, TKey, TVal>::printPQ(Node* node, int dim, string prefix) {
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
        printPQ(node->child[i], i, childPrefix);
    }
}

template <int D, long N, int R, int IDBits, typename TKey, typename TVal>
void PriorityQueue<D, N, R, IDBits, TKey, TVal>::printPQ() {
    printPQ(this->head, 0, "│");
}

template <int D, long N, int R, int IDBits, typename TKey, typename TVal>
string PriorityQueue<D, N, R, IDBits, TKey, TVal>::formatStack(Stack *s) {
    stringstream buffer;
    buffer << "Head: " << (s->head->key >> IDBits) << "-" << (s->head->key & ((1<<IDBits)-1));
    buffer << " Nodes: ";
    for (int i = 0; i < D; i++) {
        Node* curNode = s->node[i].load();
        if (ClearFlags(curNode, Fadp|Fprg) == nullptr) {
            buffer << "null ";
        } else {
            buffer << (curNode->key >> IDBits) << "-" << (curNode->key & ((1<<IDBits)-1)) << " ";
        }
    }
    return buffer.str();
}

template <int D, long N, int R, int IDBits, typename TKey, typename TVal>
void PriorityQueue<D, N, R, IDBits, TKey, TVal>::printStack() {
    cout << formatStack(stack.load()) << endl;
}

#endif