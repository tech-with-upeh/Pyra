#pragma once
#include <iostream>
#include <fstream>
#include <filesystem>
#include <string> 
#include <sstream>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include "parser.hpp" 

using namespace std;

class WebEngine {
    public:
        // A runtime environment for values known at generation-time (optional).
        // You can populate this map before calling gen() if you want generator-time resolution.
        unordered_map<string,string> env;
        stringstream filebuffer;
        stringstream mainbuffer;

        WebEngine();

        string pyxtocpp_type(enum NODE_TYPE type, AST_NODE* node);

        bool gen(AST_NODE *root);
    private:
        int idcount;
        int pagecount;
        vector<string> statevars;
        unordered_map<string, bool> variable_buffer;
        

        // Escape C-style string literal (for embedding fixed literal pieces in generated C++ code)
        static string escape_for_cpp_literal(const string &in);

        // Helper to create a C++ literal: "\"...\""
        static string cpp_literal(const string &s);
        
        string exprForNode(AST_NODE *p);

        string MakePage(AST_NODE *p, string var, bool firstpage=false); 
        
        string MakeElement(AST_NODE *p, string parent = "root", string el = "div",string eltype = "view", bool isvar=false);

        string MakeConversion(AST_NODE *p, NODE_TYPE type, bool isroot = false);

        string MakeDraw(AST_NODE *p,bool isroot=true, string parent="root");

        string makeMath(AST_NODE *p, NODE_TYPE nodetype);

        string HandleAst(AST_NODE *p, string parent = "root", bool funcdecl=false, bool fromui = true);
        // Write file to disk
        bool makefile(const string &directoryPath, const string &filebuffer);
};