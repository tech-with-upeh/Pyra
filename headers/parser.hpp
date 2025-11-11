#ifndef __PARSER_H
#define __PARSER_H

#include "lexer.hpp"
#include <vector>
#include <string>
#include <cctype>

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
    NODE_STYLE,
    //ui specifics
    NODE_app,
    NODE_page,
    NODE_VIEW,
    NODE_TEXT,
    NODE_IMAGE,
    NODE_INPUT,
    NODE_ARGS
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

string nodetostr(enum NODE_TYPE tYPE) {
    switch (tYPE) {
        case NODE_ROOT: return "ROOT"; break;
        case NODE_VARIABLE: return "VARIABLE"; break;
        case NODE_RETURN: return "RETURN"; break;
        case NODE_PRINT: return "PRINT"; break;
        case NODE_INT: return "INT"; break;
        case NODE_EXPR: return "EXPR"; break;
        case NODE_BINARY_OP: return "BINARY_OP"; break;
        case NODE_STRING: return "STRING"; break;
        case NODE_COMPARISON_OP: return "COMPARE"; break;
        case NODE_IF: return "IF"; break;
        case NODE_ELSE_IF: return "ELSE_IF"; break;
        case NODE_ELSE: return "ELSE"; break;
        case NODE_WHILE: return "WHILE"; break;
        case NODE_FOR: return "FOR"; break;
        case NODE_UNARY_OP: return "UNARY_OP"; break;
        case NODE_LIST: return "LIST"; break; 
        case NODE_BOOL: return "BOOL"; break;
        case NODE_FUNCTION_DECL: return "FUNC_INIT"; break;
        case NODE_FUNCTION_CALL: return "FUNC_CALL"; break;
        case NODE_TYPE_CHECK: return "TYPE_CHECK"; break;
        case NODE_LOOP_CTRL: return "CTRL"; break;
        case NODE_KEYVALUE: return "KEYVALUE"; break; // new
        case NODE_DICT: return "DICT"; break;
        case NODE_app: return "UIapp"; break;
        case NODE_page: return "UIpage"; break;
        case NODE_VIEW: return "UIVIEW"; break;
        case NODE_TEXT: return "UITEXT"; break;
        case NODE_IMAGE: return "UIIMAGE"; break;
        case NODE_INPUT: return "UIINPUT"; break;
        case NODE_ARGS: return "ARGS"; break;
        default: return "UNKNOWN"; break;
    }
}

class Parser {
public:
    Parser(vector<Token *> tokens) {
        parserTokens = tokens;
        limit = parserTokens.size();
        index = 0;
        current = parserTokens.at(index);
    }

    // ---------- Error Handler ----------
    void parserError(const std::string &message) {
        std::cerr << "\nParserError: " << message
                  << " at line " << current->lineno
                  << ", column " << current->charno << "\n";
        std::cerr << "  " << current->lineno << " | " << current->sourceLine << "\n";
        std::cerr << "    ";
        for (int i = 1; i < (current->charno+to_string(current->lineno).length()+1); ++i)
            std::cerr << " ";
        for (int i = 0; i < current->value.length(); i++)
        {
            std::cerr << "^";
        }
        std::cerr << "\n\n";
        std::exit(1);
    }

    // ---------- Token Consumption ----------
    Token *proceed(enum Tokentype Tokentype) {
        if (current->TYPE != Tokentype) {
            parserError("Unexpected Token: " + current->value + " (expected " + typetostring(Tokentype) + ")");
        }
        index++;
        if (index < parserTokens.size()) {
            current = parserTokens.at(index);
        } else {
            Token *eofToken = new Token();
            eofToken->TYPE = TOKEN_EOF;
            eofToken->value = "\0";
            current = eofToken;
        }
        return current;
    }

    // ---------- Atomic Parsing ----------
    AST_NODE *parseINT() {
        if (current->TYPE != TOKEN_INT) {
            parserError("Expected integer literal");
        }
        AST_NODE *node = new AST_NODE();
        node->TYPE = NODE_INT;
        node->value = &current->value;
        node->charno = current->charno;
        node->lineno = current->lineno;
        node->sourceLine = current->sourceLine;
        node->extra = current->extra;
        proceed(TOKEN_INT);
        return node;
    }

    AST_NODE *parseID() {
        string *buffer = &current->value;
        proceed(TOKEN_ID);

        if (current->TYPE == TOKEN_INCREMENT || current->TYPE == TOKEN_DECREMENT) {
            // Handle postfix i++ or i--
            string op = current->value;
            proceed(current->TYPE);

            AST_NODE *node = new AST_NODE();
            node->TYPE = NODE_UNARY_OP;
            node->value = new string(op);

            AST_NODE *varNode = new AST_NODE();
            varNode->TYPE = NODE_VARIABLE;
            varNode->value = buffer;

            node->CHILD = varNode;
            node->charno = current->charno;
            node->lineno = current->lineno;
            node->sourceLine = current->sourceLine;
            node->extra = current->extra;
            return node;
        }

         if (current->TYPE == TOKEN_LPAREN)
            {
                return parseFunctionCall(buffer);
            }

        // Otherwise assignment
        proceed(TOKEN_EQ);
        AST_NODE *node = new AST_NODE();
        node->TYPE = NODE_VARIABLE;
        node->value = buffer;
        node->CHILD = parseComparison();
            node->lineno = current->lineno;
            node->sourceLine = current->sourceLine;
            node->extra = current->extra;
            node->charno = current->charno;
        return node;
    }

