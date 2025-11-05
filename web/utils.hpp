#ifndef __UTILS_H
#define __UTILS_H

#include <string>
#include <unordered_map>

    using namespace std;
    string Map_to_Styles(unordered_map<string,string> MAP, string parent) {
        string ret;
        for (auto &i : MAP)
        {
            ret += parent+string(".style.")+string(i.first) + "= \""+  string(i.second) + string("\";\n");	
        }
        return ret;
    }
#endif