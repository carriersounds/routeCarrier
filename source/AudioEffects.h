#ifndef AUDIO_EFFECTS
#define AUDIO_EFFECTS
#include "MainHeader.h"
#include <JuceHeader.h>
#include "AudioDSPNode.h"


// channel strip

class LowpassNode final : public DSPNode
{
public:
    LowpassNode(BlockType blocktype,juce::String initDeviceName,NodeID nodeID): DSPNode(blocktype, initDeviceName, nodeID){

        cutoffHz.store(200.0f);
        resonance.store(0.7f);
        *filter.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(48000.0, cutoffHz.load());
    }

    // ===== Parameters =====
    std::atomic<float> cutoffHz{ 200.0f };
    std::atomic<float> resonance{ 0.7f };
protected: 
    EffectType getType() { return EffectType::Filter; }
    void prepareDSP(const juce::dsp::ProcessSpec& spec) override{filter.reset(); filter.prepare(spec);}
    void processDSP(juce::dsp::AudioBlock<float>& block) override;
    
    void renderInterface() override {};
    
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

    std::atomic<float> gainValueDB;
protected:
    void renderInterface() override {};

    EffectType getType() { return EffectType::Gain; }
    void prepareDSP(const juce::dsp::ProcessSpec& spec) override{gain.reset();gain.prepare(spec);}
    void processDSP(juce::dsp::AudioBlock<float>& block) override;

private:
    juce::dsp::Gain<float> gain;
};

#endif