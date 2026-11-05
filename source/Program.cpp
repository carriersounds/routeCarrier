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




Program::~Program() {
    
    Logger::DestroyLogger();
}
