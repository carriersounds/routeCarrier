#include "AudioEffects.h"



void LowpassNode::processDSP(juce::dsp::AudioBlock<float>& block) {

    //  Update coefficients once per block
    auto newCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(
        sampleRate,
        cutoffHz.load(),
        resonance.load());

    *filter.state = *newCoeffs;

    filter.process(juce::dsp::ProcessContextReplacing<float>(block));
}


void GainNode::processDSP(juce::dsp::AudioBlock<float>& block) {

    gain.setGainDecibels(gainValueDB.load());
    gain.process(juce::dsp::ProcessContextReplacing<float>(block));
}