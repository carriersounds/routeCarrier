#include "Program.h"


// Constructor / destructor
Program::Program() : gui(this), fileIO(this),audio(this){

    // set working dir
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    string dirPath = std::filesystem::path(buffer).parent_path().string();
    SetCurrentDirectoryA(dirPath.c_str());

    
    string fontpath = (fileIO.EXEpath / "assets" / "Roboto-Black.ttf").string();
    ImGui::GetIO().Fonts->AddFontFromFileTTF(fontpath.c_str(), 16.0f);

}


void Program::selectFile(bool openNewDialog, bool folder, int action, int recursive, string extFileLoad) {

    if (openNewDialog == f_openDialog) fileIO.selectFile(folder, recursive);

    if (extFileLoad != "")  fileIO.selectFile(f_openFile, 0, false, extFileLoad);

    if (action != f_current) {
        if (action == f_next) fileIO.currentImage--;
        if (action == f_previous) fileIO.currentImage++;
    }

    if (!fileIO.imgCount || !fileIO.allFilesInScope.size()) return;  // don't merge if there's no images loaded

    //fileIO.mergeImageAndDefects();              // actual pixel loading happens here, merge with .vvr (blob) data  
   // fileIO.sendToProgram(reProcessLoadedImage); // 1 = send to procThread, 0 = send to GUI only

}

Program::~Program() {
    
    Logger::DestroyLogger();
}
