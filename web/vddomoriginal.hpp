#pragma once
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
    for (auto& [k,v] : page.bodyAttrs) {
        bodyAttrsOSS << k << "=" << v << ";";
    }
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
