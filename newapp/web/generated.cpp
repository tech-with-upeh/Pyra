#include <iostream>
#include "vdom.hpp"
#include <format>
using namespace std;

void updateUI() {
    // Re-render the current page
    if (GlobalState::getCurrentPage()) {
        GlobalState::getCurrentPage()->render();
    }
}
auto page_1 = make_shared<VPage>();
int main() {
	Router::add("/",page_1);
	page_1->builder = [&](VPage& page) {
		page.setTitle("MAIN");

		page.onMount([&]() {
			Canvas2D ctx(string("hero"));
			ctx.setFill(string("#850000ff"));
			ctx.rect(10,10,70,150);
			
		});

		
	VNode canvas_1("canvas");
		canvas_1.type = VNodeType::CANVAS;
		canvas_1.setAttr("id", "hero");
		canvas_1.height = 500;
		canvas_1.width = 500;

	page.addChild(canvas_1);
		};
	EM_ASM({
		Module._handleRoute(allocateUTF8(window.location.pathname));
		window.addEventListener("popstate", () => {
		Module._handleRoute(allocateUTF8(window.location.pathname));
		});
	});return 0;
}
