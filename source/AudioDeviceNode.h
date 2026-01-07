#ifndef AUDIO_DEVICE
#define AUDIO_DEVICE
#include "MainHeader.h"
#include <JuceHeader.h>
#include "AudioNodes.h"

// manages 0, 1 or 2 hardware devices
// 
// if input, default = add DSP + add monitoring output (post/pre FX)

// if output: =  need buffer for mixing multiple inputs anyways (multiple FIFO's)

// Rule of thumb: copy only when you need to modify in parallel, otherwise just pass references.


// buffer write operations = gain included always
// if (includesDSP) create buffer, fifo no
// can have input and/or output
// if no buffer, device (fifo) NEEDS external buffer (
// type = low latency pair -> 
// # of hardware inputs / outputs require # fifo's

// DEVICE NODES DO NOT NEED TO OWN ANY BUFFERS OR FIFO'S. JUST REFERENCES if initialized

// with block = 256: output fifo depth = 3 geeft 768 frames, 16ms latency


// input en output devices apart classes!
// class inputdevice : public callback
// class outputdevice : public callback


// GEEN OS SWITCHING !!! ander apparaat doet t nie. onbekende latency, klonk als 10ms!



//++++++++++++ Features

// ------ Add new
// Live Monitor (can be drag/dropped to an input source directly (with simple dsp by default maybe? like utility/filter): description "low latency path"
// Input device -> just adds another fifo to read from
// output device creates new mixer thread? or adds something to mixer thread

/*

*/



class DeviceNode : public juce::AudioIODeviceCallback, public AudioNode
{
public:
    DeviceNode(BlockType blocktype, juce::String initDeviceName, NodeID nodeID);


    ~DeviceNode(){
        deviceManager.removeAudioCallback(this);
        deviceManager.closeAudioDevice();
    }


    const float* getDataPointer() const {
        return data.getReadPointer(0);
    }


    bool isInput() const { return (m_blockType == BlockType::InputDevice); }
    bool isOutput() const { return (m_blockType == BlockType::OutputDevice); }



    // Producer: write samples into FIFO. if device == input : called from callback ; else called from audioThread
    int writeToFifoFrom(const float* const* input, int numSamples);

    // Consumer: read samples from FIFO    if device == output : called from callback ; else called from audioThread
    int readFromFifoTo(float* const* output, int numSamples);

                                              
   
    juce::AudioDeviceManager deviceManager;                                              
    juce::WaitableEvent* trigger;           // to signal that

    // enum type (input / output / in+monitor)


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
    

    // === Members ===


    double sampleRate = 48000.0;
    int blockSize = 512;

    std::atomic<float> gainValue = 0.0f;
    std::atomic<float> currentLevel = 0.0f;

    juce::dsp::Gain<float> gainProcessor;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> doppel;


public:
    juce::AbstractFifo hardwareFIFO;    // for hardware inputs to write to and for hardware outputs to RECEIVE data from  
    juce::AudioBuffer<float> data;
    int numChannels;
private:
   
    bool m_isMainOutput;


};




#endif