    //parse list
    AST_NODE *parseList() {
    AST_NODE *listNode = new AST_NODE();
    

    // consume '['
    proceed(TOKEN_LBRACKET);

    // empty list case: []
    if (current->TYPE == TOKEN_RBRACKET) {
        proceed(TOKEN_RBRACKET);
        listNode->TYPE = NODE_LIST;
    listNode->lineno = current->lineno;
            listNode->sourceLine = current->sourceLine;
            listNode->extra = current->extra;
            listNode->charno = current->charno;
        return listNode;
    }

    // parse first element
    listNode->SUB_STATEMENTS.push_back(parseExpression());

    // parse remaining elements separated by commas
    while (current->TYPE == TOKEN_COMMA) {
        proceed(TOKEN_COMMA);
        listNode->SUB_STATEMENTS.push_back(parseExpression());
    }

    // closing bracket
    proceed(TOKEN_RBRACKET);
    listNode->TYPE = NODE_LIST;
    listNode->lineno = current->lineno;
            listNode->sourceLine = current->sourceLine;
            listNode->extra = current->extra;
            listNode->charno = current->charno;
    return listNode;
}

    AST_NODE *parseDict() {
        AST_NODE* objNode = new AST_NODE();
            objNode->TYPE = NODE_DICT;
            if (current->TYPE == TOKEN_HASH)
            {
                objNode->value = &(current->value);
                proceed(TOKEN_HASH);
            }
            
            proceed(TOKEN_LBRACE);
            while (current->TYPE == TOKEN_NEWLINE)
            {
                proceed(TOKEN_NEWLINE);
            }
            if(current->TYPE != TOKEN_RBRACE ) 
            {   
                if(current->TYPE != TOKEN_RBRACE) 
                {   
                    if (current->TYPE == TOKEN_NEWLINE)
                        {
                            proceed(TOKEN_NEWLINE);
                        }

                    AST_NODE* keyNode = new AST_NODE();
                    if (current->TYPE == TOKEN_ID)
                    {
                        keyNode->TYPE = NODE_VARIABLE;
                        keyNode->value = &(current->value);
                        proceed(TOKEN_ID); // move past key
                    } else {
                       keyNode->TYPE = NODE_STRING;
                        keyNode->value = &(current->value);
                        proceed(TOKEN_STRING); // move past key
                    }
                    // Expect colon
                    proceed(TOKEN_COLON);
                    // Parse value (can be expression, string, int, or even nested object)
                    AST_NODE* valueNode = parseExpression();
                    // Make a key–value pair node
                    AST_NODE* pairNode = new AST_NODE();
                    pairNode->TYPE = NODE_KEYVALUE;
                    pairNode->SUB_STATEMENTS = { keyNode, valueNode };
                    // Add to object node
                    objNode->SUB_STATEMENTS.push_back(pairNode);
                    while (current->TYPE == TOKEN_COMMA) {
                        proceed(TOKEN_COMMA);
                        if (current->TYPE == TOKEN_NEWLINE)
                        {
                            proceed(TOKEN_NEWLINE);
                        }
                            AST_NODE* keyNode = new AST_NODE();
                            if (current->TYPE == TOKEN_ID)
                            {
                                keyNode->TYPE = NODE_VARIABLE;
                                keyNode->value = &(current->value);
                                proceed(TOKEN_ID); // move past key
                            } else {
                            keyNode->TYPE = NODE_STRING;
                                keyNode->value = &(current->value);
                                proceed(TOKEN_STRING); // move past key
                            }
                        // Expect colon
                        proceed(TOKEN_COLON);
                        // Parse value (can be expression, string, int, or even nested object)
                        AST_NODE* valueNode = parseExpression();
                        // Make a key–value pair node
                        AST_NODE* pairNode = new AST_NODE();
                        pairNode->TYPE = NODE_KEYVALUE;
                        pairNode->SUB_STATEMENTS = { keyNode, valueNode };
                        // Add to object node
                        objNode->SUB_STATEMENTS.push_back(pairNode);
                    }
                }
                while (current->TYPE == TOKEN_NEWLINE)
                {
                    proceed(TOKEN_NEWLINE);
                }
                proceed(TOKEN_RBRACE);
                objNode->TYPE = NODE_DICT;
    objNode->lineno = current->lineno;
            objNode->sourceLine = current->sourceLine;
            objNode->extra = current->extra;
                objNode->charno = current->charno;
                return objNode;
            } else
            {
                proceed(TOKEN_RBRACE);
                objNode->TYPE = NODE_DICT;
    objNode->lineno = current->lineno;
            objNode->sourceLine = current->sourceLine;
            objNode->extra = current->extra;
            objNode->charno = current->charno;
                return objNode;
            }
                 
    }


