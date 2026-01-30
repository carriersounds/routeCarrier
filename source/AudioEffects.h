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
    allpass
};
enum class DistortionType {
    softclip,
    hardclip,
    sinfold,
    diode,
    sinfold_cubes
};

using Filt = juce::dsp::IIR::Filter<float>;
using Coeffs = juce::dsp::IIR::Coefficients<float>;
using JuceFilter = juce::dsp::ProcessorDuplicator<Filt, Coeffs>;
using JuceGain = juce::dsp::Gain<float>;
using JuceShaper = juce::dsp::WaveShaper<float>;
using JuceCompressor = juce::dsp::Compressor<float>;

class Equalizer final : public DSPNode
{
public:
    Equalizer(BlockType blocktype, juce::String initDeviceName, NodeID nodeID) : DSPNode(blocktype, initDeviceName, nodeID)
       {

        drywet.setWetLatency(0.0f);
        drywet.setMixingRule(juce::dsp::DryWetMixingRule::sin3dB);
        drywet.setWetMixProportion(1.0f);
        dryWetMix = 1.0f;

        bandSettings.resize(4);
        bandInterface.resize(4);

        chainVec.resize(4);

        X_frequencies.resize(256);
        Y_responseDB.resize(256);   // EQ graph

        bandInterface[0] = EQbandSetting<double>(100, 0.0f, 0.7f, FilterType::lowShelf);
        bandInterface[1] = EQbandSetting<double>(500, 0.0f, 0.7f);
        bandInterface[2] = EQbandSetting<double>(2000, 0.0f, 0.7f);
        bandInterface[3] = EQbandSetting<double>(9000, 0.0f, 0.7f, FilterType::highShelf);
        
        for (size_t i = 0; i < bandInterface.size(); i++) {
            bandSettings[i] = bandInterface[i].toFloats();
        }

        for (int i = 0; i < chainVec.size(); i++) {

            FilterType type = bandSettings[i].typeToCtrl;
            switch (type)
            {
            case FilterType::lowPass:*chainVec[i].state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, bandSettings[i].freq, bandSettings[i].Q);
                break;
            case FilterType::highPass:*chainVec[i].state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, bandSettings[i].freq, bandSettings[i].Q);
                break;
            case FilterType::bandPass:*chainVec[i].state = *juce::dsp::IIR::Coefficients<float>::makeBandPass(sampleRate, bandSettings[i].freq, bandSettings[i].Q);
                break;
            case FilterType::lowShelf:*chainVec[i].state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf(sampleRate, bandSettings[i].freq, bandSettings[i].Q, juce::Decibels::decibelsToGain(bandSettings[i].gainDB));
                break;
            case FilterType::highShelf:*chainVec[i].state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(sampleRate, bandSettings[i].freq, bandSettings[i].Q, juce::Decibels::decibelsToGain(bandSettings[i].gainDB));
                break;
            case FilterType::notch:*chainVec[i].state = *juce::dsp::IIR::Coefficients<float>::makeNotch(sampleRate, bandSettings[i].freq, bandSettings[i].Q);
                break;
            case FilterType::peak:*chainVec[i].state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, bandSettings[i].freq, bandSettings[i].Q, juce::Decibels::decibelsToGain(bandSettings[i].gainDB));
                break;
            case FilterType::allpass:*chainVec[i].state = *juce::dsp::IIR::Coefficients<float>::makeAllPass(sampleRate, bandSettings[i].freq, bandSettings[i].Q);
                break;
            default:
                break;
            }
        }

    }

    std::vector<float> X_frequencies;
    std::vector<float> Y_responseDB;


protected:
    EffectType getType() { return EffectType::EQ; }
    void prepareDSP(const juce::dsp::ProcessSpec& spec) override { 
        drywet.reset(), drywet.prepare(spec);
        for (auto& f : chainVec) { f.reset(); f.prepare(spec); }
    }
    void processDSP(juce::dsp::AudioBlock<float>& block) override;
    void renderInterface(float nodeW) override;
    void addBand() {



    }
