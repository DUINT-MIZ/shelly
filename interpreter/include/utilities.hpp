#include <ranges>
#include <concepts>
#include <array>

namespace util {

template <typename... Args>
struct typeholder {};


template <typename T, std::size_t S, std::size_t N>
constexpr auto sparse_array(const std::pair<std::size_t, T>(&init_values)[N]) {
    std::array<T, S> res{};
    for(auto& [index, init_val] : init_values) {
        if(index > S)
            throw std::out_of_range("Index out of range");
        res[index] = init_val;
    }
    return res;
}

template <typename T, typename... Rest, std::size_t N>
constexpr auto char_array(typeholder<T, Rest...>, const std::pair<std::size_t, T>(&init_values)[N]) {
    return sparse_array<T, 256>(init_values);
}

}