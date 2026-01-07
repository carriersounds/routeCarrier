#include "GUI.h"
#include "Program.h"

#define tRight ImGui::TableNextColumn()
#define tDown ImGui::TableNextRow()

GUI::GUI(Program* prog) : prog(prog){

    imGuiSetup();   // calls d11::setup

    node_Context = node::CreateEditor();

    timestamp = __DATE__;
    timestamp += ", ";
    timestamp += __TIME__;

}

GUI::~GUI() {

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    ImPlot::DestroyContext();
    GUI::d11.CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

}

int GUI::imGuiSetup() {

    // 1900 x 1080 = full
    wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"Carrier Example Program", nullptr }; ::RegisterClassExW(&wc);
    hwnd = ::CreateWindowW(wc.lpszClassName, L"Carrier ImGui Template", WS_CAPTION | WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1260, 720, nullptr, nullptr, wc.hInstance, nullptr);   // maximize in imguisetup 
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();

    io = &ImGui::GetIO();

    if (!d11.CreateDeviceD3D(hwnd)) {
        d11.CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ShowWindow(hwnd, SW_NORMAL);
    UpdateWindow(hwnd);


    io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io->ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
    io->ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();


    static ImPlotColormap ranges = -1;
    if (ranges == -1) {
        static const ImU32 Liars_Data[3] = { 4282515870, 4294945280, 4294921472 };
        ranges = ImPlot::AddColormap("ranges", Liars_Data, 3);
    }


    d11.setup(hwnd);        // Setup Platform/Renderer backends


    ImVec4* colors = style.Colors;

    colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.37f, 0.16, 0.54f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.26f, 0.98f, 0.40f, 0.40f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.98f, 0.42f, 0.67f);

    // green
    // colors[ImGuiCol_Button] = ImVec4(0.15f, 0.48f, 0.20f, 0.96f);
    // colors[ImGuiCol_ButtonHovered] = ImVec4(0.20f, 0.66f, 0.17f, 1.00f);
    // colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.98f, 0.23f, 1.00f);

     // gray
   // colors[ImGuiCol_Button] = ImVec4(0.38f, 0.38f, 0.38f, 0.70f);
   // colors[ImGuiCol_ButtonHovered] = ImVec4(0.55f, 0.65f, 0.54f, 1.00f);
   // colors[ImGuiCol_ButtonActive] = ImVec4(0.56f, 1.00f, 0.64f, 1.00f);
   // colors[ImGuiCol_Header] = ImVec4(0.26f, 0.98f, 0.46f, 0.31f);
   //
   // colors[ImGuiCol_TabHovered] = ImVec4(0.42f, 0.65f, 0.44f, 0.80f);
   // colors[ImGuiCol_Tab] = ImVec4(0.19f, 0.31f, 0.22f, 0.97f);
   // colors[ImGuiCol_TabSelected] = ImVec4(0.04f, 0.56f, 0.05f, 1.00f);
   // colors[ImGuiCol_TabDimmedSelected] = ImVec4(0.11f, 0.36f, 0.13f, 1.00f);
   // colors[ImGuiCol_TabSelectedOverline] = ImVec4(0.04f, 0.56f, 0.05f, 1.00f);
   // colors[ImGuiCol_TabDimmed] = ImVec4(0.07f, 0.15f, 0.07f, 0.97f);
   //
   //
   // colors[ImGuiCol_DockingPreview] = ImVec4(0.26f, 0.98f, 0.34f, 0.70f);
   // colors[ImGuiCol_SliderGrab] = ImVec4(0.28f, 0.61f, 0.25f, 1.00f);
   // colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.09f, 0.06f, 1.00f);
   // colors[ImGuiCol_TitleBgActive] = ImVec4(0.08f, 0.35f, 0.14f, 1.00f);

    style.WindowRounding = 6.0f;

    return 0;
}

void GUI::setViewport(UI viewport) {

    if (this->viewport == viewport) return;     // don't change when set to same thing

    switch (viewport)
    {
    case UI::fullscreen_admin:
    {
        // 1. Get current style
        LONG style = GetWindowLongPtr(hwnd, GWL_STYLE);

        // 2. Set back to old/default layout
        style = WS_CAPTION | WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT;

        // 3. Set the new style
        SetWindowLongPtr(hwnd, GWL_STYLE, style);

        // NORMAL SIZE
        SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 1920, 1080, SWP_SHOWWINDOW);
       // SetForegroundWindow(hwnd);  // forced top somehow?
        ShowWindow(hwnd, SW_MAXIMIZE);
        UpdateWindow(hwnd);
    }
    break;
    case UI::hide:
        SetWindowPos(hwnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_HIDEWINDOW);
        SetForegroundWindow(LRI_hwnd);
        break;
    case UI::unhide:
        SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);
        ShowWindow(hwnd, SW_SHOW);
        break;
    case UI::unlock:
        SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        break;
    case UI::lock:
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
        SetForegroundWindow(hwnd); // get focus
        ShowWindow(hwnd, SW_SHOW);
        break;
    default:
        break;
    }

    this->viewport = viewport;

    //helper windows functions, could be useful

   // DWORD dwCurrentThread = GetCurrentThreadId();
   // DWORD dwFGThread = GetWindowThreadProcessId(GetForegroundWindow(), NULL);
   // OpenIcon(prog->main_hwnd);
   // AttachThreadInput(dwCurrentThread, dwFGThread, TRUE);
   // SetForegroundWindow(prog->main_hwnd);
   // SetCapture(prog->main_hwnd);
   // SetFocus(prog->main_hwnd);
   // SetActiveWindow(prog->main_hwnd);
   // EnableWindow(prog->main_hwnd, TRUE);
   // AttachThreadInput(dwCurrentThread, dwFGThread, FALSE);
   // prog->calibrationMode = true;
}


// RENDERING FUNCTIONS

void GUI::renderAllModules() {

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();


    // ImGui Rendering functions start
    renderMainDockSpace();
    renderMixPanel();
    renderMenuBar();
    renderLog();
 
    renderGraph();
  //  renderPreview();                    // LOCKED/ NO MOVE

    if (showDemos) {
        ImGui::ShowDemoWindow(&showDemos);
        ImPlot::ShowDemoWindow(&showDemos);
    }

}

