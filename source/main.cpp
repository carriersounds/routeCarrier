#include "Program.h"

int main() {
    juce::initialiseJuce_GUI();
    
    // Launch Program & Audio Engine
    std::unique_ptr<Program> prog = std::make_unique<Program>();

    // Main GUI Loop
    while (1) {
        MSG msg;
        if (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {   
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT) break;  // close on X press
        }

        if (GUI::d11.update()) continue;                        // skip render during window resizing, minimizing or dragging

        prog->gui.renderAllModules();
        prog->gui.sendGraphicsToGPU();
    }

    prog.reset(); // so all audio-related memory is released before JUCE shutdown

    juce::shutdownJuce_GUI();

    return 0;
}