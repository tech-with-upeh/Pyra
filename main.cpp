#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>
#include <algorithm>
#include <cstdlib>
#include <cstring>

#include "headers/lexer.hpp"
#include "headers/parser.hpp"
#include "headers/astvisualise.hpp"
#include "headers/semantics.hpp"
#include "headers/web_engine.hpp"


using namespace std;

namespace fs = std::filesystem;

// --------------------- Config ---------------------
string getProjectRoot(const string& root, bool useroot = false) {
    string file;
    if(useroot) {
        file = root + "/pyra.config";
    } else {
        file = "pyra.config";
    }
    ifstream config(file);
    string line;
    if (config.is_open()) {
        while (getline(config, line)) {
            if (line.rfind("project_root=", 0) == 0) {
                return line.substr(strlen("project_root="));
            }
        }
    } else {
        cerr << "Error: Couldnt find a config file to run!'\n";
        exit(1);
    }
    // fallback to default folder "MyApp"
    return "MyApp";
}

void saveProjectRoot(const string& root) {
    ofstream config(root+ "/pyra.config");
    config << "project_root=" << root << "\n";
}

// --------------------- File Generation ---------------------
void generateFiles(const vector<string>& targets, const string& pname) {
    string root = getProjectRoot(pname, true);
    if (!fs::exists(root)) {
        cerr << "Trying to access root but doesnt exist" << endl;
        exit(1);
    };

    fs::create_directory(root + "/public");
    ofstream(root + "/index.pyx") << R"(
navh = "80px"

div_id = 'nav'


app() {
    page("my tetst page",id="kkk", cls="oppppp", style={
        "height":"100vh",
        "background-color":"pink"
    }) {
        view() {
        text("mydiv",  cls="pllllll", style={
        "height": navh,
        "background-color":"green"
    })
        img("img.png", style={
            "height":"200px",
            "width": "200px"
        })
        }
    }
})";
    for (auto& target : targets) {
        if (target == "web") {
            fs::create_directory(root + "/web");
            ofstream(root + "/web/index.html") << R"(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title></title>
</head>
<body>
</body>
<script src="main.js"></script>
</html>)";
            ofstream(root + "/web/vdom.hpp") << R"(#pragma once
#include <emscripten.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <functional>
#include <map>
#include <algorithm>

// -------------------- Callback System --------------------
inline std::map<int,std::function<void()>> g_callbacks;
inline int g_callback_counter = 1;

// Maps DOM id -> callback index
inline std::map<std::string,int> g_domid_to_callback;

extern "C" {
    void invokeCallback(int id) {
        if(g_callbacks.count(id)) g_callbacks[id]();
    }
    int getCallbackCount() { return g_callback_counter-1; }
}

// -------------------- VNode --------------------
struct VNode {
    std::string tag;
    std::string text;
    std::vector<VNode> children;
    std::unordered_map<std::string,std::string> attrs;
    std::function<void()> onclick;
    std::string dom_id;

    VNode() = default;                // <- Default constructor added
    VNode(std::string t, std::string txt="") : tag(t), text(txt) {}
};

// -------------------- VPage --------------------
struct VPage {
    std::string title;
    std::unordered_map<std::string,std::string> bodyAttrs;
    std::vector<VNode> children;
};

// -------------------- Render VNode to HTML --------------------
inline std::string renderToHTML(VNode& node) {
    std::ostringstream oss;
    oss << "<" << node.tag;

    if(node.attrs.find("id") != node.attrs.end())
        node.dom_id = node.attrs["id"];
    else if(node.onclick) {
        node.dom_id = "vnode" + std::to_string(g_callback_counter);
        node.attrs["id"] = node.dom_id;
    }

    for(auto& [k,v] : node.attrs)
        oss << " " << k << "=\"" << v << "\"";

    if(node.onclick) {
        int cb_id = g_callback_counter++;
        g_callbacks[cb_id] = node.onclick;
        g_domid_to_callback[node.dom_id] = cb_id;
    }

    oss << ">";
    if(!node.text.empty()) oss << node.text;
    for(auto& child : node.children)
        oss << renderToHTML(child);
    oss << "</" << node.tag << ">";
    return oss.str();
}

// -------------------- Wire JS onclicks --------------------
inline void wireOnClicks() {
    for(auto& [dom_id, cb_id] : g_domid_to_callback) {
        EM_ASM_({
            let el = document.getElementById(UTF8ToString($0));
            if(el) el.onclick = () => Module.ccall('invokeCallback','void',['number'],[$1]);
        }, dom_id.c_str(), cb_id);
    }
}

// -------------------- Patch --------------------
struct Patch {
    enum Type { TEXT, ATTR, ADD_CHILD, REMOVE_CHILD } type;
    std::string dom_id;
    std::string key;
    std::string value;
    VNode newNode;

    Patch(Type t, const std::string& id, const std::string& k = "", const std::string& v = "", const VNode& n = VNode(""))
        : type(t), dom_id(id), key(k), value(v), newNode(n) {}
};

// -------------------- Diff Nodes --------------------
inline void diffNodes(const VNode& prev, const VNode& next, std::vector<Patch>& patches) {
    if(prev.text != next.text)
        patches.push_back(Patch(Patch::TEXT, next.dom_id, "", next.text));

    for(auto& [k,v] : next.attrs) {
        if(prev.attrs.find(k) == prev.attrs.end() || prev.attrs.at(k) != v)
            patches.push_back(Patch(Patch::ATTR, next.dom_id, k, v));
    }

    size_t common = std::min(prev.children.size(), next.children.size());
    for(size_t i=0;i<common;i++)
        diffNodes(prev.children[i], next.children[i], patches);

    for(size_t i=common;i<next.children.size();i++)
        patches.push_back(Patch(Patch::ADD_CHILD, next.dom_id, "", "", next.children[i]));

    for(size_t i=common;i<prev.children.size();i++)
        patches.push_back(Patch(Patch::REMOVE_CHILD, prev.children[i].dom_id));
}

// -------------------- Apply Patches --------------------
inline void applyPatches(const std::vector<Patch>& patches) {
    for(auto& p : patches) {
        switch(p.type) {
            case Patch::TEXT:
                EM_ASM_({
                    let el = document.getElementById(UTF8ToString($0));
                    if(el) el.textContent = UTF8ToString($1);
                }, p.dom_id.c_str(), p.value.c_str());
                break;
            case Patch::ATTR:
                EM_ASM_({
                    let el = document.getElementById(UTF8ToString($0));
                    if(el) el.setAttribute(UTF8ToString($1), UTF8ToString($2));
                }, p.dom_id.c_str(), p.key.c_str(), p.value.c_str());
                break;
            case Patch::ADD_CHILD: {
                std::string html = renderToHTML(const_cast<VNode&>(p.newNode));
                EM_ASM_({
                    let frag = document.createRange().createContextualFragment(UTF8ToString($0));
                    let parent = document.getElementById(UTF8ToString($1));
                    if(parent) parent.appendChild(frag);
                }, html.c_str(), p.dom_id.c_str());

                if(p.newNode.onclick) {
                    int cb_id = g_callback_counter++;
                    g_callbacks[cb_id] = p.newNode.onclick;
                    g_domid_to_callback[p.newNode.dom_id] = cb_id;
                    EM_ASM_({
                        let el = document.getElementById(UTF8ToString($0));
                        if(el) el.onclick = () => Module.ccall('invokeCallback','void',['number'],[$1]);
                    }, p.newNode.dom_id.c_str(), g_domid_to_callback[p.newNode.dom_id]);
                }
                break;
            }
            case Patch::REMOVE_CHILD:
                EM_ASM_({
                    let el = document.getElementById(UTF8ToString($0));
                    if(el) el.remove();
                }, p.dom_id.c_str());
                break;
        }
    }
}

// -------------------- Render or Patch Page --------------------
inline VNode prevPageVNode; // <- now works because of default constructor

inline void renderOrPatchPage(VPage& page) {
    VNode newRoot("div"); // invisible container for body children
    newRoot.children = page.children;

    if(prevPageVNode.children.empty()) {
        // first render
        std::ostringstream html;
        for(auto& node : page.children) html << renderToHTML(node);

        std::ostringstream bodyAttrsOSS;
        for(auto& [k,v] : page.bodyAttrs)
            bodyAttrsOSS << k << "=" << v << ";";
        std::string bodyAttrsStr = bodyAttrsOSS.str();

        EM_ASM_({
            const frag = document.createRange().createContextualFragment(UTF8ToString($0));
            document.body.appendChild(frag);
            document.title = UTF8ToString($1);
            const attrs = UTF8ToString($2).split(";");
            for(let i=0;i<attrs.length;i++){
                const kv = attrs[i].split("=");
                if(kv.length===2) document.body.setAttribute(kv[0],kv[1]);
            }
        }, html.str().c_str(), page.title.c_str(), bodyAttrsStr.c_str());

        wireOnClicks();
        prevPageVNode = newRoot;
    } else {
        // patch
        std::vector<Patch> patches;
        size_t common = std::min(prevPageVNode.children.size(), newRoot.children.size());
        for(size_t i=0;i<common;i++)
            diffNodes(prevPageVNode.children[i], newRoot.children[i], patches);

        for(size_t i=common;i<newRoot.children.size();i++)
            patches.push_back(Patch(Patch::ADD_CHILD, "body", "", "", newRoot.children[i]));

        for(size_t i=common;i<prevPageVNode.children.size();i++)
            patches.push_back(Patch(Patch::REMOVE_CHILD, prevPageVNode.children[i].dom_id));

        applyPatches(patches);
        prevPageVNode = newRoot;
    }
})";
            ofstream(root + "/web/pyra.web.config") << "# placeholder";
            cout << "[web] Generated web folder and files.\n";
        } else if (target == "android") {
            fs::create_directory(root + "/android");
            ofstream(root + "/android/MainActivity.java") << "// placeholder MainActivity";
            cout << "[android] Generated android folder and files.\n";
        } else if (target == "ios") {
            fs::create_directory(root + "/ios");
            cout << "[ios] Generated ios folder.\n";
        } else {
            cout << "[!] Unknown target: " << target << "\n";
        }
    }
}


