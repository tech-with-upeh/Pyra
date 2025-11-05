#pragma once
#include "parser.hpp"
#include <iostream>
#include <unordered_map>
#include <string>
#include <vector>

enum VarType {
    TYPE_UNKNOWN,
    TYPE_INT,
    TYPE_STRING,
    TYPE_BOOL,
    TYPE_FUNCTION,
    TYPE_DICT,
};

struct VarInfo {
    VarType type;
    bool initialized;
};

class SemanticAnalyzer {
public:
    void analyze(AST_NODE *root) {
        scope.clear();
        declaredFunctions.clear();
        calledFunctions.clear();

        // Pass 1: Analyze all statements
        for (auto stmt : root->SUB_STATEMENTS) {
            checkNode(stmt);
        }

        // Pass 2: Validate functions that were called but not declared
        for (const auto &fname : calledFunctions) {
            if (declaredFunctions.find(fname) == declaredFunctions.end()) {
                semanticError("Function '" + fname + "' called but not declared.");
            }
        }
    }

private:
    std::unordered_map<std::string, VarInfo> scope;
    std::unordered_map<std::string, VarInfo> declaredFunctions;
    std::vector<std::string> calledFunctions;

    void parserError(const std::string &message, AST_NODE* current) {
        std::cerr << "\nSEM-Error: " << message
                  << " at line " << current->lineno
                  << ", column " << current->charno << "\n";
        std::cerr << "  " << current->lineno << " | " << current->sourceLine << "\n";
        std::cerr << "    ";
        for (int i = 1; i < (current->charno+to_string(current->lineno).length()+1); ++i)
            std::cerr << " ";
        for (int i = 0; i < current->value->length(); i++)
        {
            std::cerr << "^";
        }
        std::cerr << "\n\n";
        std::exit(1);
    }

