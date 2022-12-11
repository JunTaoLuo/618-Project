#include <iostream>
#include <bitset>
#include <atomic>
#include <cmath>
#include <vector>
#include <tuple>
#include "PriorityQueue.cpp"

using namespace std;

int main() {

    // auto pq = new PriorityQueue<8, 4294967295, 100, long, long>();
    auto pq = new PriorityQueue<3, 64, 100, long, long>();

    // DeleteMin on empty queue
    auto minVal = pq->deleteMin();
    cout << "deleteMin when empty -----------------------------" << endl;
    cout << get<0>(minVal) << ": " << get<1>(minVal) << "(" << get<2>(minVal) << ")" << endl;

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
        cout << get<0>(minVal) << ": " << get<1>(minVal) << "(" << get<2>(minVal) << ")" << endl;
    }
    cout << "After the deletion1 -----------------------------" << endl;
    pq->printHelper();
    pq->printStack();
    cout << "After the deletion2 -----------------------------" << endl;
    // pq->printHelper();
    minVal = pq->deleteMin();
    cout << get<0>(minVal) << ": " << get<1>(minVal) << "(" << get<2>(minVal) << ")" << endl;
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