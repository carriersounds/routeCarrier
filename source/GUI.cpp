#include "GUI.h"
#include "Program.h"
#include "AudioEffects.h"
#include "AudioDSPNode.h"

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
    wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"Route Carrier", nullptr }; ::RegisterClassExW(&wc);
    hwnd = ::CreateWindowW(wc.lpszClassName, L"Route Carrier", WS_CAPTION | WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1800, 1000, nullptr, nullptr, wc.hInstance, nullptr);   // maximize in imguisetup 
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


    static const ImU32 gainCol[3] = { IM_COL32(0,0,0,0),IM_COL32(28,139,167,255),IM_COL32(90,90,90,255) };
    ImPlot::AddColormap("gain", gainCol, 3);
    


    d11.setup(hwnd);        // Setup Platform/Renderer backends

    setCustomStyle();

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

void GUI::setCustomStyle()
{

    /*
    // Purple Comfy style by RegularLunar from ImThemes
    ImGuiStyle& style = ImGui::GetStyle();

    style.Alpha = 1.0f;
    style.DisabledAlpha = 0.1f;
    style.WindowPadding = ImVec2(8.0f, 8.0f);
    style.WindowRounding = 10.0f;
    style.WindowBorderSize = 0.0f;
    style.WindowMinSize = ImVec2(30.0f, 30.0f);
    style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
    style.WindowMenuButtonPosition = ImGuiDir_Right;
    style.ChildRounding = 5.0f;
    style.ChildBorderSize = 1.0f;
    style.PopupRounding = 10.0f;
    style.PopupBorderSize = 0.0f;
    style.FramePadding = ImVec2(5.0f, 3.5f);
    style.FrameRounding = 5.0f;
    style.FrameBorderSize = 0.0f;
    style.ItemSpacing = ImVec2(5.0f, 4.0f);
    style.ItemInnerSpacing = ImVec2(5.0f, 5.0f);
    style.CellPadding = ImVec2(4.0f, 2.0f);
    style.IndentSpacing = 5.0f;
    style.ColumnsMinSpacing = 5.0f;
    style.ScrollbarSize = 15.0f;
    style.ScrollbarRounding = 9.0f;
    style.GrabMinSize = 15.0f;
    style.GrabRounding = 5.0f;
    style.TabRounding = 5.0f;
    style.TabBorderSize = 0.0f;
    //style.TabMinWidthForCloseButton = 0.0f;
    style.ColorButtonPosition = ImGuiDir_Right;
    style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
    style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

    style.Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(1.0f, 1.0f, 1.0f, 0.360515f);
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.09803922f, 0.09803922f, 0.09803922f, 1.0f);
    style.Colors[ImGuiCol_ChildBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.0f);
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.09803922f, 0.09803922f, 0.09803922f, 1.0f);
    style.Colors[ImGuiCol_Border] = ImVec4(0.5019608f, 0.3019608f, 1.0f, 0.54901963f);
    style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.15686275f, 0.15686275f, 0.15686275f, 1.0f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.38039216f, 0.42352942f, 0.57254905f, 0.54901963f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.5019608f, 0.3019608f, 1.0f, 0.54901963f);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.09803922f, 0.09803922f, 0.09803922f, 1.0f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.09803922f, 0.09803922f, 0.09803922f, 1.0f);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.25882354f, 0.25882354f, 0.25882354f, 0.0f);
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.15686275f, 0.15686275f, 0.15686275f, 0.0f);
    style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.15686275f, 0.15686275f, 0.15686275f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.23529412f, 0.23529412f, 0.23529412f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.29411766f, 0.29411766f, 0.29411766f, 1.0f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.5019608f, 0.3019608f, 1.0f, 0.54901963f);
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.5019608f, 0.3019608f, 1.0f, 0.54901963f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.5019608f, 0.3019608f, 1.0f, 0.54901963f);

    style.Colors[ImGuiCol_Button] = ImVec4(0.5019608f, 0.3019608f, 1.0f, 0.54901963f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.5019608f, 0.3019608f, 1.0f, 0.54901963f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.5019608f, 0.3019608f, 1.0f, 0.54901963f);

    style.Colors[ImGuiCol_Header] = ImVec4(0.5019608f, 0.3019608f, 1.0f, 0.54901963f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.5019608f, 0.3019608f, 1.0f, 0.54901963f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.5019608f, 0.3019608f, 1.0f, 0.54901963f);
    style.Colors[ImGuiCol_Separator] = ImVec4(0.5019608f, 0.3019608f, 1.0f, 0.54901963f);
    style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.5019608f, 0.3019608f, 1.0f, 0.54901963f);
    style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.5019608f, 0.3019608f, 1.0f, 0.54901963f);
    style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.5019608f, 0.3019608f, 1.0f, 0.54901963f);
    style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.5019608f, 0.3019608f, 1.0f, 0.54901963f);
    style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.5019608f, 0.3019608f, 1.0f, 0.54901963f);
    style.Colors[ImGuiCol_Tab] = ImVec4(0.5019608f, 0.3019608f, 1.0f, 0.54901963f);
    style.Colors[ImGuiCol_TabHovered] = ImVec4(0.5019608f, 0.3019608f, 1.0f, 0.54901963f);
    style.Colors[ImGuiCol_TabActive] = ImVec4(0.5019608f, 0.3019608f, 1.0f, 0.54901963f);
    style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.0f, 0.4509804f, 1.0f, 0.0f);
    style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.13333334f, 0.25882354f, 0.42352942f, 0.0f);
    style.Colors[ImGuiCol_PlotLines] = ImVec4(0.29411766f, 0.29411766f, 0.29411766f, 1.0f);
    style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.5019608f, 0.3019608f, 1.0f, 0.54901963f);
    style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.5019608f, 0.3019608f, 1.0f, 0.54901963f);
    style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.7372549f, 0.69411767f, 0.8862745f, 0.54901963f);
    style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.1882353f, 0.1882353f, 0.2f, 1.0f);
    style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.5019608f, 0.3019608f, 1.0f, 0.54901963f);
    style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.5019608f, 0.3019608f, 1.0f, 0.2901961f);
    style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0f, 1.0f, 1.0f, 0.03433478f);
    style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.5019608f, 0.3019608f, 1.0f, 0.54901963f);
    style.Colors[ImGuiCol_DragDropTarget] = ImVec4(1.0f, 1.0f, 0.0f, 0.9f);
    style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
    style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.7f);
    style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.8f, 0.8f, 0.8f, 0.2f);
    style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.8f, 0.8f, 0.8f, 0.35f);
    



    */
    
    // Comfortable Dark Cyan style by SouthCraftX from ImThemes
    ImGuiStyle& style = ImGui::GetStyle();

    style.Alpha = 1.0f;
    style.DisabledAlpha = 1.0f;
    style.WindowPadding = ImVec2(20.0f, 20.0f);
    style.WindowRounding = 11.5f;
    style.WindowBorderSize = 0.0f;
    style.WindowMinSize = ImVec2(20.0f, 20.0f);
    style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
    style.WindowMenuButtonPosition = ImGuiDir_None;
    style.ChildRounding = 10.0f;
    style.ChildBorderSize = 1.0f;
    style.PopupRounding = 10.4f;
    style.PopupBorderSize = 1.0f;
    style.FramePadding = ImVec2(20.0f, 3.4f);
    style.FrameRounding = 4.0f;
    style.FrameBorderSize = 0.0f;
    style.ItemSpacing = ImVec2(5.9f, 5.4f);
    style.ItemInnerSpacing = ImVec2(7.1f, 1.8f);
    style.CellPadding = ImVec2(2.1f, 0.9f);
    style.IndentSpacing = 0.0f;
    style.ColumnsMinSpacing = 4.7f;
    style.ScrollbarSize = 1.6f;
    style.ScrollbarRounding = 5.9f;
    style.GrabMinSize = 3.7f;
    style.GrabRounding = 10.0f;
    style.TabRounding = 4.8f;
    style.TabBorderSize = 0.0f;

   // style.TabMinWidthForCloseButton = 0.0f;
    style.ColorButtonPosition = ImGuiDir_Right;
    style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
    style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

    style.Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.27450982f, 0.31764707f, 0.4509804f, 1.0f);
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.078431375f, 0.08627451f, 0.101960786f, 1.0f);
    style.Colors[ImGuiCol_ChildBg] = ImVec4(0.09411765f, 0.101960786f, 0.11764706f, 1.0f);
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.078431375f, 0.08627451f, 0.101960786f, 1.0f);
    style.Colors[ImGuiCol_Border] = ImVec4(0.15686275f, 0.16862746f, 0.19215687f, 1.0f);
    style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.078431375f, 0.08627451f, 0.101960786f, 1.0f);

    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.047058824f, 0.05490196f, 0.07058824f, 1.0f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.047058824f, 0.05490196f, 0.07058824f, 1.0f);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.078431375f, 0.08627451f, 0.101960786f, 1.0f);
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.09803922f, 0.105882354f, 0.12156863f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.047058824f, 0.05490196f, 0.07058824f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.11764706f, 0.13333334f, 0.14901961f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.15686275f, 0.16862746f, 0.19215687f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.11764706f, 0.13333334f, 0.14901961f, 1.0f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.03137255f, 0.9490196f, 0.84313726f, 1.0f);
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.03137255f, 0.9490196f, 0.84313726f, 1.0f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.6f, 0.9647059f, 0.03137255f, 1.0f);

    style.Colors[ImGuiCol_Button]       = ImVec4(0.11f, 0.23f, 0.34f, 1.0f);
    style.Colors[ImGuiCol_ButtonHovered]= ImVec4(0.11f, 0.43f, 0.54f, 1.0f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.15f, 0.75f, 0.95f, 1.0f);
    style.Colors[ImGuiCol_Header]       = ImVec4(0.15f, 0.75f, 0.95f, 0.5f);
    style.Colors[ImGuiCol_HeaderHovered]= ImVec4(0.11f, 0.23f, 0.34f, 1.0f);

    // for sliders
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.11372549f, 0.1254902f, 0.15294118f, 1.0f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.15686275f, 0.16862746f, 0.19215687f, 1.0f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.15686275f, 0.16862746f, 0.19215687f, 1.0f);

    style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);   // NO Table Dividers, so we can leave them resizable


    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.078431375f, 0.08627451f, 0.101960786f, 1.0f);
    style.Colors[ImGuiCol_Separator] = ImVec4(0.12941177f, 0.14901961f, 0.19215687f, 1.0f);
    style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.15686275f, 0.18431373f, 0.2509804f, 1.0f);
    style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.15686275f, 0.18431373f, 0.2509804f, 1.0f);
    style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.14509805f, 0.14509805f, 0.14509805f, 1.0f);
    style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.03137255f, 0.9490196f, 0.84313726f, 1.0f);
    style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_Tab] = ImVec4(0.078431375f, 0.08627451f, 0.101960786f, 1.0f);
    style.Colors[ImGuiCol_TabHovered] = ImVec4(0.31764706f, 0.33333334f, 0.39901961f, 1.0f);
    style.Colors[ImGuiCol_TabActive] = ImVec4(0.11764706f, 0.13333334f, 0.14901961f, 1.0f);
    style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.078431375f, 0.08627451f, 0.101960786f, 1.0f);
    style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.1254902f, 0.27450982f, 0.57254905f, 1.0f);
    style.Colors[ImGuiCol_PlotLines] = ImVec4(0.52156866f, 0.6f, 0.7019608f, 1.0f);
    style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.039215688f, 0.98039216f, 0.98039216f, 1.0f);
    style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.03137255f, 0.9490196f, 0.84313726f, 1.0f);
    style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.15686275f, 0.18431373f, 0.2509804f, 1.0f);
    style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.047058824f, 0.05490196f, 0.07058824f, 1.0f);
    style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.047058824f, 0.05490196f, 0.07058824f, 1.0f);

    style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.11764706f, 0.13333334f, 0.14901961f, 1.0f);
    style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.09803922f, 0.105882354f, 0.12156863f, 1.0f);
    style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.9372549f, 0.9372549f, 0.9372549f, 1.0f);
    style.Colors[ImGuiCol_DragDropTarget] = ImVec4(0.49803922f, 0.5137255f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.26666668f, 0.2901961f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.49803922f, 0.5137255f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.19607843f, 0.1764706f, 0.54509807f, 0.5019608f);
    style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.19607843f, 0.1764706f, 0.54509807f, 0.5019608f);
    
}