void builder() {
    ifstream sourcefile("index.pyx");
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
    Parser parser(tokens);
    AST_NODE * root = parser.parse();

    std::cout << "\n==== AST Visualization ====\n";
    printAST(root);
    std::cout << "\n==== AST Visualization ENDed ====\n";
    cout << "Root Node has " << root->SUB_STATEMENTS.size() << " sub-statements." << endl;
    cout << "[i] Finished Parsing [i]" << endl;

    SemanticAnalyzer analyzer;
    analyzer.analyze(root);
    WebEngine gen;
    if (!gen.gen(root)) {
        cerr << "write failed\n";
        exit(1);
    }
    cout << "[Pyra] Compiled Projects Successfully! [Pyra]\n";
    string cmd = "emcc web/generated.cpp -o web/main.js "
        "-sEXPORTED_FUNCTIONS=\"['_invokeCallback','_getCallbackCount','_main']\" "
        "-sEXPORTED_RUNTIME_METHODS=\"['ccall','cwrap']\" "
        "-sALLOW_MEMORY_GROWTH";
    system(cmd.c_str());
}


// --------------------- Run DEV ---------------------
void devTarget(const vector<string>& targets, const string& pname) {
    string root = getProjectRoot(pname);

    if (!targets.empty()) {
        string target = targets.at(0);
        if (target == "web") {
            if(targets.size() > 1) {
                string flag = targets.at(1);
                if(flag == "-b") {
                    cout << "[web] Building for development... \n";
                    builder();
                } else {
                    if (flag != "web" && flag != "android" && flag != "ios") {
                        cerr << "cant run multiple targets!!" << endl;
                    } else {
                        cerr << "Unknown Flag: " << flag << endl;
                        exit(1);
                    }
                }
            }
            cout << "[web] running for development... \n";
            string cmd = "cd web && python -m http.server 8000";
            //string cmd = "cd " + root + "/web && emcc main.cpp -o main.js -sEXPORTED_FUNCTIONS='[\"_main\"]' -sEXPORTED_RUNTIME_METHODS=[ccall,cwrap] -sALLOW_MEMORY_GROWTH";
            system(cmd.c_str());
        } else if (target == "android") {
            if(targets.size() > 1) {
                string flag = targets.at(1);
                if(flag == "-b") {
                    cout << "[android] Building for development... \n";
                } else {
                    if (flag == "web" && flag == "android" && flag == "ios") {
                        cerr << "cant run multiple targets!!" << endl;
                    } else {
                        cerr << "Unknown Flag: " << flag << endl;
                        exit(1);
                    }
                }
            }
            cout << "[Android] running for development...\n";
            cout << "[android] (to integrate with Gradle/ADB later)\n";
        } else if (target == "ios") {
            if(targets.size() > 1) {
                string flag = targets.at(1);
                if(flag == "-b") {
                    cout << "[ios] Building for development... \n";
                } else {
                    if (flag == "web" && flag == "android" && flag == "ios") {
                        cerr << "cant run multiple targets!!" << endl;
                    } else {
                        cerr << "Unknown Flag: " << flag << endl;
                        exit(1);
                    }
                }
            }
            cout << "[ios] running for development...\n";
            cout << "[ios] (to integrate with Xcode later)\n";
        } else {
            cerr << "[ERROR]: Unknown Target: " << target << endl;
            exit(1);
        }
    }
}

