#include "Program.h"

int main() {
    juce::initialiseJuce_GUI();
    {
        // Launch Program
        Program prog;

        // Main GUI Loop
        while (1) {
            MSG msg;
            if (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {   
                TranslateMessage(&msg);
                DispatchMessage(&msg);
                if (msg.message == WM_QUIT) break;  // close on X press
            }

            if (GUI::d11.update()) continue;                        // skip render during window resizing, minimizing or dragging

            prog.gui.renderAllModules();
            prog.gui.sendGraphicsToGPU();
        }
    } // scope so Program gets destroyed before JUCE shutdown is called

    juce::shutdownJuce_GUI();

    return 0;
}