void GUI::renderMainDockSpace() {
    ImGui::Begin("Main DockSpace", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoDocking);
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetWindowPos(viewport->WorkPos);
    ImGui::SetWindowSize(viewport->WorkSize);
    ImGuiID dockspace_id = ImGui::GetID("MainDockspace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
    ImGui::End();

}

void GUI::renderMenuBar() {

    int frameNumber = ImGui::GetFrameCount();
    static bool keyCommands = 0;
    static int recursiveLoad = 0;

    // Hotkeys --------------------

    if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow));
    if (ImGui::IsKeyPressed(ImGuiKey_RightArrow));

    if (ImGui::IsKeyDown(ImGuiMod_Ctrl) && ImGui::IsKeyReleased(ImGuiKey_O));
    if (ImGui::IsKeyDown(ImGuiMod_Ctrl) && ImGui::IsKeyReleased(ImGuiKey_F));
    if (ImGui::IsKeyDown(ImGuiMod_Ctrl) && ImGui::IsKeyDown(ImGuiMod_Shift) && ImGui::IsKeyReleased(ImGuiKey_F));
    if (ImGui::IsKeyDown(ImGuiMod_Ctrl) && ImGui::IsKeyReleased(ImGuiKey_L));
    if (ImGui::IsKeyDown(ImGuiMod_Ctrl) && ImGui::IsKeyReleased(ImGuiKey_S));
    if (ImGui::IsKeyDown(ImGuiMod_Ctrl) && ImGui::IsKeyDown(ImGuiMod_Shift) && ImGui::IsKeyReleased(ImGuiKey_S));

    // view modifiers

    
    if (ImGui::IsKeyDown(ImGuiMod_Alt) && ImGui::IsKeyReleased(ImGuiKey_L)) showLog = !showLog;
    if (ImGui::IsKeyDown(ImGuiMod_Alt) && ImGui::IsKeyReleased(ImGuiKey_T)) showTimings = !showTimings;

    if (ImGui::IsKeyPressed(ImGuiKey_F9)) setViewport(UI::hide);
    if (ImGui::IsKeyPressed(ImGuiKey_F10)) setViewport(UI::hide);

    if (ImGui::IsKeyReleased(ImGuiKey_Space)) {

    }

    

    //\ Hotkeys -----------------------


    if (ImGui::BeginMainMenuBar()) {

        if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Open Image", "Ctrl + O")) prog->selectFile(f_openDialog, f_openFile, f_current, recursiveLoad);
                ImGui::SetItemTooltip("Load and view image");
                if (ImGui::MenuItem("Open Folder", "Ctrl + F")) prog->selectFile(f_openDialog, f_openFolder, f_current, recursiveLoad);
                ImGui::SetItemTooltip("Load and view image");
                ImGui::Separator();

                if (ImGui::BeginMenu("Folder load behaviour")) {
                    ImGui::RadioButton("Load current folder only", &recursiveLoad, 0);
                    ImGui::RadioButton("Load sub-folders recursively", &recursiveLoad, 1);
                    ImGui::EndMenu();
                }

                ImGui::Separator();
                if (ImGui::MenuItem("Save Current Image", "Ctrl + S")) prog->fileIO.manualSaveFile(0);
                if (ImGui::MenuItem("Save as...", "Ctrl + Shift + S")) prog->fileIO.manualSaveFile(1);
                ImGui::SetItemTooltip("Save currently shown image\nSelect View > Binarized to save thresholded image");

                if (ImGui::BeginMenu("Default save directory")) {

                    static std::filesystem::path dirs[] = { prog->fileIO.getEXEDir().parent_path().parent_path() / "Recordings\\","D:\\LRI\\Vision\\" };
                    static int dirIndex = 0;

                    const char* save_directories[] =
                    {
                        "\\Recordings\\", "D:\\LRI\\Vision\\", "Custom folder"
                    };

                    static char customSave[MAX_PATH];
                    char hint[] = "Paste or type custom folder";

                    if (ImGui::Combo("Select", &dirIndex, save_directories, IM_ARRAYSIZE(save_directories), IM_ARRAYSIZE(save_directories))
                        || ImGui::InputTextWithHint("Custom", hint, customSave, MAX_PATH,
                            ImGuiInputTextFlags_EnterReturnsTrue |
                            ImGuiInputTextFlags_ElideLeft)) {

                        switch (dirIndex)
                        {
                        case 0:
                            prog->fileIO.saveDir = prog->fileIO.getEXEDir() / "assets\\Recordings\\";
                            break;
                        case 1:
                            prog->fileIO.saveDir = "D:\\LRI\\Vision\\";
                            break;
                        case 2:
                            prog->fileIO.saveDir = customSave;
                            break;
                        default:
                            break;
                        }

                    }

                    ImGui::EndMenu();
                }



                ImGui::Separator();

                ImGui::EndMenu();
            }
        
        if (ImGui::BeginMenu("Audio")) {

                static auto& deviceTypes = prog->audio.nullDevice.deviceManager.getAvailableDeviceTypes();
                static juce::AudioIODeviceType* type = deviceTypes.getFirst();                                      // takes WASAPI, maybe check for Low Latency?

                static juce::StringArray inputs;
                static juce::StringArray outputs;

                // update audio device list every 2 seconds

                if ((ImGui::GetFrameCount() % 120) == 0) {
                    type->scanForDevices();                 // must call to populate names
                    inputs = type->getDeviceNames(true);    // true = input
                    outputs = type->getDeviceNames(false);  // false = output
                }

                ImGui::MenuItem("Inputs");
                ImGui::Separator();
                
                for (auto& s : inputs) {
                    if (ImGui::MenuItem(("IN: " + s.toStdString()).c_str())) {
                        juce::String chosenInput(s.getCharPointer());
                        prog->audio.selectDevice(chosenInput, false);
                    }
                }                
                
                ImGui::Separator();
                
                ImGui::MenuItem("Outputs");
                ImGui::Separator();
                
                for (auto& s : outputs) {
                    if (ImGui::MenuItem(("OUT: " + s.toStdString()).c_str())) {
                        juce::String chosenOutput(s.getCharPointer());
                        prog->audio.selectDevice(chosenOutput, true);
                    }

                }
                

                if (ImGui::BeginMenu("Version Information")) {

                    ImGui::EndMenu();
                }

                ImGui::EndMenu();
            }
        
        if (ImGui::BeginMenu("View")) {
            
            if (ImGui::MenuItem("Keyboard Shortcuts")) keyCommands = true;
            ImGui::Separator();

            if (ImGui::BeginMenu("Show Panels")) {
                ImGui::Checkbox("Process Timers", &showTimings);
                ImGui::Checkbox("Log window", &showLog);
                ImGui::Checkbox("Demo Windows", &showDemos);
                ImGui::Checkbox("Mixer Panel", &showMixer);
                

                ImGui::EndMenu();
            }
            ImGui::Separator();

            if (ImGui::BeginMenu("Viewport")) {
                if (ImGui::MenuItem("Hidden")) setViewport(UI::hide);
                if (ImGui::MenuItem("Unlock from top")) setViewport(UI::unlock);
                if (ImGui::MenuItem("Always on top")) setViewport(UI::lock);
                if (ImGui::MenuItem("Fullscreen")) setViewport(UI::fullscreen_admin);

                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("About")) {

                if (ImGui::BeginMenu("Version Information")) {

                    ImGui::Text("routeCarrier Virtual Audio Mixer");
#ifdef _DEBUG
                    ImGui::Text("Debug Build");
#else
                    ImGui::Text("Release Build");
#endif

                    ImGui::Text("Compiled on: %s", timestamp.data());
                    ImGui::Text("ImGui version: %s", ImGui::GetVersion());

                    ImGui::EndMenu();
                }


                ImGui::EndMenu();
            }

        if (keyCommands) {

                ImGui::Begin("Keyboard Shortcuts", &keyCommands);
                ImGui::BeginTable("Shortcutss", 2);
                ImGui::TableNextRow();
                ImGui::TableNextColumn();

                ImGui::Text("Open Image"); ImGui::TableNextColumn();
                ImGui::Text("Ctrl + O"); ImGui::TableNextRow(); ImGui::TableNextColumn();
                ImGui::Text("Open Folder"); ImGui::TableNextColumn();
                ImGui::Text("Ctrl + F"); ImGui::TableNextRow(); ImGui::TableNextColumn();
                ImGui::Text("Open Folder recursively"); ImGui::TableNextColumn();
                ImGui::Text("Ctrl + Shift + F"); ImGui::TableNextRow(); ImGui::TableNextColumn();

                ImGui::Text("Goto Last Image");  ImGui::TableNextColumn();
                ImGui::Text("Left Arrow"); ImGui::TableNextRow(); ImGui::TableNextColumn();
                ImGui::Text("Goto Next Image"); ImGui::TableNextColumn();
                ImGui::Text("Right Arrow"); ImGui::TableNextRow(); ImGui::TableNextColumn();

                ImGui::Text("Save Current Image"); ImGui::TableNextColumn();
                ImGui::Text("Ctrl + S"); ImGui::TableNextRow(); ImGui::TableNextColumn();
                ImGui::Text("Save Image As..."); ImGui::TableNextColumn();
                ImGui::Text("Ctrl + Shift + S"); ImGui::TableNextRow(); ImGui::TableNextColumn();

                ImGui::Separator();
                ImGui::Text("Start/stop Cam"); ImGui::TableNextColumn();
                ImGui::Separator();
                ImGui::Text("Spacebar"); ImGui::TableNextRow(); ImGui::TableNextColumn();
                ImGui::Text("Next Image Trig"); ImGui::TableNextColumn();
                ImGui::Text("Spacebar"); ImGui::TableNextRow(); ImGui::TableNextColumn();
                ImGui::Text("Toggle Light"); ImGui::TableNextColumn();
                ImGui::Text("Ctrl + L"); ImGui::TableNextRow(); ImGui::TableNextColumn();

                ImGui::Text("Image zoom"); ImGui::TableNextColumn();
                ImGui::Text("Scrollwheel"); ImGui::TableNextRow(); ImGui::TableNextColumn();
                ImGui::Text("Image drag"); ImGui::TableNextColumn();
                ImGui::Text("Left mouse btn"); ImGui::TableNextRow(); ImGui::TableNextColumn();
                ImGui::Text("Image viewer menu"); ImGui::TableNextColumn();
                ImGui::Text("Right mouse btn"); ImGui::TableNextRow(); ImGui::TableNextColumn();

                ImGui::Separator();
                ImGui::Text("Toggle Calibration mode"); ImGui::TableNextColumn();
                ImGui::Separator();
                ImGui::Text("Alt + C"); ImGui::TableNextRow(); ImGui::TableNextColumn();
                ImGui::Text("Toggle Settings menu"); ImGui::TableNextColumn();
                ImGui::Text("Alt + S"); ImGui::TableNextRow(); ImGui::TableNextColumn();
                ImGui::Text("Variable Management"); ImGui::TableNextColumn();
                ImGui::Text("Alt + V"); ImGui::TableNextRow(); ImGui::TableNextColumn();
                ImGui::Text("Toggle timer view"); ImGui::TableNextColumn();
                ImGui::Text("Alt + T"); ImGui::TableNextRow(); ImGui::TableNextColumn();
                ImGui::Text("Toggle log window"); ImGui::TableNextColumn();
                ImGui::Text("Alt + L"); ImGui::TableNextRow(); ImGui::TableNextColumn();



                ImGui::EndTable();
                ImGui::End();
            }

        ImGui::EndMainMenuBar();
    }
    
}

