#include <emscripten.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <functional>
#include <map>

std::map<int, std::function<void()>> g_callbacks;
int g_callback_counter = 1;

struct VNode {
    std::string tag;
    std::string text;
    std::vector<VNode> children;
    std::unordered_map<std::string, std::string> attrs;
    std::function<void()> onclick;

    VNode(std::string t, std::string txt = "") : tag(t), text(txt) {}
};

// Build HTML string & assign IDs for onclick
std::string renderToHTML(const VNode& node) {
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

// Append HTML
EM_JS(void, appendHTML, (const char* html), {
    const frag = document.createRange().createContextualFragment(UTF8ToString(html));
    document.body.appendChild(frag);
});

// Attach JS click handlers that call back to C++
EM_JS(void, wireOnClicks, (), {
    for (let id = 1; id <= Module.ccall('getCallbackCount', 'number'); id++) {
        const el = document.getElementById('vnode' + id);
        if (el) {
            el.onclick = () => {
                Module.ccall('invokeCallback', 'void', ['number'], [id]);
            };
        }
    }
});

void render(const VNode& root) {
    std::string html = renderToHTML(root);
    appendHTML(html.c_str());
    wireOnClicks();
}

// Expose callback invoker to JS
extern "C" {
    void invokeCallback(int id) {
        if (g_callbacks.count(id)) g_callbacks[id]();
    }
    int getCallbackCount() {
        return g_callback_counter - 1;
    }
}

void sayGoodbye() {
    EM_ASM(alert("Goodbye!"));
}

// ---------------- Test ----------------
int main() {
    VNode app("div");
    app.attrs["style"] = "margin:20px; font-family:Arial;";

    VNode h1("h1", "my h1 tag");
    app.children.push_back(h1);

    VNode btnBye("button", "Say Goodbye");
    btnBye.onclick = sayGoodbye;
    app.children.push_back(btnBye);

    VNode btn("button", "Click Me!");
    btn.attrs["style"] = "padding:10px 20px; font-size:16px; cursor:pointer;";
    btn.onclick = []() { EM_ASM(alert('Button clicked!')); };

    app.children.push_back(btn);
    render(app);
    return 0;
}