// --------------------- Run Production ---------------------
void runTarget(const vector<string>& targets, const string& pname) {
    string root = getProjectRoot(pname);

    if (!targets.empty()) {
        string target = targets.at(0);
        if (target == "web") {
            if(targets.size() > 1) {
                string flag = targets.at(1);
                if(flag == "-b") {
                    builder();
                    cout << "[PRODUCTION][web] Compiling for production... [PRODUCTION]\n";
                } else {
                    if (flag != "web" && flag != "android" && flag != "ios") {
                       cerr << "Unknown Flag: " << flag << endl;
                       exit(1); 
                    }
                }
            }
            cout << "[PRODUCTION][web] running for production... [PRODUCTION]\n";
            string cmd = "cd web && python -m http.server 8000";
            //string cmd = "cd " + root + "/web && emcc main.cpp -o main.js -sEXPORTED_FUNCTIONS='[\"_main\"]' -sEXPORTED_RUNTIME_METHODS=[ccall,cwrap] -sALLOW_MEMORY_GROWTH";
            system(cmd.c_str());
        } else if (target == "android") {
            if(targets.size() > 1) {
                string flag = targets.at(1);
                if(flag == "-b") {
                    cout << "[PRODUCTION][android] Building for production... \n";
                } else {
                    if (flag != "web" && flag != "android" && flag != "ios") {
                        cerr << "Unknown Flag: " << flag << endl;
                        exit(1);
                    }
                }
            }
            cout << "[PRODUCTION][android] running for production...[PRODUCTION]\n";
            cout << "[PRODUCTION][android] (to integrate with Gradle/ADB later)[PRODUCTION]\n";
        } else if (target == "ios") {
            if(targets.size() > 1) {
                string flag = targets.at(1);
                if(flag == "-b") {
                    cout << "[PRODUCTION][android] Building for production... \n";
                } else {
                    if (flag == "web" && flag == "android" && flag == "ios") {
                        cerr << "Unknown Flag: " << flag << endl;
                        exit(1);
                    }
                }
            }
            cout << "[PRODUCTION][ios] running for production...[PRODUCTION]\n";
            cout << "[PRODUCTION][ios] (to integrate with Xcode later)[PRODUCTION]\n";
        } else {
            cerr << "[ERROR]: Unknown Target: " << target << endl;
            exit(1);
        }
    }
}