// RENDERING FUNCTIONS

void GUI::renderAllModules() {

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();


    // ImGui Rendering functions start
    renderMainDockSpace();
    
    renderMixPanel();
  //  renderDeviceList();
    renderToolbar();
    renderMenuBar();

 
    renderMeters();
  //  renderPreview();                    // LOCKED/ NO MOVE
    renderLog();


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
    if (ImGui::IsKeyDown(ImGuiMod_Alt) && ImGui::IsKeyReleased(ImGuiKey_T)) showMetrics = !showMetrics;

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

              

                ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("View")) {
            
            if (ImGui::MenuItem("Keyboard Shortcuts")) keyCommands = true;
            ImGui::Separator();

            if (ImGui::BeginMenu("Show Panels")) {
                ImGui::Checkbox("Process Timers", &showMetrics);
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

void GUI::renderToolbar() {

    ImGui::Begin("Toolbar");
    /*
    Drag & Drop toolbar.
    You can either press a Button to add it at a "default" position
    Drag the Button onto the graph to place it at a specific location       
    */

    DragDropBlock dropper = DragDropBlock::None;

    ImGui::SeparatorText("Audio Effects");

    if(ImGui::Button("FILTER", { 180,80 }))prog->audio.addNewDSPNode(EffectType::Filter);
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))        // triggers continuously when grabbing
    {
        dropper = DragDropBlock::Filter;
        // triggers only on 1st button click
        ImGui::SetDragDropPayload("DND_DEMO_CELL", &dropper, sizeof(DragDropBlock)); // Set payload to carry the index of our item (could be anything)
        ImGui::EndDragDropSource();

    }

    ImGui::SameLine();
    if(ImGui::Button("GAIN", { 180,80 })) prog->audio.addNewDSPNode(EffectType::Gain);
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))        // triggers continuously when grabbing
    {
        dropper = DragDropBlock::Gain;
        // triggers only on 1st button click
        ImGui::SetDragDropPayload("DND_DEMO_CELL", &dropper, sizeof(DragDropBlock)); // Set payload to carry the index of our item (could be anything)
        ImGui::EndDragDropSource();

    }
      
    if (ImGui::Button("REVERB", { 180,80 })) prog->audio.addNewDSPNode(EffectType::Reverb);
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))        // triggers continuously when grabbing
    {
        dropper = DragDropBlock::Reverb;
        // triggers only on 1st button click
        ImGui::SetDragDropPayload("DND_DEMO_CELL", &dropper, sizeof(DragDropBlock)); // Set payload to carry the index of our item (could be anything)
        ImGui::EndDragDropSource();

    }

    ImGui::SameLine();
    if (ImGui::Button("Graphic EQ", { 180,80 })) prog->audio.addNewDSPNode(EffectType::EQ);
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))        // triggers continuously when grabbing
    {
        dropper = DragDropBlock::EQ;
        // triggers only on 1st button click
        ImGui::SetDragDropPayload("DND_DEMO_CELL", &dropper, sizeof(DragDropBlock)); // Set payload to carry the index of our item (could be anything)
        ImGui::EndDragDropSource();

    }

    if (ImGui::Button("Saturator", { 180,80 })) prog->audio.addNewDSPNode(EffectType::Saturator);
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))        // triggers continuously when grabbing
    {
        dropper = DragDropBlock::Saturator;
        // triggers only on 1st button click
        ImGui::SetDragDropPayload("DND_DEMO_CELL", &dropper, sizeof(DragDropBlock)); // Set payload to carry the index of our item (could be anything)
        ImGui::EndDragDropSource();
    }

    ImGui::SameLine();
    if (ImGui::Button("Channel utility", { 180,80 })) prog->audio.addNewDSPNode(EffectType::ChannelUtil);
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))        // triggers continuously when grabbing
    {
        dropper = DragDropBlock::ChannelUtil;
        // triggers only on 1st button click
        ImGui::SetDragDropPayload("DND_DEMO_CELL", &dropper, sizeof(DragDropBlock)); // Set payload to carry the index of our item (could be anything)
        ImGui::EndDragDropSource();
    }

    if (ImGui::Button("Compressor", { 180,80 })) prog->audio.addNewDSPNode(EffectType::Compressor);
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))        // triggers continuously when grabbing
    {
        dropper = DragDropBlock::Compressor;
        // triggers only on 1st button click
        ImGui::SetDragDropPayload("DND_DEMO_CELL", &dropper, sizeof(DragDropBlock)); // Set payload to carry the index of our item (could be anything)
        ImGui::EndDragDropSource();
    }


    ImGui::NewLine();
    ImGui::NewLine();

    static auto& deviceTypes = prog->audio.nullDevice.deviceManager.getAvailableDeviceTypes();
    static juce::AudioIODeviceType* type = deviceTypes.getFirst();                                      // takes WASAPI, maybe check for Low Latency?

    static juce::StringArray inputs;
    static juce::StringArray outputs;

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.11f, 0.23f, 0.34f, 0.3f));

    // update audio device list every 2 seconds or on a button press
    if ((ImGui::GetFrameCount() % 120) == 0) {
        type->scanForDevices();                 // must call to populate names
        inputs = type->getDeviceNames(true);    // true = input
        outputs = type->getDeviceNames(false);  // false = output
    }


    ImGui::SeparatorText("Input Devices");
    ImGui::NewLine();

    for (auto& s : inputs) {
        if (ImGui::Button(("IN: " + s.toStdString()).c_str())) {
            juce::String chosenInput(s);
            prog->audio.addNewDeviceNode(BlockType::InputDevice, chosenInput);
        }

        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))        // triggers continuously when grabbing
        {
            dropper = DragDropBlock::Device;
            // triggers only on 1st button click
            ImGui::SetDragDropPayload("DND_DEMO_CELL", &dropper, sizeof(DragDropBlock)); // Set payload to carry the index of our item (could be anything)
            ImGui::EndDragDropSource();
            grabbedType = BlockType::InputDevice;
            grabbedDevice = s.toStdString();
        }


    }

   
    ImGui::NewLine();
    ImGui::SeparatorText("Output Devices");
    ImGui::NewLine();
    for (auto& s : outputs) {
        if (ImGui::Button(("OUT: " + s.toStdString()).c_str())) {
            juce::String chosenOutput(s);
            NodeID next_ID = prog->audio.addNewDeviceNode(BlockType::OutputDevice, chosenOutput);
        }
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))        // triggers continuously when grabbing
        {
            dropper = DragDropBlock::Device;
            // triggers only on 1st button click
            ImGui::SetDragDropPayload("DND_DEMO_CELL", &dropper, sizeof(DragDropBlock)); // Set payload to carry the index of our item (could be anything)
            ImGui::EndDragDropSource();
            grabbedType = BlockType::OutputDevice;
            grabbedDevice = s.toStdString();
        }
    }
    

    ImGui::PopStyleColor();

    ImGui::End();

}

