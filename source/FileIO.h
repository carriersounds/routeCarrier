#pragma once
#include "mainHeader.hpp"
#include <commdlg.h>

class Program;
class AudioNode;

class FileIO {

public:

    FileIO(Program* input);

    // struct setup in separate file
    // saveConfig(Setup& currentSetup);
    // Setup loadConfig();
    // maybe give it access to fileIO and build whole structure there

    // offline saving and loading
    void selectFile(bool folder, int recursive, bool openDialog = true, string extFileLoad = "");

    void manualSaveFile(bool as);   // saving WAVs


    void savePreset(string path);     // use for PRESETS
    void loadPreset(string imagepath);

    string OpenFolderDialog();
    string OpenFileDialog(int filetype = f_audio);
    string saveFileDialog(int filetype = f_audio);

    std::filesystem::path getEXEDir();
    std::filesystem::path EXEpath;
    std::filesystem::path saveDir;

    int amtAudioFilesLoaded;
    int currentAudioFile;
    vector<string> allFilesInScope;

    string lastSavedAudioFile;

    std::map<string, iconData> icons;      // LRI ICONS

private:
    Program* prog;
    static vector<string> glob(const std::string& foldername, bool recursive);
    vector<string> openAndLoadFiles(bool recursive, bool folder, bool openDialog, string extFileLoad);      // probably use for loading multiple audio files for soundboard


};
