export module cpp26_skeleton;

import std;

export namespace cpp26_skeleton {

[[nodiscard]] std::string greeting(std::string_view name);
[[nodiscard]] int answer() noexcept;

} // namespace cpp26_skeleton
