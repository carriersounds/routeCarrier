#ifndef AUDIO_FX
#define AUDIO_FX
#include "MainHeader.h"
#include <JuceHeader.h>
#include "AudioNodes.h"
#include <process.h>


class DSPNode : public AudioNode
{
public:

    DSPNode(BlockType blocktype, juce::String initDeviceName, NodeID nodeID);

    double sampleRate = 48000;
    int blockSize = BLOCKSIZE;
    int numChannels = 2;

    std::vector<juce::dsp::ProcessorBase*> chain;


    juce::AudioBuffer<float> inputBuffer;
    juce::AudioBuffer<float> outputBuffer;


    void prepare()
    {
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = blockSize;
        spec.numChannels = numChannels;



        fastdsp.setMix(1.0f);
        fastdsp.setDepth(1.0f);
        fastdsp.setRate(5.0f);

        fastdsp.prepare(spec);


        for (auto& p : chain)
            p->prepare(spec);


    }

    void reset()
    {
        for (auto& p : chain)
            p->reset();
    }


    juce::dsp::Phaser<float> fastdsp;


    void process()
    {
        outputBuffer.makeCopyOf(inputBuffer, false);    // output buf is the process context

        juce::dsp::AudioBlock<float> block(outputBuffer);
        juce::dsp::ProcessContextReplacing<float> ctx(block);

        

       // fastdsp.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 200.0f);

        fastdsp.process(ctx);

       // for (auto& p : chain)
       //     p->process(ctx);
    }

    void clear()
    {
        chain.clear();
    }

    void addEffect(EffectType effect)
    {

        auto derivedPtr = new juce::dsp::Chorus<float>;
      //  derivedPtr->setDepth(0.5f);
     //   derivedPtr->setMix(1.0f);
     //   derivedPtr->setRate(1.0f);
        juce::dsp::ProcessorBase* basePtr = (juce::dsp::ProcessorBase*)derivedPtr;  // vector now manages the pointer
        chain.push_back(basePtr);           


        switch (effect)
        {
        case EffectType::Filter:
            break;
        case EffectType::Phaser:
            break;
        case EffectType::Gain:
            break;
        default:
            break;
        }

                       // chain.push_back(std::make_unique<ProcessorType>());
        prepare();     // reinitialize entire chain for safety
    }
};

#endif