    // ---------- Factor / Expression ----------
    AST_NODE *parseFactor() {
        if (current->TYPE == TOKEN_LBRACE || current->TYPE == TOKEN_HASH) {
            AST_NODE *node = parseDict();
            node->TYPE = NODE_DICT;
    node->lineno = current->lineno;
            node->sourceLine = current->sourceLine;
            node->extra = current->extra;
            node->charno = current->charno;
            return node;
        }
        

        if (current->TYPE == TOKEN_LBRACKET)
        {
           AST_NODE *node = parseList();
           proceed(current->TYPE);
           node->lineno = current->lineno;
            node->sourceLine = current->sourceLine;
            node->extra = current->extra;
            node->charno = current->charno;
           return node;
        }
        
        // prefix ++ / --
        if (current->TYPE == TOKEN_INCREMENT || current->TYPE == TOKEN_DECREMENT) {
            string op = current->value;
            proceed(current->TYPE);

            if (current->TYPE != TOKEN_ID)
                parserError("Expected identifier after unary operator");

            AST_NODE *node = new AST_NODE();
            node->TYPE = NODE_UNARY_OP;
            node->value = new string(op);

            AST_NODE *varNode = new AST_NODE();
            varNode->TYPE = NODE_VARIABLE;
            varNode->value = &current->value;
            proceed(TOKEN_ID);

            node->CHILD = varNode;
            node->lineno = current->lineno;
            node->sourceLine = current->sourceLine;
            node->extra = current->extra;
            node->charno = current->charno;
            return node;
        }

        if (current->TYPE == TOKEN_INT) return parseINT();

        if (current->TYPE == TOKEN_STRING) {
            AST_NODE *node = new AST_NODE();
            node->TYPE = NODE_STRING;
            node->value = &current->value;
            proceed(TOKEN_STRING);
            node->lineno = current->lineno;
            node->sourceLine = current->sourceLine;
            node->extra = current->extra;
            node->charno = current->charno;
            return node;
        }

        // ✅ handle boolean literals
        if (current->TYPE == TOKEN_KEYWORD &&
            (current->value == "true" || current->value == "false")) {
                AST_NODE *node = parseBOOL(NODE_BOOL);
                proceed(current->TYPE);
                node->lineno = current->lineno;
            node->sourceLine = current->sourceLine;
            node->extra = current->extra;
            node->charno = current->charno;
                return node;
            }
            

        if (current->TYPE == TOKEN_ID) {
            string *varName = &current->value;
            proceed(TOKEN_ID);

            // check for postfix i++ or i--
            if (current->TYPE == TOKEN_INCREMENT || current->TYPE == TOKEN_DECREMENT) {
                string op = current->value;
                proceed(current->TYPE);

                AST_NODE *node = new AST_NODE();
                node->TYPE = NODE_UNARY_OP;
                node->value = new string(op);

                AST_NODE *varNode = new AST_NODE();
                varNode->TYPE = NODE_VARIABLE;
                varNode->value = varName;
                node->CHILD = varNode;
                node->lineno = current->lineno;
            node->sourceLine = current->sourceLine;
            node->extra = current->extra;
            node->charno = current->charno;
                return node;
            }

            if (current->TYPE == TOKEN_LPAREN)
            {
                return parseFunctionCall(varName);
            }
            

            AST_NODE *node = new AST_NODE();
            node->TYPE = NODE_VARIABLE;
            node->value = varName;
            node->lineno = current->lineno;
            node->sourceLine = current->sourceLine;
            node->extra = current->extra;
            node->charno = current->charno;
            return node;
        }
        

        if (current->TYPE == TOKEN_LPAREN) {
            proceed(TOKEN_LPAREN);
            AST_NODE *expr = parseExpression();
            proceed(TOKEN_RPAREN);
            return expr;
        }

        parserError("Unexpected token in factor: " + current->value);
        return nullptr;
    }

    AST_NODE *parseTerm() {
        AST_NODE *node = parseFactor();
        while (current->value == "*" || current->value == "/") {
            string op = current->value;
            proceed(current->TYPE);
            AST_NODE *right = parseFactor();
            AST_NODE *newNode = new AST_NODE();
            newNode->TYPE = NODE_BINARY_OP;
            newNode->value = new string(op);
            newNode->SUB_STATEMENTS = {node, right};
            node = newNode;
        }
        node->lineno = current->lineno;
            node->sourceLine = current->sourceLine;
            node->extra = current->extra;
            node->charno = current->charno;
        return node;
    }

    AST_NODE *parseExpression() {
        AST_NODE *node = parseTerm();
        while (current->value == "+" || current->value == "-") {
            string op = current->value;
            proceed(current->TYPE);
            AST_NODE *right = parseTerm();
            AST_NODE *newNode = new AST_NODE();
            newNode->TYPE = NODE_BINARY_OP;
            newNode->value = new string(op);
            newNode->SUB_STATEMENTS = {node, right};
            node = newNode;
        }
        node->lineno = current->lineno;
            node->sourceLine = current->sourceLine;
            node->extra = current->extra;
            node->charno = current->charno;
        return node;
    }

    AST_NODE *parseComparison() {
        AST_NODE *node = parseExpression();
        while (
            current->TYPE == TOKEN_EQOP || current->TYPE == TOKEN_NEQOP ||
            current->TYPE == TOKEN_GT || current->TYPE == TOKEN_LT ||
            current->TYPE == TOKEN_GTE || current->TYPE == TOKEN_LTE ||
            current->TYPE == TOKEN_NOTOP) {
            string op = current->value;
            proceed(current->TYPE);
            AST_NODE *right = parseExpression();
            AST_NODE *newNode = new AST_NODE();
            newNode->TYPE = NODE_BOOL;
            newNode->value = new string(op);
            newNode->SUB_STATEMENTS = {node, right};
            node = newNode;
        }
        node->lineno = current->lineno;
            node->sourceLine = current->sourceLine;
            node->extra = current->extra;
            node->charno = current->charno;
        return node;
    }

