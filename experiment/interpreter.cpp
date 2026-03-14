#include <variant>
#include <charconv>
#include <vector>
#include <string_view>
#include <array>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <utility>
#include <ranges>
#include <optional>

namespace stdfs = std::filesystem;

void read_file(const stdfs::path& path, std::string& buf) {
    std::streamsize size = stdfs::file_size(path);
    std::ifstream stream(path, std::ios::binary);
    stream.exceptions(std::ios::failbit | std::ios::badbit);
    buf.resize(size);
    stream.read(buf.data(), size);
}

std::string read_file(const stdfs::path& path) {
    std::string buffer;
    read_file(path, buffer);
    return buffer;
}

class Feed {
    public :

    Feed() : prompt_read(true) {}
    Feed(const stdfs::path& path) : prompt_read(false) {
        stream.exceptions(std::ios::badbit);
        stream.open(path, std::ios::binary);
        if(stream.fail())
            throw std::system_error(
                std::error_code(errno, std::generic_category()),
                ", Feed() : stream open failed"
            );
    }

    Feed& operator=(Feed&& oth) {
        this->stream = std::move(oth.stream);
        prompt_read = oth.prompt_read;
        return *this;
    }

    bool get(std::string& buffer) {
        if(prompt_read) {
            std::cout << ">>>> ";
            std::getline(std::cin, buffer);
            return !(std::cin.fail() || std::cin.bad());
        }
        std::getline(stream, buffer);
        return !stream.fail();
    }

    ~Feed() {
        stream.close();
    }

    private :
    bool prompt_read;
    std::ifstream stream;
};


using tag_base = std::uint8_t;
enum class token_tag {
    EOFILE = 0,
    NUMBER,
    PLUS,
    MINUS,
    SENT // sentinel
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
    std::array<tag_info, revert(token_tag::SENT)> {
        tag_info{{0.0f, 0.0f}, "EOFILE", false, false},
        tag_info{{0.0f, 0.0f}, "NUMBER", false, true},
        tag_info{{1.0f, 1.1f}, "PLUS", true, false},
        tag_info{{1.0f, 1.1f}, "MINUS", true, false}
    };

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

template <typename T>
struct typeholder {};

template <std::ranges::range R, typename T>
requires std::same_as<std::ranges::range_value_t<R>, std::pair<unsigned char, T>>
constexpr auto char_array(typeholder<T>, const R& init, T def = T{}) {
    std::array<T, 256> res{};
    for(auto& el : res) { el = def; }
    for(auto& it : init) { res[it.first] = it.second; }
    return res;
}

template <typename T>
constexpr auto char_array(std::initializer_list<std::pair<unsigned char, T>> init, T def = T{}){
    return char_array(typeholder<T>{}, init, def);
}

constexpr auto chrtag_table = char_array<token_tag> (
    {
        {'+', token_tag::PLUS},
        {'-', token_tag::MINUS}
    },
    token_tag::SENT
);

std::string_view cut_prefix(std::string_view& view, std::size_t n) {
    std::string_view res = view.substr(0, n);
    view.remove_prefix(n);
    return res;
}

struct token_match_result {
    bool result = false;
    const char* ptr = nullptr;
    token_match_result(bool v, const char* ptr = nullptr) : result(v), ptr(ptr) {}
    token_match_result(const char* ptr) :  ptr(ptr), result(true) {}
};

token_match_result token_1char(Token& buf, const char* it, const char* end) {
    if(it >= end) return false;
    if((buf.tag = chrtag_table[static_cast<unsigned char>(*it)]) == token_tag::SENT) return false;
    buf.value = std::string_view(it, 1);
    return token_match_result{true, ++it};
}

token_match_result token_number(Token& buf, const char* begin, const char* end) {
    const char* it = begin;
    
    if(it >= end) return false;

    bool has_comma = false;
    bool has_e = false;
    bool has_digit = false;
    bool has_pm = false;
    unsigned char c;

    while(it < end) {
        c = *it;
        if(std::isdigit(c)) {
            has_digit = true;
            ++it;
            continue;
        }

        switch (c)
        {
        case '-' :
        case '+' :
            if(has_digit || has_pm) return false;
            has_pm = true;
            break;

        case '.' :
            if(has_e || has_comma || !has_digit) return false;
            has_comma = true;
            break;
        
        case 'E' :
        case 'e' :
            if(has_e || !has_digit) return false;
            has_e = true;
            has_digit = false;
            has_pm = false;
            
            break;
        
        default: 
            end = it;
            continue;
        }
        ++it;
    }
    if(!has_digit) return false;
    buf.tag = token_tag::NUMBER;
    buf.value = std::string_view(begin, it);
    return it;
}


class Lexer {
    public :
    bool analyze(Feed& feeder) {
        line.clear();
        tokens.clear();
        if(!feeder.get(this->line)) return false;
        std::cerr << "[L] line get : \"" << line << "\"\n";
        ++line_count;
        auto it = &*line.cbegin();
        auto const begin = it;
        auto end = &*line.cend();
        Token buf;

        auto insert = [&it, this, &buf](const token_match_result& result) -> bool {
            if(!result.result) {
                return false;
            }
            
            it = result.ptr;
            this->tokens.push_back(buf);
            return true;
        };

        while(it < end) {
            if(std::isspace(*it)) {
                ++it;
                continue;
            }

            if(insert(token_1char(buf, it, end))) continue;
            if(insert(token_number(buf, it, end))) continue;

            std::cerr << "Lexer error : Invalid syntax, at line " << line_count << "\n";
            std::cerr << " | " << line << "\n | ";
            for(int i = 0; i < (it - begin); i++) { std::cerr << " "; };
            std::cerr << "^~~~~\n";
            return false;
        }

        return true;
    }

