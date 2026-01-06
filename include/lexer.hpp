#pragma once
#ifndef __LEXER_H
#define __LEXER_H
#include <iostream>
#include <string>
#include <sstream>
#include <cctype>
#include <vector>
#include <algorithm>

enum Tokentype {
    TOKEN_ID,
    TOKEN_KEYWORD,
    TOKEN_STRING,
    TOKEN_QUOTE,
    TOKEN_SINGLEQUOTE,
    TOKEN_INT,
    TOKEN_EQ,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_PLUSOP,
    TOKEN_MINUSOP,
    TOKEN_MULOP,
    TOKEN_DIVOP,
    TOKEN_EQOP,
    TOKEN_NEQOP,
    TOKEN_NOTOP,
    TOKEN_GT,
    TOKEN_LT,
    TOKEN_GTE,
    TOKEN_LTE,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_COLON,
    TOKEN_INCREMENT,
    TOKEN_DECREMENT,
    TOKEN_RBRACKET,
    TOKEN_LBRACKET,
    TOKEN_COMMA,
    TOKEN_HASH,
    TOKEN_ATSYM,
    TOKEN_ANDSYM,
    TOKEN_NEWLINE,
    TOKEN_BACKLASH,
    TOKEN_DOT,
    TOKEN_FLOAT,
    TOKEN_EOF
};

struct Token
{
    enum Tokentype TYPE;
    std::string value;
    int lineno;
    int charno;
    std::string sourceLine;
    std::string extra;
};

std::string typetostring(enum Tokentype TYPE);

class Lexer
{
    public:
        Lexer(std::string sourceCode);
        
        std::vector<std::string> keywords = {
            "if", "else", "while", "for", "return", "class", "import", "pass", "break", "continue", "def","type", "true", "false","print", "page", "app", "view", "text", "img", "canvas" , "input", "state", "go", "stylesheet", "to_int", "to_str", "to_float", "draw", "sin", "sqrt", "cos", "tan", "pow", "Platform"
        };
        
        char advance();

        void checkAndSkip();

        Token * tokenizeID_KEYWORD();
        
        Token  * tokenizeOP(enum Tokentype Type, char expected);

        Token * tokenizespecial(enum Tokentype TYPE);

        Token * tokenizeINT();
        Token * tokenizeSTR(enum Tokentype TYPE);
        std::vector<Token *> tokenize();
        char peak(int offset);
    private:
        std::string source;
        int cursor;
        int size;
        char current;
        std::stringstream currentLine;
        int linenum;
        int charnum;
        bool ctrl;
};


#endif