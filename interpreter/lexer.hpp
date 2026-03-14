#pragma once
#include "basics.hpp"
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

    bool get(std::string& buffer) {
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

    std::optional<std::string> get() {
        std::string line;
        if(!get(line)) return std::nullopt;
        return line;
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
    bool result = false;
    const char* ptr = nullptr;
    token_match_result(bool v, const char* ptr = nullptr) : result(v), ptr(ptr) {}
    token_match_result(const char* ptr) :  ptr(ptr), result(true) {}
};

token_match_result token_1char(basics::Token& buf, const char* it) {
    if((buf.tag = chrtag_table[static_cast<unsigned char>(*it)]) == ttag::SENT) return false;
    buf.value = std::string_view(it, 1);
    return token_match_result{true, ++it};
}

token_match_result token_number(basics::Token& buf, const char* begin, const char* end) {
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
    buf.tag = ttag::INTEGER;
    buf.value = std::string_view(begin, it);
    return it;
}



};