void GUI::renderLog()
{
    if (!showLog) return;
    prog->appLog.Draw("Log", &showLog);
}

void GUI::renderMixPanel() {

    
    static auto& deviceTypes = prog->audio.nullDevice.deviceManager.getAvailableDeviceTypes();
    static juce::AudioIODeviceType* type = deviceTypes.getFirst();                                      // takes WASAPI, maybe check for Low Latency?

    static juce::StringArray inputs;
    static juce::StringArray outputs;

    
   ImGui::Begin("Mixing Panel", &showMixer, ImGuiWindowFlags_NoNavInputs);

    node::SetCurrentEditor(node_Context);
    node::Begin("editore", ImVec2(0.0, 0.0f));


    static float pinSize = 12.0f;
    static float spacing = 4.0f;

    // draw input nodes
    for (auto& inputDevice : prog->audio.hardwareBlocks) {

        if (!inputDevice.second->isInput()) continue;

            node::BeginNode(inputDevice.first);
            ImGui::Text(inputDevice.second->getName().c_str());

            // Custom divider 
            ImVec2 p = ImGui::GetCursorScreenPos();
            float w = node::GetNodeSize(inputDevice.first).x - pinSize - spacing - spacing;
            ImGui::GetWindowDrawList()->AddLine(
                p,
                ImVec2(p.x + w, p.y),
                IM_COL32(120, 120, 120, 255)
            );
            ImGui::Dummy(ImVec2(0, 6)); // spacing after divider
            ImGui::NewLine();


            // PIN         
            const char* label = "From input  > ";
            ImVec2 textSize = ImGui::CalcTextSize(label);
            float avail = node::GetNodeSize(inputDevice.first).x - spacing;
            // Move cursor to the right edge
            ImGui::SetCursorPosX(
               ImGui::GetCursorPosX() + avail - textSize.x - (2*(pinSize + spacing))
            );

            node::BeginPin(inputDevice.second->outputPin, ed::PinKind::Output);  // create N input pins
            ImGui::TextUnformatted(label);
            ImGui::SameLine();
            // Pin icon
            ImDrawList* dl = ImGui::GetWindowDrawList();
            ImVec2 pos = ImGui::GetCursorScreenPos();
            dl->AddCircleFilled(
                ImVec2(pos.x, pos.y + textSize.y * 0.5f),
                textSize.y * 0.5f,
                IM_COL32(200, 200, 200, 255)
            );
            node::EndPin();
            node::EndNode();

        

    }

    // draw output nodes
    for (auto& outputDevice : prog->audio.hardwareBlocks) {

        if (!outputDevice.second->isOutput()) continue;

        node::BeginNode(outputDevice.first);
        ImGui::Text(outputDevice.second->getName().c_str());


        // Custom divider 
        ImVec2 p = ImGui::GetCursorScreenPos();
        float w = node::GetNodeSize(outputDevice.first).x - pinSize - spacing - spacing;
        ImGui::GetWindowDrawList()->AddLine(
            p,
            ImVec2(p.x + w, p.y),
            IM_COL32(120, 120, 120, 255)
        );
        ImGui::Dummy(ImVec2(0, 6)); // spacing after divider
        ImGui::NewLine();

        // PIN     
        const char* label = "   >  To Output";     
        ImVec2 textSize = ImGui::CalcTextSize(label);        
        node::BeginPin(outputDevice.second->inputPin, ed::PinKind::Input);  // create N input pins             
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + spacing * 2);
        
        // Pin icon
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 pos = ImGui::GetCursorScreenPos();
        dl->AddCircleFilled(
            ImVec2(pos.x, pos.y + textSize.y * 0.5f),
            textSize.y * 0.5f,
            IM_COL32(200, 200, 200, 255)
        );
        
        ImGui::TextUnformatted(label);       
        node::EndPin();
        node::EndNode();
    }


    for (auto& link : prog->audio.links) {
        node::Link(link.second.ID, link.second.ID_left, link.second.ID_right);
    }


    if (node::BeginCreate())                                            // GRAB PIN
    {
        node::PinId startPinId, endPinId;
        if (node::QueryNewLink(&startPinId, &endPinId))                 // HOVER OVER NEXT
        {
            if (startPinId && endPinId) // both pins are valid
            {
                // Check if connection is valid for your application
                if (startPinId != endPinId) {
                    if (node::AcceptNewItem())                          // RELEASED 
                    {
                        prog->audio.createLink(startPinId, endPinId);   // Creates new link
                    }
                }
                else
                    node::RejectNewItem();                              // Shows invalid link feedback
            }
        }
    }

    if (node::BeginDelete())
    {
        node::LinkId linkId;
        if (QueryDeletedLink(&linkId)){

            prog->audio.deleteLink(linkId.Get());
            node::AcceptDeletedItem();  
        }

        node::NodeId nodeID;
        if (node::QueryDeletedNode(&nodeID)) {
            prog->audio.deleteDeviceBlock(nodeID.Get());
            node::AcceptDeletedItem();
        }        
    }

    node::EndDelete();
    node::EndCreate();
    node::End();
    node::SetCurrentEditor(nullptr);
    ImGui::End();



    ImGui::Begin("Toolbar");

    ImVec2 size = ImGui::GetWindowSize();
    const ImVec2 cursorPos = ImGui::GetCursorScreenPos();
    ImRect bb(cursorPos, cursorPos + size);
    drawGradientBackground(ImGui::GetWindowDrawList(), "Toolbar", bb, 0, 0, 0);
    
    

    ImGui::TextWrapped("alle inputs en outputs in een mooie table krijgen\ndat je ze kan drag&droppen naar een node toe als begin/eindpunt.\nook dat je met dat drag&drop simpele DSP kan gooien op een signal chain");
        // met text sorteren op virtual in/output of fysieke (laat handmatig sorteren actually). all


    
    // put some MF ICONS BRUH
    
    
    ImGui::End();
     

    ImGui::Begin("Devices");

    // update audio device list every 2 seconds or on a button press

    if (ImGui::Button("refresh list") || (ImGui::GetFrameCount() % 120) == 0) {
        type->scanForDevices();                 // must call to populate names
        inputs = type->getDeviceNames(true);    // true = input
        outputs = type->getDeviceNames(false);  // false = output
    }

    ImGui::MenuItem("Inputs");
    ImGui::Separator();

    for (auto& s : inputs) {
        if (ImGui::MenuItem(("IN: " + s.toStdString()).c_str())) {
            juce::String chosenInput(s);
            prog->audio.addNewDeviceBlock(BlockType::InputDevice,chosenInput);
        }
    }
    
    ImGui::Separator();

    ImGui::MenuItem("Outputs");
    ImGui::Separator();

    for (auto& s : outputs) {
        if (ImGui::MenuItem(("OUT: " + s.toStdString()).c_str())) {
            juce::String chosenOutput(s);
            size_t next_ID = prog->audio.addNewDeviceBlock(BlockType::OutputDevice, chosenOutput);


        }

            

    }
    



  //  if (1) {
 //     
 //   }
 
    static float gainlevel = 0.5f;

    if (ImGui::SliderFloat("Gain", &gainlevel, 0.0f, 1.0f)) {
        prog->audio.setGain(gainlevel);
    }


    if (ImGui::Button("TOGGLE AUDIO")) prog->audio.enableRouting = !prog->audio.enableRouting;


    ImGui::End();
}

