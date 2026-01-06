#pragma once
#ifndef __PARSER_H
#define __PARSER_H

#include "lexer.hpp"
#include <vector>
#include <string>
#include <cctype>
#include <regex>

using namespace std;
 
enum NODE_TYPE {
    NODE_ROOT,
    NODE_VARIABLE,
    NODE_RETURN,
    NODE_PRINT,
    NODE_INT,
    NODE_EXPR,
    NODE_BINARY_OP,
    NODE_COMPARISON_OP,
    NODE_STRING,
    NODE_IF,
    NODE_ELSE_IF,
    NODE_ELSE,
    NODE_WHILE,
    NODE_FOR,
    NODE_UNARY_OP,
    NODE_BOOL,
    NODE_LIST,
    NODE_FUNCTION_DECL,
    NODE_FUNCTION_CALL,
    NODE_TYPE_CHECK,
    NODE_LOOP_CTRL,
    NODE_KEYVALUE,  // new
    NODE_DICT, 
    NODE_MATH_SIN,
    NODE_MATH_SQRT,
    NODE_MATH_COS,
    NODE_MATH_TAN,
    NODE_MATH_POW,
    NODE_STYLE,
    //ui specifics
    NODE_app,
    NODE_page,
    NODE_VIEW,
    NODE_TEXT,
    NODE_IMAGE,
    NODE_INPUT,
    NODE_CANVAS,
    NODE_ARGS,

    //framework logic
    NODE_SETSTATE,
    NODE_GETSTATE,
    NODE_GO,
    NODE_STYLESHEET,
    NODE_CLS,
    NODE_MEDIA_QUERY,
    NODE_TOSTR,
    NODE_TOINT,
    NODE_TOFLOAT,
    NODE_FLOAT,
    NODE_DRAW,
    NODE_INSTANCE,
    NODE_PLATFORM_CLS
};

struct AST_NODE {
    enum NODE_TYPE TYPE;
    string *value = nullptr;
    AST_NODE *CHILD = nullptr;
    vector<AST_NODE *> SUB_STATEMENTS;
    int lineno;
    int charno;
    std::string sourceLine;
    std::string extra;
};

string nodetostr(enum NODE_TYPE tYPE);

class Parser {
public:
    Parser(vector<Token *> tokens);

    // ---------- Error Handler ----------
    void parserError(const std::string &message);
     // ---------- Error Handler ----------
    void parserWarning(const std::string &message);

    // ---------- Token Consumption ----------
    Token *proceed(enum Tokentype Tokentype);

    // ---------- Atomic Parsing ----------
    AST_NODE *parseINT();

    AST_NODE *parseID();

    AST_NODE *parseInstancecall(string *buffer);

    //parse list
    AST_NODE *parseList();

    AST_NODE *parseDict(bool styleparse = false);

    AST_NODE *parseConversions(NODE_TYPE nodetype);


    // ---------- Factor ----------
    AST_NODE *parseFactor();

    AST_NODE *parseTerm();
    AST_NODE *parseExpression();
    AST_NODE *parseComparison();
    // ---------- NEW: Parse Increment ----------
    AST_NODE *parseIncrement();
    // ---------- IF ----------
    AST_NODE *parseIF();
    AST_NODE *parseConditional();
    // ---------- WHILE ----------
    AST_NODE *parseWhile();
    // ---------- FOR ----------
    AST_NODE *parseFor();

    AST_NODE *parseBOOL(enum NODE_TYPE Tokentype);
     // ---------- FUNCTION CALL ----------
    AST_NODE *parseFunctionCall(string *funcName, NODE_TYPE tyPE = NODE_FUNCTION_CALL);
    // ---------- FUNCTION DECLARATION ----------
    AST_NODE *parseFunctionDecl(bool callback=false, string *funcname = nullptr, int *noargs = NULL);

    AST_NODE *parsePageParam();
    AST_NODE *parsepage();

    AST_NODE *parseView(enum NODE_TYPE typw);

    AST_NODE* parseStylesheet();
    AST_NODE *parseCtx();
    AST_NODE *parsePlatform();

    std::vector<std::string> defined_keywords;
    std::vector<std::string> loop_keywords;
    // ---------- Keyword Dispatcher ----------
    AST_NODE *parseKEYWORDS();
    AST_NODE * parseState();
    AST_NODE * parseSetState();
    AST_NODE * parseAtSym();
    // ---------- Generic Statement ----------
    AST_NODE *parseStatement(bool ispage = false);
    // ---------- Parse Root ----------
    AST_NODE *parse();
private:
    int limit;
    int index;
    Token *current;
    vector<Token *> parserTokens;
};

#endif