private:
    template<typename T>
    struct EQbandSetting {
        EQbandSetting(T freq = 100.0f, T gainDB = 0.0f, T Q = 0.7f, FilterType type = FilterType::peak, bool enabled = true) :
            typeToCtrl(type),freq(freq),gainDB(gainDB),Q(Q),enabled(enabled){
            if (freq < 1) freq = 100.0f;
        }

        T freq;
        T gainDB;
        T Q;
        bool enabled;
        FilterType typeToCtrl;

        EQbandSetting<float> toFloats() {
            return EQbandSetting<float>((float)freq, (float)gainDB, (float)Q, typeToCtrl, enabled);
        }
    };

    string FilterTypeString[8] = {
        "LowPass",
        "HighPass",
        "BandPass",
        "Low Shelf",
        "High Shelf",
        "Notch",
        "Peak",
        "Allpass"};

    juce::dsp::DryWetMixer<float> drywet;
    int bandCount = 4;
    float dryWetMix;

    vector<JuceFilter> chainVec;
    vector<EQbandSetting<float>> bandSettings;
    vector<EQbandSetting<double>> bandInterface;

    float getChainMagnitude(const vector<JuceFilter>& chain, float freq, float sampleRate)
    {
        float mag = 1.0f;

        for (int i = 0; i < chainVec.size(); i++) {
            auto& filter = chainVec[i];

            if (bandSettings[i].enabled)
            mag *= (float)filter.state->getMagnitudeForFrequency(freq, sampleRate);
        }
   
        return mag;
    }
    void generateFrequencyResponse(const vector<JuceFilter>& chain, float sampleRate,vector<float>& freqs,vector<float>& magsDb)
    {
        constexpr int numPoints = 256;
        constexpr float minFreq = 20.0f;
        constexpr float maxFreq = 20000.0f;

        freqs.resize(numPoints);
        magsDb.resize(numPoints);

        for (int i = 0; i < numPoints; ++i)
        {
            float norm = (float)i / (numPoints - 1);
            float freq = minFreq * std::pow(maxFreq / minFreq, norm);

            float mag = getChainMagnitude(chain, freq, sampleRate);
            mag = (mag * dryWetMix + (1.0f-dryWetMix));
            float magDb = juce::Decibels::gainToDecibels(mag);

            freqs[i] = freq;
            magsDb[i] = magDb;
        }

    }
    //void addBand();

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
        outputGainDB(0.0f),
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
        case DistortionType::softclip: waveshaper.functionToUse = [](float x) {return tanh(10.0f*x)*0.1f; };
            break;
        case DistortionType::hardclip: waveshaper.functionToUse = [](float x) {return juce::jlimit(float(-0.1), float(0.1), x); };
            break;
        case DistortionType::sinfold: waveshaper.functionToUse = [](float x) {return sin(10.0f*x)*0.1f; };
            break;
        case DistortionType::diode: {
            waveshaper.functionToUse = [](float x) {
            if (x > 0.05f) {
                float abovThres = 0.05f - x;

                return 0.05f + (tanh(100.0f * abovThres) * 0.01f);

            } else {
                return tanh(10.0f * x) * 0.1f;}
            };
        }
        break;        
        case DistortionType::sinfold_cubes: waveshaper.functionToUse = [](float x) {return sin(50.0f * (float)pow(x, 3)) * 0.1f; };
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

class Compressor final :public DSPNode {
public:
    Compressor(BlockType blocktype, juce::String initDeviceName, NodeID nodeID) : DSPNode(blocktype, initDeviceName, nodeID){
        
        outputGainDB = 0;
        dryWetMix = 1.0f;

        attack = 10.0f;
        release =10.0f;
        ratio = 4.0f;
        threshold = -12.0f;

        compressor.get<0>().setAttack(attack);
        compressor.get<0>().setRelease(release);
        compressor.get<0>().setThreshold(threshold);
        compressor.get<0>().setRatio(ratio);
        compressor.get<1>().setGainDecibels(outputGainDB);
        drywet.setWetMixProportion(dryWetMix);
    }

    float outputGainDB;
    float dryWetMix;

    float attack;
    float release;
    float ratio;
    float threshold;


protected:
    void renderInterface(float nodeW) override;
    EffectType getType() { return EffectType::Gain; }
    void prepareDSP(const juce::dsp::ProcessSpec& spec) override { compressor.reset(); compressor.prepare(spec); drywet.reset(), drywet.prepare(spec);}
    void processDSP(juce::dsp::AudioBlock<float>& block) override;

private:

    juce::dsp::ProcessorChain<JuceCompressor, JuceGain> compressor;
    juce::dsp::DryWetMixer<float> drywet;

};

class ChannelUtility final : public DSPNode 
{
public:
    ChannelUtility(BlockType blocktype, juce::String initDeviceName, NodeID nodeID) : DSPNode(blocktype, initDeviceName, nodeID), gainValueDB(0.0f) {
        gain.setGainDecibels(gainValueDB);
    }

    float gainValueDB;

protected:
    void renderInterface(float nodeW) override;
    void processDSP(juce::dsp::AudioBlock<float>& block) override;
    EffectType getType() { return EffectType::Gain; }
    void prepareDSP(const juce::dsp::ProcessSpec & spec) override { gain.reset(); gain.prepare(spec); }


private:
        JuceGain gain;
    

};

#endif