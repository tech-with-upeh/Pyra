#include <string>
#include <cstring>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <functional>
#include <algorithm>

#include "vdom.hpp"
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

std::unordered_map<std::string, std::function<void()>> CallbackRegistry::callbacks;
int CallbackRegistry::nextId = 0;

std::string CallbackRegistry::registerCallback(std::function<void()> callback) {
    std::string id = "callback_" + std::to_string(nextId++);
    callbacks[id] = callback;
    return id;
}

void CallbackRegistry::invokeCallback(const std::string& id) {
    auto it = callbacks.find(id);
    if (it != callbacks.end()) {
        it->second();
    }
}

void Canvas2D::clear(){
    EM_ASM({
        if (document.getElementById($0)) {
            const ctx = document.getElementById(UTF8ToString($0)).getContext("2d");
            ctx.clearRect(0, 0, ctx.canvas.width, ctx.canvas.height);
        }
        
    }, id.c_str());
}

void Canvas2D::setFill(const std::string& color) {
    EM_ASM({
        const ctx = document.getElementById(UTF8ToString($0))?.getContext("2d");
        if (!ctx) return;
        ctx.fillStyle = UTF8ToString($1);
    }, id.c_str(), color.c_str());
}

void Canvas2D::setStroke(const std::string& color) {
    EM_ASM({
        const ctx = document.getElementById(UTF8ToString($0))?.getContext("2d");
        if (!ctx) return;
        ctx.strokeStyle = UTF8ToString($1);
    }, id.c_str(), color.c_str());
}

void Canvas2D::lineWidth(size_t w) {
    EM_ASM({
        const ctx = document.getElementById(UTF8ToString($0))?.getContext("2d");
        if (!ctx) return;
        ctx.lineWidth = $1;
    }, id.c_str(), w);
}

void Canvas2D::alpha(double a) {
    EM_ASM({
        const ctx = document.getElementById(UTF8ToString($0))?.getContext("2d");
        if (!ctx) return;
        ctx.globalAlpha = $1;
    }, id.c_str(), a);
}


 // ─────────────────────────────
// Shapes
// ─────────────────────────────

void Canvas2D::rect(size_t x, size_t y, size_t w, size_t h) {
    EM_ASM({
        const ctx = document.getElementById(UTF8ToString($0))?.getContext("2d");
        if (!ctx) return;
        ctx.fillRect($1, $2, $3, $4);
    }, id.c_str(), x, y, w, h);
}

void Canvas2D::strokeRect(size_t x, size_t y, size_t w, size_t h) {
    EM_ASM({
        const ctx = document.getElementById(UTF8ToString($0))?.getContext("2d");
        if (!ctx) return;
        ctx.strokeRect($1, $2, $3, $4);
    }, id.c_str(), x, y, w, h);
}

void Canvas2D::line(size_t x1, size_t y1, size_t x2, size_t y2) {
    EM_ASM({
        const ctx = document.getElementById(UTF8ToString($0))?.getContext("2d");
        if (!ctx) return;
        ctx.beginPath();
        ctx.moveTo($1, $2);
        ctx.lineTo($3, $4);
        ctx.stroke();
    }, id.c_str(), x1, y1, x2, y2);
}

void Canvas2D::circle(size_t x, size_t y, size_t r) {
    EM_ASM({
        const ctx = document.getElementById(UTF8ToString($0))?.getContext("2d");
        if (!ctx) return;
        ctx.beginPath();
        ctx.arc($1, $2, $3, 0, Math.PI * 2);
        ctx.fill();
    }, id.c_str(), x, y, r);
}

void Canvas2D::strokeCircle(size_t x, size_t y, size_t r) {
    EM_ASM({
        const ctx = document.getElementById(UTF8ToString($0))?.getContext("2d");
        if (!ctx) return;
        ctx.beginPath();
        ctx.arc($1, $2, $3, 0, Math.PI * 2);
        ctx.stroke();
    }, id.c_str(), x, y, r);
}


