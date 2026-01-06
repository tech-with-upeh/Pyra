
                    string varid = "page_" + to_string(pagecount);
                    stringstream ss;
                    filebuffer << "\n\tVPage "+varid+";\n";
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
                            ss << "\t" << varid << ".setTitle(\"" << *(titleArg->value) << "\");\n"; 
                        } else if (titleArg->TYPE == NODE_VARIABLE) {
                            ss << "\t" << varid << ".setTitle(" << *(titleArg->value) << ");\n"; 
                        } else {
                            ss << "\t" << varid << ".setTitle(\"(" << exprForNode(titleArg) << ")\");\n";
                        }
                    } else {
                        ss << "\t" << varid << ".setTitle(\"Create Pyra App\");\n";
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
                    pagecount++;

                    if (firstpage) {
                        ss << "\n\n\t" << varid << ".render();\n";
                    }
                    return ss.str();