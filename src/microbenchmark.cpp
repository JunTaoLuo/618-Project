#include <iostream>
#include <vector>
#include <chrono>
#include <algorithm>
#include <random>
#include <fstream>
#include "PriorityQueue.cpp"
#include "timing.h"
#include <queue>
#include "pardijk.cpp"
#include <omp.h>
using namespace std;

// reference: https://stackoverflow.com/questions/36922371/generate-different-random-numbers
vector<int> generateRandNum(int size) {
    vector<int> numbers;
    for (int i = 0; i < size; i++) {
        numbers.push_back(i + 1);
    }
    ofstream randomFile("../microbenchmark/random-ref.txt");
    // for (int i = 0; i < size; i++) {
    //     randomFile << numbers[i] << endl;
    // }
    // randomFile.close();
    unsigned seed = chrono::system_clock::now().time_since_epoch().count();
    shuffle(numbers.begin(), numbers.end(), default_random_engine(seed));
    return numbers;
}

void insertDelete(vector<int>& randNums, PriorityQueue<8, 1000001, 100, 0, int, int>* pq) {
    Timer t;
    #pragma omp parallel for schedule(static, 1)
    for (int i = 0; i < randNums.size(); i++) {
        pq->insert(randNums[i], 0);
    }
    cout << "insert time is: " << t.elapsed() << endl;
    t.reset();

    #pragma omp parallel for schedule(static, 1)
    for (int i = 0; i < randNums.size(); i++) {
        auto currMin = pq->deleteMin();
        // cout << omp_get_thread_num() << " " << get<0>(currMin) << " " << get<1>(currMin) << " " << get<2>(currMin) << endl;
    }
    cout << "delete time is: " << t.elapsed() << endl;
    return;
}

void insertDeleteSeq(vector<int>& randNums, priority_queue<int, vector<int>, greater<int>>& pq) {
    Timer t;
    for (int i = 0; i < randNums.size(); i++) {
        pq.push(randNums[i]);
    }
    cout << "sequential insert time is: " << t.elapsed() << endl;
    t.reset();
    for (int i = 0; i < randNums.size(); i++) {
        // int currMin = pq.top();
        pq.pop();
        // cout << currMin << endl;
    }
    cout << "sequential delete time is: " << t.elapsed() << endl;
}

void insertDeleteCoarse(vector<int>& randNums, GlobalLockPQ* pq) {
    Timer t;
    #pragma omp parallel for schedule(static, 1)
    for (int i = 0; i < randNums.size(); i++) {
        pq->insert(Offer(i, randNums[i]));
    }
    cout << "globalLock insert time is: " << t.elapsed() << endl;
    t.reset();
    #pragma omp parallel for schedule(static, 1)
    for (int i = 0; i < randNums.size(); i++) {
        pq->deleteMin();
    }
    cout << "globalLock delete time is: " << t.elapsed() << endl;
}

int main(int argc, char *argv[]) {
    vector<int> randNums = generateRandNum(1000000);
    // MicroTestOptions options = parseTestOptions(argc, argv);
    auto pq = new PriorityQueue<8, 1000001, 100, 0, int, int>();
    priority_queue<int, vector<int>, greater<int>> seqPQ;
    GlobalLockPQ* glPQ = new GlobalLockPQ();
    Timer t;
    t.reset();
    insertDelete(randNums, pq);
    cout << "Insert and Delele time: " << t.elapsed() << "----------------------" << endl;
    t.reset();
    insertDeleteSeq(randNums, seqPQ);
    cout << "Sequential Insert and Delele time: " << t.elapsed() << "---------------------------" << endl;
    t.reset();
    insertDeleteCoarse(randNums, glPQ);
    cout << "GlobalLock Insert and Delele time: " << t.elapsed() << "---------------------------" << endl;   
    return 0;
}