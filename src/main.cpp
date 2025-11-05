#include <iostream>
#include <thread>
#include <vector>

int main() {
    std::cout << "Hello from main thread\n";

    // Use std::jthread (C++20+) — auto joins on destruction
    std::vector<std::jthread> workers;
    workers.reserve(2);

    for (int i = 0; i < 2; ++i) {
        workers.emplace_back([i] {
            std::cout << "Hello from worker " << i << "\n";
        });
    }

    // No need to call join() — handled automatically
    return 0;
}
