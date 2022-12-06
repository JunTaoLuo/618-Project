// #include <atomic>
// #include <cmath>
// #include <vector>

// using namespace std;

// typedef struct AdoptDesc DESC;
// const int Fadp = 0x1, Fprg = 0x2, Fdel = 0x1;

// // memory model: memory_order_seq_cst

// // TODO: check correctness during implementation
// #define SetMark(p, m) ({p |= m; p;})
// #define ClearMark(p, m) ({p &= ~m; p;})
// #define IsMarked(p, m) ({p & m;})


// template <int D>
// class PriorityQueue {
// private:
//     struct Node {
//         int key;
//         vector<int> k;
//         atomic<void*> val; // atomic struct
//         atomic<Node*>* child; // atomic struct
//         DESC* adesc;
//         int fchild, fdel;
//         Node(int _key): key(_key), adesc(nullptr),fchild(0), fdel(0) {
//             val.load(nullptr);
//             k = vector<int>(D, 0);
//             child = new atomic<Node*>[D];
//             for (int i = 0; i < D; i++) {
//                 child[i].store(nullptr);
//             }
//         }
//     };
//     struct AdoptDesc {
//         Node* curr;
//         int dp, dc;
//     };
//     struct HeadNode: public Node {
//         int ver;
//         HeadNode(int _key): Node(_key), ver(0) {} 
//     };
//     struct Stack {
//         atomic<Node*>* node;
//         HeadNode* head;
//         Stack() {
//             node = new atomic<Node*>[D];
//             head = nullptr;
//         }
//     };

//     vector<int> keyToCoord(int key) {
//         int basis = ceil(power(N, double(1/D)));
//         int quotient = key;
//         vector<int> k(D);
//         for (int i = D - 1; i >= 0; i--) {
//             k[i] = quotient % basis;
//             quotient = quotient / basis;
//         }
//         return k;
//     }

//     void finishInserting(Node* n, int dp, int dc) {
//         if (!n) {
//             return;
//         }
//         AdoptDesc* ad = n->adesc;
//         if (!ad || dc < ad->dp || dp > ad->dc) {
//             return;
//         }
//         Node* child, *cur = ad->curr;
//         dp = ad->dp;
//         dc = ad->dc;
//         for (int i = dp; i < dc; i++) {
//             // TODO: check return type
//             atomic_fetch_or(&(cur->child[i].load(memory_order_relaxed)->flag), Fadp);
//             child = ClearMark(child, fchild, Fadp);
//             atomic_compare_exchange_strong(&n->child[i].load(memory_order_relaxed), nullptr, child);
//         }
//         n->adesc = nullptr;
//     }
//     void purge(HeadNode* hd, Node* prg) {
//         if (hd != head) {
//             return;
//         }
//         HeadNode* hdnew = new HeadNode(), *prgcopy = new Node();
//         *prgcopy = *prg;
//         for (int i = 0; i < D; i++) {
//             prgcopy->child[i].store(nullptr);
//         }
//         hdnew->fdel = Fdel;
//         hdnew->ver = hd->ver + 1;
//         HeadNode* pvt = hd;
//         int d = 0;
//         while (d < D) {
//             bool locatePivot = false;
//             while (prg->k[d] > pvt->k[d]) {
//                 finishInserting(pvt, d, d);
//                 pvt = ClearMark(pvt->child[d].load(memory_order_relaxed), Fadp|Fprg);
//             }
//             Node* child;
//             // TODO: check correctness
//             do {
//                 child = pvt->child[d].load(memory_order_relaxed);
//             } while(!atomic_compare_exchange_strong(&pvt->child[d], child, SetMark(child, Fprg)) && (!IsMarked(child, fchild, Fadp|Fprg)));
//             if (IsMarked(child, fchild, Fprg)) {
//                 child = ClearMark(child, fchild, Fprg);
//                 locatePivot = true;
//             }
//             if (!locatePivot()) {
//                 pvt = hd;
//                 d = 0;
//                 continue;
//             }
//             if (pvt == hd) {
//                 hdnew->child[d].store(child);
//                 prgcopy->child[d].load(memory_order_relaxed)->flag = Fdel;
//             } else {
//                 prgcopy->child[d].store(child);
//                 if (d == 0 || prgcopy->child[d-1].load(memory_order_relaxed)->fdel == Fdel) {
//                     hdnew->child[d].store(prgcopy);
//                 }
//             }
//             d++;
//         }
//         hd->val = SetMark(prg, fdel, Fdel);
//         prg->val = SetMark(hdnew, fdel, Fdel);
//         head = hdnew;
//     }

