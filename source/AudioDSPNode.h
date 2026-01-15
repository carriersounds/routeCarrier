#ifndef AUDIO_FX
#define AUDIO_FX
#include "MainHeader.h"
#include <JuceHeader.h>
#include "AudioNodes.h"


struct IDSPParameter
{
    virtual ~IDSPParameter() = default;
};

template <typename T>
struct DSPParameter : IDSPParameter
{
    DSPParameter(NodeID parent, std::string name, T initial)
        : parentNode(parent), paramName(std::move(name)), value(initial)
    {

    }

    NodeID parentNode;
    std::string paramName;
    std::atomic<T> value;
};



class DSPNode: public AudioNode, public juce::AudioProcessor
{
public:

    DSPNode(BlockType blocktype, juce::String initDeviceName, NodeID nodeID);
    ~DSPNode(){}
    //================ AudioProcessor ====================
    void prepareOutput() override { process(); }
    void process();
    void prepareToPlay(double sampleRate, int blockSize) override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override; 

    // ================ Configuration =====================

    void renderAsNode(float pinSize, float spacing, float NODE_WIDTH) override;           // render name & i/o pins
    virtual void renderInterface() = 0;     // render actual effect interface / params, should be contained in main render function

    virtual EffectType getType() = 0;                                       // used by GUI for parameter management

    const juce::String getName() const override { return "DSP Node"; }
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

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
    // 
    virtual void prepareDSP(const juce::dsp::ProcessSpec&) = 0;
    virtual void processDSP(juce::dsp::AudioBlock<float>&) = 0;

};

#endif