    // ---------- NEW: Parse Increment ----------
    AST_NODE *parseIncrement() {
        if (current->TYPE != TOKEN_ID)
            parserError("Expected identifier in increment section");

        string *idName = &current->value;
        proceed(TOKEN_ID);

        AST_NODE *node = new AST_NODE();

        if (current->TYPE == TOKEN_INCREMENT || current->TYPE == TOKEN_DECREMENT) {
            string op = current->value;
            proceed(current->TYPE);
            node->TYPE = NODE_UNARY_OP;
            node->value = new string(op);

            AST_NODE *varNode = new AST_NODE();
            varNode->TYPE = NODE_VARIABLE;
            varNode->value = idName;
            node->CHILD = varNode;
            node->lineno = current->lineno;
            node->sourceLine = current->sourceLine;
            node->extra = current->extra;
            node->charno = current->charno;
            return node;
        }

        if (current->TYPE == TOKEN_EQ) {
            proceed(TOKEN_EQ);
            AST_NODE *expr = parseComparison();
            node->TYPE = NODE_VARIABLE;
            node->value = idName;
            node->CHILD = expr;
            node->lineno = current->lineno;
            node->sourceLine = current->sourceLine;
            node->extra = current->extra;
            node->charno = current->charno;
            return node;
        }

        parserError("Expected '++', '--', or '=' after identifier in for-loop increment");
        return nullptr;
    }

    // ---------- IF ----------
    AST_NODE *parseIF() {
        AST_NODE *ifNode = new AST_NODE();
        ifNode->TYPE = NODE_IF;

        if (current->TYPE == TOKEN_LPAREN)
            proceed(TOKEN_LPAREN);

        ifNode->CHILD = parseComparison();

        if (current->TYPE == TOKEN_RPAREN)
            proceed(TOKEN_RPAREN);

        proceed(TOKEN_LBRACE);

        while (current->TYPE != TOKEN_RBRACE && current->TYPE != TOKEN_EOF) {
            while (current->TYPE == TOKEN_NEWLINE)
                proceed(TOKEN_NEWLINE);

            if (current->TYPE == TOKEN_RBRACE)
                break;

            ifNode->SUB_STATEMENTS.push_back(parseStatement());

            while (current->TYPE == TOKEN_NEWLINE)
                proceed(TOKEN_NEWLINE);
        }
        proceed(TOKEN_RBRACE);
        // handle else / else if
            bool endof = false;
            while (endof != true)
            {
                if (current->value == "else") {
                    proceed(TOKEN_KEYWORD);
                    if (current->value == "if") {
                        AST_NODE *elseIfNode = new AST_NODE();
                        elseIfNode->TYPE = NODE_ELSE_IF;
                        proceed(current->TYPE);

                        if (current->TYPE == TOKEN_LPAREN) {proceed(TOKEN_LPAREN);}
                            elseIfNode->CHILD = parseComparison();
                        if (current->TYPE == TOKEN_RPAREN) { proceed(TOKEN_RPAREN);}

                        proceed(TOKEN_LBRACE);

                        while (current->TYPE != TOKEN_RBRACE && current->TYPE != TOKEN_EOF) {
                            while (current->TYPE == TOKEN_NEWLINE)
                                proceed(TOKEN_NEWLINE);

                            if (current->TYPE == TOKEN_RBRACE)
                                break;

                            elseIfNode->SUB_STATEMENTS.push_back(parseStatement());

                            while (current->TYPE == TOKEN_NEWLINE)
                                proceed(TOKEN_NEWLINE);
                        }
                        proceed(TOKEN_RBRACE);
                        ifNode->SUB_STATEMENTS.push_back(elseIfNode);

                        while (current->TYPE == TOKEN_NEWLINE)
                        {
                            proceed(current->TYPE);
                        }
                        
                        cout << "----> " << current->value << endl;
                        if (current->value != "else")
                        {
                        endof = true;
                        }
                    } else {
                        AST_NODE *elseNode = new AST_NODE();
                        elseNode->TYPE = NODE_ELSE;
                        proceed(TOKEN_LBRACE);
                        while (current->TYPE != TOKEN_RBRACE && current->TYPE != TOKEN_EOF) {   
                            if (current->TYPE == TOKEN_NEWLINE) {
                                proceed(TOKEN_NEWLINE);
                            }
                            if (current->TYPE == TOKEN_RBRACE) {
                                proceed(TOKEN_RBRACE);
                                break;
                            }

                            elseNode->SUB_STATEMENTS.push_back(parseStatement());
                        }
                        ifNode->SUB_STATEMENTS.push_back(elseNode);
                        endof = true;
                    }
                } else {
                    endof = true;
                }
            }
        

        ifNode->lineno = current->lineno;
            ifNode->sourceLine = current->sourceLine;
            ifNode->extra = current->extra;
            ifNode->charno = current->charno;
        return ifNode;
    }

    AST_NODE *parseConditional() {
        proceed(TOKEN_KEYWORD);
        return parseIF();
    }

