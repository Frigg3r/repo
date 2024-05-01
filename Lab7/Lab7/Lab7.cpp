#include <iostream>
#include <cassert>
#include <cmath> // Добавим заголовок для функции sqrt

using namespace std;

int main()
{
    struct Expression // базовая абстрактная структура
    {
        virtual double evaluate() const = 0;
        virtual Expression* transform(Transformer* tr) const = 0;
        virtual ~Expression() { }
    };

    struct Transformer // pattern Visitor
    {
        virtual Expression* transformNumber(Number const*) = 0;
        virtual Expression* transformBinaryOperation(BinaryOperation const*) = 0;
        virtual Expression* transformFunctionCall(FunctionCall const*) = 0;
        virtual Expression* transformVariable(Variable const*) = 0;
        virtual ~Transformer() { }
    };

    struct Number : Expression
    {
        Number(double value) : value_(value) {}
        double value() const { return value_; }
        double evaluate() const { return value_; }
        Expression* transform(Transformer* tr) const
        {
            return tr->transformNumber(this);
        }
        ~Number() {}
    private:
        double value_;
    };

    struct BinaryOperation : Expression
    {
        enum {
            PLUS = '+',
            MINUS = '-',
            DIV = '/',
            MUL = '*'
        };

        BinaryOperation(Expression const* left, int op, Expression const* right) : left_(left), op_(op), right_(right)
        {
            assert(left_ && right_);
        }

        Expression const* left() const { return left_; }
        Expression const* right() const { return right_; }
        int operation() const { return op_; }
        Expression* transform(Transformer* tr) const
        {
            return tr->transformBinaryOperation(this);
        }

        double evaluate() const
        {
            double left = left_->evaluate();
            double right = right_->evaluate();
            switch (op_)
            {
            case PLUS: return left + right;
            case MINUS: return left - right;
            case DIV: return left / right;
            case MUL: return left * right;
            }
        }

        ~BinaryOperation()
        {
            delete left_;
            delete right_;
        }

    private:
        Expression const* left_;
        Expression const* right_;
        int op_;
    };

    struct FunctionCall : Expression
    {
        FunctionCall(string const& name, Expression const* arg) : name_(name), arg_(arg)
        {
            assert(arg_);
            assert(name_ == "sqrt" || name_ == "abs");
        }

        string const& name() const
        {
            return name_;
        }

        Expression const* arg() const
        {
            return arg_;
        }

        Expression* transform(Transformer* tr) const
        {
            return tr->transformFunctionCall(this);
        }

        virtual double evaluate() const
        {
            if (name_ == "sqrt")
                return sqrt(arg_->evaluate());
            else
                return fabs(arg_->evaluate());
        }

        ~FunctionCall() { delete arg_; }

    private:
        string const name_;
        Expression const* arg_;
    };

    struct Variable : Expression
    {
        Variable(string const& name) : name_(name) { }
        string const& name() const { return name_; }
        Expression* transform(Transformer* tr) const
        {
            return tr->transformVariable(this);
        }

        double evaluate() const
        {
            return 0.0;
        }

    private:
        string const name_;
    };

    struct CopySyntaxTree : Transformer {
        Expression* transformNumber(Number const* number) override {
            return new Number(number->value());
        }

        Expression* transformBinaryOperation(BinaryOperation const* binop) override {
            return new BinaryOperation(
                binop->left()->transform(this),
                binop->operation(),
                binop->right()->transform(this));
        }

        Expression* transformFunctionCall(FunctionCall const* fcall) override {
            return new FunctionCall(
                fcall->name(),
                fcall->arg()->transform(this));
        }

        Expression* transformVariable(Variable const* var) override {
            return new Variable(var->name());
        }

        ~CopySyntaxTree() {}
    };


    struct FoldConstants : Transformer {
        Expression* transformNumber(Number const* number) override {
            return new Number(number->value());
        }

        Expression* transformBinaryOperation(BinaryOperation const* binop) override {
            Expression* nleft = binop->left()->transform(this);
            Expression* nright = binop->right()->transform(this);
            int noperation = binop->operation();

            BinaryOperation* nbinop = new BinaryOperation(nleft, noperation, nright);

            Number* nleft_is_number = dynamic_cast<Number*>(nleft);
            Number* nright_is_number = dynamic_cast<Number*>(nright);

            if (nleft_is_number && nright_is_number) {
                double result = nbinop->evaluate();
                delete nbinop;
                return new Number(result);
            }
            else {
                return nbinop;
            }
        }

        Expression* transformFunctionCall(FunctionCall const* fcall) override {
            Expression* arg = fcall->arg()->transform(this);
            string const& nname = fcall->name();

            FunctionCall* nfcall = new FunctionCall(nname, arg);

            Number* arg_is_number = dynamic_cast<Number*>(arg);
            if (arg_is_number) {
                double result = nfcall->evaluate();
                delete nfcall;
                return new Number(result);
            }
            else {
                return nfcall;
            }
        }

        Expression* transformVariable(Variable const* var) override {
            return new Variable(var->name());
        }

        ~FoldConstants() {}
    };


    Variable* var = new Variable("var");
    Number* n10 = new Number(10.0);
    BinaryOperation* mult = new BinaryOperation(var, BinaryOperation::MUL, n10);
    cout << mult->evaluate() << endl;
    CopySyntaxTree CST;
    Expression* newExpr = mult->transform(&CST);
    cout << newExpr->evaluate();
}
