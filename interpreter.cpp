#include <string>
#include <string_view>
#include <iostream>
#include <vector>
#include <charconv>
#include <stdexcept>
#include <cstdint>
#include <utility>
#include <array>

using tag_base = uint16_t;
/*
Deriving from unsigned integral type (see lookup-1.0)
*/
enum class ttag : tag_base {
    EOFILE = 0,
    NUMBER,
    PLUS,
    MINUS,
    STAR,
    SLASH,
    SENT
};

constexpr tag_base to_integral(ttag tag) {
    return static_cast<tag_base>(tag);
}

struct TagInfo { // TagInfo to contain information correlated to ttag constant (see lookup-1.0)
    std::pair<float, float> binding_power;
    bool is_operator;
    bool is_value;
};

template <typename T, std::size_t Size>
struct sparse_array_result {
    std::array<T, Size> table;
    std::size_t initialized;
};

template <typename T, std::size_t Size>
auto constexpr sparse_array(
    std::initializer_list<std::pair<std::size_t, T>> init,
    T def = T{}
) {
    std::array<T, Size> res{};
    std::array<bool, Size> ini{};
    std::size_t initialized = 0;

    for(auto& el : res) { el = def; }
    for(const auto& el : init) {
        if(el.first >= res.size())
            throw std::out_of_range("sparse_array() : index request is out of range");
        res[el.first] = el.second;

        if(!ini[el.first]) {
            ini[el.first] = true;
            ++initialized;
        }
    }
    return sparse_array_result{res, initialized};
}

template <typename T>
struct char_array_result {
    std::array<T, 256> table;
    std::size_t initialized;
};

template <typename T>
auto constexpr char_array(
    std::initializer_list<std::pair<unsigned char, T>> init,
    T def = T{}
) {
    std::array<T, 256> res{};
    std::array<bool, 256> ini{};
    std::size_t initialized = 0;
    for(auto& el : res) { el = def; }
    for(const auto& el : init) {
        if(el.first >= res.size())
            throw std::out_of_range("sparse_array() : index request is out of range");
        res[el.first] = el.second;
        if(!ini[el.first]) {
            ini[el.first] = true;
            ++initialized;
        }
    }
    return char_array_result{res, initialized};
}

/*
lookup-1.0

TagInfo table simple, fast, and idiomatic.

with constraints of :
    a. enum constants must be sequence of 0, 1, 2, 3, ..., n
    b. enum must derive from UNSIGNED integral type
    c. table must be populated entirely
    d. enum must have sentinel constant with value of n
    e. each TagInfo within table must correspond to enum constant it tries to describe
*/
constexpr auto tag_infos = 
    sparse_array<TagInfo, to_integral(ttag::SENT)>(
        {
            {to_integral(ttag::EOFILE), TagInfo{{0.0, 0.0}, false, false}},
            {to_integral(ttag::PLUS), TagInfo{{1.0, 1.1}, true, false}},
            {to_integral(ttag::NUMBER), TagInfo{{0.0, 0.0}, false, true}},
            {to_integral(ttag::MINUS), TagInfo{{1.0, 1.1}, true, false}},
            {to_integral(ttag::SLASH), TagInfo{{2.0, 2.1}, true, false}},
            {to_integral(ttag::STAR), TagInfo{{2.0, 2.1}, true, false}}
        }
    );

static_assert(tag_infos.initialized == to_integral(ttag::SENT), "tag_infos must be fully populated");

struct Token {
    double num_val;
    ttag tag;
};

/*
lookup-1.1

Fetching enum constant from a char.
simply use a 256 sized array
highly expandable for adding additional feature
*/
constexpr auto scop_infos =
    char_array<ttag>(
        {
            {'+', ttag::PLUS},
            {'-', ttag::MINUS},
            {'*', ttag::STAR},
            {'/', ttag::SLASH}
        },
        ttag::SENT
    );

using match_result = std::pair<bool, std::size_t>;


/*
identify_...
    |--> match_...
    |--> match_...

Separation of concerns
Easy to debug, and prevent embedding large sum of code onto a function
*/

match_result match_opsc(
    const std::string_view& view,
    Token& buff
) {
    ttag tag_res;
    if((tag_res = scop_infos.table[static_cast<unsigned char>(view.front())]) == ttag::SENT)
        return match_result{false, -1};
    buff.tag = tag_res;
    return match_result{true, 1};
}

match_result match_linum(
    const std::string_view& view,
    Token& buff
) {
    auto [ptr, ec] = std::from_chars(view.begin(), view.end(), buff.num_val);
    if(ec != std::errc{}) return {false, -1};
    buff.tag = ttag::NUMBER;
    return {true, ptr - view.begin()};
}

match_result identify_operator(
    const std::string_view& view,
    Token& buff
) {
    match_result res;
    if((res = match_opsc(view, buff)).first)
        return res;
    return res;
}

match_result identify_literal(
    const std::string_view& view,
    Token& buff
) {
    match_result res;
    if((res = match_linum(view, buff)).first)
        return res;
    return res;
}


void tokenize(const std::string_view& str, std::vector<Token>& tokens) {
    const char* it = str.begin();
    const char* const end = str.end();
    Token buff;

    auto try_push = [&](const match_result& res) {
        if(!res.first) return false;

        it += res.second;
        tokens.push_back(buff);
        
        return true;
    };

    while(it < end) {
        if(std::isspace(*it)) {            
            ++it;
            continue;
        }
        
        /*
        operator is fetched first to prevent such thing as
        "-10" being tokenized into <-10> instead of <MINUS> <10>
        */
        if(try_push(identify_operator(std::string_view(it, end), buff)))
            continue;

        if(try_push(identify_literal(std::string_view(it, end), buff)))
            continue;

        throw std::invalid_argument("invalid string sequence");
    }
}