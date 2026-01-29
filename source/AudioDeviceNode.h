#ifndef AUDIO_DEVICE
#define AUDIO_DEVICE
#include "mainHeader.hpp"
#include <JuceHeader.h>
#include "AudioNodes.h"


class DeviceNode final : public juce::AudioIODeviceCallback, public AudioNode 
{
public:
    DeviceNode(BlockType blocktype, juce::String initDeviceName, NodeID nodeID);
    ~DeviceNode() override {
        deviceManager.removeAudioCallback(this);
        deviceManager.closeAudioDevice();
        src_delete(sampleRateConverter);
    }

    // JUCE Inherited
    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData, int numInputChannels, float* const* outputChannelData, int numOutputChannels, int numSamples, const juce::AudioIODeviceCallbackContext&) override;
    void audioDeviceAboutToStart(juce::AudioIODevice*) override;
    void audioDeviceStopped() override {}

    // JUCE Data
    juce::AbstractFifo hardwareFIFO;
    juce::AudioBuffer<float> hardwareBuffer;
    juce::AudioIODevice* devicePointer;
    juce::AudioDeviceManager deviceManager;                                              
    juce::WaitableEvent* trigger;
    juce::WaitableEvent* trigAddress;
    float deviceGain_dB = 0.0f; //dB


    //SRC
    juce::AudioBuffer<float> srcWorkBuffer;
    SRC_STATE* sampleRateConverter;
    double SRC_ratio;
    double SRC_base;
    int err4src;

    float recoverStrength = 0.0001;

#define ERRFILT 30
    float avgErr = 0;
    float fillCountBuf[ERRFILT] = { 1024 };
    int errIdx = 0;
    int next_fillCountIndex() {
        errIdx++;
        if (errIdx == ERRFILT) errIdx = 0;
        return errIdx;

    }
    float getAvgFill() {
        float err = 0;
        for (size_t i = 0; i < ERRFILT; i++)
            err += fillCountBuf[i] / (float)ERRFILT;
        avgErr = err;
        return err;
    }


    // Custom Control
    void renderAsNode(float pinSize, float spacing) override;
    void selectDevice(const juce::String& nameToFind, bool isOutput);
    void setTriggerAddress(juce::WaitableEvent* trigger);
    void setAsMainOutput(bool set);
    void prepareOutput() override;  
    bool isMainOutput() const { return trigger != nullptr; }
    int writeToFifoFrom(const float* const* input, int numSamples);
    int readFromFifoTo(float* const* output, int numSamples, bool performSRC = false);
    const float* getDataPointer() const {
        return hardwareBuffer.getReadPointer(0);
    }
};


#endif