void GUI::renderMixPanel() {

    static float pinSize = 12.0f;
    static float spacing = 4.0f;
    bool DragDropisHovered = false;
    static bool menuOpen = false;
    static NodeID nodeHovered = 0;
    static NodeID nodeRightClicked = 0;
    static bool ClearScreen = false;
    auto& io = ImGui::GetIO();


    ImGui::Begin("Mixing Panel", &showMixer, ImGuiWindowFlags_NoNavInputs);
   
    if (ImGui::Button("Find Nodes")) {      
        io.UserData = &menuOpen;  // random static memory which is not nullptr
    } else {
        io.UserData = nullptr;  // only thing usable to talk to the inner node function to override the F key zoom
    }

    ImGui::SameLine();


    if (prog->audio.enableRouting) {
        if (ImGui::Button("Pause system")) prog->audio.enableRouting = false;
    }
    else {
        if (ImGui::Button("Resume system")) prog->audio.enableRouting = true;
    }
    ImGui::SameLine();


    if (ImGui::Button("Clear Screen")) ClearScreen = true;


    node::SetCurrentEditor(node_Context);
    node::Begin("editore", ImVec2(0.0, 0.0f));

    // Handle node input from drag & drop
    if (ImGui::BeginDragDropTarget())   // triggers every frame when hovered
    {
        const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_DEMO_CELL");
        
        if (payload)
        {
            NodeID nodeToGiveInitPosition = 0;
            IM_ASSERT(payload->DataSize == sizeof(DragDropBlock));
            DragDropBlock dragPayload = *(const DragDropBlock*)payload->Data;

            switch (dragPayload)
            {
                case DragDropBlock::Device:
                    nodeToGiveInitPosition = prog->audio.addNewDeviceNode(grabbedType, grabbedDevice);
                    Logger::log("Added Device: " + grabbedDevice, level_INFO);
                    break;
                case DragDropBlock::Filter:
                    nodeToGiveInitPosition = prog->audio.addNewDSPNode(EffectType::Filter);
                    Logger::log("Added Filter",level_INFO);
                    break;
                case DragDropBlock::Gain:
                    nodeToGiveInitPosition = prog->audio.addNewDSPNode(EffectType::Gain);
                    Logger::log("Added Gain", level_INFO);
                    break;
                case DragDropBlock::Reverb:
                    nodeToGiveInitPosition = prog->audio.addNewDSPNode(EffectType::Reverb);
                    Logger::log("Added Reverb", level_INFO);
                    break;
                case DragDropBlock::EQ:
                    nodeToGiveInitPosition = prog->audio.addNewDSPNode(EffectType::EQ);
                    Logger::log("Added EQ", level_INFO);
                    break;
                case DragDropBlock::Saturator:
                    nodeToGiveInitPosition = prog->audio.addNewDSPNode(EffectType::Saturator);
                    Logger::log("Added Saturator", level_INFO);
                    break;
                case DragDropBlock::ChannelUtil:
                    nodeToGiveInitPosition = prog->audio.addNewDSPNode(EffectType::ChannelUtil);
                    Logger::log("Added Channel util", level_INFO);
                    break;
                case DragDropBlock::Compressor:
                    nodeToGiveInitPosition = prog->audio.addNewDSPNode(EffectType::Compressor);
                    Logger::log("Added Compressor", level_INFO);
                    break;
                default:
                    break;
            }

            node::SetNodePosition(nodeToGiveInitPosition, ImGui::GetMousePos()); //once, to make sure it drops on the right spot!
        }
        DragDropisHovered = true;

        ImGui::EndDragDropTarget();
        node::BeginNode(999);
        ImGui::Dummy({200,100});
        node::EndNode();
        node::SetNodePosition(999, ImGui::GetMousePos());

    }

    // Draw All Nodes
    for (auto& [ID,node] : prog->audio.nodes) node->renderAsNode(pinSize, spacing);           
    
    // Draw Links
    for (auto& link : prog->audio.links) node::Link(link.second.ID, link.second.ID_left, link.second.ID_right);
    
    // Query link creation
    if (node::BeginCreate())                                            // GRAB PIN
        {
            node::PinId startPinId, endPinId;
            if (node::QueryNewLink(&startPinId, &endPinId))                 // HOVER OVER NEXT
            {
                if (startPinId && endPinId) // both pins are valid
                {
                    NodeID node1 = prog->audio.m_PinNodePairs.at(startPinId.Get());
                    NodeID node2 = prog->audio.m_PinNodePairs.at(endPinId.Get());

                    bool nodeValid = true;      // GUI prevents 1st and 2nd order feedback. 3rd order and higher is handled by toposort

                    if(node1 == node2) nodeValid = false;

                    for (auto& nexts : prog->audio.sends.at(node1)) {
                        if (nexts == node2) nodeValid = false;
                    }  
                    for (auto& nexts : prog->audio.sends.at(node2)) {
                        if (nexts == node1) nodeValid = false;
                    }

                    if (startPinId != endPinId && nodeValid){
                        if (node::AcceptNewItem())                          // RELEASED 
                        {
                            prog->audio.createLink(startPinId, endPinId);   // Creates new link, toposort prevents feedback 3+
                        }
                    }
                    else
                        node::RejectNewItem();                              // Shows invalid link feedback


                }
            }
        }

    // Query link or node deletion
    if (node::BeginDelete())
        {
            node::LinkId linkId;
            if (QueryDeletedLink(&linkId)) {
                prog->audio.deleteLink(linkId.Get());
                node::AcceptDeletedItem();
            }

            node::NodeId nodeID;
            if (node::QueryDeletedNode(&nodeID)) {
                prog->audio.deleteNode(nodeID.Get());
                node::AcceptDeletedItem();
            }
    }

    if (ClearScreen) {
        prog->audio.clearAll();     // "hasanylinks" can only be called inside a node::begin/end context
        ClearScreen = false;
    }

    node::EndDelete();
    node::EndCreate();

    // Handle right-click ID, since it has to be outside of the node context
    nodeHovered = ed::GetHoveredNode().Get();        
    node::End();
    if (nodeHovered) nodeRightClicked = nodeHovered;   
    
    // Right-click menu
    if ((nodeHovered || menuOpen) && ImGui::BeginPopupContextItem("context_menu")) {
        menuOpen = true;
        renderLocalNodeContextMenu(nodeRightClicked);
        ImGui::EndPopup();
    } else {
        menuOpen = false;
    }
    

    node::SetCurrentEditor(nullptr);
    ImGui::End();
}

