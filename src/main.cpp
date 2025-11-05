#include <iostream>
#include <thread>
#include <vector>

int main() {
    std::cout << "Hello from main thread\n";

    std::vector<std::thread> workers;
    workers.reserve(2);

    for (int i = 0; i < 2; ++i) {
        workers.emplace_back([i] {
            std::cout << "Hello from worker " << i << "\n";
        });
    }

    for (auto &t : workers) {
        t.join();
    }
    return 0;
}