    VarType checkNode(AST_NODE *node, bool uiexcept = false) {
        if (!node) return TYPE_UNKNOWN;

        switch (node->TYPE) {
            case NODE_INT:
                return TYPE_INT;

            case NODE_STRING:
                return TYPE_STRING;

            case NODE_BOOL: {
                VarType var1 = checkNode(node->SUB_STATEMENTS[0]);
                VarType var2 = checkNode(node->SUB_STATEMENTS[1]);
                if (var1 != var2)
                {
                    parserError("Variables don't match '" + *(node->SUB_STATEMENTS[0]->value) + "'" + "  &  " + "'" + *(node->SUB_STATEMENTS[1]->value) + "' doesnt match!.", node);
                }
                
                return TYPE_BOOL;
            }
            
            case NODE_DICT: {
                for (const auto& i : node->SUB_STATEMENTS)
                {
                    if (!uiexcept)
                    {
                        if (i->SUB_STATEMENTS[0]->TYPE == NODE_VARIABLE)
                        {
                            if (node->value)
                            {
                                if (*(node->value) != string("#"))
                                {
                                   VarType rhsType = checkNode(i->SUB_STATEMENTS[0]);
                                }
                            }  else {
                                 VarType rhsType = checkNode(i->SUB_STATEMENTS[0]);
                            }            
                        }
                    }
                    
                    if (i->SUB_STATEMENTS[1]->TYPE == NODE_VARIABLE)
                    {
                        VarType rhsType = checkNode(i->SUB_STATEMENTS[1]);
                    }
                }
                
                return TYPE_DICT;
            }

            // Variable declaration or assignment
            case NODE_VARIABLE: {
                std::string name = *node->value;
                if (node->CHILD) {
                    VarType rhsType = checkNode(node->CHILD);
                    scope[name] = {rhsType, true};
                    return rhsType;
                } else {
                    // variable usage
                    if (scope.find(name) == scope.end() || !scope[name].initialized)
                        parserError("Variable '" + name + "' used before assignment.", node);
                    return scope[name].type;
                }
            }
            case NODE_WINDOW: {
                if (node->CHILD) {
                    VarType node1 = checkNode(node->CHILD->SUB_STATEMENTS[0]);
                    if (node1 != TYPE_STRING)
                    {
                       parserError("Title can only be a string but got: '" + *(node->CHILD->SUB_STATEMENTS[0]->value) + "'", node->CHILD->SUB_STATEMENTS[1]);
                    }
                    cout << "--> size " << node->CHILD->SUB_STATEMENTS.size() << endl;
                    if (node->CHILD->SUB_STATEMENTS.size() > 1)
                    {
                        VarType node2 = checkNode(node->CHILD->SUB_STATEMENTS[1]->CHILD, true);
                        cout << node2 << endl;
                        if (node2 != TYPE_DICT)
                        {
                            parserError("style can only be a Dictionary but got: '" + *(node->CHILD->SUB_STATEMENTS[1]->value) + "'", node->CHILD->SUB_STATEMENTS[1]);
                        }
                    }
                }
                if (!node->SUB_STATEMENTS.empty())
                {
                    for (auto &i : node->SUB_STATEMENTS)
                    {
                        checkNode(i);
                    }
                }
                else
                {
                    cout << "was empty" <<endl;
                }
                
                return TYPE_FUNCTION;
            }

            case NODE_TEXT:
            case NODE_IMAGE:
            case NODE_VIEW: {
                if (node->CHILD) {
                    VarType node1 = checkNode(node->CHILD->SUB_STATEMENTS[0]);
                    if (node1 != TYPE_STRING)
                    {
                       parserError("Title can only be a string but got: '" + *(node->CHILD->SUB_STATEMENTS[0]->value) + "'", node->CHILD->SUB_STATEMENTS[1]);
                    }
                    cout << "--> size " << node->CHILD->SUB_STATEMENTS.size() << endl;
                    if (node->CHILD->SUB_STATEMENTS.size() > 1)
                    {
                        VarType node2 = checkNode(node->CHILD->SUB_STATEMENTS[1]->CHILD, true);
                        cout << node2 << endl;
                        if (node2 != TYPE_DICT)
                        {
                            parserError("style can only be a Dictionary but got: '" + *(node->CHILD->SUB_STATEMENTS[1]->value) + "'", node->CHILD->SUB_STATEMENTS[1]);
                        }
                    }
                }
                if (!node->SUB_STATEMENTS.empty())
                {
                    for (auto &i : node->SUB_STATEMENTS)
                    {
                        checkNode(i);
                    }
                } else {
                    cout << "was-- empty" << endl;
                }
                return TYPE_FUNCTION;
            }
            // Binary operations (e.g., +, -, ==)
            case NODE_BINARY_OP:
            case NODE_COMPARISON_OP: {
                VarType leftType = checkNode(node->SUB_STATEMENTS[0]);
                VarType rightType = checkNode(node->SUB_STATEMENTS[1]);
                std::string op = *node->value;

                if (leftType != rightType) {
                    semanticError("Type mismatch in binary operation '" + op + "'");
                }

                if (op == "*" || op == "/") {
                    if (leftType != TYPE_INT)
                        semanticError("Operator '" + op + "' only supports numbers.");
                    return TYPE_INT;
                }

                if (op == "==" || op == "!=" || op == "<" || op == ">")
                    return TYPE_BOOL;

                return TYPE_UNKNOWN;
            }

            // Function declaration
            case NODE_FUNCTION_DECL: {
                std::string name = *node->value;
                declaredFunctions[name] = {TYPE_FUNCTION, true};
                // Check the function body
                for (auto stmt : node->SUB_STATEMENTS)
                    checkNode(stmt);
                return TYPE_FUNCTION;
            }

            // Function call (even undeclared — checked in post-pass)
            case NODE_FUNCTION_CALL: {
                std::string fname = *node->value;
                calledFunctions.push_back(fname);

                // Evaluate arguments for validity
                for (auto arg : node->SUB_STATEMENTS)
                    checkNode(arg);

                if (declaredFunctions.find(fname) != declaredFunctions.end())
                    return declaredFunctions[fname].type;

                return TYPE_UNKNOWN;
            }

            // Return and print
            case NODE_RETURN:
            case NODE_PRINT:
            case NODE_TYPE_CHECK:
                if (node->CHILD) checkNode(node->CHILD);
                return TYPE_UNKNOWN;

            // If / While / For blocks
            case NODE_IF: {
                if (node->CHILD) {
                    VarType condType = checkNode(node->CHILD);
                    if (condType != TYPE_BOOL) {
                        parserError("Condition in if statement must evaluate to a boolean.", node->CHILD);
                    }
                    for (auto stmt : node->CHILD->SUB_STATEMENTS)
                    {
                        checkNode(stmt);
                    }
                    for (auto stmt : node->SUB_STATEMENTS)
                        checkNode(stmt);
                    return TYPE_UNKNOWN;
                }
            }
            case NODE_WHILE: {
                if (node->CHILD) {
                    VarType condType = checkNode(node->CHILD);
                    if (condType != TYPE_BOOL)
                        semanticError("Condition in if/while must evaluate to a boolean.");
                }
                for (auto stmt : node->SUB_STATEMENTS)
                    checkNode(stmt);
                return TYPE_UNKNOWN;
            }
            case NODE_ELSE_IF: {
                if (node->CHILD) {
                    VarType condType = checkNode(node->CHILD);
                    if (condType != TYPE_BOOL)
                        parserError("Condition in 'else If' must evaluate to a boolean.", node->CHILD);
                }
                for (auto *stmt : node->SUB_STATEMENTS) {
                    cout << "node:---->>> " << nodetostr(stmt->TYPE) << endl;
                    checkNode(stmt);
                }
                return TYPE_UNKNOWN;
            }

            case NODE_ELSE:
            case NODE_FOR:
                for (auto stmt : node->SUB_STATEMENTS)
                    checkNode(stmt);
                return TYPE_UNKNOWN;

            default:
                cout << "got --> " << nodetostr(node->TYPE) << endl;
                return TYPE_UNKNOWN;
        }
    }

    void semanticError(const std::string &msg) {
        std::cerr << "\n[SemanticError] " << msg << std::endl;
        exit(1);
    }
};
