#ifndef AUDIO_DEVICE
#define AUDIO_DEVICE
#include "mainHeader.hpp"
#include <JuceHeader.h>
#include "AudioNodes.h"
#define ERRFILT 50


class DeviceNode final : public juce::AudioIODeviceCallback, public AudioNode 
{
public:
    DeviceNode(BlockType blocktype, juce::String initDeviceName, NodeID nodeID);
    ~DeviceNode() override {
        deviceManager.removeAudioCallback(this);
        deviceManager.closeAudioDevice();
        src_delete(sampleRateConverter);
    }

    // AudioNode
    void renderAsNode(float pinSize, float spacing) override;
    void prepareOutput() override;
    
    // Custom Control
    void selectDevice(const juce::String& nameToFind, bool isOutput);
    void setTriggerAddress(juce::WaitableEvent* trigger);
    void setAsMainOutput(bool set);
    bool isMainOutput() const { return trigger != nullptr; }

    // for GUI
    double SRC_ratio;
    float avgFill = 0;
    float currentFill = 0;
    juce::AudioDeviceManager deviceManager;

private:
    // JUCE Inherited
    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData, int numInputChannels, float* const* outputChannelData, int numOutputChannels, int numSamples, const juce::AudioIODeviceCallbackContext&) override;
    void audioDeviceAboutToStart(juce::AudioIODevice*) override;
    void audioDeviceStopped() override {}

    // JUCE Data
    juce::AbstractFifo hardwareFIFO;
    juce::AudioBuffer<float> hardwareBuffer;
    juce::AudioIODevice* devicePointer;
    juce::WaitableEvent* trigger;
    juce::WaitableEvent* trigAddress;
    float deviceGain_dB = 0.0f; //dB

    // Samplerate Converter
    int writeToFifoFrom(const float* const* input, int numSamples);
    int readFromFifoTo(float* const* output, int numSamples);
    juce::AudioBuffer<float> srcBuffer;
    SRC_STATE* sampleRateConverter;
    int fillIdx = 0;
    float fillCountBuf[ERRFILT] = { 1024 };
    double SRC_base;
    int err4src;
    float recoverStrength = 0.000001;
    int next_fillCountIndex() {
        fillIdx++;
        if (fillIdx == ERRFILT) fillIdx = 0;
        currentFill = fillCountBuf[fillIdx];
        return fillIdx;
    }
    float getAvgFill() {
        float err = 0;
        for (size_t i = 0; i < ERRFILT; i++)
            err += fillCountBuf[i] / (float)ERRFILT;
        avgFill = err;
        return err;
    }
};


#endif