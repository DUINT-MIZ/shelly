#pragma once
#include "prototypes/exprclass.hpp"
#include "prototypes/evalclass.hpp"
#include <basics.hpp>
#include <memory>
#include <optional>
#include <string>

namespace asts {

using Evaluator = evaluator::Evaluator;

class Expr {
    public :

    virtual std::optional<std::string> accept(Evaluator& ev, basics::valtype* buf) = 0;

    virtual ~Expr() = default;
};

class BinExpr : public Expr {
    public :
    
    static std::unique_ptr<BinExpr> bin_expr(
        std::unique_ptr<Expr>&& lhs,
        std::unique_ptr<Expr>&& rhs,
        basics::token_tag
    );

    std::optional<std::string> accept(Evaluator& ev, basics::valtype* buf) override;
        
    

    void set_lhs(std::unique_ptr<Expr>&& ptr);
    void set_rhs(std::unique_ptr<Expr>&& ptr);

    ~BinExpr() = default;

    private :
    friend Evaluator;
    std::unique_ptr<Expr> lhs;
    std::unique_ptr<Expr> rhs;
    basics::token_tag tag;
};

class IntExpr : public Expr {
    public :
    
    static std::unique_ptr<IntExpr> int_expr(const basics::Token&);

    std::optional<std::string> accept(Evaluator& ev, basics::valtype* buf) override;

    ~IntExpr() = default;
    
    int val;
};

class UnaryExpr : public Expr {
    public :
    
    static std::unique_ptr<UnaryExpr> bin_expr(
        std::unique_ptr<Expr>&& rhs,
        basics::token_tag
    );

    std::optional<std::string> accept(Evaluator& ev, basics::valtype* buf) override;

    ~UnaryExpr() = default;

    void set_rhs(std::unique_ptr<Expr>&& ptr);


    private :
    friend Evaluator;
    std::unique_ptr<Expr> rhs;
    basics::token_tag tag;
};

}

