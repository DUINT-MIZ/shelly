#include "lexer.hpp"
#include <iostream>

int main() {
    lexer::Feed feeder(lexer::Feed::PROMPT);
    lexer::Lexer lexer_obj;
    while(true) {
        auto res = lexer_obj.analyze(feeder);
        std::cout << "Tokens : ";
        for(auto& tok : lexer_obj.get_tokens()) {
            std::cout << basics::get_info(tok.tag).name << "(\"" << tok.value << "\") ";
        }
        std::cout << std::endl;
    }
}