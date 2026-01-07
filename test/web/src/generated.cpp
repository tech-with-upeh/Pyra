#include <iostream>
#include "vdom.hpp"
#include <format>
#include <cmath>
using namespace std;

std::shared_ptr<VPage> create_page1();

int main() {
	Router::add("/mk", create_page1());
	EM_ASM({
		Module._handleRoute(allocateUTF8(window.location.pathname));
		window.addEventListener("popstate", () => {
		Module._handleRoute(allocateUTF8(window.location.pathname));
		});
	});return 0;
}
