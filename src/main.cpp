#if defined(_MSC_VER) && defined(_DEBUG)
#  define _CRTDBG_MAP_ALLOC
#  include <crtdbg.h>
#endif

#include <mutex>
#include <print>
#include <thread>
#include <vector>

import cpp26_skeleton;

int main() {
#if defined(_MSC_VER) && defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    std::println("{}", cpp26_skeleton::greeting("C++26 Modules"));

    std::mutex output_mutex;
    std::vector<std::jthread> workers;
    workers.reserve(2);

    for (int i = 0; i < 2; ++i) {
        workers.emplace_back([i, &output_mutex] {
            const std::scoped_lock lock(output_mutex);
            std::println("Hello from worker {}", i);
        });
    }

    return cpp26_skeleton::answer() == 42 ? 0 : 1;
}