void GUI::renderGraph() {

    if (!showTimings) return;

    ImGui::Begin("Process timers", &showTimings, ImGuiWindowFlags_NoFocusOnAppearing);

    float fr = ImGui::GetIO().Framerate;


    static ScrollingBuffer guiData;
    static vector<ScrollingBuffer> levels;

    if (prog->audio.levels.size() > levels.size()) levels.push_back(ScrollingBuffer());

    static float t = 0;
    t += ImGui::GetIO().DeltaTime;

    for (int i = 0; i < levels.size(); i++) {
        auto& level = levels[i];
        level.AddPoint(t,prog->audio.levels[i]);
    } 

    guiData.AddPoint(t, 1000.0 / fr);           // TODO: make option to pause graphing (halt)

    static float history = 2.0f;
    static float sleeperMicroseconds = 500;

    ImGui::SliderFloat("History", &history, 1, 30, "%.1f s");

    if (ImGui::SliderFloat("sleep in main Thread", &sleeperMicroseconds, 1, 200, "%.1f us")) {
        prog->audio.microSleep = sleeperMicroseconds;
    }


    if (ImPlot::BeginPlot("##Scrolling", ImVec2(-1, -1), ImPlotFlags_NoMouseText)) {
        ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_NoTickLabels);
      //  ImPlot::SetupAxisScale(ImAxis_Y1, ImPlotScale_Log10);
        ImPlot::SetupAxisLimits(ImAxis_X1, (double)t - history, t, ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 3000);
        ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL, 0.5f);

        for (int i = 0; i < levels.size(); i++) {

            //string type = prog->audio.hardwareBlocks.at(i)->isInput() ? " (input)" : " (output)";
            
            int actualChannel = i >> 1;
            string actualName = "chan" + to_string(i >> 1);//prog->audio.hardwareBlocks.at(actualChannel)->getName();

            string fifoInfo = (i % 2 == 0) ? " - readable from fifo" : " - writable to fifo";
            
            // 1 is readable
            // 2 is writable

            string devicePlusID = "Device " + to_string(i) + " : " + actualName + fifoInfo;
            auto& level = levels[i];
            ImPlot::PlotLine(devicePlusID.c_str(), &level.Data[0].x, &level.Data[0].y, level.Data.size(), 0, level.Offset, 2 * sizeof(float));
        }

        ImPlot::EndPlot();
    }

    ImGui::End();

}

