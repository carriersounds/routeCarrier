#ifndef GUI_h
#define GUI_h
#include "mainHeader.h"

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
class Program;

// needs to be declared before GUI class
struct d11backend {
    d11backend() {
        g_pd3dDevice = nullptr;
        g_pd3dDeviceContext = nullptr;
        g_pSwapChain = nullptr;
        g_SwapChainOccluded = false;
        g_ResizeWidth = 0, g_ResizeHeight = 0;
        g_mainRenderTargetView = nullptr;
    }
    void setup(HWND hwnd);
    int update();
    bool CreateDeviceD3D(HWND hWnd);
    void CleanupDeviceD3D();
    void CreateRenderTarget();
    ImU64 LoadTextureFromBuffer(const uint8_t* buffer, int width, int height, int pitch);
    void ReleaseTexture(uint64_t texture);
    void CleanupRenderTarget();

    // Data
    ID3D11Device* g_pd3dDevice;
    ID3D11DeviceContext* g_pd3dDeviceContext;
    IDXGISwapChain* g_pSwapChain;
    bool                    g_SwapChainOccluded;
    UINT                    g_ResizeWidth, g_ResizeHeight;
    ID3D11RenderTargetView* g_mainRenderTargetView;
};

class GUI {
public:

    enum class DragDropBlock {
        None,
        Device,
        Filter,
        Gain
    };


    GUI(Program* prog);
    ~GUI();
    static inline d11backend d11;

    void renderAllModules();
    void sendGraphicsToGPU();
    void setViewport(UI viewport);
    static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    // Module visibility
    bool showTimings = true;
    bool showLog = true;
    bool showDemos = false;
    bool showMixer = true;


    node::EditorContext* node_Context;

    // Images and heavy graphics
    int imageToRender = viewMode::normal;

    vector<float> histogramContent;                           
    string currentLoadedFile = "";
    std::mutex guiMutex;


private:
    Program* prog;
    ImGuiIO* io;
    UI viewport = UI::fullscreen_admin;
    HWND hwnd;
    WNDCLASSEXW wc;
    string timestamp;

    int imGuiSetup();
    void renderMainDockSpace();  
    void renderMenuBar();
    void renderMixPanel();
    void renderDeviceList();
    void renderToolbar();

    void renderLog();
    void renderFIFOState();

    static void drawGradientBackground(ImDrawList* drwList, const char* label, ImRect bb, bool button, bool pressed = false, bool hovered = false);
    static void drawAnalogDial_3sections(const char* name, float ranges[], float value, ImVec2 plotSize, bool reverseColors);
    static void drawAnalogDial_5sections(const char* name, float ranges[], float value, ImVec2 plotSize);                    // aiming at the middle
    static bool customImageButton(iconData& icon, const string& label, const string& key, ImVec2 btnSize = ImVec2(70, 125)); // was ImVec2(73, 132)
    static const char* getStatusString(int status);

};



// extra gui elements
struct ScrollingBuffer {
    int MaxSize;
    int Offset;
    ImVector<ImVec2> Data;
    ScrollingBuffer(int max_size = 2000) {
        MaxSize = max_size;
        Offset = 0;
        Data.reserve(MaxSize);
    }
    void AddPoint(float x, float y) {
        if (Data.size() < MaxSize)
            Data.push_back(ImVec2(x, y));
        else {
            Data[Offset] = ImVec2(x, y);
            Offset = (Offset + 1) % MaxSize;
        }
    }
    void Erase() {
        if (Data.size() > 0) {
            Data.shrink(0);
            Offset = 0;
        }
    }
};
struct HistBuffer {

    int HIST_DEPTH = 1024;
    int HIST_WIDTH = 1024; 
    size_t maxSize;
    size_t offset;
    int histCounter;

    vector<float> flatData;
    vector<vector<float>> structuredData;


    HistBuffer() {
        maxSize = HIST_DEPTH * HIST_WIDTH;
        offset = 0;
        histCounter = 0;
        flatData.resize(maxSize);

        structuredData.resize(HIST_DEPTH, vector<float>(HIST_WIDTH, 0));

    }

    void addHist(vector<float>& in) {
        structuredData[offset].assign(in.begin(), in.end());
        for (size_t i = 0; i < HIST_DEPTH; i++) {
            size_t bufChunk = (offset + 1 + i) % HIST_DEPTH;    // Determine the current chunk in the circular buffer

            for (size_t gs = 0; gs < HIST_WIDTH; gs++) {
                size_t pixIndex = (i * HIST_WIDTH) + gs;
                flatData[pixIndex] = log10(structuredData[bufChunk][gs]);   // Use log10 for better visualization of small values
            }
        }
        offset = (offset + 1) % HIST_DEPTH; // loopback              
    }

};
struct RollingBuffer {
    float Span;
    ImVector<ImVec2> Data;
    RollingBuffer() {
        Span = 10.0f;
        Data.reserve(2000);
    }
    void AddPoint(float x, float y) {
        float xmod = fmodf(x, Span);
        if (!Data.empty() && xmod < Data.back().x)
            Data.shrink(0);
        Data.push_back(ImVec2(xmod, y));
    }
};

#endif // !GUI_h