#include <iostream>
#include "vdom.hpp"
using namespace std;

void updateUI() {
    // Re-render the current page
    if (GlobalState::getCurrentPage()) {
        GlobalState::getCurrentPage()->render();
    }
}
	
	auto page_1 = std::make_shared<VPage>();
	auto notfound = std::make_shared<VPage>();
int main() {
		Router::add("/", page_1);
	auto num = make_shared<appstate::State<int>>("num",0);
	page_1->setTitle("My APP");

	VNode view_1("div");
	view_1.setAttr("id", "mydiv");
	view_1.onClick([num]() {
			num->set((num->get() + 1));
			cout << num->get() << endl;
	});
	view_1.setAttr("style", "height:50px;background-color:green;");

	VNode text_1("p","Click me and check console!");

	view_1.addChild(text_1);
	page_1->addChild(view_1);

	Router::add("notfound", notfound);

	notfound->setTitle("Custom Not Found");
	VNode notfoundtext_1("p","My Custom Not FOunnd");
	notfound->addChild(notfoundtext_1);

	EM_ASM({
		console.log(window.location.pathname);
		Module._handleRoute(allocateUTF8(window.location.pathname));
	});
    return 0;
}