void GUI::sendGraphicsToGPU() {

    ImGui::Render();

    const float clear_color_with_alpha[4] = { 0,0,0,255 };
    GUI::d11.g_pd3dDeviceContext->OMSetRenderTargets(1, &GUI::d11.g_mainRenderTargetView, nullptr);
    GUI::d11.g_pd3dDeviceContext->ClearRenderTargetView(GUI::d11.g_mainRenderTargetView, clear_color_with_alpha);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());


    if (io->ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
    HRESULT hr = GUI::d11.g_pSwapChain->Present(1, 0);   // 1 = vsync
    GUI::d11.g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);


}


// Custom GUI Elements - helper functions

const char* GUI::getStatusString(int status) {

    static const char* statusString[] = { "UNKNOWN", "WARNING", "GOOD", "EXCELLENT" };
    if (status >= 0 && status < 4) {
        return statusString[status];
    }
    return statusString[0]; // Default to UNKNOWN
}

void GUI::drawGradientBackground(ImDrawList* drwList, const char* label, ImRect bb, bool button, bool pressed, bool hovered) {
    // Call directly after "begin" to set as background
    ImU32 colTop;
    ImU32 colBottom;

    // button
    const ImVec4 outlineColorBase = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
    ImU32 colOutline = ImGui::GetColorU32(outlineColorBase);
    float rounding = 4.0f;
    float thickness = 2.0f; // 2-pixel thickness for the border


    if (!button) {
        colTop = IM_COL32(4, 16, 7, 255);
        colBottom = IM_COL32(72, 76, 74, 255);
    }
    else {
        colTop = IM_COL32(72, 76, 74, 255);
        colBottom = IM_COL32(4, 16, 7, 255);
    }

    // hover and press handler
    if (button) {
        float pressFactor = 1.8f; // Slightly darker when pressed
        float hoverFactor = 1.2f; // Slightly brighter when hovered

        // adjust the outline color based on hover/press state if desired
        if (pressed) {
            // Example: Make the outline BRIGHT YELLOW when pressed
            colOutline = ImGui::GetColorU32(ImVec4(0.9f, 0.9f, 0.7f, 1.0f));
        }
        else if (hovered) {
            // Example: Make the outline YELLOW when hovered
            colOutline = ImGui::GetColorU32(ImVec4(0.8f, 0.8f, 0.0f, 1.0f));
        }
    }


    // 4. Draw Background
    drwList->AddRectFilledMultiColor(
        bb.Min,
        bb.Max,
        colBottom, // Color Top-Left (actually the bottom of the gradient)
        colBottom, // Color Top-Right
        colTop,    // Color Bottom-Right (actually the top of the gradient)
        colTop    // Color Bottom-Left
    );

    // 4.1 Draw Outline (if button)
    if (button) {
        drwList->AddRect(
            bb.Min,
            bb.Max,
            colOutline,
            rounding,
            ImDrawFlags_None, // Flags (can be used to specify sides)
            thickness
        );
    }

}

void GUI::drawAnalogDial_3sections(const char* name, float ranges[], float value, ImVec2 plotSize, bool reverseColors) {

    const char* labels1[] = { "Section 1","Section 2","Section 3" };
    static ImPlotColormap stopLight = -1;
    static ImPlotColormap blackBG;
    static float radius = 0.4f;
    static ImVec2 center = { 0.5f,0.5f };
    static float angle0 = -135;

    // Color map
    if (stopLight == -1) {
        ImU32 stopLight_data[3] = { IM_COL32(0, 255, 0, 255), IM_COL32(200, 200, 0, 255), IM_COL32(255, 0, 0, 255) };
        if (reverseColors) {
            ImU32 temp = stopLight_data[0];
            stopLight_data[0] = stopLight_data[2];
            stopLight_data[2] = temp;
        }
        stopLight = ImPlot::AddColormap("stoplight", stopLight_data, 3);

        ImU32 blegh[] = { IM_COL32(10, 10, 10, 255),IM_COL32(10, 10, 10, 255) };

        blackBG = ImPlot::AddColormap("black", blegh, 2);

    }

    if (ImPlot::BeginPlot(name, plotSize, ImPlotFlags_Equal | ImPlotFlags_CanvasOnly| ImPlotFlags_NoTitle | ImPlotFlags_NoMouseText | ImPlotFlags_NoLegend)) {
        ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_NoDecorations, ImPlotAxisFlags_NoDecorations);
        ImPlot::SetupAxesLimits(0, 1, 0, 1, ImPlotCond_Always);
        float plot_converted_angle0 = angle0 + 90;

        ImPlot::PushColormap("stoplight");
        ImPlot::PlotPieChartAsDial(labels1, ranges, 3, center.x, center.y, radius, "%.2f", plot_converted_angle0);
        ImPlot::PopColormap();

        ImPlot::PushColormap("black");
        const char* dumby[] = { "1","2" };
        static float dumval[] = { 0.5f,0.5f };
        ImPlot::PlotPieChart(dumby, dumval, 2, center.x, center.y, radius * 0.8f, " ", 0, ImPlotPieChartFlags_Normalize);
        ImPlot::PopColormap();

        float start_angle_radians = angle0 * 2.0f * IM_PI / 360.0;

        // Text markers
        for (int i = 0; i < 4; ++i) {
            const float percent = ranges[i] / ranges[3]; // value over max   (float 0 - 1)
            float angle = start_angle_radians - (IM_PI * 1.5f * percent); // MINUS BECAUSE ITS INVERTED                   
            ImVec2 pos = { center.x + radius * 1.15f * cos(angle), center.y + radius * 1.15f * sin(angle) };   // polar to cartesian
            string plottertext = std::format("{:.0f}", ranges[i]);
            ImPlot::PlotText(plottertext.data(), pos.x, pos.y);
        }
        ImVec2 titlePos = { center.x,center.y - (radius * 0.5f) };
        ImPlot::PlotText(name, titlePos.x,titlePos.y);

        ImVec2 lineVectorPoint;
        float v_Percent = value / ranges[3];
        float v_Angle = start_angle_radians - (IM_PI * 1.5f * v_Percent);
        lineVectorPoint = { center.x + radius * cos(v_Angle), center.y + radius * sin(v_Angle) };
        float Xs[] = { center.x,lineVectorPoint.x };
        float Ys[] = { center.y,lineVectorPoint.y };
        ImPlot::SetNextLineStyle(ImVec4(255, 255, 255, 255), 4.0f);
        ImPlot::PlotLine("CV", Xs, Ys, 2, ImPlotLineFlags_Segments);
        ImPlot::EndPlot();
    }

}

