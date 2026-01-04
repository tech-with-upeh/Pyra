#pragma once
#include "parser.hpp"
#include <iostream>
#include <unordered_map>
#include <string>
#include <vector>
#include <span>

enum VarType {
    TYPE_UNKNOWN,
    TYPE_INT,
    TYPE_STRING,
    TYPE_BOOL,
    TYPE_FUNCTION,
    TYPE_FLOAT,
    TYPE_DICT,
};

string vartypestr(VarType type) {
    switch (type) {
        case TYPE_INT: return "INT"; break;
        case TYPE_STRING: return "STRING"; break;
        case TYPE_BOOL: return "BOOL"; break;
        case TYPE_FUNCTION: return "FUNCTION"; break;
        case TYPE_DICT: return "DICT"; break;
        case TYPE_FLOAT: return "FLOAT"; break;
        default: return "UNKNOWN"; break;
    }
}

struct VarInfo {
    VarType type;
    bool initialized;
};

struct PageInfo {
    string title;
    bool index;
};

struct InstanceInfo {
    std::unordered_map<std::string, std::vector<VarType>> callables;
    bool issystemdefined;
};

class SemanticAnalyzer {
public:
    void analyze(AST_NODE *root) {
        scope.clear();
        statevars.clear();
        declaredFunctions.clear();
        calledFunctions.clear();
        pagescope.clear();
        instances.clear();

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
    std::unordered_map<std::string, VarInfo> statevars;
    std::unordered_map<std::string, VarInfo> declaredFunctions;
    std::vector<std::string> calledFunctions;
    std::unordered_map<std::string, PageInfo> pagescope;
    std::unordered_map<std::string, InstanceInfo> instances;
    std::unordered_map<std::string, std::vector<VarType>> draw_callables = {
        {"clear", {}}, 
        {"rect", {TYPE_INT,TYPE_INT,TYPE_INT,TYPE_INT}},
        {"setFill", {TYPE_STRING}}
    };



    void parserError(const std::string &message, AST_NODE* current) {
        std::cerr << "\nSemantics Error: " << message
                  << " at line " << current->lineno
                  << ", column " << current->charno << "\n";
        std::cerr << "  " << current->lineno << " | " << current->sourceLine << "\n";
        for (int i = 1; i < (current->charno+to_string(current->lineno).length()+1); ++i)
            std::cerr << " ";
        for (int i = 0; i < current->value->length(); i++)
        {
            std::cerr << "^";
        }
        std::cerr << "\n\n";
        std::exit(1);
    }

    VarType checkNode(AST_NODE *node, bool uiexceptonstylsheet = false, bool funcdecl = false, bool isfrompage = false) {
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
            case NODE_FLOAT:
                return TYPE_FLOAT;
            
            case NODE_DICT: {
                for (const auto& i : node->SUB_STATEMENTS)
                {
                    if (!uiexceptonstylsheet)
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
            case NODE_SETSTATE:
            case NODE_VARIABLE: {
                std::string name = *node->value;
                if (node->CHILD) {
                    VarType rhsType = checkNode(node->CHILD, uiexceptonstylsheet, funcdecl, isfrompage);
                    if (node->CHILD->TYPE == NODE_DRAW) {
                        instances[name] = {draw_callables, true};
                    }
                    if(node->TYPE == NODE_VARIABLE) {
                        scope[name] = {rhsType, true};
                    }
                    if(node->TYPE == NODE_SETSTATE) {
                        statevars[name] = {rhsType, true};
                    }
                    return rhsType;
                } else {
                    // variable usage
                    auto it = scope.find(name);
                    auto st = statevars.find(name);
                    
                    if(st == statevars.end()) { 
                        if(it == scope.end()) {
                            parserError("Variable '" + name + "' used before assignment.", node);
                        } 
                    }
                    if (st != statevars.end()) {
                        return statevars[name].type;
                   }
                   if (it != scope.end()) {
                        return scope[name].type;
                   }
                   
                }
            }
            case NODE_page: {
                cout << "Semantic Check on page() Node\n";
                if (node->CHILD) {
                    VarType node1 = checkNode(node->CHILD->SUB_STATEMENTS[0]);
                    if (node1 != TYPE_STRING)
                    {
                       parserError("Title can only be a string but got: '" + *(node->CHILD->SUB_STATEMENTS[0]->value) + "'", node->CHILD->SUB_STATEMENTS[1]);
                    }
                   
                    bool isindex = false;
                    if (node->CHILD->SUB_STATEMENTS.size() > 1)
                    {
                        for (auto it = node->CHILD->SUB_STATEMENTS.begin() +1; it != node->CHILD->SUB_STATEMENTS.end(); ++it)
                        {
                            AST_NODE* it_node = *it;
                            cout << *(it_node->value) << endl;
                            if(*(it_node->value) == "route") {                              
                                if (*(it_node->CHILD->value) == "/")
                                {
                                    isindex = true;
                                }
                                else
                                {
                                    isindex = false;
                                }
                                auto checkroute = pagescope.find(*(it_node->CHILD->value));
                                if (checkroute == pagescope.end()) {
                                    cout << "I---> " << *(it_node->CHILD->value) << endl;
                                    pagescope[*(it_node->CHILD->value)] = {*(node->CHILD->SUB_STATEMENTS[0]->value), isindex};
                                } else {
                                    parserError("Route '" + *(it_node->CHILD->value) + "' already: '" + checkroute->second.title + "'", node->CHILD->SUB_STATEMENTS[0]);
                                }
                                
                            }
                            VarType node2 = checkNode(it_node->CHILD, true);
                            if (node2 != TYPE_DICT && node2 != TYPE_STRING)
                            {
                                parserError("Unknown Type in Args in page() but got: '" + *(it_node->value) + "'", it_node);
                            }
                        }
                        if (isindex) {
                            auto pagesc = pagescope.find("/");
                            if (pagesc == pagescope.end()) {
                                pagescope["/"] = {*(node->CHILD->SUB_STATEMENTS[0]->value), true};
                            } else {          
                                parserError("Index page already defined: '" + pagesc->second.title + "'", node->CHILD->SUB_STATEMENTS[0]);
                            } 
                        }
                            
                    } else {
                        auto pagesc = pagescope.find("/");
                        
                        if (pagesc == pagescope.end()) {
                            pagescope["/"] = {*(node->CHILD->SUB_STATEMENTS[0]->value), true};
                        } else {          
                            parserError("Index page already defined as: '" + pagesc->second.title + "'", node->CHILD->SUB_STATEMENTS[0]);
                        }      
                    }
                }
                if (!node->SUB_STATEMENTS.empty())
                {
                    for (auto &i : node->SUB_STATEMENTS)
                    {
                        if (i->TYPE == NODE_FUNCTION_DECL && *(i->value) == "onmount")  {
                            checkNode(i, false, true, true);
                            continue;
                        }
                        checkNode(i, true,false, true);
                    }
                }
                statevars.clear();
                instances.clear();
                return TYPE_FUNCTION;
            }

            case NODE_TEXT:
            case NODE_CANVAS:
            case NODE_IMAGE:
            case NODE_VIEW: {
                if (node->CHILD) {
                    
                    VarType node1 = checkNode(node->CHILD->SUB_STATEMENTS[0]);
                    if (node1 != TYPE_STRING)
                    {
                       parserError("Title can only be a string but got: '" + *(node->CHILD->SUB_STATEMENTS[0]->value) + "'", node->CHILD->SUB_STATEMENTS[0]);
                    }
                    
                    if (node->CHILD->SUB_STATEMENTS.size() > 1)
                    {
                        for (auto it = node->CHILD->SUB_STATEMENTS.begin() +1; it != node->CHILD->SUB_STATEMENTS.end(); ++it)
                        {
                            AST_NODE* it_node = *it;
                            VarType node2 = checkNode(it_node, true, true);
                            if (node2 != TYPE_DICT && node2 != TYPE_STRING && node2 != TYPE_FUNCTION && node2 != TYPE_INT)
                            {
                                parserError("Unknown Type in Args in View() but got: '" + *(it_node->value) + "<->" + *(it_node->CHILD->value) + "'", it_node->CHILD);
                            }
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
                return TYPE_FUNCTION;
            }
            // Binary operations (e.g., +, -, ==)
            case NODE_BINARY_OP:
            case NODE_COMPARISON_OP: {
                VarType leftType = checkNode(node->SUB_STATEMENTS[0]);
                VarType rightType = checkNode(node->SUB_STATEMENTS[1]);
                std::string op = *node->value;

                if (leftType == TYPE_INT ||  leftType == TYPE_FLOAT) {
                    if (rightType == TYPE_INT || rightType == TYPE_FLOAT) {
                        return TYPE_FLOAT;
                    }
                }
                if (leftType != rightType) {
                   parserError("Type mismatch in binary operation, can't "+ vartypestr(leftType) + " " + op + " " + vartypestr(rightType), node);
                }

                if (op == "*" || op == "/") {
                    if (leftType != TYPE_INT && leftType != TYPE_FLOAT)
                        parserError("Operator '" + op + "' only supports numbers.", node);
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
                
                // Register arguments in scope
                if (node->CHILD && node->CHILD->TYPE == NODE_ARGS) {
                    for (auto param : node->CHILD->SUB_STATEMENTS) {
                        if (funcdecl)
                        {
                            std::string name = *param->value;

                            auto it = scope.find(name);
                            auto st = statevars.find(name);
                                
                                if(st == statevars.end()) { 
                                    if(it == scope.end()) {
                                        parserError("Variable '" + name + "' used before assignment.", param);
                                    } 
                                }
                            if (it != scope.end()) {
                                    return scope[name].type;
                            }
                            if (st != statevars.end()) {
                                    return statevars[name].type;
                            }
                        }
                        else
                        {
                            if (param->TYPE == NODE_VARIABLE) {
                             std::string paramName = *param->value;
                             scope[paramName] = {TYPE_UNKNOWN, true}; 
                        }
                        }
                    }
                }

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
            case NODE_TOINT:
            case NODE_TOSTR:
            case NODE_TOFLOAT:
            case NODE_RETURN:
            case NODE_PRINT:
            case NODE_GO:
            case NODE_DRAW:
            case NODE_TYPE_CHECK:
                if (node->CHILD) {
                    if (node->TYPE == NODE_DRAW) {
                        if (!isfrompage) {
                            //logic to add to instance
                        }
                        VarType ty = checkNode(node->CHILD->SUB_STATEMENTS[0]);
                        if (ty != TYPE_STRING) {
                            parserError("Type Conversion Must be Str.", node->CHILD);
                        }
                        return TYPE_FUNCTION;
                    }
                    VarType ty = checkNode(node->CHILD);

                    if(node->TYPE == NODE_TOFLOAT || node->TYPE == NODE_TOINT) {
                        if (ty != TYPE_STRING) {
                            parserError("Type Conversion Must be Str.", node->CHILD);
                        }
                        if (node->TYPE == NODE_TOINT) {
                            return TYPE_INT;
                        }
                        if (node->TYPE == NODE_TOFLOAT) {
                            return TYPE_FLOAT;
                        }
                    }
                    if (node->TYPE == NODE_TOSTR) {
                        return TYPE_STRING;
                    }
                } 
                
                return TYPE_FUNCTION;

            

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
                    checkNode(stmt);
                }
                return TYPE_UNKNOWN;
            }

            case NODE_ELSE:
            case NODE_FOR:
                for (auto stmt : node->SUB_STATEMENTS)
                    checkNode(stmt);
                return TYPE_UNKNOWN;
            case NODE_STYLESHEET:
                for (auto stmt : node->SUB_STATEMENTS)
                    checkNode(stmt, false);
                return TYPE_UNKNOWN;
            case NODE_CLS:
                checkNode(node->CHILD, true);
                return TYPE_UNKNOWN;
            case NODE_MEDIA_QUERY: {
                checkNode(node->CHILD);
                for (auto stmt : node->SUB_STATEMENTS)
                    checkNode(stmt, false);
                return TYPE_UNKNOWN;
            }

            case NODE_INSTANCE: {
                auto it = instances.find(*(node->value));
                if (it == instances.end()) {
                    parserError("'"+ *(node->value) +"' is not Callable", node);
                }
                if (node->CHILD) {
                    auto& calls = it->second.callables;
                
                    auto ch = calls.find(*(node->CHILD->value));
                    if (ch == calls.end()) {
                        parserError("'"+ *(node->value) +"' Has no member '"+ *(node->CHILD->value) +"'", node->CHILD);
                    } 
                    if(node->CHILD->SUB_STATEMENTS.empty()) {
                        std::cout << "-->" << ch->second.size() << std::endl;
                    } else {
                        if (node->CHILD->SUB_STATEMENTS.size() != ch->second.size()) {
                            parserError("'"+ *(node->CHILD->value) +"' was expecting '"+ to_string(ch->second.size()) +" arguments but got: " + to_string(node->CHILD->SUB_STATEMENTS.size()), node->CHILD);
                        }

                        for (auto &subs : node->CHILD->SUB_STATEMENTS) {
                            for (auto &chv : ch->second) {
                                VarType nodecheck = checkNode(subs, uiexceptonstylsheet, funcdecl, isfrompage);
                                if (nodecheck != chv ) {
                                    if (nodecheck == TYPE_INT && chv == TYPE_FLOAT) {
                                        continue;
                                    } else if (nodecheck == TYPE_FLOAT && chv == TYPE_INT) {
                                        continue;
                                    }
                                    parserError("'"+ *(node->CHILD->value) +"' was expecting '"+ vartypestr(chv) +"  but got: " + vartypestr(nodecheck), subs);
                                }
                            }
                        }
                    }
                }
                return TYPE_FUNCTION;
            }
            case NODE_MATH_POW: {
                if (node->CHILD) 
                {
                    VarType arg = checkNode(node->CHILD, uiexceptonstylsheet, funcdecl,isfrompage);
                    if (arg != TYPE_INT && arg != TYPE_FLOAT) {
                        parserError("power only accepts number but got "+ nodetostr(node->CHILD->TYPE), node->CHILD);
                    }
                    return TYPE_FLOAT;
                }
                for (auto &i : node->SUB_STATEMENTS)
                {
                    VarType arg = checkNode(i, uiexceptonstylsheet, funcdecl,isfrompage);
                    if (arg != TYPE_INT && arg != TYPE_FLOAT) {
                        parserError("power only--accepts number but got "+ nodetostr(i->TYPE), i);
                    }   
                }
                return TYPE_FLOAT;
            }
            case NODE_MATH_COS:
            case NODE_MATH_SQRT:
            case NODE_MATH_TAN:
            case NODE_MATH_SIN: {
                VarType arg = checkNode(node->CHILD, uiexceptonstylsheet, funcdecl,isfrompage);
                if (arg != TYPE_INT && arg != TYPE_FLOAT) {
                    parserError( nodetostr(node->TYPE) +" only accepts number but got "+ nodetostr(node->CHILD->TYPE), node->CHILD);
                }
                return TYPE_FLOAT;
            }
            default:
                return TYPE_UNKNOWN;
        }
    }

    void semanticError(const std::string &msg) {
        std::cerr << "\n[SemanticError] " << msg << std::endl;
        exit(1);
    }
};
