#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <thread>
#include <tuple>
#include <functional>
#include <algorithm>

// not sure why I'm not getting good results here. Threaded one seems slower than
// the serial one, even without -O.

std::vector<float> randvec(long sz) {
    std::vector<float> vec;
    vec.reserve(sz);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(-0.5, 0.5);
    for (long i = 0; i < sz; ++i) {
        vec.push_back(dis(gen));
    }
    return vec;
}

void compute_sum(const std::vector<float> &vec, long beg, long stride, long end, float &result) {
    float sum = 0;
    for (long i = beg; i < end; i += stride) {
        sum += vec[i];
    }
    result = sum;
}

std::tuple<long, long, long> stride_param(long i, long nthr, long sz) {
    return {i, nthr, sz};
}

long ceildiv(long n, long d) {
    return n / d + (n % d != 0);
}

std::tuple<long, long, long> batch_param(long i, long nthr, long sz) {
    long tdim = ceildiv(sz, nthr);
    return {i * tdim, 1, std::min(sz, (i+1) * tdim)};
}

using Param = std::tuple<long, long, long>(long, long, long);

float thread_sum(const std::vector<float> &vec, int nthr, Param pm) {
    std::vector<float> res(nthr, 0);
    std::vector<std::thread> ts;
    for (int i = 0; i < nthr; ++i) {
        auto p = pm(i, nthr, vec.size());
        ts.emplace_back(compute_sum, vec,
                        std::get<0>(p), std::get<1>(p), std::get<2>(p),
                        std::ref(res[i]));
    }
    for (auto &th : ts) {
        th.join();
    }
    float sum = 0;
    compute_sum(res, 0, 1, res.size(), sum);
    return sum;
}

int main() {
    auto t0 = std::chrono::steady_clock::now();
    auto vec = randvec(1L << 27);
    auto t1 = std::chrono::steady_clock::now();
    auto d10 = (t1 - t0) / std::chrono::microseconds(1);
    std::cout << "rand init took " << d10 << " microsecs \n";

    float nt_sum;
    compute_sum(vec, 0, 1, vec.size(), nt_sum);
    std::cout << "non-thread sum was " << nt_sum << '\n';
    auto t2 = std::chrono::steady_clock::now();
    auto d21 = (t2 - t1) / std::chrono::microseconds(1);
    std::cout << "non-thread sum took " << d21 << " microsecs \n";

    float s_sum = thread_sum(vec, 12, stride_param);
    std::cout << "stride sum was " << s_sum << '\n';
    auto t3 = std::chrono::steady_clock::now();
    auto d32 = (t3 - t2) / std::chrono::microseconds(1);
    std::cout << "stride thread sum took " << d32 << " microsecs \n";

    float b_sum = thread_sum(vec, 12, batch_param);
    std::cout << "batch sum was " << b_sum << '\n';
    auto t4 = std::chrono::steady_clock::now();
    auto d43 = (t4 - t3) / std::chrono::microseconds(1);
    std::cout << "stride thread sum took " << d43 << " microsecs \n";
}
