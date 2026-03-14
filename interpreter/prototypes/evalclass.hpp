#include <optional>
#include <string>
#include <variant>
#include "exprclass.hpp"

using valtype = std::variant<std::monostate, int>;

class Evaluator {
    public :
    std::optional<std::string> visit(BinExpr& obj, valtype& buf);
    std::optional<std::string> visit(NumExpr& obj, valtype& buf);
};