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


    bool isInput() const { return (m_blockType == BlockType::InputDevice); }
    bool isOutput() const { return (m_blockType == BlockType::OutputDevice); }

    // Producer: write samples into FIFO. if device == input : called from callback ; else called from audioThread
    // Consumer: read samples from FIFO    if device == output : called from callback ; else called from audioThread
    int writeToFifoFrom(const float* const* input, int numSamples);  
    int readFromFifoTo(float* const* output, int numSamples);

                                                
    juce::AudioDeviceManager deviceManager;                                              
    juce::WaitableEvent* trigger;           // to signal that


    void initDSP(double sr, int bs);
    void setGain(float g){
        gainValue.store(g);
    }


    void selectDevice(const juce::String& nameToFind, bool isOutput);
    float getLevel() const noexcept;        // for external readout  


    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData, int numInputChannels, float* const* outputChannelData, int numOutputChannels, int numSamples, const juce::AudioIODeviceCallbackContext&) override;
    
    void audioDeviceAboutToStart(juce::AudioIODevice*) override;
    
    void audioDeviceStopped() override {}

    // Helper to compute absolute peak (for UI)
    void updateLevel(const float* data, int numSamples)
    {
        float maxv = 0.0f;
        for (int i = 0; i < numSamples; i++)
            maxv = std::max(maxv, std::abs(data[i]));

        currentLevel.store(maxv, std::memory_order_relaxed);
    }

    void setAsMainOutput(juce::WaitableEvent* trigger);
  
    
    bool isMainOutput() const { return m_isMainOutput; }
    

    const float* getDataPointer() const {
        return data.getReadPointer(0);
    }

    // === Members ===


    double sampleRate = 48000.0;
    int blockSize = 512;

    std::atomic<float> gainValue = 0.0f;
    std::atomic<float> currentLevel = 0.0f;

    juce::dsp::Gain<float> gainProcessor;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> doppel;

    juce::AbstractFifo hardwareFIFO;    // for hardware inputs to write to and for hardware outputs to RECEIVE data from  
    juce::AudioBuffer<float> data;
    int numChannels;
private:
   
    bool m_isMainOutput;


};




#endif