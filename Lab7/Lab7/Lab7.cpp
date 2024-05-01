#include <iostream>
#include <cassert>

using namespace std;

int main()
{
    struct Transformer;
    struct Number;
    struct BinaryOperation;
    struct FunctionCall;
    struct Variable;

    struct Expression // базовая абстрактная структура
    {
        virtual double evaluate() const = 0; // абстрактный метод «вычислить»
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

    struct Number : Expression // стуктура «Число»
    {
        Number(double value) : value_(value) {}
        double value() const { return value_; } // метод чтения значения числа
        double evaluate() const { return value_; } // реализация виртуального метода «вычислить»
        Expression* transform(Transformer* tr) const
        {
            return tr->transformNumber(this);
        }
        ~Number() {}
    private:
        double value_;
    };

    struct BinaryOperation : Expression // «Бинарная операция»
    {
        enum { // перечислим константы, которыми зашифруем символы операций
            PLUS = '+',
            MINUS = '-',
            DIV = '/',
            MUL = '*'
        };

        // в конструкторе надо указать 2 операнда — левый и правый, а также сам символ операции
        BinaryOperation(Expression const* left, int op, Expression const* right) : left_(left), op_(op), right_(right)
        {
            assert(left_ && right_);
        }

        Expression const* left() const { return left_; } // чтение левого операнда
        Expression const* right() const { return right_; } // чтение правого операнда
        int operation() const { return op_; } // чтение символа операции
        Expression* transform(Transformer* tr) const
        {
            return tr->transformBinaryOperation(this);
        }

        double evaluate() const // реализация виртуального метода «вычислить»
        {
            double left = left_->evaluate(); // вычисляем левую часть
            double right = right_->evaluate(); // вычисляем правую часть
            switch (op_) // в зависимости от вида операции, складываем, вычитаем, умножаем или делим левую и правую части
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
        Expression const* left_; // указатель на левый операнд
        Expression const* right_; // указатель на правый операнд
        int op_; // символ операции
    };

    struct FunctionCall : Expression // структура «Вызов функции»
    {
        // в конструкторе надо учесть имя функции и ее аргумент
        FunctionCall(string const& name, Expression const* arg) : name_(name), arg_(arg)
        {
            assert(arg_);
            assert(name_ == "sqrt" || name_ == "abs"); // разрешены только вызов sqrt и abs
        }

        string const& name() const
        {
            return name_;
        }

        Expression const* arg() const // чтение аргумента функции
        {
            return arg_;
        }

        Expression* transform(Transformer* tr) const
        {
            return tr->transformFunctionCall(this);
        }

        virtual double evaluate() const { // реализация виртуального метода «вычислить»
            if (name_ == "sqrt")
                return sqrt(arg_->evaluate()); // либо вычисляем корень квадратный
            else
                return fabs(arg_->evaluate()); // либо модуль — остальные функции запрещены
        }

        ~FunctionCall() { delete arg_; }

    private:
        string const name_; // имя функции
        Expression const* arg_; // указатель на ее аргумент
    };

    struct Variable : Expression // структура «Переменная»
    {
        Variable(string const& name) : name_(name) { } // в конструкторе указываем имя переменной
        string const& name() const { return name_; } // чтение имени переменной
        Expression* transform(Transformer* tr) const
        {
            return tr->transformVariable(this);
        }

        double evaluate() const // реализация виртуального метода «вычислить»
        {
            return 0.0;
        }

    private:
        string const name_; // имя переменной
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


    /*  Number* n32 = new Number(32.0);
      Number* n16 = new Number(16.0);
      BinaryOperation* minus = new BinaryOperation(n32, BinaryOperation::MINUS, n16);
      FunctionCall* callSqrt = new FunctionCall("sqrt", minus);
      Variable* var = new Variable("var");
      Number* n8 = new Number(8.0);
      BinaryOperation* mult = new BinaryOperation(n8, BinaryOperation::MUL, callSqrt);
      FunctionCall* callAbs = new FunctionCall("abs", mult);
      cout << callAbs->evaluate() << endl;
      CopySyntaxTree CST;
      Expression* newExpr = callAbs->transform(&CST);
      cout << newExpr->evaluate();*/


    Number* n32 = new Number(32.0);
    Number* n16 = new Number(16.0);
    BinaryOperation* minus = new BinaryOperation(n32, BinaryOperation::MINUS, n16);
    FunctionCall* callSqrt = new FunctionCall("sqrt", minus);
    //Variable* var = new Variable("var");
    Number* n10 = new Number(10.0);
    BinaryOperation* mult = new BinaryOperation(n10, BinaryOperation::MUL, callSqrt);
    FunctionCall* callAbs = new FunctionCall("abs", mult);
    cout << callAbs->evaluate() << endl;
    FoldConstants FC;
    Expression* newExpr = callAbs->transform(&FC);
    cout << newExpr->evaluate();
}