    // ---------- WHILE ----------
    AST_NODE *parseWhile() {
        AST_NODE *whileNode = new AST_NODE();
        whileNode->TYPE = NODE_WHILE;
        proceed(TOKEN_KEYWORD);

        if (current->TYPE == TOKEN_LPAREN)
            proceed(TOKEN_LPAREN);

        whileNode->CHILD = parseComparison();

        if (current->TYPE == TOKEN_RPAREN)
            proceed(TOKEN_RPAREN);

        proceed(TOKEN_LBRACE);
        while (current->TYPE != TOKEN_RBRACE && current->TYPE != TOKEN_EOF) {        
            while (current->TYPE == TOKEN_NEWLINE)
                proceed(TOKEN_NEWLINE);

            if (current->TYPE == TOKEN_RBRACE)
                break;

            whileNode->SUB_STATEMENTS.push_back(parseStatement());

            while (current->TYPE == TOKEN_NEWLINE)
                proceed(TOKEN_NEWLINE);
        }
        proceed(TOKEN_RBRACE);

        return whileNode;
    }

    // ---------- FOR ----------
    AST_NODE *parseFor() {
        AST_NODE *forNode = new AST_NODE();
        forNode->TYPE = NODE_FOR;

        proceed(TOKEN_KEYWORD);

        bool paran = false;

        if (current->TYPE == TOKEN_LPAREN)
            paran = true;
            proceed(TOKEN_LPAREN);

        // init
        AST_NODE *initNode = nullptr;
        if (current->TYPE == TOKEN_ID)
            initNode = parseID();
        else
            parserError("Expected initialization in for-loop");

        proceed(TOKEN_COLON);

        // condition
        AST_NODE *conditionNode = parseComparison();

        proceed(TOKEN_COLON);

        // increment
        AST_NODE *incrementNode = parseIncrement();

        if (paran)
            proceed(TOKEN_RPAREN);

        forNode->SUB_STATEMENTS.push_back(initNode);
        forNode->SUB_STATEMENTS.push_back(conditionNode);
        forNode->SUB_STATEMENTS.push_back(incrementNode);

        // body
        proceed(TOKEN_LBRACE);
        while (current->TYPE != TOKEN_RBRACE && current->TYPE != TOKEN_EOF) {
            while (current->TYPE == TOKEN_NEWLINE)
                proceed(TOKEN_NEWLINE);

            if (current->TYPE == TOKEN_RBRACE)
                break;

            forNode->SUB_STATEMENTS.push_back(parseStatement());

            while (current->TYPE == TOKEN_NEWLINE)
                proceed(TOKEN_NEWLINE);
        }
        proceed(TOKEN_RBRACE);

        return forNode;
    }

    AST_NODE *parseBOOL(enum NODE_TYPE Tokentype) {
        AST_NODE *node = new AST_NODE();
            node->TYPE = Tokentype;
            node->value = new string(current->value);
            node->charno = current->charno;
            return node;
    }


     // ---------- FUNCTION CALL ----------
    AST_NODE *parseFunctionCall(string *funcName) {
        AST_NODE *callNode = new AST_NODE();
        callNode->TYPE = NODE_FUNCTION_CALL;
        callNode->value = funcName;

        proceed(TOKEN_LPAREN);
        if (current->TYPE != TOKEN_RPAREN) {
            callNode->SUB_STATEMENTS.push_back(parseExpression());
            while (current->TYPE == TOKEN_COMMA) {
                proceed(TOKEN_COMMA);
                callNode->SUB_STATEMENTS.push_back(parseExpression());
            }
        }
        proceed(TOKEN_RPAREN);
        return callNode;
    }
    

    // ---------- FUNCTION DECLARATION ----------
    AST_NODE *parseFunctionDecl(bool callback=false, string typeCLK="onclick") {
       AST_NODE *funcNode = new AST_NODE();
        if (!callback) {
             proceed(TOKEN_KEYWORD); // "function"
            if (current->TYPE != TOKEN_ID) {
                parserError("Expected function name after 'function'"); 
            }
            string *funcName = &current->value;
            proceed(TOKEN_ID);

            funcNode->TYPE = NODE_FUNCTION_DECL;
            funcNode->value = funcName;
        } else {
            string *funcName = &typeCLK;

            funcNode->TYPE = NODE_FUNCTION_DECL;
            funcNode->value = funcName;
        }
        

        proceed(TOKEN_LPAREN);
        if (current->TYPE != TOKEN_RPAREN) {
            AST_NODE *param = new AST_NODE();
            param->TYPE = NODE_VARIABLE;
            param->value = &current->value;
            proceed(TOKEN_ID);
            funcNode->SUB_STATEMENTS.push_back(param);

            while (current->TYPE == TOKEN_COMMA) {
                proceed(TOKEN_COMMA);
                AST_NODE *param = new AST_NODE();
                param->TYPE = NODE_VARIABLE;
                param->value = &current->value;
                proceed(TOKEN_ID);
                funcNode->SUB_STATEMENTS.push_back(param);
            }
        }
        proceed(TOKEN_RPAREN);

        // Parse function body
        proceed(TOKEN_LBRACE);
        while (current->TYPE != TOKEN_RBRACE && current->TYPE != TOKEN_EOF) {
            while (current->TYPE == TOKEN_NEWLINE)
                proceed(TOKEN_NEWLINE);

            if (current->TYPE == TOKEN_RBRACE)
                break;
            
            if (current->TYPE == TOKEN_KEYWORD)
                // || current->value != "true" || current->value != "false"
                if (current->value == "continue")
                {
                    parserError("Unexpected Keyword in Function declaration : "+ current->value);
                }
            funcNode->SUB_STATEMENTS.push_back(parseStatement());

            while (current->TYPE == TOKEN_NEWLINE)
                proceed(TOKEN_NEWLINE);
        }
        proceed(TOKEN_RBRACE);
        return funcNode;
    }

