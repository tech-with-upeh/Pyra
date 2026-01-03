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
        stringstream filebuffer;
        stringstream mainbuffer;

        WebEngine() : idcount(1) , pagecount(1) {}

        string pyxtocpp_type(enum NODE_TYPE type, AST_NODE* node) {
            switch (type)
            {
            case NODE_BINARY_OP: {
                string lhs = pyxtocpp_type(node->SUB_STATEMENTS[0]->TYPE, node->SUB_STATEMENTS[0]); 
                return lhs;
            }
            case NODE_BOOL:
                return "bool";
            case NODE_INT:
                return "int";
            case NODE_STRING:
                return "string";
            case NODE_DICT:
                return "unordered_map<string,string>";
            case NODE_TOFLOAT:
                return "float";
            case NODE_TOINT:
                return "int";
            case NODE_TOSTR:
                return "string";
            default:
                cerr << "Error unknown Type\n";
                break;
            }
            return "ERROR";
        }

        bool gen(AST_NODE *root) {
            filebuffer << "#include <iostream>\n";
            filebuffer << "#include \"vdom.hpp\"\n";
            filebuffer << "#include <format>\n";
            filebuffer << "using namespace std;\n\n";

            filebuffer << R"(void updateUI() {
    // Re-render the current page
    if (GlobalState::getCurrentPage()) {
        GlobalState::getCurrentPage()->render();
    }
})";

            stringstream codebuffer;

            codebuffer << "\n";

            // Top-level: convert each root sub-statement into C++ statements
            for (auto &stmt : root->SUB_STATEMENTS) {
                string out = HandleAst(stmt, "root", true, false);
                if (!out.empty()) {
                    codebuffer << out << "\n";
                }
            }
            filebuffer << "int main() {" << mainbuffer.str() << codebuffer.str();
            filebuffer << "\tEM_ASM({\n\t\tModule._handleRoute(allocateUTF8(window.location.pathname));\n\t\twindow.addEventListener(\"popstate\", () => {\n\t\tModule._handleRoute(allocateUTF8(window.location.pathname));\n\t\t});\n\t});return 0;\n}\n";

           


            for (const auto &pair : variable_buffer) {
                cout << pair.first << " <=> " << pair.second << endl;
            }
            return makefile("web/generated.cpp", filebuffer.str());
        }
    private:
        int idcount;
        int pagecount;
        vector<string> statevars;
        unordered_map<string, bool> variable_buffer;
        

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
        
        string exprForNode(AST_NODE *p) {
            if (!p) return string("/*null*/");

            switch (p->TYPE) {
                case NODE_STRING:
                    return "string(" + cpp_literal(*(p->value)) + ")";
                case NODE_FLOAT:
                case NODE_INT:
                    return *(p->value);
                case NODE_VARIABLE:
                    // use variable name directly — assumes the generated C++ defines this name earlier
                    return HandleAst(p, "root", true);
                case NODE_BOOL:
                case NODE_BINARY_OP: {
                    string lhs = exprForNode(p->SUB_STATEMENTS[0]);
                    string rhs = exprForNode(p->SUB_STATEMENTS[1]);
                    string op = *(p->value);
                    return "(" + lhs + " " + op + " " + rhs + ")";
                }

                /*
                    def jd() {
                        return 5 + 3;
                    }

                */
                case NODE_UNARY_OP: {
                    string operand = exprForNode(p->SUB_STATEMENTS[0]);
                    string op = *(p->value);
                    if (op == "++")
                    {
                        op = "+";
                    }
                    else
                    {
                        op = "-";
                    }
                    return "(" + operand + " " + op + " " + operand + ")";
                }
                default:
                    return MakeConversion(p, p->TYPE, false);
            }
        }

        string MakePage(AST_NODE *p, string var, bool firstpage=false) {
                    statevars.clear();
                    string varid = var;
                    stringstream ss;
                    filebuffer << "\nauto "+varid+ " = make_shared<VPage>();\n";
                    ss << "\t"+varid+"->builder = [&";
                    vector<string> stylesheetimports;
                    if (!variable_buffer.empty()) {
                        for (const auto &pair : variable_buffer) {
                            if (pair.second == true) {
                                stylesheetimports.push_back(pair.first);
                            }
                            ss << ", " << pair.first; 
                        }
                    }
                    ss << "](VPage& page) {\n";

                    for (const auto &imps : stylesheetimports) {
                        ss << "\t\tpage.addStyle(" << imps << ");\n";
                    }
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
                                
                                styleParam = param; // styleParam->CHILD -> NODE_DICT
                            }
                            if (param && param->TYPE == NODE_VARIABLE && param->value && *(param->value) == "route") {
            
                                routeParam = param; // styleParam->CHILD -> NODE_DICT
                            }
                            if (param && param->TYPE == NODE_VARIABLE && param->value && *(param->value) == "cls") {
                           
                                clsparam = param; // styleParam->CHILD -> NODE_DICT
                            }
                            if (param && param->TYPE == NODE_VARIABLE && param->value && *(param->value) == "id") {
                                idparam = param; // styleParam->CHILD -> NODE_DICT
                            }
                        }
                    }

                    if (routeParam) {
                        // Router::add("/", page_1);
                        mainbuffer << "\n\tRouter::add(\""+*(routeParam->CHILD->value) + "\","+ varid +");";
                    } else {
                        mainbuffer << "\n\tRouter::add(\"/\","+ varid +");";
                    }
                    if (titleArg) {
                        
                        if (titleArg->TYPE == NODE_STRING) {
                            ss << "\t\tpage.setTitle(\"" << *(titleArg->value) << "\");\n"; 
                        } else if (titleArg->TYPE == NODE_VARIABLE) {
                            ss << "\t\tpage.setTitle(" << *(titleArg->value) << ");\n"; 
                        } else {
                            ss << "\t\tpage.setTitle(\"(" << exprForNode(titleArg) << ")\");\n";
                        }
                    } else {
                        ss << "\t\tpage.setTitle(\"Create Pyra App\");\n";
                    }

                    if (clsparam) {
                        //page.bodyAttrs["class"] = "main-body";
                        if (clsparam->CHILD->TYPE == NODE_STRING) {
                            ss << "\t\tpage.bodyAttrs[\"class\"] = \"" << *(clsparam->CHILD->value) << "\";\n";
                        } else if (clsparam->CHILD->TYPE == NODE_VARIABLE) {
                            ss << "\t\tpage.bodyAttrs[\"class\"] = " << *(clsparam->CHILD->value) << ";\n";
                        } else {
                            ss << "\t\tpage.bodyAttrs[\"class\"] =\"(" << exprForNode(clsparam->CHILD) << ")\";\n";
                        }
                    }

                    if (styleParam && styleParam->CHILD && styleParam->CHILD->TYPE == NODE_DICT) {
                        AST_NODE *dict = styleParam->CHILD;
                        ss << "\t\tpage.bodyAttrs[\"style\"] = \"";
                        for (auto &kv : dict->SUB_STATEMENTS) {
                            AST_NODE *keyNode = kv->SUB_STATEMENTS[0];
                            AST_NODE *valNode = kv->SUB_STATEMENTS[1];
                            string key = (keyNode->TYPE == NODE_STRING) ? *(keyNode->value) : *(keyNode->value);

                            // Build page.bodyAttrs["style"] = "background-color:#f0f0f0; font-family:Arial,sans-serif; margin:20px;";
                            if (valNode->TYPE == NODE_STRING) {
                                ss << key << ":" << *(valNode->value) << ";";
                            }
                             if (valNode->TYPE == NODE_VARIABLE) {
                                if (find(statevars.begin(), statevars.end(), *(valNode->value)) != statevars.end())
                                    {
                                        ss << key << ":" << "\"+" <<  *(valNode->value) << "->get()" << "+\";\"+\"";
                                    } else {
                                        ss << key << ":" << "\"+" << *(valNode->value) << "+\";\"+\"";
                                    }
                            }
                        }
                        ss << "\";\n";
                    }

                    for (auto &child : p->SUB_STATEMENTS) {
                        ss << "\n\t\t" << HandleAst(child, "page", true);
                    }
                    ss << "\n\t\t};";
                    
                    pagecount++;
                    return ss.str();
                }

        string MakeElement(AST_NODE *p, string parent = "root", string el = "div",string eltype = "view", bool isvar=false) {
            
            
            string varid = eltype+"_" + to_string(idcount);
            if (isvar)
            {
                varid = eltype;
            }
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
            AST_NODE *idparam = nullptr;
            AST_NODE *heightparam = nullptr;
            AST_NODE *widthparam = nullptr;

            if (args && !args->SUB_STATEMENTS.empty()) {
                firstparam = args->SUB_STATEMENTS[0];
                for (size_t i = 1; i < args->SUB_STATEMENTS.size(); ++i) {
                    AST_NODE *param = args->SUB_STATEMENTS[i];
                    if (param && param->TYPE == NODE_VARIABLE && param->value && *(param->value) == "style") {
                        styleParam = param;
                    }
                    if (param && param->TYPE == NODE_VARIABLE && param->value && *(param->value) == "onclick") {
                        onclkParam = param;
                    }
                    if (param && param->TYPE == NODE_VARIABLE && param->value && *(param->value) == "cls") {
                        clsparam = param;
                    }
                    if (param && param->TYPE == NODE_VARIABLE && param->value && *(param->value) == "id")
                    {
                        idparam = param; 
                    }
                    if (param && param->TYPE == NODE_VARIABLE && param->value && *(param->value) == "height") {
                        heightparam = param;
                    }
                    if (param && param->TYPE == NODE_VARIABLE && param->value && *(param->value) == "width") {
                        widthparam = param;
                    }

                }
            }
            if (firstparam)
            {
                if (el == "img")
                {
                    if (firstparam->TYPE == NODE_STRING) {
                            ss << "\t" << varid << ".setAttr(\"src\", \"" << *(firstparam->value) << "\");\n"; 
                        } else if (firstparam->TYPE == NODE_VARIABLE) {
                            ss << "\t" << varid << ".setAttr(\"src\"," << HandleAst(firstparam->CHILD, parent)  << ");\n"; 
                        } else {
                            ss << "\t" << varid << ".setAttr(\"src\", \"" << exprForNode(firstparam) << "\");\n";
                    }
                }
                if (el == "p")
                {
                    if (firstparam->TYPE == NODE_STRING) {
                            ss << "\n\t\tVNode "+varid+"(\""+ el + "\",\"" + *(firstparam->value) +"\");\n";
                        } else if (firstparam->TYPE == NODE_VARIABLE) {
                            ss << "\n\t\tVNode "+varid+"(\""+ el + "\"," + HandleAst(firstparam, parent) +");\n"; 
                        } else {
                            cout << "gor error P text is not a string or variable";
                            exit(1);
                    }
                }
                if (el == "div")
                {
                    if (firstparam->TYPE == NODE_STRING) {
                            ss << "\t\t" << varid << ".setAttr(\"id\", \"" << *(firstparam->value) << "\");\n"; 
                        } else if (firstparam->TYPE == NODE_VARIABLE) {
                            ss << "\t\t" << varid << ".setAttr(\"id\"," << *(firstparam->value) << ");\n"; 
                        } else {
                            ss << "\t\t" << varid << ".setAttr(\"id\", \"" << exprForNode(firstparam) << "\");\n";
                    }
                }
                if (el == "canvas")
                {
                    // canvas.type = VNodeType::CANVAS;
                    ss  << "\t\t" << varid << ".type = VNodeType::CANVAS;\n";
                    if (firstparam->TYPE == NODE_STRING) {
                            ss << "\t\t" << varid << ".setAttr(\"id\", \"" << *(firstparam->value) << "\");\n"; 
                        } else if (firstparam->TYPE == NODE_VARIABLE) {
                            ss << "\t\t" << varid << ".setAttr(\"id\"," << *(firstparam->value) << ");\n"; 
                        } else {
                            ss << "\t\t" << varid << ".setAttr(\"id\", \"" << exprForNode(firstparam) << "\");\n";
                    }
                }
                
                
            }
            
            if (idparam)
            {
                if (idparam->CHILD->TYPE == NODE_STRING) {
                            ss << "\t\t" << varid << ".setAttr(\"id\", \"" << *(idparam->CHILD->value) << "\");\n"; 
                        } else if (idparam->CHILD->TYPE == NODE_VARIABLE) {
                            ss << "\t\t" << varid << ".setAttr(\"id\"," << *(idparam->CHILD->value) << ");\n"; 
                        } else {
                            ss << "\t\t" << varid << ".setAttr(\"id\", \"" << exprForNode(idparam->CHILD) << "\");\n";
                    }
            }

            if (heightparam)
            {
                if (heightparam->CHILD->TYPE == NODE_INT) {
                            ss << "\t\t" << varid << ".height = " << *(heightparam->CHILD->value) << ";\n"; 
                } else if (heightparam->CHILD->TYPE == NODE_VARIABLE) {
                    ss << "\t\t" << varid << ".height = " << *(heightparam->CHILD->value) << ";\n"; 
                } else {
                    ss << "\t\t" << varid << ".height = " << exprForNode(heightparam->CHILD) << ";\n";
                }
            }

            if (widthparam)
            {
                if (widthparam->CHILD->TYPE == NODE_INT) {
                            ss << "\t\t" << varid << ".width = " << *(widthparam->CHILD->value) << ";\n"; 
                } else if (widthparam->CHILD->TYPE == NODE_VARIABLE) {
                    ss << "\t\t" << varid << ".width = " << *(widthparam->CHILD->value) << ";\n"; 
                } else {
                    ss << "\t\t" << varid << ".width = " << exprForNode(widthparam->CHILD) << ";\n";
                }
            }

            if (clsparam) {
                //page.bodyAttrs["class"] = "main-body";
                if (clsparam->CHILD->TYPE == NODE_STRING) {
                    ss << "\t" << varid << ".setAttr(\"class\",  \"" << *(clsparam->CHILD->value) << "\");\n";
                } else if (clsparam->CHILD->TYPE == NODE_VARIABLE) {
                    ss << "\t" << varid << ".setAttr(\"class\",  " << *(clsparam->CHILD->value) << ");\n";
                } else {
                    ss << "\t" << varid << ".setAttr(\"class\", \"(" << exprForNode(clsparam->CHILD) << "\");\n";
                }
            }

            if (onclkParam)
            {
                AST_NODE *chld = onclkParam->CHILD->CHILD;

                ss << "\t" << varid << ".onClick([";
                if (chld) {
                    for (auto &i : chld->SUB_STATEMENTS)
                {
                    ss << *(i->value);
                    if (&i != &chld->SUB_STATEMENTS.back())
                    {
                        ss << ", ";
                    }
                }
                }
                
                
                ss << "]() {\n";
                for (auto &i : onclkParam->CHILD->SUB_STATEMENTS)
                {
                    ss << "\t\t" << HandleAst(i, parent, true) << "\n";   
                }
                ss << "\t\tupdateUI();\t});\n";
            }
            

            if (styleParam && styleParam->CHILD && styleParam->CHILD->TYPE == NODE_DICT) {
                AST_NODE *dict = styleParam->CHILD;
                ss << "\t" << varid << ".setAttr(\"style\", \"";
                for (auto &kv : dict->SUB_STATEMENTS) {
                    AST_NODE *keyNode = kv->SUB_STATEMENTS[0];
                    AST_NODE *valNode = kv->SUB_STATEMENTS[1];
                    string key = (keyNode->TYPE == NODE_STRING) ? *(keyNode->value) : *(keyNode->value);

                    // Build page.bodyAttrs["style"] = "background-color:#f0f0f0; font-family:Arial,sans-serif; margin:20px;";
                    if (valNode->TYPE == NODE_STRING) {
                        ss << key << ":" << *(valNode->value) << ";";
                    }
                    if (valNode->TYPE == NODE_VARIABLE) {
                        if (find(statevars.begin(), statevars.end(), *(valNode->value)) != statevars.end())
                            {
                                ss << key << ":" << "\"+" <<  *(valNode->value) << "->get()" << "+\";\"+\"";
                            } else {
                                ss << key << ":" << "\"+" << *(valNode->value) << "+\";\"+\"";
                            }
                    }
                }
                ss << "\");\n";
            }

            for (auto &child : p->SUB_STATEMENTS) {
                ss << HandleAst(child, varid);
            }

            if (!isvar)
            {
                ss << "\n\t" << parent << ".addChild(" << varid << ");";
            }
            

            idcount++;
            return ss.str();
        }

        string MakeConversion(AST_NODE *p, NODE_TYPE type, bool isroot = false) {
            switch (type) {
                case NODE_TOSTR: {
                    stringstream ss;
                    ss << "to_string(";
                    ss << exprForNode(p->CHILD);
                    ss << ")";
                    if(isroot) {
                        ss << ";";
                    }
                    return ss.str();
                }
                case NODE_TOINT: {
                    stringstream ss;
                    ss << "stoi(";
                    ss << exprForNode(p->CHILD);
                    ss << ")";
                    if(isroot) {
                        ss << ";";
                    }
                    return ss.str();
                }
                case NODE_TOFLOAT: {
                    stringstream ss;
                    ss << "stof(";
                    ss << exprForNode(p->CHILD);
                    ss << ")";
                    if(isroot) {
                        ss << ";";
                    }
                    return ss.str();
                }
                default:
                    return "";
            }
            return "";
        }

        string MakeDraw(AST_NODE *p,bool isroot=true, string parent="root") {
            stringstream ss;
            if (isroot) {
                // Canvas2D ctx("id");
                ss << "Canvas2D ";
                ss << parent << *(p->CHILD->SUB_STATEMENTS[0]->value) << idcount;
                ss << "(";
                ss << HandleAst(p->CHILD->SUB_STATEMENTS[0]);
                ss << ");";
            } else {
                ss << "(";
                ss << HandleAst(p->CHILD->SUB_STATEMENTS[0]);
                ss << ");";  
            }
            return ss.str();
        }
        string HandleAst(AST_NODE *p, string parent = "root", bool funcdecl=false, bool fromui = true) { 
            if (!p) return "";
             switch (p->TYPE) {
                case NODE_VARIABLE: {
                    // variable assignment or bare reference
                    if (!p->CHILD) {
                        // bare variable reference (as statement? unlikely). Return empty.
                        
                            stringstream ss;
                            if (find(statevars.begin(), statevars.end(), *(p->value)) != statevars.end())
                            {
                                ss << *(p->value) << "->get()";
                                return ss.str();
                            } else {
                                if (funcdecl)
                                {
                                    return *(p->value);
                                }
                                else
                                {
                                     return "\n\t"+parent + ".addChild("+ *(p->value) +");\n";
                                }
                            }
                    } else {
                        // declaration: infer type from child
                        NODE_TYPE dtype = p->CHILD->TYPE;
                        if (find(statevars.begin(), statevars.end(), *(p->value)) != statevars.end())
                        {
                            // state variable assignment
                            string varName = *(p->value);
                            string expr = exprForNode(p->CHILD);
                            return "\t" + varName + "->set(" + expr + ");";
                        }
                        else
                        {
                             string varbufName = *(p->value);
                            
                            if (!fromui) {
                                variable_buffer[varbufName] = false;
                            }
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
                            } else if (dtype == NODE_page) {
                                string varName = *(p->value);
                                return MakePage(p->CHILD, varName);
                            } else if (dtype == NODE_VIEW || dtype == NODE_TEXT || dtype == NODE_IMAGE || dtype == NODE_INPUT) {
                                string el = "div";
                                string eltype = *(p->value);
                                switch (dtype)
                                {
                                case NODE_VIEW:
                                    el = "div";
                                    
                                    break;
                                case NODE_TEXT:
                                    el = "p";
                                    
                                    break;
                                case NODE_IMAGE:
                                    el = "img";
                                    
                                    break;
                                case NODE_INPUT:
                                    el = "input";
                                
                                    break;
                                default:
                                    break;
                                }
                                return MakeElement(p->CHILD, "root", el, eltype, true);
                            } else if (dtype == NODE_TOINT || dtype == NODE_TOFLOAT || dtype == NODE_TOSTR) {
                                return MakeConversion(p, dtype, false);
                            } else if (dtype == NODE_DRAW) {
                                stringstream ss;
                                ss << "Canvas2D " << *(p->value) << MakeDraw(p->CHILD, false, parent);
                                return ss.str();
                            }
                            else {
                                // simple assignment: type name = expr;
                                string varName = *(p->value);
                                string expr = exprForNode(p->CHILD);
                                string ctype = pyxtocpp_type(p->CHILD->TYPE, p->CHILD);
                                if (ctype == "ERROR") ctype = "auto";
                                return "    " + ctype + " " + varName + " = " + expr + ";";
                            }
                        }
                                 
                    }
                }
                case NODE_STRING: {
                    return exprForNode(p);
                }
                case NODE_PRINT: {
                    if (p->CHILD->TYPE == NODE_VARIABLE)
                    {
                        stringstream ss;
                        ss << "cout << " << HandleAst(p->CHILD, parent, true) << " << endl;";
                        return  ss.str();
                    }
                    else
                    {
                        
                       string expr = exprForNode(p->CHILD);
                    return "\tcout << " + expr + " << endl;";
                    }
                    
                    
                }
                case NODE_GO: {
                    stringstream ss;
                    ss << "Router::go(\"";
                    for (auto i : p->SUB_STATEMENTS) {
                        ss << *(i->value);
                    }
                    ss << "\");";
                    return ss.str();
                }
                case NODE_IF: {
                    bool haselsif = false;
                    stringstream ss;
                    ss << "\tif";
                    ss << exprForNode(p->CHILD);
                    ss << "{";
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
                    ss << "\n\telse if";
                    ss << exprForNode(p->CHILD);
                    ss << "{";
                    for (auto i : p->SUB_STATEMENTS)
                    {
                        ss << "\n\t" << HandleAst(i);
                    }
                    
                    return ss.str();
                } case NODE_page: {
                    
                    return MakePage(p, "page_"+to_string(pagecount));
                } case NODE_VIEW: {
                    return MakeElement(p, parent, "div", "view");
                } case NODE_TEXT: {
                    return MakeElement(p, parent, "p", "text");
                } case NODE_IMAGE: {
                    return MakeElement(p, parent, "img", "img");
                } case NODE_SETSTATE: {
                    string varname = *(p->value);
                    string ctype = pyxtocpp_type(p->CHILD->TYPE, p->CHILD);
                    // auto counter = make_shared<appstate::State<auto>>("counter", 0);
                    string makeendvar;
                    if (ctype == "string") {
                        makeendvar = "\""+  *(p->CHILD->value) +"\"";
                    } else {
                        makeendvar =  *(p->CHILD->value);
                    }
                    stringstream ss;
                    string endvar = ">>(\""+ varname +"\"," + makeendvar + ");";
                    ss << "\n\tauto " << varname << " = make_shared<appstate::State<" << ctype << endvar;
                    statevars.push_back(varname);
                    return ss.str();
                }
                case NODE_STYLESHEET: {
                    stringstream ss;
                    ss << "\tstring " << *(p->value) << " = R\"(\n\t";
                    bool isuniversal = false;
                    if(p->CHILD) {
                        isuniversal = true;
                    }
                    variable_buffer[*(p->value)] = isuniversal;
                    for (auto &subs : p->SUB_STATEMENTS) {
                        ss << HandleAst(subs, parent, true);
                    }
                    ss << ")\";\n\t";
                    return ss.str();
                }
                case NODE_CLS: {
                    stringstream ss;
                    ss << "\t." << *(p->value) << "{ \n";
                    if (p->CHILD) {
                        for (auto &kv : p->CHILD->SUB_STATEMENTS) {
                            ss << "\t\t\t" << *(kv->SUB_STATEMENTS[0]->value) << " : ";
                            if (kv->SUB_STATEMENTS[1]->TYPE == NODE_VARIABLE) {
                                ss << ")\" +" <<  HandleAst(kv->SUB_STATEMENTS[1], parent, true)  << "+ R\"(";
                            } else if (kv->SUB_STATEMENTS[1]->TYPE == NODE_STRING) {
                                ss << *(kv->SUB_STATEMENTS[1]->value);
                            } else {
                                ss << HandleAst(kv->SUB_STATEMENTS[1], parent, true);
                            }
                            //  
                            ss << ";\n";
                        }
                    }
                    ss << "\t\t}";
                    return ss.str();
                }
                case NODE_MEDIA_QUERY: {
                    // string unis = R"(
                    //     .btn {
                    //         background-color: purple
                    //     }
                    // )";
                    stringstream ss;
                    ss << "\n\t\t@media only screen and (";
                    if(p->CHILD->TYPE == NODE_STRING) {
                        ss << *(p->CHILD->value);
                    } else {
                        ss <<  HandleAst(p->CHILD, parent);
                    }
                    ss << ") {"; 
                    for (auto &cls : p->SUB_STATEMENTS) {
                        ss << "\n\t\t\t" << HandleAst(cls, parent, true);
                    }
                    ss << "\n\t\t}\n";
                    return ss.str();
                }
                case NODE_CANVAS: {
                    return MakeElement(p, parent, "canvas", "canvas");  
                }
                case NODE_INPUT: {
                    return MakeElement(p, parent, "input", "input");
                }
                case NODE_FUNCTION_DECL: {
                    if (*(p->value) == "onmount") {
                        stringstream ss;
                        ss << parent << ".onMount([&";
                        if (!statevars.empty()) {
                            for (auto &i : statevars) {
                                ss << ", " << i;
                            }
                        }
                        ss << "]() {\n";
                        for (auto &stmt : p->SUB_STATEMENTS) {
                            ss << "\t\t\t" << HandleAst(stmt, parent) << "\n";
                        }
                        ss << "\t\t});\n";
                        return ss.str();
                    }
                    stringstream ss;
                    stringstream tmpl;
                    tmpl << "";
                    string ftype = "\nvoid";
                    ss << *(p->value) << "(";
                    AST_NODE *args = p->CHILD;
                    if (args && !args->SUB_STATEMENTS.empty()) {
                        tmpl << "\ntemplate <";
                        for (size_t i = 0; i < args->SUB_STATEMENTS.size(); ++i) {
                            
                            AST_NODE *param = args->SUB_STATEMENTS[i];
                            tmpl << "typename " << *(param->value) << to_string(i);

                            ss << *(param->value) << to_string(i) << "&&" << " " << *(param->value);
                            if (i < args->SUB_STATEMENTS.size() - 1) {
                                ss << ", ";
                                tmpl << ", ";
                            }
                        }
                        tmpl << ">\n";
                    }
                    ss << ") {\n";
                    for (auto &stmt : p->SUB_STATEMENTS) {
                        if (stmt->TYPE == NODE_RETURN)
                        {
                            ftype = pyxtocpp_type(stmt->CHILD->TYPE, stmt->CHILD);
                            if (ftype == "ERROR") ftype = "auto";
                            ss << "\treturn ";
                            ss << exprForNode(stmt->CHILD) << ";\n";
                            continue;
                        }
                        ss << HandleAst(stmt, parent);
                    }
                    ss << "\n}\n";

                    filebuffer << tmpl.str() << ftype << " " << ss.str();
                    return "";
                }
                case NODE_FUNCTION_CALL: {
                    stringstream ss;
                    ss << *(p->value) << "(";
                    if (!p->SUB_STATEMENTS.empty()) {
                        for (size_t i = 0; i < p->SUB_STATEMENTS.size(); ++i) {
                            ss << exprForNode(p->SUB_STATEMENTS[i]);
                            if (i < p->SUB_STATEMENTS.size() - 1) {
                                ss << ",";
                            }
                        }
                    }
                    ss << ");";
                    return ss.str();
                }
                case NODE_TOFLOAT: {
                    // static_cast<float>(integerValue);
                    return MakeConversion(p, NODE_TOFLOAT, true);
                }
                case NODE_TOINT: {
                    return MakeConversion(p, NODE_TOINT, true);
                }
                case NODE_TOSTR: {
                    return MakeConversion(p, NODE_TOSTR, true);
                }
                case NODE_DRAW: {
                    return MakeDraw(p, true, parent);
                }
                case NODE_INSTANCE: {
                    stringstream ss;
                    ss << *(p->value) << "." << HandleAst(p->CHILD,parent,funcdecl, fromui);
                    return ss.str();
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