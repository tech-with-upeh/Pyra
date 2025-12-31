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

auto page_2 = make_shared<VPage>();

auto page_3 = make_shared<VPage>();
int main() {Router::add("/",page_1);Router::add("/notfound",page_2);Router::add("/about",page_3);
	page_1->builder = [&](VPage& page) {
	page.setTitle("My APP");
	page.bodyAttrs["style"] = "height:100%;width:100%;padding:0;margin:0;background-color:pink;";

	    int pi = 3142;
	
	auto num = make_shared<appstate::State<string>>("num","red");
	
	VNode view_1("div");
	view_1.setAttr("id", "mydiv");
	view_1.onClick([num]() {
			cout << num->get() << endl;
		updateUI();	});
	view_1.setAttr("style", "height:50px;background-color:green;padding:0;");

	VNode text_1("p","Clime and check console!");

	view_1.addChild(text_1);
	page.addChild(view_1);
	
	VNode view_3("div");
	view_3.setAttr("id", "myview");
	view_3.onClick([num]() {
			if(num->get() == "red"){
    	num->set("yellow");
    
    }
    else {	num->set("red");
    }
			cout << ("printing: ->" + num->get()) << endl;
		updateUI();	});
	view_3.setAttr("style", "height:100px;background-color:"+num->get()+";"+"");

	page.addChild(view_3);
};
	page_2->builder = [&](VPage& page) {
	page.setTitle("Oh Oh hhhhh");

	
	VNode text_4("p","My custidwjbdsbkjj");

	page.addChild(text_4);
};
	page_3->builder = [&](VPage& page) {
	page.setTitle("About Page");

	
	VNode text_5("p","My About");

	page.addChild(text_5);
};
	EM_ASM({
		Module._handleRoute(allocateUTF8(window.location.pathname));
		window.addEventListener("popstate", () => {
		Module._handleRoute(allocateUTF8(window.location.pathname));
		});
	});return 0;
}
