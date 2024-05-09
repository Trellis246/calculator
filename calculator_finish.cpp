#include <iostream>
#include <stdexcept>
#include <string>
#include <map>
#include <cmath>
#include <sstream>
#include <math.h>

// Token “kind” values:
char const number = '8';
char const quit = 'q';
char const print = ';';
char const name = 'a';
char const let = 'L';

class token
{
    char kind_;
    double value_;
    std::string name_;

public:
    token(char ch) : kind_(ch), value_(0) {}
    token(char ch, double val) : kind_(ch), value_(val) {}
    token(char ch, std::string name) : kind_(ch), name_(name) {}

    char kind() const { return kind_; }
    double value() const { return value_; }
    std::string name() const { return name_; }
};

std::string const prompt = "> ";
std::string const result = "= ";

class token_stream
{
    bool full;
    token buffer;

public:
    token get();
    void putback(token);
    void ignore(char c);

    token_stream() : full(false), buffer('\0') {}
};

token_stream ts;

void token_stream::putback(token t)
{
    if (full)
        throw std::runtime_error("putback() into a full buffer");
    buffer = t;
    full = true;
}

token token_stream::get()
{
    if (full)
    {
        full = false;
        return buffer;
    }

    char ch;
    std::cin >> ch;

    switch (ch)
    {
    case '(':
    case ')':
    case ';':
    case 'q':
    case '+':
    case '-':
    case '*':
    case '/':
    case '%':
    case '=':
        return token(ch);
    case '.':
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    {
        std::cin.putback(ch);
        double val;
        std::cin >> val;
        return token(number, val);
    }
    default:
        if (isalpha(ch))
        {
            std::string s;
            s += ch;
            while (std::cin.get(ch) && (isalpha(ch) || isdigit(ch)))
                s += ch;
            std::cin.putback(ch);
            if (s == "let")
                return token(let);
            return token(name, s);
        }
        throw std::runtime_error("Bad token");
    }
}

void token_stream::ignore(char c)
{
    if (full && c == buffer.kind())
    {
        full = false;
        return;
    }
    full = false;

    char ch = 0;
    while (std::cin >> ch)
    {
        if (ch == c)
            break;
    }
}

std::map<std::string, double> symbol_table;

double expression();

double primary()
{
    token t = ts.get();
    switch (t.kind())
    {
    case '(':
    {
        double d = expression();
        t = ts.get();
        if (t.kind() != ')')
            throw std::runtime_error("')' expected");
        return d;
    }
    case number:
        return t.value();
    case '-':
        return -primary();
    case name:
    {
        if (symbol_table.find(t.name()) != symbol_table.end())
            return symbol_table[t.name()];
        else
            throw std::runtime_error("Undefined variable: " + t.name());
    }
    default:
        throw std::runtime_error("primary expected");
    }
}

double term()
{
    double left = primary();
    while (true)
    {
        token t = ts.get();
        switch (t.kind())
        {
        case '*':
            left *= primary();
            break;
        case '/':
        {
            double d = primary();
            if (d == 0)
                throw std::runtime_error("divide by zero");
            left /= d;
            break;
        }
        case '%':
        {
            double d = primary();
            if (d == 0)
                throw std::runtime_error("divide by zero");
            left = fmod(left, d);
            break;
        }
        default:
            ts.putback(t);
            return left;
        }
    }
}

double expression()
{
    double left = term();
    bool continue_calculation = true;
    while (continue_calculation)
    {
        token t = ts.get();
        switch (t.kind())
        {
        case '+':
            left += term();
            break;
        case '-':
            left -= term();
            break;
        case '*':
            left *= term();
            break;
        case '/':
        {
            double divisor = term();
            if (divisor == 0)
                throw std::runtime_error("divide by zero");
            left /= divisor;
        }
        break;
        case '%':
        {
            double divise = term();
            if (divise == 0)
                throw std::runtime_error("divide by zero");
            left = fmod(left, divise);
        }
        break;
        default:
            ts.putback(t);
            continue_calculation = false;
        }
    }
    return left;
}

void clean_up_mess()
{
    ts.ignore(print);
}

void calculate(const std::string &express)
{
    std::istringstream iss(express);
    while (iss)
    {
        try
        {
            std::cout << prompt;
            token t = ts.get();

            while (t.kind() == print)
                t = ts.get();

            if (t.kind() == quit)
                return;

            if (t.kind() == let)
            {
                t = ts.get();
                if (t.kind() != name)
                    throw std::runtime_error("Name expected in declaration");
                std::string var_name = t.name();
                t = ts.get();
                if (t.kind() != '=')
                    throw std::runtime_error("= missing in declaration of " + var_name);
                double d = expression();
                symbol_table[var_name] = d;
                std::cout << result << var_name << " = " << d << std::endl;
            }
            else
            {
                ts.putback(t);

                std::cout << result << expression() << std::endl;
            }
        }
        catch (std::runtime_error const &e)
        {
            std::cerr << e.what() << std::endl;
            clean_up_mess();
            break;
        }
    }
}

int main()
{
    
    {
        std::string express;
        std::cout << "Enter an expression: ";
        std::getline(std::cin, express);
        calculate(express);
    }
    /*catch (std::exception const &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
*/
    return 0;
}
