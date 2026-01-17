#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>
#include <algorithm>
#include <cstdlib>
#include <cstring>

#include "core.hpp"


using namespace std;

namespace fs = std::filesystem;





int main(int argc, char ** argv) {
    if (argc < 2) {
        cout << "Usage: helios.exe <command> [target]\n";
        return 0;
    }

    string cmd = argv[1];
    vector<string> targets;
    transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);
    if (argc >= 3) {
        targets.assign(argv + 2, argv + argc);
    } else {
        // default targets
        if(cmd != "create") {targets = {"web", "android", "ios"};}
    }

    Core Core;
    
    
    if (cmd == "create") {
        if (targets.empty()) { cerr << "[Error]: Name is required\n";
        return 0; }
        string projectName = targets[0];
        if(fs::exists(projectName)) {
            cerr << "[Error]: Cant Create " << projectName <<" \n[Error]: It already exists" << endl;
            return 0; 
        } else {
            fs::create_directory(projectName);
        }
        Core.saveProjectRoot(projectName);
        cout << "[create] Created project folder: " << projectName << "\n";
        Core.generateFiles({"web", "android", "ios"}, projectName);
    } else if (cmd == "dev") {
        string projectName = targets[0];
       
        Core.devTarget(targets, projectName);
    }else if (cmd == "run") {
        string projectName = targets[0];
       
        Core.runTarget(targets, projectName);
    } else if (cmd == "build") {
        string projectName = targets[0];
        
        Core.buildTarget(targets, projectName);
    } else if (cmd == "clean") {
        string projectName = targets[0];
        Core.cleanProject(projectName);
    } else if (cmd == "help") {
        cout << "Available commands:\n";
        cout << "  create <project_name> - Create a new project\n";
        cout << "  dev <project_name> - Start development server\n";
        cout << "  run <project_name> - Run the project\n";
        cout << "  build <project_name> - Build the project\n";
        cout << "  clean <project_name> - Clean the project\n";
        cout << "  help - Show this help message\n";
    } else if (cmd == "--version" || cmd == "-v") {
        cout << "Helios CLI Version 0.1.0\n";

    } else {
        cout << "Unknown command: " << cmd << "\n";
    }
    return 0;
}