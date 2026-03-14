#include <cstdint>
#include <utility>
#include <array>
#include <stdexcept>
#include <string_view>
#include "utilities.hpp"

namespace basics {

using tag_base = std::uint8_t;
enum class token_tag {
    SENT,
    EOFILE,
    INTEGER,
    PLUS,
    MINUS,
    COUNT
};

constexpr tag_base revert(token_tag tag) {
    return static_cast<tag_base>(tag);
}

struct tag_info {
    std::pair<float, float> bp = {};
    const char* name = "UNKNOWN";
    bool is_operator = false;
    bool is_value = false;
};

constexpr auto tag_table =
    util::sparse_array<tag_info, revert(token_tag::COUNT)>({
        {revert(token_tag::EOFILE), tag_info{{0, 0}, "EOFILE", false, false}},
        {revert(token_tag::INTEGER), tag_info{{0, 0}, "INTEGER", false, true}},
        {revert(token_tag::PLUS), tag_info{{0, 0}, "PLUS", true, false}},
        {revert(token_tag::MINUS), tag_info{{0, 0}, "MINUS", true, false}}
    });

constexpr tag_info empty_info{};

constexpr bool info_available(tag_base idx) {
    return ((idx >= tag_table.size()) ? false : true);
}

constexpr bool info_available(token_tag tag) {
    return info_available(revert(tag));
}

constexpr const tag_info& info_at(token_tag tag) {
    auto idx = revert(tag);
    if(!info_available(idx))
        throw std::out_of_range("info_at(token_tag) : out of range access");
    return tag_table[idx];
}

constexpr const tag_info& get_info(token_tag tag) {
    auto idx = revert(tag);
    return (info_available(idx) ?  tag_table[idx] : empty_info);
}

struct Token {
    token_tag tag;
    std::string_view value;
};

}