void GUI::renderDeviceList() {

    static auto& deviceTypes = prog->audio.nullDevice.deviceManager.getAvailableDeviceTypes();
    static juce::AudioIODeviceType* type = deviceTypes.getFirst();                                      // takes WASAPI, maybe check for Low Latency?

    static juce::StringArray inputs;
    static juce::StringArray outputs;

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
            prog->audio.addNewDeviceNode(BlockType::InputDevice, chosenInput);
        }
    }

    ImGui::Separator();

    ImGui::MenuItem("Outputs");
    ImGui::Separator();

    for (auto& s : outputs) {
        if (ImGui::MenuItem(("OUT: " + s.toStdString()).c_str())) {
            juce::String chosenOutput(s);
            NodeID next_ID = prog->audio.addNewDeviceNode(BlockType::OutputDevice, chosenOutput);

        }

    }

    ImGui::End();
}

void GUI::renderMeters() {


    if (!showMetrics) return;

    ImGui::Begin("System Metrics", &showMetrics);

    ImGui::Indent(10);



    ImGui::NewLine();
    ImGui::SeparatorText("Engine");
    ImGui::Text("Engine Clock Period: %05.2f ms (%04.1f Hz)", prog->audio.engineClockTimeMs, 1000.0f/prog->audio.engineClockTimeMs);
    ImGui::Text("Engine Process time: %05.2f ms", prog->audio.engineProcessTimeMs);
    ImGui::NewLine();
    ImGui::Text("Single threaded load");
    ImGui::ProgressBar(prog->audio.engineProcessTimeMs / prog->audio.engineClockTimeMs, {ImGui::GetContentRegionAvail().x - 80, 20});
    ImGui::SameLine(); ImGui::Text("CPU LOAD");

    ImGui::NewLine();

    if (ImGui::TreeNode("Hardware Buffers")) {

        float fr = ImGui::GetIO().Framerate;
        static ScrollingBuffer guiData;
        static std::map<NodeID, fifoScroll> levels;

        for (auto& dev : prog->audio.fifoLevels) {
        if (!levels.contains(dev.first)) {
            levels.emplace(dev.first, fifoScroll());      // if it doesnt contain an entry for the device/pin yet, add it!         
        }
    }

        vector<NodeID> lvlToDelete;
        for (auto& lvl : levels) 
            if (!prog->audio.fifoLevels.contains(lvl.first)) 
                lvlToDelete.push_back(lvl.first);
             
        for (auto& lvl : lvlToDelete)
            levels.erase(lvl);

        static float t = 0;
        t += ImGui::GetIO().DeltaTime;
        for (auto& dev : prog->audio.fifoLevels){

         levels[dev.first].avgFill.AddPoint(t, prog->audio.fifoLevels[dev.first].avgFill);
         levels[dev.first].totFill.AddPoint(t, prog->audio.fifoLevels[dev.first].totFill);
         levels[dev.first].ratio.AddPoint(t, prog->audio.fifoLevels[dev.first].ratio);
    } 

        // guiData.AddPoint(t, 1000.0 / fr);           // TODO: make option to pause graphing (halt)
        static float history = 10.0f;

        ImGui::SliderFloat("History", &history, 1, 14, "%.1f s");

        if (ImPlot::BeginPlot("##Scrolling", ImVec2(-1, -1), ImPlotFlags_NoMouseText)) {
        ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_NoTickLabels);
      //  ImPlot::SetupAxisScale(ImAxis_Y1, ImPlotScale_Log10);
        ImPlot::SetupAxisLimits(ImAxis_X1, (double)t - history, t, ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, -100, 3000);
        ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL, 0.5f);

        for (auto& level : levels) {
            string actualName = prog->audio.nodes.at(level.first)->getBlockName();

            string devicePlusID = actualName;

            auto& lvlAvg = level.second.avgFill;
            auto& lvlTot = level.second.totFill;
            auto& ratio = level.second.ratio;

            ImPlot::PlotLine((devicePlusID + "average").c_str(), &lvlAvg.Data[0].x, &lvlAvg.Data[0].y, lvlAvg.Data.size(), 0, lvlAvg.Offset, 2 * sizeof(float));
            ImPlot::PlotLine((devicePlusID + "current").c_str(), &lvlTot.Data[0].x, &lvlTot.Data[0].y, lvlTot.Data.size(), 0, lvlTot.Offset, 2 * sizeof(float));
            ImPlot::PlotLine((devicePlusID + "ratio").c_str(), &ratio.Data[0].x, &ratio.Data[0].y, ratio.Data.size(), 0, ratio.Offset, 2 * sizeof(float));
        }

            
        

        ImPlot::EndPlot();
    }
    
        ImGui::TreePop();
    }


    ImGui::Unindent();
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

