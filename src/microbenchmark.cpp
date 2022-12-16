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

#define BenchmarkDim 8

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

void benchmarkMDList(vector<int>& randNums) {
    auto pq = new PriorityQueue<BenchmarkDim, 1000001, 100, 0, int, int>();

    Timer t;
    #pragma omp parallel for schedule(static, 1)
    for (int i = 0; i < randNums.size(); i++) {
        pq->insert(randNums[i], 0);
    }
    cout << "MDList Insert: " << t.elapsed() << endl;

    t.reset();
    #pragma omp parallel for schedule(static, 1)
    for (int i = 0; i < randNums.size(); i++) {
        auto currMin = pq->deleteMin();
    }
    cout << "MDList Delete: " << t.elapsed() << endl;

    t.reset();
    #pragma omp parallel for schedule(static, 1)
    for (int i = 0; i < randNums.size(); i++) {
        if (rand() % 2) {
            pq->insert(randNums[i], 0);
        }
        else {
            auto currMin = pq->deleteMin();
        }
    }
    cout << "MDList Mixed: " << t.elapsed() << endl;
    return;
}

void benchmarkSeq(vector<int>& randNums) {
    priority_queue<Offer> pq;

    Timer t;
    for (int i = 0; i < randNums.size(); i++) {
        pq.push(Offer(randNums[i], randNums[i]));
    }
    cout << "Seq Insert: " << t.elapsed() << endl;

    t.reset();
    for (int i = 0; i < randNums.size(); i++) {
        auto currMin = pq.top();
        pq.pop();
        // cout << currMin << endl;
    }
    cout << "Seq DeleteMin: " << t.elapsed() << endl;

    t.reset();
    for (int i = 0; i < randNums.size(); i++) {
        if (rand() % 2) {
            pq.push(Offer(randNums[i], randNums[i]));
        }
        else if (pq.size() > 0) {
            auto currMin = pq.top();
            pq.pop();
        }
    }
    cout << "Seq Mixed: " << t.elapsed() << endl;
}

void benchamrkGLock(vector<int>& randNums) {
    GlobalLockPQ* pq = new GlobalLockPQ();

    Timer t;
    #pragma omp parallel for schedule(static, 1)
    for (int i = 0; i < randNums.size(); i++) {
        pq->insert(Offer(i, randNums[i]));
    }
    cout << "GLock Insert: " << t.elapsed() << endl;

    t.reset();
    #pragma omp parallel for schedule(static, 1)
    for (int i = 0; i < randNums.size(); i++) {
        pq->deleteMin();
    }
    cout << "GLock Delete: " << t.elapsed() << endl;

    t.reset();
    #pragma omp parallel for schedule(static, 1)
    for (int i = 0; i < randNums.size(); i++) {
        if (rand() % 2) {
            pq->insert(Offer(i, randNums[i]));
        }
        else {
            pq->deleteMin();
        }
    }
    cout << "Seq Mixed: " << t.elapsed() << endl;
}

int main(int argc, char *argv[]) {
    vector<int> randNums = generateRandNum(1000000);
    // MicroTestOptions options = parseTestOptions(argc, argv);

    // benchmarkSeq(randNums);
    benchamrkGLock(randNums);
    benchmarkMDList(randNums);
    return 0;
}