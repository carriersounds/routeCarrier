#ifndef AUDIO_FX
#define AUDIO_FX
#include "MainHeader.h"
#include <JuceHeader.h>
#include "AudioNodes.h"

using SignalProcessor = std::unique_ptr<juce::dsp::ProcessorBase>;

class simpleFilter : public AudioNode {



};


class DSPNode : public AudioNode
{
    double sampleRate = 48000;
    int blockSize = 128;
    int numChannels = 2;

    std::vector<SignalProcessor> chain;

    void prepare()
    {
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = blockSize;
        spec.numChannels = numChannels;

        for (auto& p : chain)
            p->prepare(spec);
    }

    void reset()
    {
        for (auto& p : chain)
            p->reset();
    }

    void processBlock(juce::AudioBuffer<float>& buffer)
    {
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> ctx(block);

        for (auto& p : chain)
            p->process(ctx);
    }

    // ---------------------------
    // Dynamic management
    // ---------------------------
    void clear()
    {
        chain.clear();
    }

    template<typename ProcessorType>
    void addEffect()
    {
        chain.push_back(std::make_unique<ProcessorType>());
        prepare();     // reinitialize entire chain for safety
    }
};

#endif




