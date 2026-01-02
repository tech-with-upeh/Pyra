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

auto page_2 = make_shared<VPage>();
int main() {
	Router::add("/",page_1);
	Router::add("/about",page_2);
    string ht = "100px";
	string uni = R"(
		.body{ 
			height : 100vh;
			width : 100vw;
			background-color : dark-grey;
		}	.nav{ 
			height : 0px;
			background-color : red;
			overflow : hidden;
		}	.tx{ 
			color : light-grey;
			margin : 0px;
		}	.primarysec{ 
			color : dark-grey;
			margin : 0px;
		}
		@media only screen and (min-width: 600px) {
				.nav{ 
			height : 100px;
			color : yellow;
		}
		}
)";
	
	page_1->builder = [&, uni, ht](VPage& page) {
		page.addStyle(uni);
		page.setTitle("Home Page");
		page.bodyAttrs["class"] = "body";

		
	auto txt = make_shared<appstate::State<string>>("txt","predef");
		
	VNode view_1("div");
	view_1.setAttr("id", "Nav");
	view_1.setAttr("class",  "nav");
	view_1.onClick([]() {
			cout << "hello" << endl;
		updateUI();	});

	VNode text_1("p",txt->get());

	view_1.addChild(text_1);
	page.addChild(view_1);
		
	VNode text_3("p","another txt");

	page.addChild(text_3);
		};
	page_2->builder = [&, uni, ht](VPage& page) {
		page.addStyle(uni);
		page.setTitle("about Page");

		
	VNode view_4("div");
	view_4.setAttr("id", "nav");

	VNode text_4("p","My win");
	text_4.setAttr("class",  "mytx");

	view_4.addChild(text_4);
	page.addChild(view_4);
		};
	EM_ASM({
		Module._handleRoute(allocateUTF8(window.location.pathname));
		window.addEventListener("popstate", () => {
		Module._handleRoute(allocateUTF8(window.location.pathname));
		});
	});return 0;
}
