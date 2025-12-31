#include <iostream>
#include "vdom.hpp"
using namespace std;

void updateUI() {
    // Re-render the current page
    if (GlobalState::getCurrentPage()) {
        GlobalState::getCurrentPage()->render();
    }
}

auto page_1 = make_shared<VPage>();

int main() {
	Router::add("/", page_1);
    auto mytxt = make_shared<appstate::State<string>>("mytxt", "My state text");

    page_1->builder = [&](VPage& page) {
        page.setTitle("My APP");

        VNode view("div");
        view.setAttr("id", "mydiv");
        view.setAttr("style", "height:50px;background-color:green;");

        view.onClick([mytxt]() {
            mytxt->set("changed text");
			cout << mytxt->get() << endl;
            updateUI();
        });

        VNode text("p", mytxt->get());
        view.addChild(text);

        page.addChild(view);
    }; 
	page_1->rebuild();

	EM_ASM({
		console.log(window.location.pathname);
		Module._handleRoute(allocateUTF8(window.location.pathname));
	});
    return 0;
}

