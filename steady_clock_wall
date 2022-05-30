#include <iostream>
#include <thread>
#include <chrono>

void wait() {
    auto t0 = std::chrono::steady_clock::now();
    std::this_thread::sleep_for(std::chrono::seconds(3));
    auto t1 = std::chrono::steady_clock::now();
    std::cout << (t1 - t0) / std::chrono::milliseconds(1) << " inside\n";
}

int main() {
    auto t0 = std::chrono::steady_clock::now();
    std::thread th0(wait);
    std::thread th1(wait);
    th0.join();
    th1.join();
    auto t1 = std::chrono::steady_clock::now();
    std::cout << (t1 - t0) / std::chrono::milliseconds(1) << " outside\n";
}

// 3000 inside
// 3000 inside
// 3000 outside
