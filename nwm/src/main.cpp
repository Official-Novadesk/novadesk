#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <windows.h>

namespace fs = std::filesystem;

// Constants
const std::string NOVADESK_EXE = "Novadesk.exe";
const std::string WIDGETS_DIR = "Widgets";

// Helper to get executable directory
fs::path GetExeDir() {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);
    return fs::path(path).parent_path();
}

// Scaffolding templates
const std::string INDEX_JS_TEMPLATE = "novadesk.log('Widget initialized');\n";
const std::string UI_JS_TEMPLATE = "// UI logic here\n";
const std::string META_JSON_TEMPLATE = R"({
  "name": "{NAME}",
  "version": "1.0.0",
  "author": "Your Name",
  "description": "Widget Description",
  "icon": "assets/icon.ico"
})";

// Helper to execute a command and wait
bool ExecuteCommand(const std::string& command) {
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    if (CreateProcessA(NULL, (LPSTR)command.c_str(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        DWORD exitCode;
        GetExitCodeProcess(pi.hProcess, &exitCode);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return exitCode == 0;
    }
    return false;
}

void PrintUsage() {
    std::cout << "Novadesk Widget Maker (nwm)" << std::endl;
    std::cout << "Usage:" << std::endl;
    std::cout << "  nwm init <widget-name>    - Create a new widget in current directory" << std::endl;
    std::cout << "  nwm run                   - Run widget in current directory" << std::endl;
    std::cout << "  nwm build                 - Build widget in current directory" << std::endl;
}

bool InitWidget(const std::string& name) {
    if (name.empty()) {
        std::cerr << "Error: Widget name required for init" << std::endl;
        return false;
    }
    fs::path baseDir = fs::current_path() / name;
    if (fs::exists(baseDir)) {
        std::cerr << "Error: Widget directory already exists: " << baseDir << std::endl;
        return false;
    }

    try {
        fs::path exeDir = GetExeDir();
        fs::path templateDir = exeDir / "widget";
        
        // Check if template exists in exe dir or source dir (for dev)
        if (!fs::exists(templateDir)) {
             templateDir = exeDir.parent_path().parent_path().parent_path() / "nwm" / "widget";
        }

        if (fs::exists(templateDir)) {
            fs::copy(templateDir, baseDir, fs::copy_options::recursive);
            
            // Replace name in meta.json
            fs::path metaPath = baseDir / "meta.json";
            if (fs::exists(metaPath)) {
                std::ifstream inFile(metaPath);
                std::string content((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
                inFile.close();

                // Improved replacement logic: find "name" key and replace value between quotes
                size_t nameKeyPos = content.find("\"name\"");
                if (nameKeyPos != std::string::npos) {
                    size_t colonPos = content.find(":", nameKeyPos);
                    if (colonPos != std::string::npos) {
                        size_t firstQuote = content.find("\"", colonPos);
                        size_t secondQuote = content.find("\"", firstQuote + 1);
                        if (firstQuote != std::string::npos && secondQuote != std::string::npos) {
                            content.replace(firstQuote + 1, secondQuote - firstQuote - 1, name);
                        }
                    }
                } else {
                    // Try fallback placeholder
                    size_t pos = content.find("{NAME}");
                    if (pos != std::string::npos) content.replace(pos, 6, name);
                }

                std::ofstream outFile(metaPath);
                outFile << content;
            }
            
            std::cout << "Widget initialized from template." << std::endl;
        } else {
            // Fallback to hardcoded templates
            fs::create_directories(baseDir / "ui");
            
            std::ofstream indexFile(baseDir / "index.js");
            indexFile << INDEX_JS_TEMPLATE;
            
            std::ofstream uiFile(baseDir / "ui" / "ui.js");
            uiFile << UI_JS_TEMPLATE;
            
            std::ofstream metaFile(baseDir / "meta.json");
            std::string metaContent = META_JSON_TEMPLATE;
            size_t pos = metaContent.find("{NAME}");
            if (pos != std::string::npos) metaContent.replace(pos, 6, name);
            metaFile << metaContent;
            std::cout << "Widget initialized from default boilerplate." << std::endl;
        }

        std::cout << "Widget '" << name << "' initialized successfully at " << baseDir << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return false;
    }
}

bool RunWidget() {
    fs::path widgetPath = fs::current_path();
    if (!fs::exists(widgetPath / "index.js")) {
        std::cerr << "Error: Could not find index.js in current directory." << std::endl;
        return false;
    }

    fs::path exeDir = GetExeDir();
    fs::path novadeskExe = exeDir.parent_path() / NOVADESK_EXE;
    if (!fs::exists(novadeskExe)) {
        // Fallback for dev environment
        novadeskExe = exeDir.parent_path().parent_path() / NOVADESK_EXE;
    }

    if (!fs::exists(novadeskExe)) {
        std::cerr << "Error: Novadesk.exe not found." << std::endl;
        return false;
    }

    fs::path scriptPath = widgetPath / "index.js";
    std::string command = "\"" + novadeskExe.string() + "\" \"" + scriptPath.string() + "\"";
    std::cout << "Running: " << command << std::endl;
    
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    if (CreateProcessA(NULL, (LPSTR)command.c_str(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        std::cout << "Waiting for process to exit..." << std::endl;
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return true;
    } else {
        std::cerr << "Error: Failed to launch Novadesk.exe" << std::endl;
        return false;
    }
}

bool BuildWidget() {
    fs::path widgetPath = fs::current_path();
    fs::path metaPath = widgetPath / "meta.json";
    if (!fs::exists(metaPath)) {
        std::cerr << "Error: meta.json not found in current directory." << std::endl;
        return false;
    }

    // Basic JSON parsing
    std::ifstream metaFile(metaPath);
    std::string line, content;
    while (std::getline(metaFile, line)) content += line;

    auto getValue = [&](const std::string& key) {
        size_t pos = content.find("\"" + key + "\"");
        if (pos == std::string::npos) return std::string("");
        size_t start = content.find("\"", pos + key.length() + 2) + 1;
        size_t end = content.find("\"", start);
        return content.substr(start, end - start);
    };

    std::string widgetRealName = getValue("name");
    std::string version = getValue("version");
    std::string icon = getValue("icon");
    std::string author = getValue("author");
    std::string description = getValue("description");

    bool missing = false;
    if (widgetRealName.empty()) { std::cerr << "Error: 'name' is missing in meta.json" << std::endl; missing = true; }
    if (version.empty()) { std::cerr << "Error: 'version' is missing in meta.json" << std::endl; missing = true; }
    if (icon.empty()) { std::cerr << "Error: 'icon' is missing in meta.json" << std::endl; missing = true; }
    if (author.empty()) { std::cerr << "Error: 'author' is missing in meta.json" << std::endl; missing = true; }
    if (description.empty()) { std::cerr << "Error: 'description' is missing in meta.json" << std::endl; missing = true; }
    
    if (missing) return false;

    try {
        fs::path distDir = widgetPath / "dist";
        if (fs::exists(distDir)) fs::remove_all(distDir);
        fs::create_directories(distDir);

        fs::path exeDir = GetExeDir();
        fs::path srcExe = exeDir.parent_path() / NOVADESK_EXE;
        if (!fs::exists(srcExe)) {
             srcExe = exeDir.parent_path().parent_path() / NOVADESK_EXE;
        }

        fs::path destExe = distDir / (widgetRealName + ".exe");
        fs::copy_file(srcExe, destExe, fs::copy_options::overwrite_existing);

        fs::copy_file(widgetPath / "index.js", distDir / "index.js", fs::copy_options::overwrite_existing);
        if (fs::exists(widgetPath / "ui")) {
            fs::copy(widgetPath / "ui", distDir / "ui", fs::copy_options::recursive);
        }
        if (fs::exists(widgetPath / "assets")) {
            fs::copy(widgetPath / "assets", distDir / "assets", fs::copy_options::recursive);
        }

        fs::path rceditPath = exeDir / "rcedit-x86.exe";
        if (fs::exists(rceditPath)) {
            std::string rceditCmd = "\"" + rceditPath.string() + "\" \"" + destExe.string() + "\"";
            rceditCmd += " --set-file-version " + version + " --set-product-version " + version;
            rceditCmd += " --set-version-string \"CompanyName\" \"" + author + "\"";
            rceditCmd += " --set-version-string \"FileDescription\" \"" + description + "\"";
            rceditCmd += " --set-version-string \"ProductName\" \"" + widgetRealName + "\"";

            if (!icon.empty()) {
                fs::path iconPath = widgetPath / icon;
                if (fs::exists(iconPath)) {
                    rceditCmd += " --set-icon \"" + iconPath.string() + "\"";
                }
            }
            
            std::cout << "Applying metadata via rcedit..." << std::endl;
            if (!ExecuteCommand(rceditCmd)) {
                std::cerr << "Warning: rcedit failed with some errors." << std::endl;
            }
        } else {
            std::cout << "Warning: rcedit-x86.exe not found. Skipping metadata." << std::endl;
        }

        std::cout << "Successfully built widget in " << distDir << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Build Error: " << e.what() << std::endl;
        return false;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        PrintUsage();
        return 1;
    }

    std::string command = argv[1];

    if (command == "init") {
        std::string widgetName = (argc > 2) ? argv[2] : "";
        return InitWidget(widgetName) ? 0 : 1;
    } else if (command == "run") {
        return RunWidget() ? 0 : 1;
    } else if (command == "build") {
        return BuildWidget() ? 0 : 1;
    } else {
        PrintUsage();
        return 1;
    }
}
