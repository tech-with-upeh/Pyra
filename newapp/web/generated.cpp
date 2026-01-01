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
	Router::add("/abt",page_1);
	Router::add("/",page_2);
    string ht = "100px";
	string uni = R"(
		.body{ 
			height : )" +ht+ R"(;
			width : 100px;
		}	.btn{ 
			height : 100px;
			color : red;
		}
		@media only screen and (min-width: 600px) {
				.btn{ 
			height : 50px;
			color : yellow;
		}
		}
)";
	
	page_1->builder = [&, uni, ht](VPage& page) {
		page.addStyle(uni);
		page.setTitle("HOME PAGE");
		page.bodyAttrs["style"] = "background-color:black;padding:0px;margin:0px;color:white;";

		    int vsr = 0;
		
	auto num = make_shared<appstate::State<int>>("num",0);
		
	auto mytext = make_shared<appstate::State<string>>("mytext","Click me and check console!");
		
	VNode view_1("div");
	view_1.setAttr("id", "mydiv");
	view_1.onClick([num, mytext]() {
			num->set((num->get() + 1));
			mytext->set("Clicked!!");
			cout << num->get() << endl;
		updateUI();	});
	view_1.setAttr("style", "height:50px;background-color:green;");

	VNode text_1("p",mytext->get());
	text_1.setAttr("style", "text-align:center;margin:0px;");

	view_1.addChild(text_1);
	page.addChild(view_1);
		
	VNode view_3("div");
	view_3.setAttr("id", "countdiv");
	view_3.setAttr("style", "margin-top:20px;font-size:20px;");

	VNode text_3("p","go to about");
	text_3.setAttr("class",  "btn");
	text_3.onClick([]() {
		Router::go("/");
		updateUI();	});

	view_3.addChild(text_3);
	page.addChild(view_3);
		};
	page_2->builder = [&, uni, ht](VPage& page) {
		page.addStyle(uni);
		page.setTitle("about");

		    string inpage = "sdkd";
		
	VNode text_5("p","This is the Ho.");
	text_5.setAttr("class",  "btn");

	page.addChild(text_5);
		};
	EM_ASM({
		Module._handleRoute(allocateUTF8(window.location.pathname));
		window.addEventListener("popstate", () => {
		Module._handleRoute(allocateUTF8(window.location.pathname));
		});
	});return 0;
}
