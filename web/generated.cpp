#include "vdom.hpp"
using namespace std;

int main() {
    string navh = "80px";
    string div_id = "nav";

	VPage page_1;
	page_1.title = "my tetst page";
	page_1.bodyAttrs["class"] = "llii";
	page_1.bodyAttrs["style"] = "height:100vh;backgroundColor:red;";

	VNode view_1;
	view_1.attrs["class"] = "uioo";
	view_1.attrs["style"] = "height:"+navh+"backgroundColor:green;";

	page_1.children.push_back(view_1);
    return 0;
}
