#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <sstream>
#include "utils.hpp"
#include <emscripten.h>
using namespace std;

int main() {
    unordered_map<string,string> nav;
    nav.insert({ "height", "80px" });
    nav.insert({ "backgroundColor", "green" });

    string div_id = "nav";

    string js_1 = "document.title = \"my tetst page\";\ndocument.body.style.height = \"100vh\";\ndocument.body.style.backgroundColor = \"red\";\n";

    emscripten_run_script(js_1.c_str());
    string js_2= string("var mydiv = document.createElement('div');\n") + string("mydiv.id = '") + string("mydiv") + string("' ;\n") + string("document.body.appendChild(mydiv);\n") + Map_to_Styles(nav,"mydiv");

    emscripten_run_script(js_2.c_str());
    string js_3 = "var img_2 = document.createElement('img');\nimg_2.src = 'img.png' ;\ndocument.getElementById('mydiv').appendChild(img_2);\nimg_2.style.height = \"200px\";\nimg_2.style.width = \"200px\";\n";

    emscripten_run_script(js_3.c_str());

    return 0;
}