// ─────────────────────────────
// Text
// ─────────────────────────────

void Canvas2D::font(const std::string& f) {
    EM_ASM({
        const ctx = document.getElementById(UTF8ToString($0))?.getContext("2d");
        if (!ctx) return;
        ctx.font = UTF8ToString($1);
    }, id.c_str(), f.c_str());
}

void Canvas2D::text(const std::string& t, size_t x, size_t y) {
    EM_ASM({
        const ctx = document.getElementById(UTF8ToString($0))?.getContext("2d");
        if (!ctx) return;
        ctx.fillText(UTF8ToString($1), $2, $3);
    }, id.c_str(), t.c_str(), x, y);
}

// ─────────────────────────────
// Transforms
// ─────────────────────────────

void Canvas2D::move(size_t x, size_t y) {
    EM_ASM({
        const ctx = document.getElementById(UTF8ToString($0))?.getContext("2d");
        if (!ctx) return;
        ctx.translate($1, $2);
    }, id.c_str(), x, y);
}

void Canvas2D::rotate(double r) {
    EM_ASM({
        const ctx = document.getElementById(UTF8ToString($0))?.getContext("2d");
        if (!ctx) return;
        ctx.rotate($1);
    }, id.c_str(), r);
}

void Canvas2D::scale(double x, double y) {
    EM_ASM({
        const ctx = document.getElementById(UTF8ToString($0))?.getContext("2d");
        if (!ctx) return;
        ctx.scale($1, $2);
    }, id.c_str(), x, y);
}



// PLatform class

int Platform::height() {
            return EM_ASM_INT({
                return window.innerHeight || document.documentElement.clientHeight || document.body.clientHeight;
            });
        }
int Platform::width() {
    return EM_ASM_INT({
        return window.innerWidth || document.documentElement.clientWidth || document.body.clientWidth;
    });
}


// Router 
void Router::add(const std::string& path, Handler handler) {
    routes()[path] = handler;
}

// Navigate to a path
void Router::navigate(const std::string& path) {
    auto it = routes().find(path);
    if (it != routes().end() && it->second) {
        currentPath() = path;
        it->second->render();
    } else {
        get404()->render();
    }
}

void Router::go(const std::string& path, bool push) {
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
std::string Router::getCurrentPath() {
    return currentPath();
}


// Route map
std::unordered_map<std::string, Router::Handler> &Router::routes() {
    static std::unordered_map<std::string, Handler> r;
    return r;
}

// Current path
std::string& Router::currentPath() {
    static std::string p;
    return p;
}

// 404 page
Router::Handler Router::get404() {
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

    EMSCRIPTEN_KEEPALIVE
    void js_mountCanvas(const char* id, const char* html) {
        EM_ASM({
            const id = UTF8ToString($0);
            if (!document.getElementById(id)) {
                document.body.insertAdjacentHTML("beforeend", UTF8ToString($1));
            }
        }, id, html);
    }

    EMSCRIPTEN_KEEPALIVE
    void animatefps() {
        GlobalState::getCurrentPage()->onanimate();
    }

    EMSCRIPTEN_KEEPALIVE
    void js_reqfps() { 
        EM_ASM({
            function rafLoop() {
                Module._animatefps();
                requestAnimationFrame(rafLoop);
            }
            requestAnimationFrame(rafLoop);
        });
    }

    EMSCRIPTEN_KEEPALIVE
    void handleEvent(const char* event) {
        auto it = GlobalState::getCurrentPage()->page_callbacks.find(event);

        if (it != GlobalState::getCurrentPage()->page_callbacks.end()) {
            it->second();
        }
    }

    EMSCRIPTEN_KEEPALIVE
    void js_addpageEventlisteners(const char* event) {
        EM_ASM({
            window.addEventListener(UTF8ToString($0), function () {
			    Module._handleEvent($0);
		    });
        }, event);
    }
}