// public:
//     const int N, R;
//     HeadNode* head;
//     atomic<Stack*> stack;
//     PriorityQueue(int _N, int _R): N(_N), R(_R) {
//         // Dummy head node with minimal key 0
//         this->head = new HeadNode(0);
//         Stack* newStack = new Stack();
//         newStack->node[0].store(head);
//         this->stack.store(newStack);
//         // TODO: what about the headnode in stack initialization
//     }
//     Node* deleteMin() {
//         Node* min = nullptr;
//         Stack* sOld = this->stack.load(memory_order_relaxed); 
//         // TODO: *s <- *sold??
//         Stack* s = sOld;
//         int d = D - 1;
//         while (d > 0) {
//             Node* last = s->node[d].load(memory_order_relaxed);
//             finishInserting(last, d, d);
//             Node* child = last->child[d];
//             // TODO: Fadp, Fprg
//             ClearMark(child, fchild, Fadp|Fprg);
//             if (!child) {
//                 d = d - 1;
//                 continue;
//             }
//             void* val = child->val.load(memory_order_relaxed);
//             if (IsMarked(child, fdel, Fdel)) {
//                 // TODO: check correctness
//                 // s.node[d].child[d] has been marked by competing DELETEMIN threads
//                 if (!ClearMark(child, fdel, Fdel)) {
//                     for (int i = d; i < D; i++) {
//                         s->node[i].store(child);
//                     }
//                     d = D - 1;
//                 } else {
//                     s->head = ClearMark(child, fdel, Fdel);
//                     for (int i = 0; i < D; i++) {
//                         s->node[i].store(s->head);
//                     }
//                     d = D - 1;
//                 }
//             } else if (atomic_compare_exchange_strong(&child->val, &val, SetMark(child, fdel, Fdel))) {
//                 for (int i = d; i < D; i++) {
//                     s->node[i].store(child);
//                 }
//                 min = child;
//                 atomic_compare_exchange_strong(&this->stack, &sOld, s);
//                 // TODO: add PURGE
// //                 if (marked_node > R && not_purging) {
// //                     purge(s.head, s.node[D-1]);
// //                 }
//                 break;
//             }
//         }
//         return min;
//     }

//     void insert(int key, void* val) {
//         Stack* s = new Stack();
//         Node* node = new Node();
//         node->key = key;
//         node->val.store(val);
//         node->k = keyToCoord(key);
//         for (int i = 0; i < D; i++) {
//             node->child[i].store(nullptr);
//         }
//         while (true) {
//             Node* pred = nullptr, *curr = head;
//             int dp = 0, dc = 0;
//             s->head = curr;
//             // locatePred;
//             while (dc < D) {
//                 while (curr && node->k[dc] > curr->k[dc]) {
//                     pred = curr;
//                     dp = dc;
//                     finishInserting(curr, dc, dc);
//                     curr = curr->child[dc].load(memory_order_relaxed);
//                 }
//                 if (curr == nullptr || node->k[dc] < curr->k[dc]) {
//                     break;
//                 } else {
//                     s->node[dc].store(curr);
//                     dc++;
//                 }
//             }

//             if dc == D {
//                 break;
//             }
//             finishInserting(curr, dp, dc);
//             // fillNewNode();
//             node->adesc = nullptr;
//             if dp < dc {
//                 node->adesc = new AdoptDesc();
//                 node->adesc->curr = curr;
//                 node->adesc->dp = dp;
//                 node->adesc->dc = dc;
//             }
//             // TODO: check the upper boundary
//             for (int i = 0; i < dp; i++) {
//                 SetMark(node->child[i].load(memory_order_relaxed), fchild, Fadp);
//             }
//             for (int i = dp; i < D; i++) {
//                 node->child[i].store(nullptr);
//             }
//             node->child[dc].store(curr);
//             if (&pred->child[dp],curr, node) {
//                 finishInserting(node, dp, dc);
// //                 rewindStack();
//                 // TODO: check sNew correctness
//                 Stack* sOld = stack, *sNew = s;
//                 bool first_iteration = true;
//                 do {
//                     if (s->head->ver == sOld->head->ver) {
//                         if (node->key <= sNew->node[D-1].load(memory_order_relaxed)->key) {
//                             for (int i = dp; i < D; i++) {
//                                 s->node[i].store(pred);
//                             }
//                         } else if (first_iteration) {
//                             *s = *sNew;
//                             first_iteration = true;
//                         } else {
//                             break;
//                         }
//                     } else if (s->head->ver > sOld->head->ver) {
//                         Node* prg = ClearMark(sOld->head, fdel, Fdel);
//                         if (prg->key <= sOld->node[D-1].load(memory_order_relaxed)->key) {
//                             s->head = ClearMark(prg, fdel, Fdel);
//                             for (int i = 0; i < D; i++) {
//                                 s->node[i].store(s->head);
//                             }
//                         } else if (first_iteration) {
//                             *s = *sOld;
//                             first_iteration = false;
//                         } else {
//                             break;
//                         }
//                     } else {
//                         // TODO: modify to correctness
//                         Node* prg = ClearMark(s->head, fdel, Fdel);
//                         if prg->key <= node->key {
//                             s->head = ClearMark(prg, fdel, Fdel);
//                             for (int i = 0; i < D; i++) {
//                                 s->node[i].store(s->head);
//                             }
//                         } else {
//                             for (int i = dp; i < D; i++) {
//                                 s->node[i] = pred;
//                             }
//                         }
//                     }
//                 } while(atomic_compare_exchange_strong(&stack, sNew, s) || IsMarked(prg, fdel, Fdel));
//                 break;
//              }
//         }
//     }
// };
