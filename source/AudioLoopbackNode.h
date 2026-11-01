#ifndef AUDIO_LOOPBACK
#define AUDIO_LOOPBACK
#include "mainHeader.hpp"
#include <JuceHeader.h>
#include "AudioNodes.h"

#include <mmdeviceapi.h>
#include <audioclient.h>
#include <functional>
#include <functiondiscoverykeys_devpkey.h>
#pragma comment(lib, "propsys.lib")

struct AudioEndpoint {
    std::wstring id;
    std::string friendlyName;
    IMMDevice* device = nullptr;
};

class LoopbackCaptureNode {
public:
    std::vector<AudioEndpoint> endpoints;
    int selectedIndex = -1;

    std::function<void(const float*, int numFrames, int numChannels, int sampleRate)> onAudio;

    LoopbackCaptureNode() {
        CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
            CLSCTX_ALL, IID_PPV_ARGS(&enumerator));
        refreshDevices();
    }

    ~LoopbackCaptureNode() {
        stop();
        for (auto& e : endpoints) e.device->Release();
        if (enumerator) enumerator->Release();
    }

    void refreshDevices() {
        for (auto& e : endpoints) e.device->Release();
        endpoints.clear();

        IMMDeviceCollection* collection = nullptr;
        enumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &collection);
        UINT count; collection->GetCount(&count);

        for (UINT i = 0; i < count; i++) {
            AudioEndpoint ep;
            collection->Item(i, &ep.device);

            // get friendly name
            IPropertyStore* props = nullptr;
            ep.device->OpenPropertyStore(STGM_READ, &props);
            PROPVARIANT name; PropVariantInit(&name);
            props->GetValue(PKEY_Device_FriendlyName, &name);
            // convert wstring to string for imgui
            int len = WideCharToMultiByte(CP_UTF8, 0, name.pwszVal, -1, nullptr, 0, nullptr, nullptr);
            ep.friendlyName.resize(len);
            WideCharToMultiByte(CP_UTF8, 0, name.pwszVal, -1, ep.friendlyName.data(), len, nullptr, nullptr);
            PropVariantClear(&name);
            props->Release();

            endpoints.push_back(ep);
        }
        collection->Release();
    }

    void startCapture(int index) {
        stop();
        if (index < 0 || index >= (int)endpoints.size()) return;
        selectedIndex = index;
        running = true;
        captureThread = std::thread([this, index] { captureLoop(endpoints[index].device); });
    }

    void stop() {
        running = false;
        if (captureThread.joinable()) captureThread.join();
    }

    void drawImGui() {
        if (ImGui::Button("Refresh")) refreshDevices();
        ImGui::SameLine();
        ImGui::SetNextItemWidth(200);
        if (ImGui::BeginCombo("Output Device", selectedIndex >= 0
            ? endpoints[selectedIndex].friendlyName.c_str() : "Select..."))
        {
            for (int i = 0; i < (int)endpoints.size(); i++) {
                bool selected = (i == selectedIndex);
                if (ImGui::Selectable(endpoints[i].friendlyName.c_str(), selected))
                    startCapture(i);
                if (selected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
    }

private:
    IMMDeviceEnumerator* enumerator = nullptr;
    std::thread captureThread;
    std::atomic<bool> running = false;

    void captureLoop(IMMDevice* device) {
        IAudioClient* audioClient = nullptr;
        device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&audioClient);

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
    }
};

#endif