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
        file = root + "/helios.config";
    } else {
        file = "helios.config";
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
    ofstream config(root+ "/helios.config");
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
@state num : 0
    page("My APP") {
        view("mydiv", style={
            "height": "50px",
            "background-color": "green"
        }, onclick=(num) {
            num = num + 1
            print(num)
        }) {
            text('Click me and check console!')
        } 
    }
)";
    for (auto& target : targets) {
        if (target == "web") {
            fs::create_directory(root + "/web");
            ofstream(root + "/web/index.html") << R"(<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <title></title>
</head>
<body>
    <script>
        var Module = {
            onRuntimeInitialized: function() {
                console.log('[HELIOS] WASM Module initialized [HELIOS]');
            },
            print: function(text) {
                console.log('[HELIOS]:', text);
            }
        };

        document.addEventListener('click', function(event) {
            const target = event.target;
            const callbackId = target.getAttribute('data-callback');
            
            if (callbackId && Module && Module._invokeVNodeCallback) {
                const length = Module.lengthBytesUTF8(callbackId) + 1;
                const buffer = Module._malloc(length);
                Module.stringToUTF8(callbackId, buffer, length);
                
                Module._invokeVNodeCallback(buffer);
                Module._free(buffer);
            }
        });
    </script>
    
    <script src="main.js"></script>
</body>
</html>)";
            ofstream(root + "/web/vdom.hpp") << R"(#pragma once
#include <emscripten.h>
#include <string>
#include <cstring>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <functional>
#include <algorithm>

// -------------------- Forward declarations --------------------
struct VPage;
void renderPage(VPage& page);

// -------------------- Global Page State --------------------
namespace GlobalState {
    static VPage* currentPage = nullptr;
    
    static void setCurrentPage(VPage* page) {
        currentPage = page;
    }
    
    static VPage* getCurrentPage() {
        return currentPage;
    }
   
}

// -------------------- Proper State Management --------------------
namespace appstate {
    template<typename T>
    class State {
    private:
        std::string key;
        
    public:
        State(const std::string& k, T initial) : key(k) {
            // Force initialization in JS storage
            initialize(initial);
        }
        
        void initialize(T initial_value) {
            if constexpr (std::is_same_v<T, int>) {
                EM_ASM({
                    window.wasmState = window.wasmState || {};
                    // Only initialize if not already set
                    if (window.wasmState[UTF8ToString($0)] === undefined) {
                        window.wasmState[UTF8ToString($0)] = $1;
                    }
                }, key.c_str(), initial_value);
            } else if constexpr (std::is_same_v<T, std::string>) {
                EM_ASM({
                    window.wasmState = window.wasmState || {};
                    if (window.wasmState[UTF8ToString($0)] === undefined) {
                        window.wasmState[UTF8ToString($0)] = UTF8ToString($1);
                    }
                }, key.c_str(), initial_value.c_str());
            }
        }
        
        void set(T new_value) {
            if constexpr (std::is_same_v<T, int>) {
                EM_ASM({
                    window.wasmState = window.wasmState || {};
                    window.wasmState[UTF8ToString($0)] = $1;
                }, key.c_str(), new_value);
            } else if constexpr (std::is_same_v<T, std::string>) {
                EM_ASM({
                    window.wasmState = window.wasmState || {};
                    window.wasmState[UTF8ToString($0)] = UTF8ToString($1);
                }, key.c_str(), new_value.c_str());
            }
        }
        
        T get() const {
            if constexpr (std::is_same_v<T, int>) {
                return EM_ASM_INT({
                    window.wasmState = window.wasmState || {};
                    var val = window.wasmState[UTF8ToString($0)];
                    // Return default if not initialized
                    return (val === undefined) ? 0 : val;
                }, key.c_str());
            } else if constexpr (std::is_same_v<T, std::string>) {
                char* result = (char*)EM_ASM_INT({
                    window.wasmState = window.wasmState || {};
                    var val = window.wasmState[UTF8ToString($0)];
                    if (val === undefined) {
                        val = ""; // Default value
                    }
                    var length = lengthBytesUTF8(val) + 1;
                    var buffer = _malloc(length);
                    stringToUTF8(val, buffer, length);
                    return buffer;
                }, key.c_str());
                
                if (result) {
                    std::string str(result);
                    free(result);
                    return str;
                }
                return "";
            }
            return T();
        }
    };
}




// -------------------- Callback Registry --------------------
class CallbackRegistry {
private:
    static std::unordered_map<std::string, std::function<void()>> callbacks;
    static int nextId;
    
public:
    static std::string registerCallback(std::function<void()> callback) {
        std::string id = "callback_" + std::to_string(nextId++);
        callbacks[id] = callback;
        return id;
    }
    
    static void invokeCallback(const std::string& id) {
        auto it = callbacks.find(id);
        if (it != callbacks.end()) {
            it->second();
        }
    }
};

std::unordered_map<std::string, std::function<void()>> CallbackRegistry::callbacks;
int CallbackRegistry::nextId = 0;

// -------------------- VNode --------------------
struct VNode {
    std::string tag;
    std::string text;
    std::vector<VNode> children;
    std::unordered_map<std::string, std::string> attrs;
    std::function<void()> onclick;
    std::string callback_id;

    VNode() = default;
    VNode(std::string t, std::string txt="") : tag(t), text(txt) {}
    
    // Helper methods for building VNodes
    VNode& setText(const std::string& newText) {
        text = newText;
        return *this;
    }
    
    VNode& setAttr(const std::string& key, const std::string& value) {
        attrs[key] = value;
        return *this;
    }
    
    VNode& addChild(const VNode& child) {
        children.push_back(child);
        return *this;
    }
    
    VNode& onClick(std::function<void()> handler) {
        onclick = handler;
        return *this;
    }
};

// -------------------- VPage --------------------
struct VPage {
    std::string title;
    std::vector<VNode> children;
    std::unordered_map<std::string, std::string> bodyAttrs;
    std::string stylesheet;

    std::function<void(VPage&)> builder; 

    
    // Helper methods
    VPage& setTitle(const std::string& newTitle) {
        title = newTitle;
        return *this;
    }

    VPage& addStyle(const std::string& newstylesheet) {
        std::cout << "adding style" << std::endl;
        stylesheet = newstylesheet;
        return *this;
    }
    
    VPage& addChild(const VNode& child) {
        children.push_back(child);
        return *this;
    }
    
    VPage& clearChildren() {
        children.clear();
        return *this;
    }

    void rebuild() {
        if (!builder) return;
        clearChildren();
        builder(*this);
    }
    
    // Render this page
    void render() {
        rebuild();
        renderPage(*this);
    }
};

// --------------------Router ------------------------------
class Router {
public:
    using Handler = std::shared_ptr<VPage>; // store shared_ptr to avoid copies

    // Add a route
    static void add(const std::string& path, Handler handler) {
        routes()[path] = handler;
    }

    // Navigate to a path
    static void navigate(const std::string& path) {
        auto it = routes().find(path);
        if (it != routes().end() && it->second) {
            currentPath() = path;
            it->second->render();
        } else {
            get404()->render();
        }
    }

    static void go(const std::string& path, bool push = true) {
        auto it = routes().find(path);
        if (it != routes().end()) {
            currentPath() = path;
            GlobalState::setCurrentPage(&(*(it->second)));
            it->second->render(); 

            if (push) {
                EM_ASM({
                    history.pushState({}, "", UTF8ToString($0));
                }, path.c_str());
            }
        } else {
            get404()->rebuild();
        }
    }

    // Get the current path
    static std::string getCurrentPath() {
        return currentPath();
    }

private:
    // Route map
    static std::unordered_map<std::string, Handler>& routes() {
        static std::unordered_map<std::string, Handler> r;
        return r;
    }

    // Current path
    static std::string& currentPath() {
        static std::string p;
        return p;
    }

    // 404 page
    static Handler get404() {
        static Handler notfound;
        std::string notfound_path = "/notfound";
        auto it = routes().find(notfound_path);
        if (it != routes().end() && it->second) {
            notfound = [it]() {
                
                return it->second;
            }();
        } else {
            notfound = []() {
                auto page = std::make_shared<VPage>();
                page->setTitle("Page Not Found");
                page->addChild(VNode("p", "Page not found"));
                return page;
            }();
        }
        
        return notfound;
    }
};

// -------------------- JavaScript Interop --------------------
extern "C" {
    EMSCRIPTEN_KEEPALIVE
    void invokeVNodeCallback(const char* callbackId) {
        std::string id(callbackId);
        CallbackRegistry::invokeCallback(id);
    }

    EMSCRIPTEN_KEEPALIVE
    void js_insertHTML(const char* html) {
        EM_ASM({
            document.body.style = allocateUTF8(""); 
            document.body.innerHTML = UTF8ToString($0);
        }, html);
    }

    EMSCRIPTEN_KEEPALIVE
    void handleRoute(const char* route) {
        Router::navigate(route);
    }

    EMSCRIPTEN_KEEPALIVE
    void js_setTitle(const char* title) {
        EM_ASM({
            document.title = UTF8ToString($0);
        }, title);
    }

    EMSCRIPTEN_KEEPALIVE
    void js_insertCSS(const char* css) {
        if (strcmp(css, "") != 0){    
            EM_ASM({
                if (!document.getElementById("__ink_styles")) {
                const style = document.createElement("style");
                style.id = "__ink_styles";
                style.innerHTML = UTF8ToString($0);
                document.head.appendChild(style);
                }
            }, css);
        }else {
            std::cout << "Empty css" << std::endl;
        }
    }

    EMSCRIPTEN_KEEPALIVE
    void js_setBodyAttr(const char* key, const char* val) {
        if (strcmp(key, "")) {
            EM_ASM({
                document.body.setAttribute(UTF8ToString($0), UTF8ToString($1));
            }, key, val);
        }
    }
    
    EMSCRIPTEN_KEEPALIVE
    char* allocateString(const char* str) {
        size_t len = strlen(str) + 1;
        char* buffer = (char*)malloc(len);
        strcpy(buffer, str);
        return buffer;
    }
    
    EMSCRIPTEN_KEEPALIVE
    void freeString(char* str) {
        free(str);
    }
}

// -------------------- Render VNode to HTML --------------------
inline std::string renderToHTML(const VNode& node) {
    std::ostringstream oss;
    oss << "<" << node.tag;

    if(node.attrs.find("id") != node.attrs.end()) {
        oss << " id=\"" << node.attrs.at("id") << "\"";
    }

    for(const auto& [k,v] : node.attrs) {
        if(k != "id") oss << " " << k << "=\"" << v << "\"";
    }

    if(!node.callback_id.empty()) {
        oss << " data-callback=\"" << node.callback_id << "\"";
    }

    oss << ">";
    if(!node.text.empty()) oss << node.text;
    for(const auto& child : node.children)
        oss << renderToHTML(child);
    oss << "</" << node.tag << ">";
    return oss.str();
}

// -------------------- Bind onclick --------------------
inline void bindOnClick(VNode& node) {
    if(node.onclick) {
        node.callback_id = CallbackRegistry::registerCallback(node.onclick);
    }
    for(auto& child : node.children)
        bindOnClick(child);
}

// -------------------- Render Page --------------------
inline void renderPage(VPage& page) {
    // Set as current page for callbacks to access
    GlobalState::setCurrentPage(&page);
    
    std::ostringstream html;
    for(auto& node : page.children) {
        bindOnClick(node);
        html << renderToHTML(node);
    }

    js_insertHTML(html.str().c_str());
    js_setTitle(page.title.c_str());
    js_insertCSS(page.stylesheet.c_str());

    // Apply body attributes
    for(const auto& [key, value] : page.bodyAttrs) {
        js_setBodyAttr(key.c_str(), value.c_str());
    }
})";
            ofstream(root + "/web/helios.web.config") << "# placeholder";
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
    cout << "[i] Finished Semantic Analysing [i]" << endl;
    WebEngine gen;
    if (!gen.gen(root)) {
        cerr << "write failed\n";
        exit(1);
    }
    cout << "[Helios] Compiled Projects Successfully! [Helios]\n";

    string cmd = "em++ web/generated.cpp -o web/main.js " 
        "-sEXPORTED_FUNCTIONS=\"['_main','_invokeVNodeCallback','_js_insertHTML','_js_setTitle','_malloc','_free', '_handleRoute']\" "
        "-sEXPORTED_RUNTIME_METHODS=\"['ccall','cwrap','stringToUTF8','lengthBytesUTF8']\" "
        "-sALLOW_MEMORY_GROWTH=1 -sASSERTIONS=1 -sDEFAULT_LIBRARY_FUNCS_TO_INCLUDE='$allocateUTF8'";
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
}

 
int main(int argc, char ** argv) {
    if (argc < 2) {
        cout << "Usage: helios.exe <command> [target]\n";
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