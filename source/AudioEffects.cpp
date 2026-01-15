#include "AudioEffects.h"


void EqualizerNode::processDSP(juce::dsp::AudioBlock<float>& block) {

    if (parameterChanged.load()) {

        parameterChanged.store(false);
    }

    EQ.process(juce::dsp::ProcessContextReplacing<float>(block));
}

void EqualizerNode::renderInterface() {

    ImPlot::BeginPlot("EQ GRAF", {400,200},ImPlotFlags_CanvasOnly | ImPlotFlags_NoTitle);

   
    ImPlot::SetupAxisLimits(ImAxis_X1,10,20000, ImGuiCond_Always);
    ImPlot::SetupAxisLimits(ImAxis_Y1, -12, 12, ImGuiCond_Always);
    ImPlot::SetupAxisScale(ImAxis_X1, ImPlotScale_Log10);
    ImPlot::SetupAxis(ImAxis_X1, "Frequency", ImPlotAxisFlags_NoLabel);
    ImPlot::SetupAxis(ImAxis_Y1, "Gain", ImPlotAxisFlags_NoLabel);

    ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL, 0.5f);


    float Xlogfreqs[80];
    float Yvalues[80];

    for (int i = 0; i < 80;i++) {

        Xlogfreqs[i] = 10 * pow(1.1, i);         // approx log range from 10 to 20k
        Yvalues[i] = sin(0.001 * Xlogfreqs[i]);
    }

    ImPlot::PlotLine("zinus", Xlogfreqs, Yvalues, 80);
    
    ImPlot::EndPlot();

}


void FilterNode::processDSP(juce::dsp::AudioBlock<float>& block) {

    if (parameterChanged.load()) {
        auto newCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(
            sampleRate,
            cutoffHz,
            resonance);
        *filter.state = *newCoeffs;
        parameterChanged.store(false);
    }
   
    filter.process(juce::dsp::ProcessContextReplacing<float>(block));
}

void FilterNode::renderInterface() {

    if(ImGui_InvertedFloatSlider("Freq", cutoffHz, 20.0f, 20000.0f, " % .1f Hz", ImGuiSliderFlags_Logarithmic))parameterChanged.store(true);
      
    ImGui::Dummy(ImVec2(0, 6.0f));  // ROW SPACING

    if(ImGui_InvertedFloatSlider("Reso", resonance, 0.1, 20)) parameterChanged.store(true);
        
}

void GainNode::processDSP(juce::dsp::AudioBlock<float>& block) {
    if (parameterChanged.load()) {
        gain.setGainDecibels(gainValueDB);
        parameterChanged.store(false);
    }
    
    gain.process(juce::dsp::ProcessContextReplacing<float>(block));
}

void GainNode::renderInterface() {
    if(ImGui_InvertedFloatSlider("Gain", gainValueDB, -60, 36)) parameterChanged.store(true);
}

void ReverbNode::processDSP(juce::dsp::AudioBlock<float>& block) {

    if (parameterChanged.load()) {
    
        reverbParams = juce::dsp::Reverb::Parameters(roomSize, damping, wetLevel, dryLevel, width, freezeMode);
        reverb.setParameters(reverbParams);
        parameterChanged.store(false);
    }

    reverb.process(juce::dsp::ProcessContextReplacing<float>(block));

}

void ReverbNode::renderInterface() {
  
    if (ImGui_InvertedFloatSlider("roomSize", roomSize, 0, 1)|| 
        ImGui_InvertedFloatSlider("damping", damping, 0, 1)  ||
        ImGui_InvertedFloatSlider("wetLevel", wetLevel, 0, 1)||
        ImGui_InvertedFloatSlider("dryLevel", dryLevel, 0, 1)||
        ImGui_InvertedFloatSlider("width", width, 0, 1)      ||
        ImGui_InvertedFloatSlider("freeze", freezeMode, 0, 1)) {
        parameterChanged.store(true);
    }      
}