void GUI::drawAnalogDial_5sections(const char* name, float ranges[], float value, ImVec2 plotSize) {

    const char* labels5[] = { "Section 1","Section 2","Section 3", "Section 4", "Section 5"};
    static ImPlotColormap stopLight5 = -1;
    static ImPlotColormap blackBG5;
    static float radius = 0.4f;
    static ImVec2 center = { 0.5f,0.5f };
    static float angle0 = -135;

    // Color map
    if (stopLight5 == -1) {
        ImU32 stopLight_data[5] = { IM_COL32(255, 0, 0, 255),IM_COL32(200, 200, 0, 255),IM_COL32(0, 255, 0, 255), IM_COL32(200, 200, 0, 255), IM_COL32(255, 0, 0, 255) };

        stopLight5 = ImPlot::AddColormap("stoplight5", stopLight_data, 5);

        ImU32 blegh[] = { IM_COL32(10, 10, 10, 255),IM_COL32(10, 10, 10, 255) };
        blackBG5 = ImPlot::AddColormap("black5", blegh, 2);

    }

    if (ImPlot::BeginPlot(name, plotSize, ImPlotFlags_Equal | ImPlotFlags_CanvasOnly | ImPlotFlags_NoTitle | ImPlotFlags_NoMouseText | ImPlotFlags_NoLegend)) {
        ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_NoDecorations, ImPlotAxisFlags_NoDecorations);
        ImPlot::SetupAxesLimits(0, 1, 0, 1, ImPlotCond_Always);
        float plot_converted_angle0 = angle0 + 90;

        ImPlot::PushColormap("stoplight5");
        ImPlot::PlotPieChartAsDial(labels5, ranges, 5, center.x, center.y, radius, "%.2f", plot_converted_angle0);
        ImPlot::PopColormap();

        ImPlot::PushColormap("black5");
        const char* dumby[] = { "1","2" };
        static float dumval[] = { 0.5f,0.5f };
        ImPlot::PlotPieChart(dumby, dumval, 2, center.x, center.y, radius * 0.8f, " ", 0, ImPlotPieChartFlags_Normalize);
        ImPlot::PopColormap();

        float start_angle_radians = angle0 * 2.0f * IM_PI / 360.0;

        // Text markers
        for (int i = 0; i < 6; ++i) {
            const float percent = ranges[i] / ranges[5]; // value over max   (float 0 - 1)
            float angle = start_angle_radians - (IM_PI * 1.5f * percent); // MINUS BECAUSE ITS INVERTED                   
            ImVec2 pos = { center.x + radius * 1.15f * cos(angle), center.y + radius * 1.15f * sin(angle) };   // polar to cartesian
            string plottertext = std::format("{:.0f}", ranges[i]);
            ImPlot::PlotText(plottertext.data(), pos.x, pos.y);
        }
        ImVec2 titlePos = { center.x,center.y - (radius * 0.5f) };
        ImPlot::PlotText(name, titlePos.x, titlePos.y);

        ImVec2 lineVectorPoint;
        float v_Percent = value / ranges[5];
        float v_Angle = start_angle_radians - (IM_PI * 1.5f * v_Percent);
        lineVectorPoint = { center.x + radius * cos(v_Angle), center.y + radius * sin(v_Angle) };
        float Xs[] = { center.x,lineVectorPoint.x };
        float Ys[] = { center.y,lineVectorPoint.y };
        ImPlot::SetNextLineStyle(ImVec4(255, 255, 255, 255), 4.0f);
        ImPlot::PlotLine("CV", Xs, Ys, 2, ImPlotLineFlags_Segments);
        ImPlot::EndPlot();
    }

}

