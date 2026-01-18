#ifndef PROGRAMH
#define PROGRAMH

#define JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED 1


#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")             // disable console window
#include "mainHeader.hpp"
#include "AudioEngine.h"
#include "GUI.h"
#include "FileIO.h"
#include "Logger.h"


class Program {
public:

    Program();
    ~Program();

    void selectFile(bool openNewDialog, bool folder = 0, int action = 0, int recursive = 0, string extFileLoad = "");
   
    GUI gui;            // Main GUI functions. Node GUIs are implemented per-element
    AppLog appLog;	    // internal log, can be accessed with Alt+L
    FileIO fileIO;      // presets & audio files
    AudioEngine audio;  // audio routing 

};



#endif