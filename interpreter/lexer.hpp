#pragma once
#include "basics.hpp"
#include <expected>
#include <vector>
#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>

namespace lexer {

using ttag = basics::token_tag;

namespace stdfs = std::filesystem;

void read_file(const stdfs::path& path, std::string& buf) {
    std::streamsize file_size = stdfs::file_size(path);
    std::ifstream stream(path, std::ios::binary);
    stream.exceptions(std::ios::failbit | std::ios::badbit);
    buf.resize(file_size);
    stream.read(buf.data(), file_size);
}

class Feed {
    public :
    enum readsrc {
        PROMPT = 1,
        FILE
    };

    Feed(readsrc read_source, const stdfs::path& path = "") : read_source(read_source) {
        stream.exceptions(std::ios::badbit);
        if(read_source == FILE) {
            stream.open(path, std::ios::binary);
            if(stream.fail())
                throw std::system_error(
                    std::error_code(errno, std::system_category()),
                    ", Feed(ctor) : failed to open path of -> " + path.string()
                );
            return;
        }
        if(read_source != PROMPT)
            throw std::invalid_argument("Feed(ctor) : Invalid read_source argument");
    }

    bool line_get(std::string& buffer) {
        switch (this->read_source)
        {
        case PROMPT :
            std::cout << ">>>> ";
            std::getline(std::cin, buffer);
            return !(std::cin.fail() || std::cin.bad());
        
        case FILE :
            std::getline(this->stream, buffer);
            return !stream.fail();

        default :
            throw std::runtime_error("Feed.get() : read_source (member) is unknown");
        }
        return false;
    }

    std::optional<std::string> line_get() {
        std::string line;
        if(!line_get(line)) return std::nullopt;
        return line;
    }

    bool eof() {
        return stream.eof();
    }

    ~Feed() {
        stream.close();
    }

    private :
    readsrc read_source;
    std::ifstream stream;
};

constexpr auto chrtag_table =
    util::char_array(util::typeholder<ttag>{}, {
        {'+', ttag::PLUS},
        {'-', ttag::MINUS}
    });

struct token_match_result {
    bool status = false;
    const char* ptr = nullptr;
    token_match_result(bool v, const char* ptr = nullptr) : status(v), ptr(ptr) {}
    token_match_result(const char* ptr) :  ptr(ptr), status(true) {}
};

token_match_result token_1char(basics::Token& buf, const char* it) {
    if((buf.tag = chrtag_table[static_cast<unsigned char>(*it)]) == ttag::SENT) return false;
    buf.value = std::string_view(it, 1);
    return token_match_result{true, ++it};
}

token_match_result token_number(basics::Token& buf, const char* begin, const char* end) {
    const char* it = begin;

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
            if(has_digit || has_pm) {
                end = it;
                continue;
            }
            has_pm = true;
            break;

        case '.' :
            if(has_e || has_comma || !has_digit) {
                end = it;
                continue;
            }
            has_comma = true;
            break;
        
        case 'E' :
        case 'e' :
            if(has_e || !has_digit) {
                end = it;
                continue;;
            }
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

    buf.tag = ttag::INTEGER;
    buf.value = std::string_view(begin, it);
    return it;
}

class Lexer {
    public :
    Lexer() = default;

    bool analyze(Feed& feeder) {
        this->line.clear(); 
        this->tokens.clear();
        if(!feeder.line_get(this->line)) return false;
        ++this->line_count;
        const auto begin = &*this->line.cbegin();
        const auto end = &*this->line.cend();
        auto it = begin;
        basics::Token buf;

        auto insert = [&it, &buf, this](const token_match_result& result) -> bool {
            if(!result.status)
                return false;
            
            this->tokens.push_back(buf);
            it = result.ptr;
            return true;
        };

        while(it < end) {
            if(std::isspace(*it)) { ++it; continue; }

            if(insert(token_1char(buf, it))) continue;
            if(insert(token_number(buf, it, end))) continue;
            
            throw std::runtime_error(
                "Lexer error at line "
                + std::to_string(this->line_count)
                + ", column "
                + std::to_string(it - begin)
            );
        }
        return true;
    }

    const auto& get_tokens() const noexcept { return tokens; }

    private :
    std::size_t line_count = 0;
    std::string line;
    std::vector<basics::Token> tokens;
};

};