bool GUI::customImageButton(iconData& icon, const string& label, const string& key, ImVec2 btnSize) {

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;

    // --- 1. Split Multi-Line Label ---
    std::vector<std::string> labelLines;
    size_t start = 0;
    size_t end = label.find('\n');
    while (end != std::string::npos) {
        labelLines.push_back(label.substr(start, end - start));
        start = end + 1;
        end = label.find('\n', start);
    }
    labelLines.push_back(label.substr(start));

    float totalLabelHeight = 0.0f;
    for (const auto& line : labelLines) {
        totalLabelHeight += ImGui::CalcTextSize(line.c_str()).y;
    }

    float lineHeight = ImGui::GetTextLineHeight();          // Calculate the height of a single line of text for spacing

    // --- 2. Calculate Bounding Box and Reserve Space ---
    const ImVec2 size = btnSize;
    const ImGuiID id = ImGui::GetID(label.c_str());
    const ImVec2 cursorPos = ImGui::GetCursorScreenPos();
    ImRect bb(cursorPos, cursorPos + size);

    ImGui::ItemSize(bb, style.FramePadding.y);
    if (!ImGui::ItemAdd(bb, id)) {
        return false;
    }

    // --- 3. Handle State and Drawing (Background) ---
    bool hovered = ImGui::IsItemHovered();
    bool pressed = ImGui::IsItemActive();
    bool clicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    drawGradientBackground(drawList, label.c_str(), bb, true, pressed, hovered);

    // --- 4. Calculate Positions (Vertical Stacking) ---
    ImVec2 innerMin = bb.Min + style.FramePadding;
    float innerWidth = bb.Max.x - bb.Min.x - style.FramePadding.x * 2.0f;
    ImVec2 keySize = ImGui::CalcTextSize(key.c_str());

    // Calculate the total content height and spacing
    float contentHeight = keySize.y + icon.size.y + totalLabelHeight;
    float freeSpace = size.y - contentHeight - (style.FramePadding.y * 2.0f);

    float vSpace = freeSpace / 3.0f;    // Divide the free space into segments (above key, key/icon, icon/label)

    // --- Top Text (Key) - Icon (Middle) - Bottom Text (Label)
    ImVec2 keyPos = ImVec2(innerMin.x + (innerWidth - keySize.x) * 0.5f, bb.Min.y + style.FramePadding.y + vSpace); // Center X 
    ImVec2 iconPos = ImVec2(innerMin.x + (innerWidth - icon.size.x) * 0.5f, keyPos.y + keySize.y + vSpace);         // Center X,Start position for the first line of the label                                                                                                                
    ImVec2 labelStartPos = ImVec2(0.0f, iconPos.y + icon.size.y + vSpace);                                         // X will be calculated per-line

    // 5. Draw Content
    // Draw Top Text (Key)
    ImGui::RenderText(keyPos, key.c_str());

    // Draw Icon (Image)
    drawList->AddImage(
        icon.pixbuf,
        iconPos,
        iconPos + icon.size,
        ImVec2(0.0f, 0.0f), // uv0
        ImVec2(1.0f, 1.0f)  // uv1
    );

    // Draw Bottom Text (Label Lines)
    ImVec2 currentLabelPos = labelStartPos;
    for (const auto& line : labelLines) {
        ImVec2 lineSize = ImGui::CalcTextSize(line.c_str());
        currentLabelPos.x = innerMin.x + (innerWidth - lineSize.x) * 0.5f; // Calculate X position to center this specific line
        ImGui::RenderText(currentLabelPos, line.c_str());
        currentLabelPos.y += lineSize.y;        // Move the Y position down for the next line
    }

    return hovered && clicked;
}

// ------------- BACKEND -----------------

LRESULT WINAPI GUI::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        d11.g_ResizeWidth = (UINT)LOWORD(lParam); // Queue resize
        d11.g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    case WM_DPICHANGED:
        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DpiEnableScaleViewports)
        {
            //const int dpi = HIWORD(wParam);
            //printf("WM_DPICHANGED to %d (%.0f%%)\n", dpi, (float)dpi / 96.0f * 100.0f);
            const RECT* suggested_rect = (RECT*)lParam;
            ::SetWindowPos(hWnd, nullptr, suggested_rect->left, suggested_rect->top, suggested_rect->right - suggested_rect->left, suggested_rect->bottom - suggested_rect->top, SWP_NOZORDER | SWP_NOACTIVATE);
        }
        break;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
void d11backend::setup(HWND hwnd) {
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
    ImGui_ImplWin32_EnableAlphaCompositing(hwnd);
}
int d11backend::update() {
    // Handle window being minimized or screen locked
    if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
    {
        ::Sleep(10);
        return 1;
    }
    g_SwapChainOccluded = false;

    // Handle window resize (we don't resize directly in the WM_SIZE handler)
    if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
    {
        CleanupRenderTarget();
        g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
        g_ResizeWidth = g_ResizeHeight = 0;
        CreateRenderTarget();
    }
    return 0;
}
bool d11backend::CreateDeviceD3D(HWND hWnd)
{
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}
void d11backend::CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}
void d11backend::CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));

    if (!pBackBuffer) return;

    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}
ImU64 d11backend::LoadTextureFromBuffer(const uint8_t* buffer, int width, int height, int pitch) {

    if (!g_pd3dDevice || !buffer) return (ImU64)nullptr;

    // Define texture description
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;

    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;  // 32-bit RGBA format (other format doesnt work)
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    // Define subresource data (raw pixel data)
    D3D11_SUBRESOURCE_DATA subresource = {};
    subresource.pSysMem = buffer;
    subresource.SysMemPitch = pitch;

    ID3D11Texture2D* texture = nullptr;

    if ((g_pd3dDevice->CreateTexture2D(&desc, &subresource, &texture)) < 0) {
        return (ImU64)nullptr;
    }

    // Create a shader resource view (SRV) for ImGui
    ID3D11ShaderResourceView* srv = nullptr;
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = desc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;

    if (FAILED(g_pd3dDevice->CreateShaderResourceView(texture, &srvDesc, &srv))) {
        texture->Release();
        return (ImU64)nullptr;
    }

    texture->Release(); // The SRV now manages the texture
    return (ImU64)srv;
}
void d11backend::ReleaseTexture(uint64_t texture) {
    if (texture) {
        ID3D11ShaderResourceView* srv = reinterpret_cast<ID3D11ShaderResourceView*>(texture);
        srv->Release();  // Release DirectX texture
    }
}
void d11backend::CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}


// UNUSED GRAPHS (incl spectrogram)
#if 0


// Small histogram

if (advancedView)
if (ImPlot::BeginPlot("Histogramme", ImVec2(-1, 180), ImPlotFlags_NoTitle | ImPlotFlags_NoLegend)) {

    float maxPeak = 0;
    float maxPeakIndex = 0;

    for (size_t i = 0; i < 256; i++)
    {
        histogramContent[i] = (float)guiBuffer.histogram[i];

        if (maxPeak < histogramContent[i]) {
            maxPeak = histogramContent[i];
            maxPeakIndex = i;
        }
    }
    ImPlot::SetupAxes(nullptr, nullptr, 0, ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_NoTickLabels);
    ImPlot::SetupAxisScale(ImAxis_Y1, ImPlotScale_Log10);
    ImPlot::SetupLegend(ImPlotLocation_East | ImPlotLocation_North, 0);
    ImPlot::PlotBars("Pixel Histogram", histogramContent.data(), 255);
    ImPlot::PlotInfLines("Effective Threshold", &prog->calibrationRegister[caliParamIndex::threshold].value, 1);
    ImPlot::EndPlot();
}


// BRIGHTNESS RANGE

// Variance and deviation 
            // Calculate the height of each segment
