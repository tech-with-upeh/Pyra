#include <iostream>
#include <fstream>
#include <filesystem>
#include <string> 
#include <sstream>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include "parser.hpp" // your parser's AST_NODE and NODE_TYPE definitions

using namespace std;

class WebEngine {
    public:
        // A runtime environment for values known at generation-time (optional).
        // You can populate this map before calling gen() if you want generator-time resolution.
        unordered_map<string,string> env;

        WebEngine() : idcount(1) {}

        string pyxtocpp_type(enum NODE_TYPE type) {
            switch (type)
            {
            case NODE_BINARY_OP:
                return "float";
            case NODE_BOOL:
                return "bool";
            case NODE_INT:
                return "int";
            case NODE_STRING:
                return "string";
            case NODE_DICT:
                return "unordered_map<string,string>";
            default:
                cerr << "Error unknown Type\n";
                break;
            }
            return "ERROR";
        }

        bool gen(AST_NODE *root) {
            stringstream filebuffer;
            filebuffer << "#include \"vdom.hpp\"\n";
            filebuffer << "using namespace std;\n\n";

            // stringstream mainbuffer;

            filebuffer << "int main() {\n";

            // Top-level: convert each root sub-statement into C++ statements
            for (auto &stmt : root->SUB_STATEMENTS) {
                string out = HandleAst(stmt);
                if (!out.empty()) {
                    filebuffer << out << "\n";
                }
            }

            filebuffer << "    return 0;\n}\n";

           


            return makefile("web/generated.cpp", filebuffer.str());
        }
    private:
        int idcount;
        

        // Escape C-style string literal (for embedding fixed literal pieces in generated C++ code)
        static string escape_for_cpp_literal(const string &in) {
            string out;
            out.reserve(in.size()*2);
            for (char c : in) {
                switch (c) {
                    case '\\': out += "\\\\"; break;
                    case '\"': out += "\\\""; break;
                    case '\n': out += "\\n"; break;
                    case '\r': out += "\\r"; break;
                    case '\t': out += "\\t"; break;
                    default: out += c; break;
                }
            }
            return out;
        }

        // Helper to create a C++ literal: "\"...\""
        static string cpp_literal(const string &s) {
            return "\"" + escape_for_cpp_literal(s) + "\"";
        }
        // Resolve a node for expression usage when generating C++ expressions.
        // For NODE_STRING: returns "\"literal\"" (C++ literal)
        // For NODE_INT: returns the integer literal string
        // For NODE_VARIABLE: returns the variable name (so calling code will reference its value)
        // For NODE_BINARY_OP: returns an expression combining children's expressions.
        // This function returns a C++ expression string (no trailing semicolon).
        string exprForNode(AST_NODE *p) {
            if (!p) return string("/*null*/");

            switch (p->TYPE) {
                case NODE_STRING:
                    return cpp_literal(*(p->value));
                case NODE_INT:
                    return *(p->value);
                case NODE_VARIABLE:
                    // use variable name directly — assumes the generated C++ defines this name earlier
                    return *(p->value);
                case NODE_BINARY_OP: {
                    string lhs = exprForNode(p->SUB_STATEMENTS[0]);
                    string rhs = exprForNode(p->SUB_STATEMENTS[1]);
                    string op = *(p->value);
                    return "(" + lhs + " " + op + " " + rhs + ")";
                }
                default:
                    // fallback: try to use value if present
                    if (p->value) return *(p->value);
                    return "/*expr_err*/";
            }
        }

