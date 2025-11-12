#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>
#include <algorithm>
#include <cstdlib>

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

// -------------------- Callback System --------------------
inline std::map<int, std::function<void()>> g_callbacks;
inline int g_callback_counter = 1;

extern "C" {
    void invokeCallback(int id) {
        if (g_callbacks.count(id)) g_callbacks[id]();
    }
    int getCallbackCount() {
        return g_callback_counter - 1;
    }
}

// -------------------- VNode --------------------
struct VNode {
    std::string tag;
    std::string text;
    std::vector<VNode> children;
    std::unordered_map<std::string, std::string> attrs;
    std::function<void()> onclick;

    VNode(std::string t, std::string txt = "") : tag(t), text(txt) {}
};

// -------------------- VPage --------------------
struct VPage {
    std::string title;
    std::unordered_map<std::string, std::string> bodyAttrs;
    std::vector<VNode> children;
};

// -------------------- Render VNode to HTML --------------------
inline std::string renderToHTML(const VNode& node) {
    std::ostringstream oss;
    oss << "<" << node.tag;

    for (auto& [k, v] : node.attrs)
        oss << " " << k << "=\"" << v << "\"";

    int node_id = 0;
    if (node.onclick) {
        node_id = g_callback_counter++;
        oss << " id=\"vnode" << node_id << "\"";
        g_callbacks[node_id] = node.onclick;
    }

    oss << ">";
    if (!node.text.empty()) oss << node.text;

    for (auto& child : node.children)
        oss << renderToHTML(child);

    oss << "</" << node.tag << ">";
    return oss.str();
}

// -------------------- Wire JS onclicks --------------------
EM_JS(void, wireOnClicks, (), {
    for (let id = 1; id <= Module.ccall('getCallbackCount','number'); id++) {
        const el = document.getElementById('vnode' + id);
        if (el) el.onclick = () => Module.ccall('invokeCallback','void',['number'],[id]);
    }
});

// -------------------- Render Full Page --------------------
inline void renderPage(const VPage& page) {
    std::ostringstream html;
    for (auto& node : page.children)
        html << renderToHTML(node);

    // Precompute body attributes string
    std::ostringstream bodyAttrsOSS;
    for (auto& [k,v] : page.bodyAttrs)
        bodyAttrsOSS << k << "=" << v << ";";
    std::string bodyAttrsStr = bodyAttrsOSS.str();

    EM_ASM_({
        const frag = document.createRange().createContextualFragment(UTF8ToString($0));
        document.body.appendChild(frag);
        document.title = UTF8ToString($1);
        const attrs = UTF8ToString($2).split(";");
        for (let i = 0; i < attrs.length; i++) {
            const kv = attrs[i].split("=");
        console.log(kv[0]);
        console.log(kv[1]);
            if (kv.length === 2) document.body.setAttribute(kv[0], kv[1]);
        }
    }, html.str().c_str(), page.title.c_str(), bodyAttrsStr.c_str());

    wireOnClicks();
}
)";
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
    int count = 0;
    for(Token * temp : tokens)
    {
        count++;
        cout << count << "). " << temp->value << " : " << typetostring( temp->TYPE) << endl;
    }
    Parser parser(tokens);
    AST_NODE * root = parser.parse();
    SemanticAnalyzer analyzer;
    analyzer.analyze(root);
    WebEngine gen;
    if (!gen.gen(root)) {
        cerr << "write failed\n";
        exit(1);
    }
    cout << "[Pyra] Compiled Projects Successfully! [Pyra]\n";
    string cmd = "emcc web/generated.cpp -o web/main.js -sEXPORTED_FUNCTIONS='[\"_invokeCallback\",\"_getCallbackCount\", \"_main\"]' -sEXPORTED_RUNTIME_METHODS='[\"ccall\",\"cwrap\"]' -sALLOW_MEMORY_GROWTH";
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