if (ImPlot::BeginPlot("Data Variance", ImVec2(-1, -1), ImPlotFlags_NoLegend | ImPlotFlags_NoTitle)) {

    const char* labels[] = { "Min", "Range", "Max" };
    static float dataBr[3];
    dataBr[0] = minBright;
    dataBr[1] = maxBright - minBright;
    dataBr[2] = 255 - maxBright;
    ImPlot::SetupAxis(ImAxis_X1, nullptr, ImPlotAxisFlags_NoLabel | ImPlotAxisFlags_NoTickLabels);
    ImPlot::SetupAxisLimits(ImAxis_X1, 1, 3);
    ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 300);

    // Brightness variance
    ImPlot::PushColormap("ranges");
    ImPlot::PlotBarGroups(labels, dataBr, 3, 1, 1.0f, 2.0f, ImPlotBarGroupsFlags_Stacked);
    ImPlot::PopColormap();
    ImPlot::PlotScatter("a", &two, &prog->calibrationRegister[caliParamIndex::brightness].value, 1);              // brightPoint 
    string currentBrighttxt = std::format("{:.2f}", prog->calibrationRegister[caliParamIndex::brightness].value);
    string brightVarianceText = (string)"Variance\n = " + std::format("{:2.2f}", prog->calibrationRegister[caliParamIndex::br_variance].value);
    ImPlot::PlotText("Brightness", two, 290.0f);
    ImPlot::PlotText(currentBrighttxt.data(), two, std::clamp(prog->calibrationRegister[caliParamIndex::brightness].value, 0.0f, 150.0f) + 5.0f);  // brightText
    ImPlot::PlotText(brightVarianceText.data(), two, 265.0f, { 0,0 });

    ImPlot::EndPlot();
}

     SHADED HISTOGRAM
    if (calibrationMode) {

        static float y_data[256], x_data[256];
        for (size_t i = 0; i < 256; i++)
        {
            x_data[i] = (float)i;
            y_data[i] = (i > 15 && i < 50) ? maxPeak : 0;
        }

        if (maxPeakIndex > 15 && maxPeakIndex < 50) {
            ImPlot::PushStyleColor(ImPlotCol_Fill, IM_COL32(20, 200, 20, 100));
            ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.8f);
        }
        else {
            ImPlot::PushStyleColor(ImPlotCol_Fill, IM_COL32(200, 20, 20, 60));
            ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.3f);
        }
        ImPlot::PlotShaded("Highlight", x_data, y_data, 256, 0);
        ImPlot::PopStyleColor(1);
        ImPlot::PopStyleVar();
    }
    


// OLD FOCUS BAR
static float maxFocus = 1.0f;
maxFocus = max(guiImagestats.focus, maxFocus);
double edge = 1.5f * (double)maxFocus;
if (!holdValues && EVERY_5_SECONDS) maxFocus = 0;

if (ImPlot::BeginPlot("e", ImVec2(-1, 250), ImPlotFlags_NoTitle)) {

    ImPlot::SetupLegend(ImPlotLocation_North, ImPlotLegendFlags_Outside | ImPlotLegendFlags_Horizontal);
    ImPlot::SetupAxes(nullptr, nullptr, 0 | ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_NoTickMarks | ImPlotAxisFlags_NoGridLines);
    ImPlot::SetupAxisLimits(ImAxis_X1, -1, 1);
    ImPlot::SetupAxisLimits(ImAxis_Y1, 0, edge, ImPlotCond_Always);


    // GOOD FOCUS THING
    ImPlot::SetNextFillStyle(ImVec4(1, 1, 1, 0.2), 1);
    ImPlot::SetNextLineStyle(ImVec4(1, 1, 1, 0.5), 1);

    if (localStatCopy.focus > 4.8f) {
        ImPlot::SetNextFillStyle(ImVec4(0.80, 1, 1, 0.7), 1);
        ImPlot::SetNextLineStyle(ImVec4(0.40, 0.4, 1, 0.8), 4);
    }

    ImPlot::PlotBars("Focus Quality", &zero, &localStatCopy.focus, 1, 1);
    string focusTxt = std::format("{:.2f}", localStatCopy.focus);
    ImPlot::PlotText(focusTxt.data(), zero, localStatCopy.focus + 0.5f);

    // target focus
    ImPlot::SetNextLineStyle(ImVec4(0.40, 1, 0.4, 1), 2);
    ImPlot::PlotInfLines("Target", &targetFocus, 1, ImPlotInfLinesFlags_Horizontal);


    //  ImPlot::PlotInfLines("Max focus", &maxFocus, 1, ImPlotInfLinesFlags_Horizontal);
    ImPlot::EndPlot();




}


// HISTOGRAM COPY

// Histogram copy
if (ImPlot::BeginPlot("Histogramme", ImVec2(-1, -1), ImPlotFlags_NoTitle)) {

    float maxPeak = 0;
    float maxPeakIndex = 0;

    for (size_t i = 0; i < 256; i++)
    {
        histogramContent[i] = (float)guiBuffer.histogram[i];

        if (maxPeak < histogramContent[i]) {
            maxPeak = histogramContent[i];
            maxPeakIndex = i;
        }
    }


    ImPlot::SetupAxes(nullptr, nullptr, 0, ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_NoTickLabels);
    ImPlot::SetupAxisScale(ImAxis_Y1, ImPlotScale_Log10);
    ImPlot::SetupLegend(ImPlotLocation_East | ImPlotLocation_North, 0);
    ImPlot::PlotBars("Pixel Histogram", histogramContent.data(), 255);
    ImPlot::PlotInfLines("Effective Threshold", &tre, 1);
    //ImPlot::PlotInfLines("Average brightness", &guiImagestats.pixelAverage, 1);

    ImPlot::EndPlot();
}

tRight;



// SPECTROGRAM

// Histogram History
if (ImPlot::BeginPlot("Histogram history", ImVec2(-1, 275), ImPlotFlags_NoLegend | ImPlotFlags_NoMouseText)) {

    static HistBuffer histMap;
    histMap.addHist(histogramContent);
    ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_NoLabel | ImPlotAxisFlags_NoTickLabels, ImPlotAxisFlags_NoLabel | ImPlotAxisFlags_NoTickLabels);
    ImPlot::SetupAxisLimits(ImAxis_Y1, 0, HIST_DEPTH, ImGuiCond_Always);
    ImPlot::SetupAxisLimits(ImAxis_X1, 0, HIST_WIDTH, ImGuiCond_Always);

    // ( Jet, Plasma, Viridis)
    ImPlot::PushColormap(ImPlotColormap_Plasma);

    ImPlot::PlotHeatmap(
        "Histogram-gram",
        histMap.flatData.data(),       // Pointer to the flattened data
        HIST_DEPTH,                    // Number of rows (time slices currently in buffer)
        HIST_WIDTH,                    // Number of columns (256 bins per slice)
        0, 5.5f,                       // color range
        nullptr,                       // Value format string (e.g., "%.2f")
        ImPlotPoint(0, 0),         // Bottom-left corner of the heatmap
        ImPlotPoint(HIST_WIDTH - 1, HIST_DEPTH) // Top-right corner of the heatmap
    );

    ImPlot::PopColormap(); // Pop the colormap after plotting
    ImPlot::EndPlot();
}
#endif