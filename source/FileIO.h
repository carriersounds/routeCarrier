#pragma once
#include "mainHeader.hpp"
#include <commdlg.h>

class Program;

class Preset {

    // nodes
    // links
    // settings within dsp / device node (gain)
    // missing devices
    // message if the preset is from an older/newer version
    // text or bytecode representation function built-in, easy input for fileIO


    // queue preset representation in audioEngine "exportToPresetQueue()"


    // no pls just compile but make id creation happen in runtime,
    // how to set ID without making uniqueID
    // 
    // what do I need to create? on creation, queue action.. or look at the current mixer state and build that with some algorithm

    // parse first, then allocate
    // what to save? actions... ID's handle themselves

    // node location????
    // order in preset: device nodes first, then dsp nodes, then links, finally... settings 

    // topologically sorted




    //const char* preset = "hw=1004,x,y";
    //const char* preset = "hw=1008,x,y";
    //const char* prezet = "fx=1002,x,y";
    //const char* prozxt = "fx=1002,x,y";
    //const char* preset = "hw=1004,x,y";
    //const char* preset = "hw=1008,x,y";
    //const char* prezet = "lk=1002,pin1,pin2";
    //const char* prozxt = "lk=1002,pin1,pin2";
    //const char* preset = "lk=1004,pin1,pin2";
    //const char* preset = "lk=1008,pin1,pin2"; // will always sort topologically
    //
    //const char* preset = "lastID=1008"; //finally set newID to this, so it is above all loaded things (prevent ID clash)

    // algorithm, keeping original ID's (different ID order would make ID debugging a living hell)
};


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