    // ---------- FUNCTION DECLARATION ----------

    AST_NODE *parseApp() {
        string *funcName = &current->value;
        proceed(TOKEN_KEYWORD); // "app"
        
        AST_NODE *appNode = new AST_NODE();
        appNode->TYPE = NODE_app;
        appNode->value = funcName;
        appNode->lineno = current->lineno;
        appNode->sourceLine = current->sourceLine;
        appNode->extra = current->extra;
        appNode->charno = current->charno;

        proceed(TOKEN_LPAREN);
        if (current->TYPE != TOKEN_RPAREN)
            parserError("app() should not have arguments");
        proceed(TOKEN_RPAREN);

        proceed(TOKEN_LBRACE);

        while (current->TYPE != TOKEN_RBRACE && current->TYPE != TOKEN_EOF) {
            while (current->TYPE == TOKEN_NEWLINE)
                proceed(TOKEN_NEWLINE);

            if (current->TYPE == TOKEN_RBRACE)
                break;

            if (current->value != "page") {
                parserError("Only page() is allowed inside app()");
            }

            appNode->SUB_STATEMENTS.push_back(parsepage());

            while (current->TYPE == TOKEN_NEWLINE)
                proceed(TOKEN_NEWLINE);
        }

        proceed(TOKEN_RBRACE);
        return appNode;
    }



    /*
        page() {

        }
    */
    AST_NODE *parsepage() {
        string *funcName = &current->value;
        proceed(TOKEN_KEYWORD); // "page"
        
        AST_NODE *funcNode = new AST_NODE();
        funcNode->TYPE = NODE_page;
        funcNode->value = funcName;

        proceed(TOKEN_LPAREN);
        if (current->TYPE != TOKEN_RPAREN) {
            AST_NODE *args = new AST_NODE();
            args->TYPE = NODE_ARGS;

            AST_NODE *param = new AST_NODE();
            if (current->TYPE == TOKEN_STRING || current->TYPE == TOKEN_ID)
            {
                if (current->TYPE == TOKEN_STRING)
                {
                    param->TYPE = NODE_STRING;
                    param->value = &current->value;
                    param->lineno = current->lineno;
                    param->sourceLine = current->sourceLine;
                    param->extra = current->extra;
                    param->charno = current->charno;
                    proceed(TOKEN_STRING);
                }
                if (current->TYPE == TOKEN_ID)
                {
                    param->TYPE = NODE_VARIABLE;
                    param->value = &current->value;
                    param->lineno = current->lineno;
                    param->sourceLine = current->sourceLine;
                    param->extra = current->extra;
                    param->charno = current->charno;
                    proceed(TOKEN_ID);
                }
                
            } else {
                parserError("Unexpected in page Call : "+ current->value);
            }


            args->SUB_STATEMENTS.push_back(param);
            

           while (current->TYPE == TOKEN_COMMA) {
                proceed(TOKEN_COMMA);

                // Allowed parameters
                if (current->value != "style" &&
                    current->value != "route" &&
                    current->value != "id" &&
                    current->value != "cls") 
                {
                    parserError("Expecting one of: 'id', 'cls', 'style', 'route' but got: " + current->value);
                }

                AST_NODE *param = new AST_NODE();
                param->TYPE = NODE_VARIABLE;
                param->value = &current->value;
                param->lineno = current->lineno;
                param->sourceLine = current->sourceLine;
                param->extra = current->extra;
                param->charno = current->charno;

                std::string paramName = current->value;
                proceed(TOKEN_ID);
                proceed(TOKEN_EQ);

                // ───────────────────────────────────────────────────────────────
                // Handle different parameter types based on paramName
                // ───────────────────────────────────────────────────────────────

                if (paramName == "style") {
                    // style must be a dict always
                    param->CHILD = parseDict();
                }
                else if (paramName == "route") {
                    // route must be string only
                    for (unsigned char ch : current->value) {
                        if (isspace(ch)) {
                            parserError("Spaces not allowed in route: " + current->value);
                        }
                    }
                    if (current->TYPE != TOKEN_STRING) {
                        parserError("Route expects a string literal");
                    }

                    AST_NODE *routeNode = new AST_NODE();
                    routeNode->TYPE = NODE_STRING;
                    routeNode->value = &current->value;
                    routeNode->lineno = current->lineno;
                    routeNode->sourceLine = current->sourceLine;
                    routeNode->extra = current->extra;
                    routeNode->charno = current->charno;
                    param->CHILD = routeNode;
                    proceed(TOKEN_STRING);
                }
                else if (paramName == "id" || paramName == "cls") {
                    // id or cls can be identifier or string
                    AST_NODE *valNode = new AST_NODE();

                    if (current->TYPE == TOKEN_ID) {
                        valNode->TYPE = NODE_VARIABLE;
                        valNode->value = &current->value;
                        valNode->lineno = current->lineno;
                        valNode->sourceLine = current->sourceLine;
                        valNode->extra = current->extra;
                        valNode->charno = current->charno;
                        param->CHILD = valNode;
                        proceed(TOKEN_ID);
                    }
                    else if (current->TYPE == TOKEN_STRING) {
                        valNode->TYPE = NODE_STRING;
                        valNode->value = &current->value;
                        valNode->lineno = current->lineno;
                        valNode->sourceLine = current->sourceLine;
                        valNode->extra = current->extra;
                        valNode->charno = current->charno;
                        param->CHILD = valNode;
                        proceed(TOKEN_STRING);
                    }
                    else {
                        parserError(paramName + " expects an identifier or a string literal");
                    }
                }

                // ───────────────────────────────────────────────────────────────

                args->SUB_STATEMENTS.push_back(param);
            }


            funcNode->CHILD = args;
        }
        proceed(TOKEN_RPAREN);

        // Parse function body
        proceed(TOKEN_LBRACE);
        while (current->TYPE != TOKEN_RBRACE && current->TYPE != TOKEN_EOF) {
            while (current->TYPE == TOKEN_NEWLINE)
                proceed(TOKEN_NEWLINE);

            if (current->TYPE == TOKEN_RBRACE)
                break;
            funcNode->SUB_STATEMENTS.push_back(parseStatement());

            while (current->TYPE == TOKEN_NEWLINE)
                proceed(TOKEN_NEWLINE);
        }
        proceed(TOKEN_RBRACE);
        funcNode->lineno = current->lineno;
            funcNode->sourceLine = current->sourceLine;
           funcNode->extra = current->extra;
            funcNode->charno = current->charno;
        return funcNode;
    }

