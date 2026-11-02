module cpp26_skeleton;

import std;

namespace cpp26_skeleton {

std::string greeting(const std::string_view name) {
    return std::format("Hello from {}!", name);
}

int answer() noexcept {
    return 42;
}

} // namespace cpp26_skeleton
