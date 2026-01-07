#include "../include/vdom.hpp"
#include <iostream>
using namespace std;


std::shared_ptr<VPage> create_page1() {
    auto page_1 = make_shared<VPage>();
    page_1->builder = [&](VPage& page) {
		page.setTitle("MyAPP");

		auto num = make_shared<appstate::State<int>>("num",0);
		VNode view_1("div");
		view_1.setAttr("id", "mydiv");
        view_1.onClick([num]() {
                num->set((num->get() + 1));
            cout << num->get() << endl;
            updateUI();	});
	    view_1.setAttr("style", "height:50px;background-color:green;");

		VNode text_1("p","Click me and  check console!");

        view_1.addChild(text_1);
        page.addChild(view_1);
	};
    return page_1;
}
