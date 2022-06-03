#include <thread>
#include <iostream>
#include <functional>
#include <atomic>
#include <vector>

// Surprising that there's a difference even on AMD64. From
// SO I get the impression that std::atomic is for primitives, not
// user-defined types. Main issue is that load() returns a COPY, not
// the stored object itself, therefore I'm not sure how to actually
// mutate the T object stored in atomic<T> if T is mutable.
//
// Also for primitives, atomic provides ++, +=, --, etc.
//
// The below example was the simplest that gave different results
// for with and without atomic.

void inc(std::atomic<int> &x) {
    for (int i = 0; i < 1000; ++i) {
        ++x;
    }
}

void bar() {
    std::atomic<int> x;
    x.store(0);
    std::vector<std::thread> ts;
    for (int i = 0; i < 64; ++i) {
        ts.emplace_back(inc, std::ref(x));
    }
    for (auto &t : ts) {
        t.join();
    }
    std::cout << x << '\n';
}

// exact same as above except without atomic.
void inc0(int &x) {
    for (int i = 0; i < 10000; ++i) {
        ++x;
    }
}

void bar0() {
    int x = 0;
    std::vector<std::thread> ts;
    for (int i = 0; i < 32; ++i) {
        ts.emplace_back(inc0, std::ref(x));
    }
    for (auto &t : ts) {
        t.join();
    }
    std::cout << x << '\n';
}

int main(int argc, char **argv) {
    bar();  // consistently 64000
    bar0();  // varies greatly
}
