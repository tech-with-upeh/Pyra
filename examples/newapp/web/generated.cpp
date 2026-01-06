#include <iostream>
#include "vdom.hpp"
#include <format>
#include <cmath>
using namespace std;

void updateUI() {
    // Re-render the current page
    if (GlobalState::getCurrentPage()) {
        GlobalState::getCurrentPage()->render();
    }
}
auto page_1 = make_shared<VPage>();
int main() {
	Router::add("/about",page_1);
	page_1->builder = [&](VPage& page) {
		page.setTitle("Create Helios App");

		auto h = Platform().height();
		auto w = Platform().width();
		page.onMount([&]() {
			Canvas2D ctx(string("hero"));
			ctx.setStroke(string("rgba(255, 255, 255, 0.3)"));
			ctx.lineWidth(1);
			ctx.line(50,50,55,55);
		});

		page.onAnimatefps([&]() {
				cout << string("resized") << endl;
		});

		page.addevent("mousemove", [&]() {
				cout << string("kk") << endl;
		});

		
	VNode canvas_1("canvas");
		canvas_1.type = VNodeType::CANVAS;
		canvas_1.setAttr("id", "hero");
		canvas_1.height = h;
		canvas_1.width = w;

	page.addChild(canvas_1);
		};
	EM_ASM({
		Module._handleRoute(allocateUTF8(window.location.pathname));
		window.addEventListener("popstate", () => {
		Module._handleRoute(allocateUTF8(window.location.pathname));
		});
	});return 0;
}
