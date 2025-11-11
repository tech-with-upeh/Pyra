// pyxsemantics.cpp
// WebEngine generator — emits emscripten-compatible C++ that uses runtime variable values.

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

    // Generate the final C++ file for emscripten (writes web/generated.cpp)
    bool gen(AST_NODE *root) {
        stringstream filebuffer;
        filebuffer << "#include <iostream>\n";
        filebuffer << "#include <vector>\n";
        filebuffer << "#include <string>\n";
        filebuffer << "#include <unordered_map>\n";
        filebuffer << "#include <sstream>\n";
        filebuffer << "#include \"utils.hpp\"\n";
        filebuffer << "#include <emscripten.h>\n";
        filebuffer << "using namespace std;\n\n";
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
    vector<string> els;

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

    // Build C++ code that constructs a JS string at runtime.
    // `pieces` is a vector of pairs: (isLiteral, content)
    // For literal pieces we embed as C++ literal; for non-literal we append the variable name (no quotes).
    // Returns a C++ statement like:
    //    std::string js = std::string("literal1") + varName + std::string("literal2");
    string build_js_string_code(const vector<pair<bool,string>> &pieces, const string &indent = "    ") {
        // if all pieces are literal, we can join them into a single literal
        bool allLiteral = true;
        for (auto &p : pieces) if (!p.first) { allLiteral = false; break; }

        stringstream ss;
        if (allLiteral) {
            // single literal
            string joined;
            for (auto &p : pieces) joined += p.second;
            ss << indent << "string js_" << to_string(idcount) <<" = " << cpp_literal(joined) << ";\n";
        } else {
            // construct via concatenation: start with first piece as std::string(...)
            bool started = false;
            ss << indent << "string js_" << to_string(idcount) <<"= ";
            for (size_t i = 0; i < pieces.size(); ++i) {
                auto &p = pieces[i];
                if (i > 0) ss << " + ";
                if (p.first) {
                    // literal piece must be wrapped as std::string(...)
                    ss << "string(" << cpp_literal(p.second) << ")";
                } else {
                    // variable piece — we assume it's a C++ variable available in generated program
                    ss << p.second;
                }
            }
            ss << ";\n";
        }
        return ss.str();
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

    // Main AST handler — returns full C++ statements (with semicolons and indentation)
    // UI nodes produce code that builds JS string and calls emscripten_run_script(js.c_str());
    string HandleAst(AST_NODE *p, const string &parentId = "document.body", const string &parentvar = "") {
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
                    
                    ss << HandleAst(i, parentId, parentvar);
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
                    ss << HandleAst(i, parentId, parentvar);
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
                    ss << "\n    " << HandleAst(i, parentId, parentvar);
                }
                
                return ss.str();
            }

            case NODE_page: {
                // Build JS for page: set title, optionally styles, then child UI nodes.
                // But title might be NODE_STRING or NODE_VARIABLE or expression.
                stringstream stmt;
                vector<pair<bool,string>> jsPieces;

                // We'll accumulate JS into parts and then call build_js_string_code to
                // produce the C++ code that constructs the JS at runtime.
                // Start with title part
                AST_NODE *args = p->CHILD;
                AST_NODE *styleParam = nullptr;
                AST_NODE *titleArg = nullptr;
                if (args && !args->SUB_STATEMENTS.empty()) {
                    titleArg = args->SUB_STATEMENTS[0];
                    // look for style=... among later args
                    for (size_t i = 1; i < args->SUB_STATEMENTS.size(); ++i) {
                        AST_NODE *param = args->SUB_STATEMENTS[i];
                        if (param && param->TYPE == NODE_VARIABLE && param->value && *(param->value) == "style") {
                            cout << "stylessss "  << endl;
                            styleParam = param; // styleParam->CHILD -> NODE_DICT
                        }
                    }
                }

                // Build js pieces for "document.title = " + (title) + ";\n"
                jsPieces.clear();
                jsPieces.push_back({true, "document.title = "});
                if (titleArg) {
                    if (titleArg->TYPE == NODE_STRING) {
                        jsPieces.push_back({true, "\"" + *(titleArg->value)+ "\""});
                    } else if (titleArg->TYPE == NODE_VARIABLE) {
                        // variable: append C++ variable at runtime (not quoted name)
                        // but in JS we want a quoted string: so produce: "\"" + var + "\""
                        // We'll represent as three pieces: literal "\"", var, literal "\""
                        jsPieces.push_back({true, "\""});                 // opening quote
                        jsPieces.push_back({false, *(titleArg->value)});  // variable name (C++ var)
                        jsPieces.push_back({true, "\""});                 // closing quote
                    } else {
                        // generic expression: embed as expression value (we'll wrap via exprForNode)
                        string exprCpp = exprForNode(titleArg);
                        // we need to append at runtime: so pieces { "\"", exprCpp, "\"" }
                        jsPieces.push_back({true, "\""});
                        jsPieces.push_back({false, "(" + exprCpp + ")"});
                        jsPieces.push_back({true, "\""});
                    }
                } else {
                    jsPieces.push_back({true, "\"PYRA-APP\""});
                }
                jsPieces.push_back({true, ";\n"});

                // Styles for document.body if provided as style=...
                if (styleParam && styleParam->CHILD && styleParam->CHILD->TYPE == NODE_DICT) {
                    cout << "got here"<< endl;
                    AST_NODE *dict = styleParam->CHILD;
                    for (auto &kv : dict->SUB_STATEMENTS) {
                        AST_NODE *keyNode = kv->SUB_STATEMENTS[0];
                        AST_NODE *valNode = kv->SUB_STATEMENTS[1];
                        string key = (keyNode->TYPE == NODE_STRING) ? *(keyNode->value) : *(keyNode->value);

                        // Build "document.body.style.<key> = <value>;\n"
                        jsPieces.push_back({true, string("document.body.style.") + key + " = "});
                        if (valNode->TYPE == NODE_STRING) {
                            jsPieces.push_back({true, "\"" + *(valNode->value) + "\""});
                        } else if (valNode->TYPE == NODE_VARIABLE) {
                            jsPieces.push_back({true, "\""});
                            jsPieces.push_back({false, *(valNode->value)});
                            jsPieces.push_back({true, "\""});
                        } else {
                            // expression
                            string e = exprForNode(valNode);
                            jsPieces.push_back({false, "(" + e + ")"});
                        }
                        jsPieces.push_back({true, ";\n"});
                    }
                }

                if (styleParam && styleParam->CHILD && styleParam->CHILD->TYPE == NODE_VARIABLE) {
                    
                    jsPieces.push_back({false, "Map_to_Styles("+*(styleParam->CHILD->value)+",\"" +parentId+"\")"});
                }
                stmt << build_js_string_code(jsPieces, "\n    ");
                stmt << "\n    emscripten_run_script(js_" << to_string(idcount) <<".c_str());";


                // Children: views and texts — for each we will append JS (generated via helper that returns raw JS string)
                for (auto &child : p->SUB_STATEMENTS) {
                    if (child->TYPE == NODE_VIEW) {
                        // generate JS string for view and append to jsPieces as a literal piece
                        auto viewPieces = generateViewJsCode(child, "document.body");
                        stmt << viewPieces << endl;
                    } else if (child->TYPE == NODE_TEXT) {
                        // generate js for text appended to body
                        auto txtPieces = generateTextJsCode(child, "document.body");
                        stmt << txtPieces << endl;
                    } else if (child->TYPE == NODE_IMAGE){
                        // generate js for text appended to body
                        auto txtPieces = generateImageJsCode(child, "document.body");
                        stmt << txtPieces << endl;
                    } else {
                        string c_code = HandleAst(child);
                        stmt << c_code;
                    }
                }
                return stmt.str();
            }

            case NODE_VIEW: {
                // top-level view: produce JS appended to parentId
                // Build variable-aware JS pieces (here generateViewJsCode returns a fully literal JS chunk,
                // but it may contain placeholders / variable references indicated as special markers if we had them.
                // In our implementation generateViewJsCode already uses variable-aware pieces — so it returns
                // a JS string (literal) when possible. For simplicity we'll treat it as a literal piece here.

                stringstream stmt;
                auto jsPieces = generateViewJsCode(p, parentId);
                stmt << jsPieces;
                stmt << "\n    emscripten_run_script(js_" << to_string(idcount) <<".c_str());";
                return stmt.str();
            }

            case NODE_TEXT: {
                // text appended to parentId
                stringstream stmt;
                string textJs = generateTextJsCode(p, parentId);
                vector<pair<bool,string>> jsPieces = { {true, textJs} };
                idcount++;
                stmt << build_js_string_code(jsPieces, "    ");
                stmt << "\n    emscripten_run_script(js_" << to_string(idcount) <<".c_str());";
                return stmt.str();
            }
            case NODE_IMAGE: {
                // image appended to parentId
                stringstream stmt;
                string textJs = generateImageJsCode(p, parentId);
                vector<pair<bool,string>> jsPieces = { {true, textJs} };
                idcount++;
                stmt << build_js_string_code(jsPieces, "    ");
                stmt << "\n    emscripten_run_script(js_" << to_string(idcount) <<".c_str());";
                return stmt.str();
            } 
            default:
                return "";
        }
    }

    
    // Generate JS for a Image node appended to parentId (returns raw JS string)
    string generateImageJsCode(AST_NODE *textNode, const string &parentId, bool fromview = false) {
        stringstream stmt;
        vector<pair<bool,string>> pieces;

        bool isId = false;

        string id = "img_" + to_string(idcount);
        string divid = id;
        AST_NODE *args = textNode->CHILD;
        AST_NODE *styleParam = nullptr;

        if (args && !args->SUB_STATEMENTS.empty()) {
            AST_NODE *first = args->SUB_STATEMENTS[0];
            isId = true;
            if (first->TYPE == NODE_STRING) {
                pieces.push_back({true, "var " + id + " = document.createElement('img');\n"}); 
                divid = *(first->value);
                pieces.push_back({true, id + ".src = '"});
                pieces.push_back({true, divid});
                pieces.push_back({true,  "' ;\n"});
                if (fromview)
                {
                    pieces.push_back({true, "document.getElementById('"});
                    pieces.push_back({true,  parentId});
                    pieces.push_back({true, "').appendChild(" + id + ");\n"});
                }
                else
                {
                pieces.push_back({true, parentId + ".appendChild(" + id + ");\n"});
                }
            }else if (first->TYPE == NODE_VARIABLE) {
                pieces.push_back({true, "var " + id + " = document.createElement('img');\n"}); 
                divid = *(first->value);
                pieces.push_back({true, id + ".src = '"});
                pieces.push_back({false, divid});
                pieces.push_back({true,  "' ;\n"});
                if (fromview)
                {
                    pieces.push_back({true, "document.getElementById('"});
                    pieces.push_back({false,  parentId});
                    pieces.push_back({true, "').appendChild(" + id + ");\n"});
                }
                else
                {
                pieces.push_back({true, parentId + ".appendChild(" + id + ");\n"});
                }
            }

            for (size_t i = 1; i < args->SUB_STATEMENTS.size(); ++i) {
                AST_NODE *param = args->SUB_STATEMENTS[i];
                if (param && param->TYPE == NODE_VARIABLE && param->value && *(param->value) == "style") {
                    styleParam = param;
                }
            }
        } else {
            // Create image element
            pieces.push_back({true, "var " + id + " = document.createElement('img');\n"});
            pieces.push_back({true, id + ".src = '" + escape_for_cpp_literal(id) + "';\n"});
           if (fromview)
            {
                pieces.push_back({true, "document.getElementById('"});
                pieces.push_back({false,  parentId});
                pieces.push_back({true, "').appendChild(" + id + ");\n"});
            }
            else
            {
               pieces.push_back({true, parentId + ".appendChild(" + id + ");\n"});
            }
        }

        // Styles (dict)
        if (styleParam && styleParam->CHILD && styleParam->CHILD->TYPE == NODE_DICT) {
            AST_NODE *dict = styleParam->CHILD;
            for (auto &kv : dict->SUB_STATEMENTS) {
                AST_NODE *k = kv->SUB_STATEMENTS[0];
                AST_NODE *v = kv->SUB_STATEMENTS[1];
                string key = (k->TYPE == NODE_STRING) ? *(k->value) : *(k->value);

                pieces.push_back({true, id + ".style." + key + " = "});

                if (v->TYPE == NODE_STRING) {
                    pieces.push_back({true, "\"" + escape_for_cpp_literal(*(v->value)) + "\""});
                } else if (v->TYPE == NODE_VARIABLE) {
                    // insert variable value at runtime (not quoted)
                    pieces.push_back({true, "\""});
                    pieces.push_back({false, *(v->value)});
                    pieces.push_back({true, "\""});
                }
                pieces.push_back({true, ";\n"});
            }
        }

        // Styles via variable (Map_to_Styles)
        if (styleParam && styleParam->CHILD && styleParam->CHILD->TYPE == NODE_VARIABLE) {
                        pieces.push_back({false, "Map_to_Styles("+*(styleParam->CHILD->value)+",\"" +id+"\")"});

            // pieces.push_back({false, "\"+Map_to_Styles("});
            // pieces.push_back({false, *(styleParam->CHILD->value)});
            // pieces.push_back({false, ",\\\"" + id + "\\\")+\""});
        }

        if (pieces.size() > 0)
        {
            idcount++;
            // Now build C++ code that constructs 'js' and calls emscripten_run_script
            stmt << build_js_string_code(pieces, "\n    ");
            stmt << "\n    emscripten_run_script(js_" << to_string(idcount) <<".c_str());";
        }
        // Children (views or text)
        for (auto &c : textNode->SUB_STATEMENTS) {
            if (c->TYPE == NODE_TEXT) {
                auto txtPieces = generateTextJsCode(c, divid, fromview=true);
                stmt << txtPieces;
            } else if (c->TYPE == NODE_VIEW) {
                // recurse — flatten child view pieces into this one
                if (isId)
                {
                    auto childPieces = generateViewJsCode(c, divid, fromview=true);
                    stmt << childPieces;
                }
                else
                {
                    auto childPieces = generateViewJsCode(c, divid, fromview=false);
                    stmt << childPieces;
                }
            } else {
                string c_code = HandleAst(c);
                stmt << c_code;
            }
        }
        return stmt.str();
    }

    // Generate JS for a text node appended to parentId (returns raw JS string)
    string generateTextJsCode(AST_NODE *textNode, const string &parentId, bool fromview = false) {
        stringstream stmt;
        vector<pair<bool,string>> pieces;

        bool isId = false;

        string id = "p_" + to_string(idcount);
        string divid = id;
        AST_NODE *args = textNode->CHILD;
        AST_NODE *styleParam = nullptr;

        if (args && !args->SUB_STATEMENTS.empty()) {
            AST_NODE *first = args->SUB_STATEMENTS[0];
            isId = true;
            if (first->TYPE == NODE_STRING) {
                pieces.push_back({true, "var " + id + " = document.createElement('p');\n"}); 
                divid = *(first->value);
                pieces.push_back({true, id + ".innerText = '"});
                pieces.push_back({true, divid});
                pieces.push_back({true,  "' ;\n"});
                if (fromview)
                {
                    pieces.push_back({true, "document.getElementById('"});
                    pieces.push_back({true,  parentId});
                    pieces.push_back({true, "').appendChildtext(" + id + ");\n"});
                }
                else
                {
                pieces.push_back({true, parentId + ".appendChild(" + id + ");\n"});
                }
            }else if (first->TYPE == NODE_VARIABLE) {
                pieces.push_back({true, "var " + id + " = document.createElement('p');\n"}); 
                divid = *(first->value);
                pieces.push_back({true, id + ".innerText = '"});
                pieces.push_back({false, divid});
                pieces.push_back({true,  "' ;\n"});
                if (fromview)
                {
                    pieces.push_back({true, "document.getElementById('"});
                    pieces.push_back({true,  parentId});
                    pieces.push_back({true, "').appendChild(" + id + ");\n"});
                }
                else
                {
                pieces.push_back({true, parentId + ".appendChild(" + id + ");\n"});
                }
            }

            for (size_t i = 1; i < args->SUB_STATEMENTS.size(); ++i) {
                AST_NODE *param = args->SUB_STATEMENTS[i];
                if (param && param->TYPE == NODE_VARIABLE && param->value && *(param->value) == "style") {
                    styleParam = param;
                }
            }
        } else {
            // Create div element
            pieces.push_back({true, "var " + id + " = document.createElement('p');\n"});
            pieces.push_back({true, id + ".innerText = '" + escape_for_cpp_literal(id) + "';\n"});
           if (fromview)
            {
                pieces.push_back({true, "document.getElementById('"});
                pieces.push_back({true,  parentId});
                pieces.push_back({true, "').appendChild(" + id + ");\n"});
            }
            else
            {
               pieces.push_back({true, parentId + ".appendChild(" + id + ");\n"});
            }
        }

        // Styles (dict)
        if (styleParam && styleParam->CHILD && styleParam->CHILD->TYPE == NODE_DICT) {
            AST_NODE *dict = styleParam->CHILD;
            for (auto &kv : dict->SUB_STATEMENTS) {
                AST_NODE *k = kv->SUB_STATEMENTS[0];
                AST_NODE *v = kv->SUB_STATEMENTS[1];
                string key = (k->TYPE == NODE_STRING) ? *(k->value) : *(k->value);

                pieces.push_back({true, id + ".style." + key + " = "});

                if (v->TYPE == NODE_STRING) {
                    pieces.push_back({true, "\"" + escape_for_cpp_literal(*(v->value)) + "\""});
                } else if (v->TYPE == NODE_VARIABLE) {
                    // insert variable value at runtime (not quoted)
                    pieces.push_back({true, "\""});
                    pieces.push_back({false, *(v->value)});
                    pieces.push_back({true, "\""});
                }
                pieces.push_back({true, ";\n"});
            }
        }

        // Styles via variable (Map_to_Styles)
        if (styleParam && styleParam->CHILD && styleParam->CHILD->TYPE == NODE_VARIABLE) {
            cout << "got VAR: " << styleParam->CHILD->value << endl;
                        pieces.push_back({false, "Map_to_Styles("+*(styleParam->CHILD->value)+",\"" +id+"\")"});

            // pieces.push_back({false, "\"+Map_to_Styles("});
            // pieces.push_back({false, *(styleParam->CHILD->value)});
            // pieces.push_back({false, ",\\\"" + id + "\\\")+\""});
        }

        if (pieces.size() > 0)
        {
            idcount++;
            // Now build C++ code that constructs 'js' and calls emscripten_run_script

            cout << "got here pieces :-" << pieces.size() << endl;
            stmt << build_js_string_code(pieces, "\n    ");
            stmt << "\n    emscripten_run_script(js_" << to_string(idcount) <<".c_str());";
        }
        // Children (views or text)
        for (auto &c : textNode->SUB_STATEMENTS) {
            if (c->TYPE == NODE_TEXT) {
                auto txtPieces = generateTextJsCode(c, divid, fromview=true);
                stmt << txtPieces;
            } else if (c->TYPE == NODE_VIEW) {
                // recurse — flatten child view pieces into this one
                if (isId)
                {
                    auto childPieces = generateViewJsCode(c, divid, fromview=true);
                    stmt << childPieces;
                }
                else
                {
                    auto childPieces = generateViewJsCode(c, divid, fromview=false);
                    stmt << childPieces;
                }
            } else {
                string c_code = HandleAst(c);
                stmt << c_code;
            }
        }
        return stmt.str();
    }

    // Generate raw JS for a view and its children; supports string-only children and string keys/values.
    // When child text contains special marker "__PYX_VAR::name::END" that indicates a variable, we return it as-is
    // so HandleAst builds the concatenation pieces properly.
    // Generate JS pieces for a <div> view and its children
    string generateViewJsCode(AST_NODE *viewNode, const string &parentId, bool fromview = false) {
        stringstream stmt;
        vector<pair<bool,string>> pieces;

        bool isId = false;
        string id = "div_" + to_string(idcount);
        string divid = id;
        AST_NODE *args = viewNode->CHILD;
        AST_NODE *styleParam = nullptr;

        if (args && !args->SUB_STATEMENTS.empty()) {
            AST_NODE *first = args->SUB_STATEMENTS[0];
            isId = true;
            if (first->TYPE == NODE_STRING) {
                 id = *(first->value); 
                 pieces.push_back({true, "var " + id + " = document.createElement('div');\n"});
                divid = *(first->value);
                pieces.push_back({true, id + ".id = '"});
                pieces.push_back({true, divid});
                pieces.push_back({true,  "' ;\n"});
                if (fromview) {
                    pieces.push_back({true, "document.getElementById('"});
                    pieces.push_back({false,  parentId});
                    pieces.push_back({true, "').appendChild(" + id + ");\n"});
                }else{
                    pieces.push_back({true, parentId + ".appendChild(" + id + ");\n"});
                }
            } else if (first->TYPE == NODE_VARIABLE) {
                pieces.push_back({true, "var " + id + " = document.createElement('div');\n"});
                divid = *(first->value);
                pieces.push_back({true, id + ".id = '"});
                pieces.push_back({false, divid});
                pieces.push_back({true,  "' ;\n"});
                if (fromview) {
                    pieces.push_back({true, "document.getElementById('"});
                    pieces.push_back({false,  parentId});
                    pieces.push_back({true, "').appendChild(" + id + ");\n"});
                }else{
                    pieces.push_back({true, parentId + ".appendChild(" + id + ");\n"});
                }
            }

            for (size_t i = 1; i < args->SUB_STATEMENTS.size(); ++i) {
                AST_NODE *param = args->SUB_STATEMENTS[i];
                if (param && param->TYPE == NODE_VARIABLE && param->value && *(param->value) == "style") {
                    styleParam = param;
                }
            }
        } else {
            // Create div element
            pieces.push_back({true, "var " + id + " = document.createElement('div');\n"});
            pieces.push_back({true, id + ".id = '" + escape_for_cpp_literal(id) + "';\n"});
            if (fromview)
            {
                 pieces.push_back({true, "document.getElementById('"});
                pieces.push_back({false,  parentId});
                pieces.push_back({true, "').appendChild(" + id + ");\n"});
            }
            else
            {
               pieces.push_back({true, parentId + ".appendChild(" + id + ");\n"});
            }
        }

        // Styles (dict)
        if (styleParam && styleParam->CHILD && styleParam->CHILD->TYPE == NODE_DICT) {
            AST_NODE *dict = styleParam->CHILD;
            for (auto &kv : dict->SUB_STATEMENTS) {
                AST_NODE *k = kv->SUB_STATEMENTS[0];
                AST_NODE *v = kv->SUB_STATEMENTS[1];
                string key = (k->TYPE == NODE_STRING) ? *(k->value) : *(k->value);

                pieces.push_back({true, id + ".style." + key + " = "});

                if (v->TYPE == NODE_STRING) {
                    pieces.push_back({true, "\"" + escape_for_cpp_literal(*(v->value)) + "\""});
                } else if (v->TYPE == NODE_VARIABLE) {
                    // insert variable value at runtime (not quoted)
                    pieces.push_back({true, "\""});
                    pieces.push_back({false, *(v->value)});
                    pieces.push_back({true, "\""});
                }
                pieces.push_back({true, ";\n"});
            }
        }

        // Styles via variable (Map_to_Styles)
        if (styleParam && styleParam->CHILD && styleParam->CHILD->TYPE == NODE_VARIABLE) {
            cout << "got VAR: " << styleParam->CHILD->value << endl;
                        pieces.push_back({false, "Map_to_Styles("+*(styleParam->CHILD->value)+",\"" +id+"\")"});

            // pieces.push_back({false, "\"+Map_to_Styles("});
            // pieces.push_back({false, *(styleParam->CHILD->value)});
            // pieces.push_back({false, ",\\\"" + id + "\\\")+\""});
        }

        idcount++;
        // Now build C++ code that constructs 'js' and calls emscripten_run_script
        stmt << build_js_string_code(pieces, "\n    ");
        stmt << "\n    emscripten_run_script(js_" << to_string(idcount) <<".c_str());";
        // Children (views or text)
        for (auto &c : viewNode->SUB_STATEMENTS) {
            if (c->TYPE == NODE_TEXT) {
                if (isId)
                {
                    auto childPieces =generateTextJsCode(c, divid, fromview=true);
                    stmt << childPieces;
                }
                else
                {
                    auto childPieces = generateTextJsCode(c, divid);
                    stmt << childPieces;
                }
            } else if (c->TYPE == NODE_VIEW) {
                // recurse — flatten child view pieces into this one
                
                if (isId)
                {
                    auto childPieces = generateViewJsCode(c, divid, fromview=true);
                    stmt << childPieces;
                }
                else
                {
                    auto childPieces = generateViewJsCode(c, divid);
                    stmt << childPieces;
                }
                
                
            } else if (c->TYPE == NODE_IMAGE) {
                // recurse — flatten child view pieces into this one
                
                if (isId)
                {
                    auto childPieces = generateImageJsCode(c, divid, fromview=true);
                    stmt << childPieces;
                }
                else
                {
                    auto childPieces = generateImageJsCode(c, divid);
                    stmt << childPieces;
                }
                
                
            } else {
                string c_code = HandleAst(c);
                stmt << c_code;
            }
        }
        return stmt.str();
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