void GUI::renderLocalNodeContextMenu(NodeID ID) {

    if(!prog->audio.nodes.contains(ID)) return;

    AudioNode* node = prog->audio.nodes.at(ID).get();

    BlockType type = node->getBlockType();


    if (ImGui::MenuItem("Copy"));
    if (ImGui::MenuItem("Cut")) prog->audio.deleteNode(ID);
    if (ImGui::MenuItem("Delete Node")) prog->audio.deleteNode(ID);
    if (ImGui::MenuItem("Break all links")) prog->audio.breakAllLinks(ID);

    ImGui::Separator();

    switch (type)
    {
    case BlockType::InputDevice:

        if (!prog->audio.nodes.contains(ID)) return;

        if (ImGui::BeginMenu("Change Input Device")) {

            static auto& deviceTypes = prog->audio.nullDevice.deviceManager.getAvailableDeviceTypes();
            static juce::AudioIODeviceType* type = deviceTypes.getFirst();                                      // takes WASAPI, maybe check for Low Latency?

            static juce::StringArray inputs;

            // update audio device list every 2 seconds or on a button press
            if ((ImGui::GetFrameCount() % 20) == 0) {
                type->scanForDevices();                 // must call to populate names
                inputs = type->getDeviceNames(true);    // true = input
               
            }

            for (auto& s : inputs) {
                if (ImGui::MenuItem(("IN: " + s.toStdString()).c_str())) {
                    juce::String chosenInput(s);
                    prog->audio.changeAudioDevice(ID, chosenInput,false);
                }
            }

            ImGui::EndMenu();
        }
        
        break;
    case BlockType::OutputDevice:
    {
    
        if (!prog->audio.nodes.contains(ID)) return;

        if (ImGui::BeginMenu("Change Output Device")) {

            static auto& deviceTypes = prog->audio.nullDevice.deviceManager.getAvailableDeviceTypes();
            static juce::AudioIODeviceType* type = deviceTypes.getFirst();                                      // takes WASAPI, maybe check for Low Latency?

            static juce::StringArray outputs;

            // update audio device list every 2 seconds or on a button press
            if ((ImGui::GetFrameCount() % 20) == 0) {
                type->scanForDevices();                 // must call to populate names
                outputs = type->getDeviceNames(false);  // false = output
            }

            for (auto& s : outputs) {
                if (ImGui::MenuItem(("OUT: " + s.toStdString()).c_str())) {
                    juce::String chosenOutput(s);
                    prog->audio.changeAudioDevice(ID, chosenOutput, true);
                }
            }

            ImGui::EndMenu();
        }
 
        DeviceNode* devptr = dynamic_cast<DeviceNode*>(node);
        ImGui::BeginDisabled(devptr->isMainOutput());
        if (ImGui::MenuItem("Set as main output")) prog->audio.selectMainOutput(ID);
        ImGui::EndDisabled();
        if (devptr->isMainOutput()) ImGui::SetItemTooltip("Already selected as main out");
    }
        break;
    case BlockType::DSP:
        break;
    case BlockType::FileInput:
        break;
    default:
        break;
    }


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


