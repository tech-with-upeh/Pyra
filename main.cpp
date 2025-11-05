#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "headers/lexer.hpp"
#include "headers/parser.hpp"
#include "headers/astvisualise.hpp"
#include "headers/semantics.hpp"
#include "headers/web.hpp"
using namespace std;

int main(int argc, char ** argv) {
    if (argc < 2)
    {
        cout << "Enter File name" << endl;
        exit(1);
    }
    
    cout << "File name is: " << argv[1] << endl;
    ifstream sourcefile(argv[1]);

    stringstream buffer;
    char temp;
    while (sourcefile.get(temp))
    {
       buffer << temp; 
    } 
    buffer << ' '; // EOF marker
    string sourcecode = buffer.str();
    Lexer lexer(sourcecode);
    vector<Token *> tokens = lexer.tokenize();
    int count = 0;
    for(Token * temp : tokens)
    {
        count++;
        cout << count << "). " << temp->value << " : " << typetostring( temp->TYPE) << endl;
    }
    cout << "[i] Finished tokkesshhhnizing [i]" << endl;
    Parser parser(tokens);
    AST_NODE * root = parser.parse();

    std::cout << "\n==== AST Visualization ====\n";
    printAST(root);
    std::cout << "\n==== AST Visualization ENDed ====\n";
    cout << "Root Node has " << root->SUB_STATEMENTS.size() << " sub-statements." << endl;
    cout << "[i] Finished Parsing [i]" << endl;

    SemanticAnalyzer analyzer;
    analyzer.analyze(root);
    cout << "[i] Finished Semantic Analyser [i]" << endl;
    WebEngine gen;
    if (!gen.gen(root)) {
        std::cerr << "write failed\n";
        return 1;
    }
    std::cout << "written generated.cpp\n";

    // WebEngine webengine;
    // cout << webengine.HandleAst(root);
    return 0;
}