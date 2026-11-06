#include "Program.h"


// Constructor / destructor
Program::Program() : gui(this), fileIO(this),audio(this){

    // set working dir
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    string dirPath = std::filesystem::path(buffer).parent_path().string();
    SetCurrentDirectoryA(dirPath.c_str());

    ImGui::GetIO().Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\arialbd.ttf", 16.0f);
}

void Program::OpenFile(bool openNewDialog, bool folder, string extFileLoad) {
        

}

Program::~Program() {
    
    Logger::DestroyLogger();
}
