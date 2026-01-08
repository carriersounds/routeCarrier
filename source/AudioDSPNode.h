#ifndef AUDIO_FX
#define AUDIO_FX
#include "MainHeader.h"
#include <JuceHeader.h>
#include "AudioNodes.h"


class DSPNode: public AudioNode, public juce::AudioProcessor
{
public:

    juce::AudioBuffer<float> inputBuffer;
    juce::AudioBuffer<float> outputBuffer;

    DSPNode(BlockType blocktype, juce::String initDeviceName, NodeID nodeID);

    //================ AudioProcessor ====================

    void process() {
        outputBuffer.makeCopyOf(inputBuffer, false);    // output buf is the process context
        juce::MidiBuffer empt;                  // function needs it RIP
        processBlock(outputBuffer, empt);
    }

    void prepareToPlay(double sampleRate, int blockSize) override
    {
        juce::dsp::ProcessSpec spec{
            sampleRate,
            static_cast<int>(blockSize),
            static_cast<int>(numOutputs)
        };

        prepareDSP(spec);
    }

    void processBlock(juce::AudioBuffer<float>& buffer,juce::MidiBuffer&) override
    {
        juce::dsp::AudioBlock<float> block(buffer);

        if (!bypassed) {
            processDSP(block);
        } else {
            outputBuffer.makeCopyOf(inputBuffer, false);
        }

        
    }

    // ================ Configuration =====================
    const juce::String getName() const override { return "Device Node"; }

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override
    {
        return layouts.getMainInputChannelSet().size() == numInputs &&
            layouts.getMainOutputChannelSet().size() == numOutputs;
    }


    int numInputs = 2;
    int numOutputs = 2;
    bool bypassed = false;


private:
    //================ Boilerplate =======================
    bool hasEditor() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    void releaseResources() override {}
    double getTailLengthSeconds() const override { return 0.0; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}

protected:
    //=========== IMPLEMENTED BY SPECIFIC DSP EFFECTS ===========

    virtual void prepareDSP(const juce::dsp::ProcessSpec&) = 0;
    virtual void processDSP(juce::dsp::AudioBlock<float>&) = 0;


};

#endif




