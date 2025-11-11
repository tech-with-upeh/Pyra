#include <iostream>
#include "vdom.hpp"
using namespace std;

int main() {
    string navh = "80px";
    string div_id = "nav";

	VPage page_1;
	page_1.title = "my tetst page";
	page_1.bodyAttrs["class"] = "oppppp";
	page_1.bodyAttrs["style"] = "height:100vh;background-color:pink;";
	VNode view_1("div");
	view_1.attrs["id"] = "mydivid";  
	view_1.onclick = [] () {
		EM_ASM(console.log("clicked!!!!"));
	};

	VNode text_1("p","mydiv txt");
	text_1.attrs["class"] = "pllllll";
	text_1.attrs["style"] = "height:"+navh+";"+"background-color:green;";

	view_1.children.push_back(text_1);
	VNode img_2("img");
	img_2.attrs["src"] = "img.png";
	img_2.attrs["style"] = "height:200px;width:200px;";

	view_1.children.push_back(img_2);
	page_1.children.push_back(view_1);

	renderPage(page_1);
    return 0;
}
