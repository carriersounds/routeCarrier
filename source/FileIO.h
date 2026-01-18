#pragma once
#include "mainHeader.hpp"
#include <commdlg.h>

class Program;

class FileIO {

public:

    FileIO(Program* input);

    // struct setup in separate file
    // saveConfig(Setup& currentSetup);
    // Setup loadConfig();
    // maybe give it access to fileIO and build whole structure there

    // offline saving and loading
    void selectFile(bool folder, int recursive, bool openDialog = true, string extFileLoad = "");
    void manualSaveFile(bool as);
    void saveTextFile(string path);

    string OpenFolderDialog();
    string OpenFileDialog(int filetype = f_image);
    string saveFileDialog(int filetype = f_image);

    std::filesystem::path getEXEDir();
    std::filesystem::path EXEpath;
    std::filesystem::path saveDir;

    int imgCount;
    int currentImage;
    vector<string> allFilesInScope;

    string lastSavedFile;

    std::map<string, iconData> icons;      // LRI ICONS

private:
    Program* prog;
    static vector<string> glob(const std::string& foldername, bool recursive);
    vector<string> openAndLoadFiles(bool recursive, bool folder, bool openDialog, string extFileLoad);
    void loadTextFile(string imagepath);

};