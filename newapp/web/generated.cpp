#include <iostream>
#include "vdom.hpp"
using namespace std;

void updateUI() {
    // Re-render the current page
    if (GlobalState::getCurrentPage()) {
        GlobalState::getCurrentPage()->render();
    }
}
	VPage page_1;

	VPage page_2;

	VPage page_3;
int main() {

	auto num = make_shared<appstate::State<int>>("num",0);
	page_1.setTitle("My APP");

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

	page_2.setTitle("My about page");

	VNode view_3("div");
	view_3.setAttr("id", "mydiv");
	view_3.onClick([]() {
			cout << "jsjj" << endl;
	});
	view_3.setAttr("style", "height:50px;background-color:green;");

	VNode text_3("p","Click me and check console!");

	view_3.addChild(text_3);
	page_2.addChild(view_3);
	page_3.setTitle("sjvjjkjk;");

	VNode view_5("div");
	view_5.setAttr("id", "mydiv");
	view_5.onClick([]() {
			cout << "jsjj" << endl;
	});
	view_5.setAttr("style", "height:50px;background-color:green;");

	VNode text_5("p","Click me and check console!");

	view_5.addChild(text_5);
	page_3.addChild(view_5);
    return 0;
}
