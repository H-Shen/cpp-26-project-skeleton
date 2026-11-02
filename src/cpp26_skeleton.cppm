module;

#include <string>
#include <string_view>

export module cpp26_skeleton;

export namespace cpp26_skeleton {

[[nodiscard]] std::string greeting(std::string_view name);
[[nodiscard]] int answer() noexcept;

} // namespace cpp26_skeleton
