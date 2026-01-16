#ifndef AUDIO_EFFECTS
#define AUDIO_EFFECTS
#include "MainHeader.h"
#include <JuceHeader.h>
#include "AudioDSPNode.h"

enum class filterType {
    lowPass,
    highPass,
    bandPass,
    lowShelf,
    highShelf,
    notch,
    peak,
};

class EqualizerNode final : public DSPNode
{
public:
    EqualizerNode(BlockType blocktype, juce::String initDeviceName, NodeID nodeID) : DSPNode(blocktype, initDeviceName, nodeID) {

        *(EQ.get<0>().state) = *juce::dsp::IIR::Coefficients<float>::makeLowPass(48000.0, 100);
        *(EQ.get<1>().state) = *juce::dsp::IIR::Coefficients<float>::makeNotch(48000.0, 200);
        *(EQ.get<2>().state) = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(48000.0, 300,0.7,4);        // = EQ Node
        *(EQ.get<3>().state) = *juce::dsp::IIR::Coefficients<float>::makeHighPass(48000.0, 600);
 
    }


protected:
    EffectType getType() { return EffectType::EQ; }
    void prepareDSP(const juce::dsp::ProcessSpec& spec) override { EQ.reset(); EQ.prepare(spec); }
    void processDSP(juce::dsp::AudioBlock<float>& block) override;

    void renderInterface(float NODE_WIDTH) override;

private:
    using Filter = juce::dsp::IIR::Filter<float>;
    using Coeffs = juce::dsp::IIR::Coefficients<float>;
    using fullFilter = juce::dsp::ProcessorDuplicator<Filter, Coeffs>;

    juce::dsp::ProcessorChain<fullFilter, fullFilter, fullFilter, fullFilter> EQ;
};


class FilterNode final : public DSPNode
{
public:
    FilterNode(BlockType blocktype,juce::String initDeviceName,NodeID nodeID): DSPNode(blocktype, initDeviceName, nodeID){

        cutoffHz = 200.0f;
        resonance = 0.7f;
        *filter.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(48000.0, cutoffHz);
    }

    // ===== Parameters =====
    float cutoffHz{ 200.0f };
    float resonance{ 0.7f };
    int filterType;

protected: 
    EffectType getType() { return EffectType::Filter; }
    void prepareDSP(const juce::dsp::ProcessSpec& spec) override{filter.reset(); filter.prepare(spec);}
    void processDSP(juce::dsp::AudioBlock<float>& block) override;
    
    void renderInterface(float NODE_WIDTH) override;
    
private:
    using Filter = juce::dsp::IIR::Filter<float>;
    using Coeffs = juce::dsp::IIR::Coefficients<float>;
    juce::dsp::ProcessorDuplicator<Filter, Coeffs> filter;

};


class GainNode final : public DSPNode
{
public:
    GainNode(BlockType blocktype, juce::String initDeviceName, NodeID nodeID): DSPNode(blocktype, initDeviceName, nodeID), gainValueDB(0.0f) {
        gain.setGainDecibels(gainValueDB);
    }

    float gainValueDB;
protected:
    void renderInterface(float NODE_WIDTH) override;

    EffectType getType() { return EffectType::Gain; }
    void prepareDSP(const juce::dsp::ProcessSpec& spec) override{gain.reset();gain.prepare(spec);}
    void processDSP(juce::dsp::AudioBlock<float>& block) override;

private:
    juce::dsp::Gain<float> gain;
};

class ReverbNode final : public DSPNode
{
public:
    ReverbNode(BlockType blocktype, juce::String initDeviceName, NodeID nodeID) : DSPNode(blocktype, initDeviceName, nodeID) {
        
        roomSize = 0.5f;
        damping = 0.5f;
        wetLevel = 0.33f;
        dryLevel = 0.4f;
        width = 1.0f;
        freezeMode = 0.0f;

        reverbParams = juce::dsp::Reverb::Parameters(roomSize,damping,wetLevel,dryLevel,width,freezeMode);
        reverb.setParameters(reverbParams);
    }

    juce::dsp::Reverb::Parameters reverbParams;

    float roomSize = 0.5f;      //*< Room size, 0 to 1.0, where 1.0 is big, 0 is small.
    float damping = 0.5f;       //*< Damping, 0 to 1.0, where 0 is not damped, 1.0 is fully damped.
    float wetLevel = 0.33f;     //*< Wet level, 0 to 1.0 
    float dryLevel = 0.4f;      //*< Dry level, 0 to 1.0
    float width = 1.0f;         //*< Reverb width, 0 to 1.0, where 1.0 is very wide.
    float freezeMode = 0.0f;    //*< Freeze mode - values < 0.5 are "normal" mode, values > 0.5 put the reverb into a continuous feedback loop.

protected:
    void renderInterface(float NODE_WIDTH) override;

    EffectType getType() { return EffectType::Reverb; }
    void prepareDSP(const juce::dsp::ProcessSpec& spec) override { reverb.reset(); reverb.prepare(spec); }
    void processDSP(juce::dsp::AudioBlock<float>& block) override;

private:
    juce::dsp::Reverb reverb;
};



#endif