#ifndef AUDIO_DEVICE
#define AUDIO_DEVICE
#include "MainHeader.h"
#include <JuceHeader.h>
#include "AudioNodes.h"


class DeviceNode : public juce::AudioIODeviceCallback, public AudioNode
{
public:
    DeviceNode(BlockType blocktype, juce::String initDeviceName, NodeID nodeID);

    ~DeviceNode(){
        deviceManager.removeAudioCallback(this);
        deviceManager.closeAudioDevice();
    }


    // Producer: write samples into FIFO. if device == input : called from callback ; else called from audioThread
    // Consumer: read samples from FIFO    if device == output : called from callback ; else called from audioThread
    int writeToFifoFrom(const float* const* input, int numSamples);  
    int readFromFifoTo(float* const* output, int numSamples);

    juce::AudioIODevice* devicePointer;
    juce::AudioDeviceManager deviceManager;                                              
    juce::WaitableEvent* trigger;           // to signal that

    void render() override;

    void initDSP(double sr, int bs);

    void selectDevice(const juce::String& nameToFind, bool isOutput);

    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData, int numInputChannels, float* const* outputChannelData, int numOutputChannels, int numSamples, const juce::AudioIODeviceCallbackContext&) override;
    
    void audioDeviceAboutToStart(juce::AudioIODevice*) override;
    
    void audioDeviceStopped() override {}

    void setAsMainOutput(juce::WaitableEvent* trigger);
  
    void prepareOutput() override;
    
    bool isMainOutput() const { return m_isMainOutput; }
    
    const float* getDataPointer() const {
        return hardwareBuffer.getReadPointer(0);
    }

    // === Members ===
    juce::AbstractFifo hardwareFIFO;    // for hardware inputs to write to and for hardware outputs to RECEIVE data from  
    juce::AudioBuffer<float> hardwareBuffer;

private:  
    bool m_isMainOutput;
};




#endif