#ifndef AUDIO_EFFECTS
#define AUDIO_EFFECTS
#include "mainHeader.hpp"
#include <JuceHeader.h>
#include "AudioDSPNode.h"

enum class FilterType {
    lowPass,
    highPass,
    bandPass,
    lowShelf,
    highShelf,
    notch,
    peak,
};
enum class DistortionType {
    softclip,
    hardclip,
    sinfold,
    diode
};

using Filt = juce::dsp::IIR::Filter<float>;
using Coeffs = juce::dsp::IIR::Coefficients<float>;
using JuceFilter = juce::dsp::ProcessorDuplicator<Filt, Coeffs>;
using JuceGain = juce::dsp::Gain<float>;
using JuceShaper = juce::dsp::WaveShaper<float>;

// Add graphs / output meters for better 


class Equalizer final : public DSPNode
{
public:
    Equalizer(BlockType blocktype, juce::String initDeviceName, NodeID nodeID) : DSPNode(blocktype, initDeviceName, nodeID) {

        X_frequencies.resize(500);
        Y_responseDB.resize(500);   // EQ graph

        *(EQ4.get<0>().state) = *juce::dsp::IIR::Coefficients<float>::makeLowPass(48000.0, 100);
        *(EQ4.get<1>().state) = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(48000.0, 500, 0.7, -4);
        *(EQ4.get<2>().state) = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(48000.0, 3000,0.7,4);        // = EQ Node
        *(EQ4.get<3>().state) = *juce::dsp::IIR::Coefficients<float>::makeHighPass(48000.0, 10000);
    }

    void getMagnetudeCurve();
    std::vector<float> X_frequencies;
    std::vector<float> Y_responseDB;


protected:
    EffectType getType() { return EffectType::EQ; }
    void prepareDSP(const juce::dsp::ProcessSpec& spec) override { EQ4.reset(); EQ4.prepare(spec); }
    void processDSP(juce::dsp::AudioBlock<float>& block) override;
    void renderInterface(float nodeW) override;

private:

    juce::dsp::ProcessorChain<JuceFilter, JuceFilter, JuceFilter, JuceFilter> EQ4;
};

class Filter final : public DSPNode
{
public:
    Filter(BlockType blocktype,juce::String initDeviceName,NodeID nodeID): DSPNode(blocktype, initDeviceName, nodeID){
        cutoffHz = 200.0f;
        resonance = 0.7f;
        filterType = 0;
        *filter.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(48000.0, cutoffHz);
    }

    float cutoffHz{ 200.0f };
    float resonance{ 0.7f };
    int filterType{ 0 };

protected: 
    EffectType getType() { return EffectType::Filter; }
    void prepareDSP(const juce::dsp::ProcessSpec& spec) override{filter.reset(); filter.prepare(spec);}
    void processDSP(juce::dsp::AudioBlock<float>& block) override;
    void renderInterface(float nodeW) override;
    
private:
    JuceFilter filter;
};

class Gain final : public DSPNode
{
public:
    Gain(BlockType blocktype, juce::String initDeviceName, NodeID nodeID): DSPNode(blocktype, initDeviceName, nodeID), gainValueDB(0.0f) {
        gain.setGainDecibels(gainValueDB);
    }

    float gainValueDB;

protected:
    void renderInterface(float nodeW) override;
    EffectType getType() { return EffectType::Gain; }
    void prepareDSP(const juce::dsp::ProcessSpec& spec) override{gain.reset();gain.prepare(spec);}
    void processDSP(juce::dsp::AudioBlock<float>& block) override;

private:
    JuceGain gain;
};

class Reverb final : public DSPNode
{
public:
    Reverb(BlockType blocktype, juce::String initDeviceName, NodeID nodeID) : DSPNode(blocktype, initDeviceName, nodeID) {
        
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
    void renderInterface(float nodeW) override;

    EffectType getType() { return EffectType::Reverb; }
    void prepareDSP(const juce::dsp::ProcessSpec& spec) override { reverb.reset(); reverb.prepare(spec); }
    void processDSP(juce::dsp::AudioBlock<float>& block) override;

private:
    juce::dsp::Reverb reverb;
};

class Saturator final : public DSPNode
{
public:
    Saturator(BlockType blocktype, juce::String initDeviceName, NodeID nodeID) : DSPNode(blocktype, initDeviceName, nodeID), 
        inputGainDB(0.0f),
        outputGainDB(0.0f) ,
        distortionType(0),
        dryWetMix(1.0f){
 
        // set distortion type
        drywet.setWetLatency(0.0f);
        drywet.setMixingRule(juce::dsp::DryWetMixingRule::sin3dB);

        saturator.get<0>().setGainDecibels(inputGainDB);
        setType(DistortionType::softclip);
        saturator.get<2>().setGainDecibels(outputGainDB);
    }

    float inputGainDB;
    float outputGainDB;
    float dryWetMix;
    int distortionType;

protected:
    
    void renderInterface(float nodeW) override;
    EffectType getType() { return EffectType::Gain; }
    void prepareDSP(const juce::dsp::ProcessSpec& spec) override { saturator.reset(); saturator.prepare(spec);  drywet.reset(), drywet.prepare(spec); }
    void processDSP(juce::dsp::AudioBlock<float>& block) override;

    void setType(DistortionType type) {
        auto& waveshaper = saturator.template get<1>();
        switch (type)
        {
        case DistortionType::softclip:waveshaper.functionToUse = [](float x) {return tanh(10.0f*x)*0.1f; };
            break;
        case DistortionType::hardclip:waveshaper.functionToUse = [](float x) {return juce::jlimit(float(-0.1), float(0.1), x); };
            break;
        case DistortionType::sinfold:waveshaper.functionToUse = [](float x) {return sin(10.0f*x)*0.1f; };
            break;
        case DistortionType::diode:waveshaper.functionToUse = [](float x) {
            if (x > 0.1f) {
                float abovThres = 0.1f - x;
                return 0.1f + (0.2f * abovThres);
            } else {
                return x;}     
            };
            break;
        default:
            break;
        }
    }
private:

    juce::dsp::ProcessorChain<JuceGain,JuceShaper,JuceGain> saturator;
    juce::dsp::DryWetMixer<float> drywet;

};

class EffectRack final : public DSPNode
{
public:
    EffectRack(BlockType blocktype, juce::String initDeviceName, NodeID nodeID) : DSPNode(blocktype, initDeviceName, nodeID)
    {

    }

    vector<unique_ptr<DSPNode>> chain;

protected:

    void addEffect(EffectType type) {
        switch (type)
        {
        case EffectType::Filter:
            chain.push_back(make_unique<Filter>(BlockType::DSP, "Filter", ID));
            
            break;
        case EffectType::Phaser:
            break;
        case EffectType::Gain:
            break;
        case EffectType::Reverb:
            break;
        case EffectType::EQ:
            break;
        case EffectType::Saturator:
            break;
        default:
            break;
        }
    }

    void refreshAudioPath() {


        chain.size();

        chain.back()->nextNodes.emplace();

        //DSPNode* next / prev = 




     //   chain.back().erase();
    }

    void renderInterface(float nodeW) override;
    EffectType getType() { return EffectType::Gain; }
    void prepareDSP(const juce::dsp::ProcessSpec& spec) override { saturator.reset(); saturator.prepare(spec);  drywet.reset(), drywet.prepare(spec); }
    void processDSP(juce::dsp::AudioBlock<float>& block) override;

private:

    juce::dsp::ProcessorChain<JuceGain, JuceShaper, JuceGain> saturator;
    juce::dsp::DryWetMixer<float> drywet;

};

#endif