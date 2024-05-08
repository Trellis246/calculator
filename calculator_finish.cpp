#include <iostream>
#include <stdexcept>
#include <string>
#include <map> // Include map for symbol table
#include <cmath>
#include <sstream>
#include <math.h>

// Token “kind” values:
char const number = '8';   // a floating-point number
char const quit = 'q';     // an exit command
char const print = ';';    // a print command
char const name = 'a';     // name token
char const let = 'L';      // declaration token

// Token stuff
class token
{
    char kind_;        // what kind of token
    double value_;     // for numbers: a value
    std::string name_; // for variables: name

public:
    // constructors
    token(char ch) : kind_(ch), value_(0) {}
    token(char ch, double val) : kind_(ch), value_(val) {}
    token(char ch, std::string name) : kind_(ch), name_(name) {}

    char kind() const { return kind_; }
    double value() const { return value_; }
    std::string name() const { return name_; }
};

// User interaction strings:
std::string const prompt = "> ";
std::string const result = "= "; // indicate that a result follows

// Token Stream class
class token_stream
{
    // representation: not directly accessible to users:
    bool full;       // is there a token in the buffer?
    token buffer;    // here is where we keep a Token put back using putback()
public:
    // user interface:
    token get();            // get a token
    void putback(token);    // put a token back into the token_stream
    void ignore(char c);    // discard tokens up to and including a c

    // constructor: make a token_stream, the buffer starts empty
    token_stream() : full(false), buffer('\0') {}
};

// single global instance of the token_stream
token_stream ts;

void token_stream::putback(token t)
{
    if (full)
        throw std::runtime_error("putback() into a full buffer");
    buffer = t;
    full = true;
}

token token_stream::get() // read a token from the token_stream
{
    // check if we already have a Token ready
    if (full)
    {
        full = false;
        return buffer;
    }

    // note that >> skips whitespace (space, newline, tab, etc.)
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
        return token(ch); // let each character represent itself
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
        std::cin.putback(ch); // put digit back into the input stream
        double val;
        std::cin >> val; // read a floating-point number
        return token(number, val);
    }
    default:
        if (isalpha(ch)) // handle variables
        {
            std::string s;
            s += ch;
            while (std::cin.get(ch) && (isalpha(ch) || isdigit(ch)))
                s += ch;
            std::cin.putback(ch);
            if (s == "let") // declaration keyword
                return token(let);
            return token(name, s);
        }
        throw std::runtime_error("Bad token");
    }
}

// discard tokens up to and including a c
void token_stream::ignore(char c)
{
    // first look in buffer:
    if (full && c == buffer.kind()) // && means 'and'
    {
        full = false;
        return;
    }
    full = false; // discard the contents of buffer

    // now search input:
    char ch = 0;
    while (std::cin >> ch)
    {
        if (ch == c)
            break;
    }
}

// Symbol table for variables
std::map<std::string, double> symbol_table;

double expression();

double primary() // Number or ‘(‘ Expression ‘)’
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
        return t.value(); // return the number’s value
    case '-': // Handle negation
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

// exactly like expression(), but for * and /
double term()
{
    double left = primary(); // get the Primary
    while (true)
    {
        token t = ts.get(); // get the next Token ...
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
            ts.putback(t); // <<< put the unused token back
            return left;   // return the value
        }
    }
}

// read and evaluate: 1   1+2.5   1+2+3.14  etc.
//   return the sum, difference, or product
double expression()
{
    double left = term(); // get the Term
    while (true)
    {
        token t = ts.get(); // get the next token…
        switch (t.kind())   // ... and do the right thing with it
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
            double divisor = term();
            if (divisor == 0)
                throw std::runtime_error("divide by zero");
            left /= divisor;
            break;
        case '%':
            double divisor = term();
            if (divisor == 0)
                throw std::runtime_error("divide by zero");
            left = fmod(left, divisor);
            break;
        default:
            ts.putback(t); // <<< put the unused token back
            return left;   // return the value of the expression
        }
    }
}

void clean_up_mess()
{
    ts.ignore(print);
}

void calculate(const std::string& express)
{
    std::istringstream iss(express);
    while (iss)
    {
        try
        {
            std::cout << prompt; // print prompt
            token t = ts.get();

            // first discard all “prints”
            while (t.kind() == print)
                t = ts.get();

            if (t.kind() == quit)
                return; // ‘q’ for “quit”

            if (t.kind() == let) // handle variable declaration
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
            std::cerr << e.what() << std::endl; // write error message
            clean_up_mess();                    // <<< The tricky part!
            break;
        }
    }
}

int main()
{
    try{
        std::string express;
        std::cout << "Enter an expression: ";
        std::getline(std::cin, express);
        calculate(express);
        return 0;
    }
    catch (...)
    {
        // other errors (don't try to recover)
        std::cerr << "exception\n";
        return 2;
    }
}
