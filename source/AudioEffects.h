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
            *filter.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(48000.0, 200);
        }

    protected:
        void prepareDSP(const juce::dsp::ProcessSpec& spec) override{
            filter.reset();
            filter.prepare(spec);
        }

        void processDSP(juce::dsp::AudioBlock<float>& block) override{
            filter.process(juce::dsp::ProcessContextReplacing<float>(block));
        }

    private:
        using Filter = juce::dsp::IIR::Filter<float>;
        using Coeffs = juce::dsp::IIR::Coefficients<float>;
        juce::dsp::ProcessorDuplicator<Filter, Coeffs> filter;
    };


class GainNode final : public DSPNode
{
public:
    GainNode(BlockType blocktype, juce::String initDeviceName, NodeID nodeID): DSPNode(blocktype, initDeviceName, nodeID) {
        gain.setGainLinear(0.2f);
    }

protected:
    void prepareDSP(const juce::dsp::ProcessSpec& spec) override{
        gain.reset();
        gain.prepare(spec);
    }

    void processDSP(juce::dsp::AudioBlock<float>& block) override{
        gain.process(juce::dsp::ProcessContextReplacing<float>(block));
    }

private:
    juce::dsp::Gain<float> gain;
};







#endif