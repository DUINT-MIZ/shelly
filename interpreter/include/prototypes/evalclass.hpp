#pragma once
#include <optional>
#include <string>
#include <variant>
#include "exprclass.hpp"
#include "../basics.hpp"

namespace evaluator {

class Evaluator {
    public :
    std::optional<std::string> visit(asts::BinExpr& obj, basics::valtype* buf);
    std::optional<std::string> visit(asts::IntExpr& obj, basics::valtype* buf);
    std::optional<std::string> visit(asts::UnaryExpr& obj, basics::valtype* buf);
};

}