    const auto& get_tokens() const noexcept { return tokens; }

    private :
    std::size_t line_count = 0;
    std::string line;
    std::vector<Token> tokens;
};

using valtype = std::variant<std::monostate, int>;


class NumExpr;
class BinExpr;

class Evaluator {
    public :
    std::optional<std::string> visit(NumExpr& num_expr, valtype& val_buf);
    std::optional<std::string> visit(BinExpr& num_expr, valtype& val_buf);
};

class Expr {
    public :

    ~Expr() = default;

    // AST Nodes are part of "general procedure"
    // don't throw errors, this could be anywhere
    virtual std::optional<std::string> accept(Evaluator& ev, valtype* val_buf) = 0;
};

class NumExpr : public Expr {
    public :
    
    NumExpr(int n) : num_dat(n) {}
    
    static std::unique_ptr<NumExpr> num_expr(const Token& tok) {
        if(tok.tag != token_tag::NUMBER) return nullptr;
        int buf = 0;
        auto res = std::from_chars(&*tok.value.cbegin(), &*tok.value.cend(), buf);
        if(res.ec == std::errc{} && res.ptr == &*tok.value.cend())
            return std::make_unique<NumExpr>(NumExpr(buf));
        return nullptr;
    }
    
    std::optional<std::string> accept(Evaluator& ev, valtype* val_buf) override {
        return ev.visit(*this, *val_buf);
    }
    
    int num_dat;
};

class BinExpr : public Expr {
    public :
    
    BinExpr(
        std::unique_ptr<Expr>&& lhs,
        std::unique_ptr<Expr>&& rhs,
        token_tag tag
    ) : tag(tag), lhs(std::move(lhs)), rhs(std::move(rhs)) {}

    static std::unique_ptr<BinExpr> 
        bin_expr(
            std::unique_ptr<Expr>&& lhs,
            std::unique_ptr<Expr>&& rhs,
            token_tag tag
    ) {
        if((tag != token_tag::PLUS) || (tag != token_tag::MINUS)) return nullptr;
        if(!lhs || !rhs) return nullptr;

        return std::make_unique<BinExpr>(BinExpr(std::move(lhs), std::move(rhs), tag));
    }

    std::optional<std::string> accept(Evaluator& ev, valtype* val_buf) {
        return ev.visit(*this, *val_buf);
    }

    std::unique_ptr<Expr> lhs;
    std::unique_ptr<Expr> rhs;
    token_tag tag;

};


/*
int main() {
    Feed feeder;
    Lexer lexer;

    while(true) {
        lexer.analyze(feeder);
        std::cerr << "Tokens : ";
        for(auto& tok : lexer.get_tokens()) {
            std::cerr << get_info(tok.tag).name << " ";
        }
        std::cerr << "\n";
    }

    return 0;
}

/*
int main() {
    std::string inp;
    std::cout << "Enter string : ";
    std::getline(std::cin, inp);
    std::string_view view1(inp);
    std::cout << "Original view : \"" << view1 << "\"\n";
    Token buf{};
    bool result = token_number(buf, view1);
    constexpr const char* name = get_info(token_tag::NUMBER).name;
    std::cout << "Op Result : " << std::boolalpha << result << "\n";
    std::cout << "Buffer tag : \"" << get_info(buf.tag).name << "\"\n";
    std::cout << "Buffer view : \"" << buf.value << "\"\n";
    std::cout << "Cutted view : \"" << view1 << "\"\n";
    return 0;
}
*/