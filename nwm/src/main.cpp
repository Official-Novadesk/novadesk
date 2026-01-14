#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <windows.h>
#include "rescle.h"

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

// Helper for string conversion
std::wstring ToWString(const std::string& str) {
    if (str.empty()) return L"";
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

// Helper to parse version string "1.0.0.0" into 4 shorts
bool ParseVersion(const std::string& version, unsigned short& v1, unsigned short& v2, unsigned short& v3, unsigned short& v4) {
    v1 = v2 = v3 = v4 = 0;
    std::vector<int> parts;
    std::string current;
    for (char c : version) {
        if (c == '.') {
            if (!current.empty()) parts.push_back(std::stoi(current));
            current.clear();
        } else if (isdigit(c)) {
            current += c;
        }
    }
    if (!current.empty()) parts.push_back(std::stoi(current));
    
    if (parts.size() >= 1) v1 = parts[0];
    if (parts.size() >= 2) v2 = parts[1];
    if (parts.size() >= 3) v3 = parts[2];
    if (parts.size() >= 4) v4 = parts[3];
    return parts.size() > 0;
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
            std::cerr << "Error: Widget template folder not found at " << templateDir << std::endl;
            return false;
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

        fs::path widgetsSubDir = distDir / "Widgets";
        fs::create_directories(widgetsSubDir);

        for (const auto& entry : fs::directory_iterator(widgetPath)) {
            const auto& path = entry.path();
            std::string filename = path.filename().string();

            if (filename == "dist") continue;

            if (fs::is_directory(path)) {
                fs::copy(path, widgetsSubDir / filename, fs::copy_options::recursive);
            } else if (filename != (widgetRealName + ".exe")) {
                fs::copy_file(path, widgetsSubDir / filename, fs::copy_options::overwrite_existing);
            }
        }

        std::cout << "Applying metadata via internal rescle..." << std::endl;
        rescle::ResourceUpdater updater;
        if (updater.Load(destExe.c_str())) {
            updater.SetVersionString(RU_VS_PRODUCT_NAME, ToWString(widgetRealName).c_str());
            updater.SetVersionString(RU_VS_COMPANY_NAME, ToWString(author).c_str());
            updater.SetVersionString(RU_VS_FILE_DESCRIPTION, ToWString(description).c_str());
            updater.SetVersionString(RU_VS_FILE_VERSION, ToWString(version).c_str());
            updater.SetVersionString(RU_VS_PRODUCT_VERSION, ToWString(version).c_str());

            unsigned short v1, v2, v3, v4;
            if (ParseVersion(version, v1, v2, v3, v4)) {
                updater.SetFileVersion(v1, v2, v3, v4);
                updater.SetProductVersion(v1, v2, v3, v4);
            }

            if (!icon.empty()) {
                fs::path iconPath = widgetPath / icon;
                if (fs::exists(iconPath)) {
                    updater.SetIcon(iconPath.c_str());
                }
            }

            if (!updater.Commit()) {
                std::cerr << "Error: Failed to commit metadata updates via rescle." << std::endl;
                return false;
            }
        } else {
            std::cerr << "Error: Failed to load executable for metadata update." << std::endl;
            return false;
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
