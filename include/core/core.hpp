#pragma once
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>



using namespace std;

class Core {
    public:
        Core();
            // --------------------- Config ---------------------
        string getProjectRoot(const string& root, bool useroot = false);
        
        void saveProjectRoot(const string& root);
        // --------------------- File Generation ---------------------
        void generateFiles(const vector<string>& targets, const string& pname);

        void builder();

        void devTarget(const vector<string>& targets, const string& pname);
        // --------------------- Run Production ---------------------
        void runTarget(const vector<string>& targets, const string& pname);
        // --------------------- Build ---------------------
        void buildTarget(const vector<string>& targets, const string& pname);


        // --------------------- Clean ---------------------
        void cleanProject(const string& pname);

    private:
        
};