// --------------------- Build ---------------------
void buildTarget(const vector<string>& targets, const string& pname) {
    string root = getProjectRoot(pname);

    for (auto& target : targets) {
        if (target == "web") {
            builder();
            cout << "[web] Compiled Web Engine..... \n";
        } else if (target == "android") {
            cout << "[android] Compiling MainActivity...\n";
        } else if (target == "ios") {
            cout << "[ios] Compiling iOS app...\n";
        } else {
            cerr << "[ERROR]: Unknown Target: " << target << endl;
            exit(1);
        }
    }
}



// --------------------- Clean ---------------------
void cleanProject(const string& pname) {
    string root = getProjectRoot(pname);
    if (fs::exists("web/generated.cpp")) {
        fs::remove("web/generated.cpp");
        cout << "[clean] cleaned project folder: " << root << endl;
    }
    // if (fs::exists("pyra.config")) {
    //     fs::remove("pyra.config");
    //     cout << "[clean] Removed pyra.config\n";
    // }
}

 
int main(int argc, char ** argv) {
    if (argc < 2) {
        cout << "Usage: pyra.exe <command> [target]\n";
        return 0;
    }

    string cmd = argv[1];
    vector<string> targets;
    transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);
    if (argc >= 3) {
        targets.assign(argv + 2, argv + argc);
    } else {
        // default targets
        if(cmd != "create") {targets = {"web", "android", "ios"};}
    }

    
    
    if (cmd == "create") {
        if (targets.empty()) { cerr << "[Error]: Name is required\n";
        return 0; }
        string projectName = targets[0];
        if(fs::exists(projectName)) {
            cerr << "[Error]: Cant Create " << projectName <<" \n[Error]: It already exists" << endl;
            return 0; 
        } else {
            fs::create_directory(projectName);
        }
        saveProjectRoot(projectName);
        cout << "[create] Created project folder: " << projectName << "\n";
        generateFiles({"web", "android", "ios"}, projectName);
    } else if (cmd == "dev") {
        string projectName = targets[0];
       
        devTarget(targets, projectName);
    }else if (cmd == "run") {
        string projectName = targets[0];
       
        runTarget(targets, projectName);
    } else if (cmd == "build") {
        string projectName = targets[0];
        
        buildTarget(targets, projectName);
    } else if (cmd == "clean") {
        string projectName = targets[0];
        cleanProject(projectName);
    } else {
        cout << "Unknown command: " << cmd << "\n";
    }
    
    // WebEngine webengine;
    // cout << webengine.HandleAst(root);
    return 0;
}