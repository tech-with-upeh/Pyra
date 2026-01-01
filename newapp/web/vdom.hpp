#pragma once
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
}