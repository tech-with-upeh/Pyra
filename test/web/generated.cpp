#include <iostream>
#include "vdom.hpp"
using namespace std;

void updateUI() {
    // Re-render the current page
    if (GlobalState::getCurrentPage()) {
        GlobalState::getCurrentPage()->render();
    }
}
	VPage abt;
	VPage page_1;
int main() {
	abt.setTitle("About Page");
	abt.addChild(VNode("h1", "This is the about page"));
	
	AppRouter::addRoute("/", &page_1);
	AppRouter::addRoute("/about", &abt);
	EM_ASM({
        console.log("WASM Module Loaded", window.location.pathname);

        
        // Notify JavaScript that WASM is ready
        if (typeof window !== 'undefined') {
            window.wasmModuleReady = true;
			console.log("WASM Module is ready.");
            // Trigger initial route check
            if (window.location.pathname == "/about") {
                console.log("Initial route:", window.location.pathname);
				Module._navigateTo(UTF8ToString($0));
            }
        }
    }, std::string("/about").c_str());

	auto num = make_shared<appstate::State<int>>("num",0);
	page_1.setTitle("My APP----->>>");

	VNode view_1("div");
	view_1.setAttr("id", "mydiv");
	view_1.onClick([num]() {
			num->set((num->get() + 1));
			cout << num->get() << endl;
	});
	view_1.setAttr("style", "height:50px;background-color:green;");

	VNode text_1("p","Click me and check console!");

	view_1.addChild(text_1);
	page_1.addChild(view_1);

	page_1.render();

    return 0;
}
