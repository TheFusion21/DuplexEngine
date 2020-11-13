
// EXTERNAL INCLUDES
#if defined(_WINDOWS)
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#endif
#include <fstream>
#include <iostream>
// INTERNAL INCLUDES
#include "editor.h"

using namespace DUPLEX_NS_EDITOR;
using namespace DUPLEX_NS_WINDOW;
using namespace DUPLEX_NS_CONFIG;
using namespace DUPLEX_NS_MATH;
using namespace DUPLEX_NS_UTIL;

bool Editor::InitEditor(std::string file)
{
    return true;
}

bool Editor::InitProjectExplorer()
{
    WindowCreateInfo wci = {};
    wci.windowName = "Duplex Project Hub";
    wci.position = defaultPosition;
    wci.resolution = Vec2Int(800, 500);
    if (windowManager.AddWindow(&wci, &window) != DRESULT::OK)
        return false;

    return true;
}

bool Editor::Init(std::string file)
{
    std::fstream configFile(configFileName, std::fstream::in);
    if (!configFile.good())
    {
        config = defaultConfig;
    }
    else
    {
        configFile.close();
    }
    if (file.empty())
        return InitProjectExplorer();
    return InitEditor(file);
}
void Editor::Run()
{
    while (!window->shouldClose)
    {
        windowManager.PollForEvents();
    }
}
void Editor::Shutdown()
{
    // Save config file
    std::fstream configFile(configFileName, std::fstream::out);
    if (configFile.good())
    {
        configFile << "[General]" << std::endl;
        configFile << "lastProjects=(";
        for (int i = 0;i<(int)config.lastProjectFiles.size();i++)
        {
            configFile << config.lastProjectFiles[i];
            if (i < ((int)config.lastProjectFiles.size()) - 1)
                configFile << ",";
        }
        configFile << ")" << std::endl;

        configFile.close();
    }
    windowManager.Shutdown();
}

std::string Editor::OpenFileDialog(std::string extFilter)
{
#if defined(_WINDOWS)
    char filename[MAX_PATH];
    OPENFILENAME ofn;
    ZeroMemory(&filename, sizeof(filename));
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;  // If you have a window to center over, put its HANDLE here
    ofn.lpstrFilter = extFilter.c_str();
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = "Select a project file to open";
    ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST;

    if (GetOpenFileNameA(&ofn))
    {
        return std::string(filename);
    }
#elif defined(_LINUX)

#endif
    return std::string();
}