    AST_NODE *parseView(enum NODE_TYPE typw) {
        string *funcName = &current->value;
        proceed(TOKEN_KEYWORD); // "View"
        
        AST_NODE *funcNode = new AST_NODE();
        funcNode->TYPE = typw;
        funcNode->value = funcName;
        funcNode->lineno = current->lineno;
            funcNode->sourceLine = current->sourceLine;
           funcNode->extra = current->extra;
            funcNode->charno = current->charno;

        proceed(TOKEN_LPAREN);
        if (current->TYPE != TOKEN_RPAREN) {
            AST_NODE *args = new AST_NODE();
            args->TYPE = NODE_ARGS;

            AST_NODE *param = new AST_NODE();
            if (current->TYPE == TOKEN_STRING || current->TYPE == TOKEN_ID)
            {
                if (current->TYPE == TOKEN_STRING)
                {
                    param->TYPE = NODE_STRING;
                    param->value = &current->value;
                    param->lineno = current->lineno;
            param->sourceLine = current->sourceLine;
           param->extra = current->extra;
            param->charno = current->charno;
                    proceed(TOKEN_STRING);
                }
                if (current->TYPE == TOKEN_ID)
                {
                    param->TYPE = NODE_VARIABLE;
                    param->value = &current->value;
                    param->lineno = current->lineno;
            param->sourceLine = current->sourceLine;
           param->extra = current->extra;
            param->charno = current->charno;
                    proceed(TOKEN_ID);
                }
                
            } else {
                parserError("Unexpected in View Call : "+ current->value);
            }


            args->SUB_STATEMENTS.push_back(param);
            

           while (current->TYPE == TOKEN_COMMA) {
                proceed(TOKEN_COMMA);

                // Only allow certain parameter names
                std::string paramName = current->value;
                if (paramName != "style" && paramName != "cls" && paramName != "onclick" && paramName != "onlongpress") {
                    parserError("Unexpected parameter: " + paramName);
                }

                AST_NODE *param = new AST_NODE();
                param->TYPE = NODE_VARIABLE;
                param->value = &current->value;
                param->lineno = current->lineno;
                param->sourceLine = current->sourceLine;
                param->extra = current->extra;
                param->charno = current->charno;

                proceed(TOKEN_ID);
                proceed(TOKEN_EQ);

                if (paramName == "style") {
                    // style can be a dict or identifier
                    if (current->TYPE == TOKEN_ID) {
                        AST_NODE *idnode = new AST_NODE();
                        idnode->TYPE = NODE_VARIABLE;
                        idnode->value = &current->value;
                        idnode->lineno = current->lineno;
                        idnode->sourceLine = current->sourceLine;
                        idnode->extra = current->extra;
                        idnode->charno = current->charno;
                        param->CHILD = idnode;
                        proceed(TOKEN_ID);
                    } else {
                        param->CHILD = parseDict();
                    }
                } 
                else if (paramName == "cls") {
                    // cls can be string or identifier
                    AST_NODE *clsNode = new AST_NODE();
                    if (current->TYPE == TOKEN_STRING) {
                        clsNode->TYPE = NODE_STRING;
                        clsNode->value = &current->value;
                        proceed(TOKEN_STRING);
                    } else if (current->TYPE == TOKEN_ID) {
                        clsNode->TYPE = NODE_VARIABLE;
                        clsNode->value = &current->value;
                        proceed(TOKEN_ID);
                    } else {
                        parserError("cls must be a string or identifier");
                    }
                    clsNode->lineno = current->lineno;
                    clsNode->sourceLine = current->sourceLine;
                    clsNode->extra = current->extra;
                    clsNode->charno = current->charno;
                    param->CHILD = clsNode;
                } 
                else if (paramName == "onclick" || paramName == "onlongpress") {
                    // onclick / onlongpress can be function identifier or inline function
                    if (current->TYPE == TOKEN_ID) {
                        // function call
                        param->CHILD = parseFunctionCall(&current->value);
                    } else if (current->TYPE == TOKEN_KEYWORD && current->value == "def") {
                        // inline function
                        param->CHILD = parseFunctionDecl();
                    } else if (current->TYPE == TOKEN_LPAREN) {
                        // arrow function / anonymous function like () { ... }
                        param->CHILD = parseFunctionDecl(true, paramName); // reuse parseFunctionDecl to handle body
                    } else {
                        parserError(paramName + " must be a function identifier or inline function");
                    }
                }


                args->SUB_STATEMENTS.push_back(param);
            }

            funcNode->CHILD = args;
        }
        proceed(TOKEN_RPAREN);

        // Parse function body
        if(current->TYPE == TOKEN_LBRACE)
        {
            proceed(TOKEN_LBRACE);
            while (current->TYPE != TOKEN_RBRACE && current->TYPE != TOKEN_EOF) {
                while (current->TYPE == TOKEN_NEWLINE)
                    proceed(TOKEN_NEWLINE);

                if (current->TYPE == TOKEN_RBRACE)
                    break;

                funcNode->SUB_STATEMENTS.push_back(parseStatement());

                while (current->TYPE == TOKEN_NEWLINE)
                    proceed(TOKEN_NEWLINE);
            }
            proceed(TOKEN_RBRACE);
        } else {
                proceed(TOKEN_NEWLINE);
        }
        funcNode->lineno = current->lineno;
            funcNode->sourceLine = current->sourceLine;
           funcNode->extra = current->extra;
            funcNode->charno = current->charno;
        return funcNode;
    }


