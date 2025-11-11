#pragma once
#include <emscripten.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <functional>
#include <map>

// -------------------- Callback System --------------------
inline std::map<int,std::function<void()>> g_callbacks;
inline int g_callback_counter = 1;

// Maps dom_id to callback id
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
    std::string dom_id; // actual DOM id

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

    // Determine DOM id
    if(node.attrs.find("id") != node.attrs.end()) {
        node.dom_id = node.attrs["id"];
    } else if(node.onclick) {
        node.dom_id = "vnode" + std::to_string(g_callback_counter);
        node.attrs["id"] = node.dom_id;
    }

    // Add attributes
    for(auto& [k,v] : node.attrs)
        oss << " " << k << "=\"" << v << "\"";

    // Register callback if exists
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

// -------------------- Render Full Page --------------------
inline void renderPage(VPage& page) {
    std::ostringstream html;
    for(auto& node : page.children)
        html << renderToHTML(node);

    // Precompute body attributes string
    std::ostringstream bodyAttrsOSS;
    for(auto& [k,v] : page.bodyAttrs) {
        bodyAttrsOSS << k << "=" << v << ";";
    }
    std::string bodyAttrsStr = bodyAttrsOSS.str();

    EM_ASM_({
        const frag = document.createRange().createContextualFragment(UTF8ToString($0));
        document.body.appendChild(frag);
        document.title = UTF8ToString($1);
        const attrs = UTF8ToString($2).split(";;");
        for(let i=0;i<attrs.length;i++){
            const kv = attrs[i].split("=");
            if(kv.length===2) document.body.setAttribute(kv[0],kv[1]);
        }
    }, html.str().c_str(), page.title.c_str(), bodyAttrsStr.c_str());

    wireOnClicks();
}
