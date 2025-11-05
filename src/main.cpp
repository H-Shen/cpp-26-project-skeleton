#if defined(_MSC_VER) && defined(_DEBUG)
#  define _CRTDBG_MAP_ALLOC
#  include <crtdbg.h>
#endif

#include <iostream>
#include <thread>
#include <vector>

int main() {

#if defined(_MSC_VER) && defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
    
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