    std::vector<std::string> defined_keywords = { "true", "false", "null" };
    std::vector<std::string> loop_keywords = { "continue", "break", "pass" };
    // ---------- Keyword Dispatcher ----------
    AST_NODE *parseKEYWORDS() {
        if (std::find(defined_keywords.begin(), defined_keywords.end(), current->value) != defined_keywords.end())
        {   
            return parseBOOL(NODE_BOOL);
        } else if (std::find(loop_keywords.begin(), loop_keywords.end(), current->value) != loop_keywords.end()) {
            AST_NODE * newnode = parseBOOL(NODE_LOOP_CTRL);
            proceed(TOKEN_KEYWORD);
            return newnode;
        }
        else {
            if (current->value == "return") {
            proceed(TOKEN_KEYWORD);
            AST_NODE *node = new AST_NODE();
            node->TYPE = NODE_RETURN;
            node->CHILD = parseComparison();
            return node;
            } else if (current->value == "print") {
                proceed(TOKEN_KEYWORD);
                AST_NODE *node = new AST_NODE();
                node->TYPE = NODE_PRINT;
                proceed(TOKEN_LPAREN);
                node->CHILD = parseComparison();
                proceed(TOKEN_RPAREN);
                return node;
            } else if (current->value == "img") {
                return parseView(NODE_IMAGE);
            }else if (current->value == "if") {
                return parseConditional();
            } else if (current->value == "while") {
                return parseWhile();
            } else if (current->value == "for") {
                return parseFor();
            } else if (current->value == "type") {
                proceed(TOKEN_KEYWORD);
                AST_NODE *node = new AST_NODE();
                node->TYPE = NODE_TYPE_CHECK;
                proceed(TOKEN_LPAREN);
                node->CHILD = parseFactor();
                proceed(TOKEN_RPAREN);
                node->charno = current->charno;
                return node;
            }  else if (current->value == "def") {
                return parseFunctionDecl();
            } else if (current->value == "page") {
                return parsepage();
            } else if (current->value == "view") {
                return parseView(NODE_VIEW);
            } else if (current->value == "text") {
                return parseView(NODE_TEXT);
            }else if (current->value == "app") {
                return parseApp();
            }
            else {
                parserError("Unknown keyword: " + current->value);
            }
        }
        return nullptr;
    }

    // ---------- Generic Statement ----------
    AST_NODE *parseStatement() {
        cout << typetostring(current->TYPE) << endl;
        if (current->TYPE == TOKEN_ID)
            return parseID();
        else if (current->TYPE == TOKEN_HASH)
            {proceed(current->TYPE);
            while (current->TYPE != TOKEN_NEWLINE)
            {
                proceed(current->TYPE);
            }
            return nullptr;}
        else if (current->TYPE == TOKEN_DIVOP){
            proceed(current->TYPE);
            proceed(TOKEN_MULOP);
            bool endofcomment = false;
            while (!endofcomment && current->TYPE != TOKEN_EOF)
            {
                if (current->TYPE == TOKEN_MULOP)
                {
                    proceed(TOKEN_MULOP);
                    if (current->TYPE == TOKEN_DIVOP)
                    {
                        endofcomment = true;
                    }
                    
                }
                
                proceed(current->TYPE);
            }
            return nullptr;
        }else if (current->TYPE == TOKEN_KEYWORD)
            return parseKEYWORDS();
        else {
            parserError("Unexpected token in statement : " + current->value);
        }
        return nullptr;
    }

    // ---------- Parse Root ----------
    AST_NODE *parse() {
        AST_NODE *ROOT = new AST_NODE();
        ROOT->TYPE = NODE_ROOT;

        while (current->TYPE != TOKEN_EOF) {
            if (current->TYPE == TOKEN_NEWLINE) {
                proceed(TOKEN_NEWLINE);
                continue;
            }
            ROOT->SUB_STATEMENTS.push_back(parseStatement());
            if (current->TYPE == TOKEN_NEWLINE)
                proceed(TOKEN_NEWLINE);
        }
        return ROOT;
    }

private:
    int limit;
    int index;
    Token *current;
    vector<Token *> parserTokens;
};

#endif
