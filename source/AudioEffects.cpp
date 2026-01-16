#include "AudioEffects.h"
#include "Logger.h"

#define INDENT_NEXT ImGui::Dummy({ 10,10 }); ImGui::SameLine();

//========= PROCESSING ===========
void EqualizerNode::processDSP(juce::dsp::AudioBlock<float>& block) {

    if (parameterChanged.load()) {

        parameterChanged.store(false);
    }

    EQ.process(juce::dsp::ProcessContextReplacing<float>(block));
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

void GainNode::processDSP(juce::dsp::AudioBlock<float>& block) {
    if (parameterChanged.load()) {
        gain.setGainDecibels(gainValueDB);
        parameterChanged.store(false);
    }

    gain.process(juce::dsp::ProcessContextReplacing<float>(block));
}

void ReverbNode::processDSP(juce::dsp::AudioBlock<float>& block) {

    if (parameterChanged.load()) {

        reverbParams = juce::dsp::Reverb::Parameters(roomSize, damping, wetLevel, dryLevel, width, freezeMode);
        reverb.setParameters(reverbParams);
        parameterChanged.store(false);
    }

    reverb.process(juce::dsp::ProcessContextReplacing<float>(block));

}


//========= GRAPHICAL INTERFACE ===========
void EqualizerNode::renderInterface() {


    ;

    ImPlot::BeginPlot("EQ GRAF", {400,150},ImPlotFlags_CanvasOnly | ImPlotFlags_NoTitle);       // USE FULL WIDTH

    ImPlot::SetupAxisLimits(ImAxis_X1,10,20000, ImGuiCond_Always);
    ImPlot::SetupAxisLimits(ImAxis_Y1, -12, 12, ImGuiCond_Always);
    ImPlot::SetupAxisScale(ImAxis_X1, ImPlotScale_Log10);
    ImPlot::SetupAxis(ImAxis_X1, "Frequency", ImPlotAxisFlags_NoLabel | ImPlotAxisFlags_NoTickLabels);
    ImPlot::SetupAxis(ImAxis_Y1, "Gain", ImPlotAxisFlags_NoLabel | ImPlotAxisFlags_NoTickLabels);

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

void FilterNode::renderInterface() {

    if (ImGui::BeginTable("f_tab", 3, ImGuiTableFlags_BordersInner , {300,100})) {

        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        INDENT_NEXT

        if (ImGuiKnobs::Knob("Freq", &cutoffHz, 20.0f, 20000, 0.0f, "%.1f Hz", ImGuiKnobVariant_WiperOnly, 0.0f, ImGuiKnobFlags_Logarithmic)) {
            parameterChanged.store(true);
        }

        ImGui::TableNextColumn();

        INDENT_NEXT
        
        if (ImGuiKnobs::Knob("Reso", &resonance, 0.1f, 20.0f, 0.0f, "%.1f", ImGuiKnobVariant_WiperOnly)) {
            parameterChanged.store(true);
        }

        ImGui::TableNextColumn();

        static int selected = 0;
        const char* typelabels[3] = { "Lowpass", "HighPass", "Bandpass" };
        INDENT_NEXT
        ImGui::Text("Filter Type");

        INDENT_NEXT
        ImGui::Separator();

        for (int i = 0; i < IM_ARRAYSIZE(typelabels); i++)
        {
            INDENT_NEXT
            if (ImGui::Selectable(typelabels[i], selected == i))
                selected = i;
        }

        ImGui::SameLine();
        ImGui::Dummy({ 20,20 });
       // INDENT_NEXT

        ImGui::EndTable();
    }
}

void GainNode::renderInterface() {


    ImGui::Dummy({ 20,30 });
    ImGui::SameLine();

    if(ImGuiKnobs::Knob("Gain", &gainValueDB, -60, 36, 0.0f, "%.1f dB", ImGuiKnobVariant_WiperOnly)) parameterChanged.store(true);

    ImGui::SameLine();
    ImGui::Dummy({ 20,30 });
   

}

void ReverbNode::renderInterface() {
  
    bool isChanged = false;   

    if(ImGuiKnobs::Knob("Size", &roomSize, 0, 1, 0.0f, "%.2f", ImGuiKnobVariant_WiperOnly)) isChanged = true;
    ImGui::SameLine();
    if(ImGuiKnobs::Knob("Damping", &damping, 0, 1, 0.0f, "%.2f", ImGuiKnobVariant_WiperOnly)) isChanged = true;
    ImGui::SameLine();
    if(ImGuiKnobs::Knob("Width", &width, 0, 1, 0.0f, "%.2f", ImGuiKnobVariant_WiperOnly)) isChanged = true;
    ImGui::SameLine();
    if(ImGuiKnobs::Knob("Hold", &freezeMode, 0, 1, 0.0f, "%.2f", ImGuiKnobVariant_WiperOnly)) isChanged = true;
    ImGui::GetWindowDrawList()->AddLine(ImGui::GetCursorScreenPos(), (ImGui::GetCursorScreenPos() + ImVec2(270, 0)), IM_COL32(120, 120, 120, 255));
    ImGui::NewLine();
    ImGui::Dummy({ 132,20 });
    ImGui::SameLine(); 
    if(ImGuiKnobs::Knob("Dry Level", &dryLevel, 0, 1, 0.0f, "%.2f", ImGuiKnobVariant_WiperOnly)) isChanged = true;
    ImGui::SameLine();
    if(ImGuiKnobs::Knob("Wet Level", &wetLevel, 0, 1, 0.0f, "%.2f", ImGuiKnobVariant_WiperOnly)) isChanged = true;

    if(isChanged)
    parameterChanged.store(true);

}