        string MakeElement(AST_NODE *p, string parent = "root", string el = "div",string eltype = "view") {
            string varid = eltype+"_" + to_string(idcount);
            stringstream ss;
            if (el != "p")
            {
                ss << "\n\tVNode "+varid+"(\""+ el +"\");\n";
            }
            
            AST_NODE *args = p->CHILD;
            AST_NODE *styleParam = nullptr;
            AST_NODE *firstparam = nullptr;
            AST_NODE *clsparam = nullptr;
            AST_NODE *onclkParam = nullptr;

            if (args && !args->SUB_STATEMENTS.empty()) {
                firstparam = args->SUB_STATEMENTS[0];
                for (size_t i = 1; i < args->SUB_STATEMENTS.size(); ++i) {
                    AST_NODE *param = args->SUB_STATEMENTS[i];
                    if (param && param->TYPE == NODE_VARIABLE && param->value && *(param->value) == "style") {
                        styleParam = param; // styleParam->CHILD -> NODE_DICT
                    }
                    if (param && param->TYPE == NODE_VARIABLE && param->value && *(param->value) == "onclick") {
                        onclkParam = param; // styleParam->CHILD -> NODE_DICT
                    }
                    if (param && param->TYPE == NODE_VARIABLE && param->value && *(param->value) == "cls") {
                        clsparam = param; // styleParam->CHILD -> NODE_DICT
                    }
                    // if (param && param->TYPE == NODE_VARIABLE && param->value && *(param->value) == "id") {
                    //     cout << "stylessss "  << endl;
                    //     idparam = param; // styleParam->CHILD -> NODE_DICT
                    // }
                }
            }
            if (firstparam)
            {
                if (el == "img")
                {
                    if (firstparam->TYPE == NODE_STRING) {
                            ss << "\t" << varid << ".attrs[\"src\"] = \"" << *(firstparam->value) << "\";\n"; 
                        } else if (firstparam->TYPE == NODE_VARIABLE) {
                            ss << "\t" << varid << ".attrs[\"src\"] = \"" << *(firstparam->value) << ";\n"; 
                        } else {
                            ss << "\t" << varid << ".attrs[\"src\"] = \"(" << exprForNode(firstparam) << ")\";\n";
                    }
                }
                if (el == "p")
                {
                    if (firstparam->TYPE == NODE_STRING) {
                            ss << "\n\tVNode "+varid+"(\""+ el + "\",\"" + *(firstparam->value) +"\");\n";
                        } else if (firstparam->TYPE == NODE_VARIABLE) {
                            ss << "\n\tVNode "+varid+"(\""+ el + "\"," + *(firstparam->value) +");\n"; 
                        } else {
                            cout << "gor error P text is not a string or variable";
                            exit(1);
                    }
                }
                
                
            }
            

            if (clsparam) {
                //page.bodyAttrs["class"] = "main-body";
                if (clsparam->CHILD->TYPE == NODE_STRING) {
                    ss << "\t" << varid << ".attrs[\"class\"] = \"" << *(clsparam->CHILD->value) << "\";\n";
                } else if (clsparam->CHILD->TYPE == NODE_VARIABLE) {
                    ss << "\t" << varid << ".attrs[\"class\"] = " << *(clsparam->CHILD->value) << ";\n";
                } else {
                    ss << "\t" << varid << ".attrs[\"class\"] =\"(" << exprForNode(clsparam->CHILD) << ")\";\n";
                }
            }

            if (styleParam && styleParam->CHILD && styleParam->CHILD->TYPE == NODE_DICT) {
                AST_NODE *dict = styleParam->CHILD;
                ss << "\t" << varid << ".attrs[\"style\"] = \"";
                for (auto &kv : dict->SUB_STATEMENTS) {
                    AST_NODE *keyNode = kv->SUB_STATEMENTS[0];
                    AST_NODE *valNode = kv->SUB_STATEMENTS[1];
                    string key = (keyNode->TYPE == NODE_STRING) ? *(keyNode->value) : *(keyNode->value);

                    // Build page.bodyAttrs["style"] = "background-color:#f0f0f0; font-family:Arial,sans-serif; margin:20px;";
                    if (valNode->TYPE == NODE_STRING) {
                        ss << key << ":" << *(valNode->value) << ";";
                    }
                    if (valNode->TYPE == NODE_VARIABLE) {
                        ss << key << ":" << "\"+" << *(valNode->value) << "+\";\"+\"";
                    }
                }
                ss << "\";\n";
            }

            for (auto &child : p->SUB_STATEMENTS) {
                ss << HandleAst(child, varid);
            }
            ss << "\n\t" << parent << ".children.push_back(" << varid << ");";

            idcount++;
            return ss.str();
        }

