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

void LowpassNode::renderInterface() {

    float cutoff = cutoffHz.load(std::memory_order_relaxed);
    ImGui::Text("Freq");
    ImGui::SameLine(60);
    if (ImGui::SliderFloat(string("##freq" + to_string(ID)).c_str(), &cutoff, 20.0f, 20000.0f, "%.1f Hz", ImGuiSliderFlags_Logarithmic))
        cutoffHz.store(cutoff);

    ImGui::Dummy(ImVec2(0, 6.0f));  // ROW SPACING
    ImGui::Text("Reso");
    ImGui::SameLine(60);
    float res = resonance.load(std::memory_order_relaxed);
    if (ImGui::SliderFloat(string("##reso" + to_string(ID)).c_str(), &res, 0.1f, 10.0f))
        resonance.store(res);
}

void GainNode::processDSP(juce::dsp::AudioBlock<float>& block) {

    gain.setGainDecibels(gainValueDB.load());
    gain.process(juce::dsp::ProcessContextReplacing<float>(block));
}

void GainNode::renderInterface() {
    float gainVal = gainValueDB.load(std::memory_order_relaxed);
    ImGui::Text("Gain");
    ImGui::SameLine(60);
    if (ImGui::SliderFloat(string("##dB" + to_string(ID)).c_str(), &gainVal, -60.0f, 24.0f))
        gainValueDB.store(gainVal);
}