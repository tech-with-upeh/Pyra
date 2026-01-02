#include <iostream>
#include <string>
#include <vector>
#include "parser.hpp"

void printAST(AST_NODE* node, int depth = 0, const std::string& relation = "ROOT") {
    if (!node) return;

    // Indent by depth
    for (int i = 0; i < depth; i++)
        std::cout << "  ";

    // Label relation (ROOT, CHILD, or SUB)
    std::cout << "[" << relation << "] ";

    // Print node type
    std::cout << "Node(";
    switch (node->TYPE) {
        case NODE_ROOT: std::cout << "ROOT"; break;
        case NODE_VARIABLE: std::cout << "VARIABLE"; break;
        case NODE_RETURN: std::cout << "RETURN"; break;
        case NODE_PRINT: std::cout << "PRINT"; break;
        case NODE_INT: std::cout << "INT"; break;
        case NODE_EXPR: std::cout << "EXPR"; break;
        case NODE_BINARY_OP: std::cout << "BINARY_OP"; break;
        case NODE_STRING: std::cout << "STRING"; break;
        case NODE_COMPARISON_OP: std::cout << "COMPARE"; break;
        case NODE_IF: std::cout << "IF"; break;
        case NODE_ELSE_IF: std::cout << "ELSE_IF"; break;
        case NODE_ELSE: std::cout << "ELSE"; break;
        case NODE_WHILE: std::cout << "WHILE"; break;
        case NODE_FOR: std::cout << "FOR"; break;
        case NODE_UNARY_OP: std::cout << "UNARY_OP"; break;
        case NODE_LIST: std::cout << "LIST"; break; 
        case NODE_BOOL: std::cout << "BOOL"; break;
        case NODE_FUNCTION_DECL: std::cout << "FUNC_INIT"; break;
        case NODE_FUNCTION_CALL: std::cout << "FUNC_CALL"; break;
        case NODE_TYPE_CHECK: std::cout << "TYPE_CHECK"; break;
        case NODE_LOOP_CTRL: std::cout << "CTRL"; break;
        case NODE_app: std::cout << "UIapp"; break;
        case NODE_page: std::cout << "UIpage"; break;
        case NODE_VIEW: std::cout << "UIVIEW"; break;
        case NODE_TEXT: std::cout << "UITEXT"; break;
        case NODE_ARGS: std::cout << "ARGS"; break;
        case NODE_DICT: std::cout << "DICTIONARY"; break;
        case NODE_KEYVALUE: std::cout << "KEYVALUE"; break;
        case NODE_SETSTATE: std::cout << "SETSTATE"; break;
        case NODE_GETSTATE: std::cout << "GETSTATE"; break;
        case NODE_GO: std::cout << "GO"; break;
        case NODE_STYLESHEET: std::cout << "STYLESHEET"; break;
        case NODE_CLS: std::cout << "CLASS"; break;
        case NODE_MEDIA_QUERY: std::cout << "MEDIA_QUERY"; break;
        case NODE_CANVAS: std::cout << "CANVAS"; break;
        default: std::cout << "UNKNOWN"; break;
    }

    // Print node value
    if (node->value && !node->value->empty())
        std::cout << ", value=\"" << *(node->value) << "\"";

    std::cout << ")\n";

    // --- Print CHILD node ---
    if (node->CHILD)
        printAST(node->CHILD, depth + 1, "CHILD");

    // --- Print SUB_STATEMENTS ---
    for (size_t i = 0; i < node->SUB_STATEMENTS.size(); i++) {
        printAST(node->SUB_STATEMENTS[i], depth + 1, "SUB");
    }
}