        string HandleAst(AST_NODE *p, string parent = "root") { 
            if (!p) return "";
             switch (p->TYPE) {
                case NODE_VARIABLE: {
                    // variable assignment or bare reference
                    if (!p->CHILD) {
                        // bare variable reference (as statement? unlikely). Return empty.
                        return "";
                    } else {
                        // declaration: infer type from child
                        NODE_TYPE dtype = p->CHILD->TYPE;
                        if (dtype == NODE_DICT) {
                            // create unordered_map and insert key-values
                            string varName = *(p->value);
                            stringstream ss;
                            ss << "    unordered_map<string,string> " << varName << ";\n";
                            for (auto &kv : p->CHILD->SUB_STATEMENTS) {
                                // kv is NODE_KEYVALUE: substatements[0]=key, [1]=value
                                AST_NODE *keyN = kv->SUB_STATEMENTS[0];
                                AST_NODE *valN = kv->SUB_STATEMENTS[1];

                                // keys are typically strings or ids; treat as string literal
                                string keyLiteral = (keyN->TYPE == NODE_STRING) ? *(keyN->value) : *(keyN->value);
                                string keyCpp = cpp_literal(keyLiteral);

                                // value: if string -> literal; if variable -> variable name expression; else -> expr
                                string valExpr;
                                if (valN->TYPE == NODE_STRING) {
                                    valExpr = cpp_literal(*(valN->value));
                                } else {
                                    valExpr = exprForNode(valN);
                                }

                                ss << "    " << varName << ".insert({ " << keyCpp << ", " << valExpr << " });\n";
                            }
                            return ss.str();
                        } else {
                            // simple assignment: type name = expr;
                            string varName = *(p->value);
                            string expr = exprForNode(p->CHILD);
                            string ctype = pyxtocpp_type(p->CHILD->TYPE);
                            if (ctype == "ERROR") ctype = "auto";
                            return "    " + ctype + " " + varName + " = " + expr + ";";
                        }
                    }
                }

                case NODE_PRINT: {
                    string expr = exprForNode(p->CHILD);
                    return "    cout << " + expr + " << endl;";
                }
                case NODE_IF: {
                    bool haselsif = false;
                    stringstream ss;
                    ss << "    if (";
                    ss << *(p->CHILD->SUB_STATEMENTS[0]->value);
                    ss << *(p->CHILD->value);
                    ss << *(p->CHILD->SUB_STATEMENTS[1]->value);
                    ss << "    ){";
                    for (auto i : p->SUB_STATEMENTS)
                    {
                        ss << "\n    ";
                        if (i->TYPE == NODE_ELSE || i->TYPE == NODE_ELSE_IF)
                        {
                            haselsif = true;
                            ss << "\n    }"; 
                        }
                        
                        ss << HandleAst(i);
                    }
                    if (!haselsif)
                    {
                        ss << "}";
                    }
                    return ss.str();
                }
                case NODE_ELSE: {
                    stringstream ss;
                    ss << "\n    else {";
                    for (auto i : p->SUB_STATEMENTS) {
                        ss << HandleAst(i);
                    }
                    ss << "\n    }";
                    return ss.str();
                }
                case NODE_ELSE_IF: {
                    stringstream ss;
                    ss << "\n     else if (";
                    ss << *(p->CHILD->SUB_STATEMENTS[0]->value);
                    ss << *(p->CHILD->value);
                    ss << *(p->CHILD->SUB_STATEMENTS[1]->value);
                    ss << ") {";
                    for (auto i : p->SUB_STATEMENTS)
                    {
                        ss << "\n    " << HandleAst(i);
                    }
                    
                    return ss.str();
                }case NODE_app: {
                    stringstream ss;
                    if (!p->SUB_STATEMENTS.empty()) {
                        for (auto &i : p->SUB_STATEMENTS) {
                            ss << HandleAst(i);
                        }
                    }
                    return ss.str();
                } case NODE_page: {
                    bool firstpage;
                    if (idcount == 1) {
                        firstpage = true;
                    }
                    string varid = "page_" + to_string(idcount);
                    stringstream ss;
                     ss << "\n\tVPage "+varid+";\n";
                    AST_NODE *args = p->CHILD;
                    AST_NODE *styleParam = nullptr;
                    AST_NODE *idparam = nullptr;
                    AST_NODE *clsparam = nullptr;
                    AST_NODE *routeParam = nullptr;
                    AST_NODE *titleArg = nullptr;
                    if (args && !args->SUB_STATEMENTS.empty()) {
                        titleArg = args->SUB_STATEMENTS[0];
                        for (size_t i = 1; i < args->SUB_STATEMENTS.size(); ++i) {
                            AST_NODE *param = args->SUB_STATEMENTS[i];
                            if (param && param->TYPE == NODE_VARIABLE && param->value && *(param->value) == "style") {
                                cout << "stylessss "  << endl;
                                styleParam = param; // styleParam->CHILD -> NODE_DICT
                            }
                            if (param && param->TYPE == NODE_VARIABLE && param->value && *(param->value) == "route") {
                                cout << "stylessss "  << endl;
                                routeParam = param; // styleParam->CHILD -> NODE_DICT
                            }
                            if (param && param->TYPE == NODE_VARIABLE && param->value && *(param->value) == "cls") {
                                cout << "stylessss "  << endl;
                                clsparam = param; // styleParam->CHILD -> NODE_DICT
                            }
                            if (param && param->TYPE == NODE_VARIABLE && param->value && *(param->value) == "id") {
                                cout << "stylessss "  << endl;
                                idparam = param; // styleParam->CHILD -> NODE_DICT
                            }
                        }
                    }

                    if (titleArg) {
                        
                        if (titleArg->TYPE == NODE_STRING) {
                            ss << "\t" << varid << ".title = \"" << *(titleArg->value) << "\";\n"; 
                        } else if (titleArg->TYPE == NODE_VARIABLE) {
                            ss << "\t" << varid << ".title = " << *(titleArg->value) << ";\n"; 
                        } else {
                            ss << "\t" << varid << ".title =\"(" << exprForNode(titleArg) << ")\";\n";
                        }
                    } else {
                        ss << "\t" << varid << ".title = \"Create Pyra App\"\n";
                    }

                    if (clsparam) {
                        //page.bodyAttrs["class"] = "main-body";
                        if (clsparam->CHILD->TYPE == NODE_STRING) {
                            ss << "\t" << varid << ".bodyAttrs[\"class\"] = \"" << *(clsparam->CHILD->value) << "\";\n";
                        } else if (clsparam->CHILD->TYPE == NODE_VARIABLE) {
                            ss << "\t" << varid << ".bodyAttrs[\"class\"] = " << *(clsparam->CHILD->value) << ";\n";
                        } else {
                            ss << "\t" << varid << ".bodyAttrs[\"class\"] =\"(" << exprForNode(clsparam->CHILD) << ")\";\n";
                        }
                    }

                    if (styleParam && styleParam->CHILD && styleParam->CHILD->TYPE == NODE_DICT) {
                        AST_NODE *dict = styleParam->CHILD;
                        ss << "\t" << varid << ".bodyAttrs[\"style\"] = \"";
                        for (auto &kv : dict->SUB_STATEMENTS) {
                            AST_NODE *keyNode = kv->SUB_STATEMENTS[0];
                            AST_NODE *valNode = kv->SUB_STATEMENTS[1];
                            string key = (keyNode->TYPE == NODE_STRING) ? *(keyNode->value) : *(keyNode->value);

                            // Build page.bodyAttrs["style"] = "background-color:#f0f0f0; font-family:Arial,sans-serif; margin:20px;";
                            if (valNode->TYPE == NODE_STRING) {
                                ss << key << ":" << *(valNode->value) << ";";
                            }
                            if (valNode->TYPE == NODE_VARIABLE) {
                                ss << key << ":" << "\"+" << *(valNode->value) << "+\"";
                            }
                        }
                        ss << "\";\n";
                    }

                    for (auto &child : p->SUB_STATEMENTS) {
                        ss << HandleAst(child, varid);
                    }
                    idcount++;

                    if (firstpage) {
                        ss << "\n\n\trenderPage(" << varid << ");";
                    }
                    return ss.str();
                } case NODE_VIEW: {
                    return MakeElement(p, parent, "div", "view");
                } case NODE_TEXT: {
                    return MakeElement(p, parent, "p", "text");
                } case NODE_IMAGE: {
                    return MakeElement(p, parent, "img", "img");
                }
                 default:
                    return "";
            }
        }
        // Write file to disk
        bool makefile(const string &directoryPath, const string &filebuffer) {
            try {
                filesystem::path p(directoryPath);
                if (!p.parent_path().empty() && !filesystem::exists(p.parent_path())) {
                    filesystem::create_directories(p.parent_path());
                }
                ofstream MyFile(directoryPath);
                MyFile << filebuffer;
                MyFile.close();
                return true;
            } catch (const std::exception &e) {
                cerr << "makefile error: " << e.what() << endl;
                return false;
            }
        }
};