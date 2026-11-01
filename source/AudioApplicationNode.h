#ifndef AUDIO_CAPTURE
#define AUDIO_CAPTURE
#include "mainHeader.hpp"
#include <JuceHeader.h>
#include "AudioNodes.h"

#include <mmdeviceapi.h>
#include <audioclient.h>
#include <audioclientactivationparams.h>
#pragma comment(lib, "mmdevapi.lib")
#include <tlhelp32.h>
#include <functional>

struct ProcessEntry {
    DWORD pid;
    std::string name;
};

// COM completion handler for ActivateAudioInterfaceAsync
class ActivationHandler : public IActivateAudioInterfaceCompletionHandler {
public:
    IAudioClient* audioClient = nullptr;
    HANDLE doneEvent;

    ActivationHandler() { doneEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr); }
    ~ActivationHandler() { CloseHandle(doneEvent); }

    HRESULT STDMETHODCALLTYPE ActivateCompleted(IActivateAudioInterfaceAsyncOperation* op) override {
        HRESULT hrActivate;
        IUnknown* pUnk = nullptr;
        op->GetActivateResult(&hrActivate, &pUnk);
        if (SUCCEEDED(hrActivate))
            pUnk->QueryInterface(IID_PPV_ARGS(&audioClient));
        if (pUnk) pUnk->Release();
        SetEvent(doneEvent);
        return S_OK;
    }

    // IUnknown boilerplate (minimal, no ref counting needed here)
    ULONG STDMETHODCALLTYPE AddRef() override { return 1; }
    ULONG STDMETHODCALLTYPE Release() override { return 1; }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override {
        if (riid == __uuidof(IActivateAudioInterfaceCompletionHandler) || riid == __uuidof(IUnknown)) {
            *ppv = this; return S_OK;
        }
        *ppv = nullptr; return E_NOINTERFACE;
    }
};

class AppCaptureNode {
public:
    std::vector<ProcessEntry> processes;
    int selectedIndex = -1;

    std::function<void(const float*, int numFrames, int numChannels, int sampleRate)> onAudio;

    AppCaptureNode() { refreshProcesses(); }
    ~AppCaptureNode() { stop(); }

    void refreshProcesses() {
        processes.clear();
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        PROCESSENTRY32 pe; pe.dwSize = sizeof(pe);
        if (Process32First(snap, &pe)) {
            do {
                ProcessEntry entry;
                entry.pid = pe.th32ProcessID;
                // convert wchar process name to utf8
                int len = WideCharToMultiByte(CP_UTF8, 0, pe.szExeFile, -1, nullptr, 0, nullptr, nullptr);
                entry.name.resize(len);
                WideCharToMultiByte(CP_UTF8, 0, pe.szExeFile, -1, entry.name.data(), len, nullptr, nullptr);
                processes.push_back(entry);
            } while (Process32Next(snap, &pe));
        }
        CloseHandle(snap);
    }

    void startCapture(int index) {
        stop();
        if (index < 0 || index >= (int)processes.size()) return;
        selectedIndex = index;
        running = true;
        DWORD pid = processes[index].pid;
        captureThread = std::thread([this, pid] { captureLoop(pid); });
    }

    void stop() {
        running = false;
        if (captureThread.joinable()) captureThread.join();
    }

    void drawImGui() {
        if (ImGui::Button("Refresh")) refreshProcesses();
        ImGui::SameLine();
        ImGui::SetNextItemWidth(220);
        const char* preview = selectedIndex >= 0
            ? processes[selectedIndex].name.c_str() : "Select process...";

        if (ImGui::BeginCombo("Application", preview)) {
            for (int i = 0; i < (int)processes.size(); i++) {
                // show pid next to name so duplicates are distinguishable
                char label[256];
                snprintf(label, sizeof(label), "%s  [%lu]",
                    processes[i].name.c_str(), processes[i].pid);
                bool selected = (i == selectedIndex);
                if (ImGui::Selectable(label, selected))
                    startCapture(i);
                if (selected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
    }

//private:
    std::thread captureThread;
    std::atomic<bool> running = false;

    void captureLoop(DWORD pid) {
        // Must CoInitialize on this thread independently
        CoInitializeEx(nullptr, COINIT_MULTITHREADED);

        AUDIOCLIENT_ACTIVATION_PARAMS params = {};
        params.ActivationType = AUDIOCLIENT_ACTIVATION_TYPE_PROCESS_LOOPBACK;
        params.ProcessLoopbackParams.TargetProcessId = pid;
        params.ProcessLoopbackParams.ProcessLoopbackMode =
            PROCESS_LOOPBACK_MODE_INCLUDE_TARGET_PROCESS_TREE;

        PROPVARIANT activateParams = {};
        activateParams.vt = VT_BLOB;
        activateParams.blob.cbSize = sizeof(params);
        activateParams.blob.pBlobData = (BYTE*)&params;

        ActivationHandler handler;
        IActivateAudioInterfaceAsyncOperation* asyncOp = nullptr;
        ActivateAudioInterfaceAsync(
            VIRTUAL_AUDIO_DEVICE_PROCESS_LOOPBACK,
            __uuidof(IAudioClient),
            &activateParams,
            &handler,
            &asyncOp);

        // block until activation completes
        WaitForSingleObject(handler.doneEvent, INFINITE);
        asyncOp->Release();

        IAudioClient* audioClient = handler.audioClient;
        if (!audioClient) { CoUninitialize(); return; }

        WAVEFORMATEX* fmt = nullptr;
        audioClient->GetMixFormat(&fmt);
        int sr = fmt->nSamplesPerSec;
        int ch = fmt->nChannels;

        audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED,
            AUDCLNT_STREAMFLAGS_LOOPBACK,
            10000000, 0, fmt, nullptr);

        IAudioCaptureClient* captureClient = nullptr;
        audioClient->GetService(__uuidof(IAudioCaptureClient), (void**)&captureClient);
        audioClient->Start();

        while (running) {
            UINT32 packetSize = 0;
            captureClient->GetNextPacketSize(&packetSize);
            while (packetSize > 0) {
                BYTE* data; UINT32 frames; DWORD flags;
                captureClient->GetBuffer(&data, &frames, &flags, nullptr, nullptr);

                if (onAudio && !(flags & AUDCLNT_BUFFERFLAGS_SILENT))
                    onAudio(reinterpret_cast<float*>(data), frames, ch, sr);

                captureClient->ReleaseBuffer(frames);
                captureClient->GetNextPacketSize(&packetSize);
            }
            Sleep(5);
        }

        audioClient->Stop();
        captureClient->Release();
        CoTaskMemFree(fmt);
        audioClient->Release();
        CoUninitialize();
    }
};

#endif