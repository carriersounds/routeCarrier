#include "AudioEffects.h"
#include "Logger.h"

#define INDENT_NEXT ImGui::Dummy({ 10,10 }); ImGui::SameLine();

#define ifDoubleClicked if (ImGui::IsItemActive() && ImGui::IsMouseDoubleClicked(0))

//========= PROCESSING ===========
void Equalizer::processDSP(juce::dsp::AudioBlock<float>& block) {

    if (parameterChanged.load()) {

        parameterChanged.store(false);
    }

    EQ4.process(juce::dsp::ProcessContextReplacing<float>(block));
}

void Filter::processDSP(juce::dsp::AudioBlock<float>& block) {

    if (parameterChanged.load()) {

        switch (static_cast<FilterType>(filterType))
        {
        case FilterType::lowPass:*filter.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, cutoffHz, resonance);
            break;
        case FilterType::highPass:*filter.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, cutoffHz, resonance);
            break;
        case FilterType::bandPass:*filter.state = *juce::dsp::IIR::Coefficients<float>::makeBandPass(sampleRate, cutoffHz, resonance);
            break;
        case FilterType::notch: *filter.state = *juce::dsp::IIR::Coefficients<float>::makeNotch(sampleRate, cutoffHz);
            break;
        default:
            break;
        }      
        parameterChanged.store(false);
    }

    filter.process(juce::dsp::ProcessContextReplacing<float>(block));
}

void Gain::processDSP(juce::dsp::AudioBlock<float>& block) {
    if (parameterChanged.load()) {
        gain.setGainDecibels(gainValueDB);
        parameterChanged.store(false);
    }

    gain.process(juce::dsp::ProcessContextReplacing<float>(block));
}

void Reverb::processDSP(juce::dsp::AudioBlock<float>& block) {

    if (parameterChanged.load()) {

        reverbParams = juce::dsp::Reverb::Parameters(roomSize, damping, wetLevel, dryLevel, width, freezeMode);
        reverb.setParameters(reverbParams);
        parameterChanged.store(false);
    }

    reverb.process(juce::dsp::ProcessContextReplacing<float>(block));
}

void Saturator::processDSP(juce::dsp::AudioBlock<float>& block) {

    if (parameterChanged.load()) {
        saturator.get<0>().setGainDecibels(inputGainDB);
        setType((DistortionType)distortionType);
        saturator.get<2>().setGainDecibels(outputGainDB);
        drywet.setWetMixProportion(dryWetMix);
        parameterChanged.store(false);
    }

    drywet.pushDrySamples(block);

    saturator.process(juce::dsp::ProcessContextReplacing<float>(block));
   
    drywet.mixWetSamples(block);

}

void EffectRack::processDSP(juce::dsp::AudioBlock<float>& block) {
    

    if (chain.size() > 0) {
        chain[0]->inputBuffer.clear();
        mixInto(&chain[0]->inputBuffer, &inputBuffer);
    }
    for (size_t block = 0; block < chain.size(); block++) {
        chain[block]->process();

    }
}

//========= GRAPHICAL INTERFACE ===========
void Equalizer::renderInterface(float nodeW) {

    ImPlot::BeginPlot("EQ GRAF", {500,200},ImPlotFlags_CanvasOnly | ImPlotFlags_NoTitle);       // USE FULL WIDTH

    ImPlot::SetupAxisLimits(ImAxis_X1,20,20000, ImGuiCond_Always);
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




#if 0
    ImGui::BulletText("Click and drag each point.");
    static ImPlotDragToolFlags flags = ImPlotDragToolFlags_None;
    ImGui::CheckboxFlags("NoCursors", (unsigned int*)&flags, ImPlotDragToolFlags_NoCursors); ImGui::SameLine();
    ImGui::CheckboxFlags("NoFit", (unsigned int*)&flags, ImPlotDragToolFlags_NoFit); ImGui::SameLine();
    ImGui::CheckboxFlags("NoInput", (unsigned int*)&flags, ImPlotDragToolFlags_NoInputs);
    ImPlotAxisFlags ax_flags = ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_NoTickMarks;
    bool clicked[4] = { false, false, false, false };
    bool hovered[4] = { false, false, false, false };
    bool held[4] = { false, false, false, false };
    if (ImPlot::BeginPlot("##Bezier", ImVec2(400, 300), ImPlotFlags_CanvasOnly)) {
        ImPlot::SetupAxes(nullptr, nullptr, ax_flags, ax_flags);
        ImPlot::SetupAxesLimits(0, 1, 0, 1);
        static ImPlotPoint P[] = { ImPlotPoint(.05f,.05f), ImPlotPoint(0.2,0.4),  ImPlotPoint(0.8,0.6),  ImPlotPoint(.95f,.95f) };

        ImPlot::DragPoint(0, &P[0].x, &P[0].y, ImVec4(0, 0.9f, 0, 1), 4, flags, &clicked[0], &hovered[0], &held[0]);
        ImPlot::DragPoint(1, &P[1].x, &P[1].y, ImVec4(1, 0.5f, 1, 1), 4, flags, &clicked[1], &hovered[1], &held[1]);
        ImPlot::DragPoint(2, &P[2].x, &P[2].y, ImVec4(0, 0.5f, 1, 1), 4, flags, &clicked[2], &hovered[2], &held[2]);
        ImPlot::DragPoint(3, &P[3].x, &P[3].y, ImVec4(0, 0.9f, 0, 1), 4, flags, &clicked[3], &hovered[3], &held[3]);

        static ImPlotPoint B[100];
        for (int i = 0; i < 100; ++i) {
            double t = i / 99.0;
            double u = 1 - t;
            double w1 = u * u * u;
            double w2 = 3 * u * u * t;
            double w3 = 3 * u * t * t;
            double w4 = t * t * t;
            B[i] = ImPlotPoint(w1 * P[0].x + w2 * P[1].x + w3 * P[2].x + w4 * P[3].x, w1 * P[0].y + w2 * P[1].y + w3 * P[2].y + w4 * P[3].y);
        }

        ImPlot::SetNextLineStyle(ImVec4(1, 0.5f, 1, 1), hovered[1] || held[1] ? 2.0f : 1.0f);
        ImPlot::PlotLine("##h1", &P[0].x, &P[0].y, 2, 0, 0, sizeof(ImPlotPoint));
        ImPlot::SetNextLineStyle(ImVec4(0, 0.5f, 1, 1), hovered[2] || held[2] ? 2.0f : 1.0f);
        ImPlot::PlotLine("##h2", &P[2].x, &P[2].y, 2, 0, 0, sizeof(ImPlotPoint));
        ImPlot::SetNextLineStyle(ImVec4(0, 0.9f, 0, 1), hovered[0] || held[0] || hovered[3] || held[3] ? 3.0f : 2.0f);
        ImPlot::PlotLine("##bez", &B[0].x, &B[0].y, 100, 0, 0, sizeof(ImPlotPoint));
        ImPlot::EndPlot();
    }
#endif

}

void Filter::renderInterface(float nodeW) {

    guiMtx.lock();
    tools::drawGainMonitorHoriz(GUIbuffer, nodeW, ID);
    guiMtx.unlock();
    ImGui::NewLine();

    bool isChanged = false;

    if (ImGui::BeginTable("f_tab", 3, ImGuiTableFlags_None, {330,100})) {

        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        INDENT_NEXT

        if (ImGuiKnobs::Knob("Freq", &cutoffHz, 20.0f, 20000, 0.0f, "%.1f Hz", ImGuiKnobVariant_WiperOnly, 0.0f, ImGuiKnobFlags_Logarithmic)) {
            isChanged = true;
        }

        ImGui::TableNextColumn();

        INDENT_NEXT
        
        if (ImGuiKnobs::Knob("Reso", &resonance, 0.1f, 20.0f, 0.0f, "%.1f", ImGuiKnobVariant_WiperOnly)) {
            isChanged = true;
        }

        ImGui::TableNextColumn();
        
       // INDENT_NEXT
        if(ImGui::RadioButton("Lowpass", &filterType, 0))isChanged = true;
      //  INDENT_NEXT
        if(ImGui::RadioButton("Highpass", &filterType, 1))isChanged = true;
      //  INDENT_NEXT
        if(ImGui::RadioButton("Bandpass", &filterType, 2))isChanged = true;
            
        if (ImGui::RadioButton("Notch", &filterType, 3))isChanged = true;
        ImGui::SameLine();
        ImGui::Dummy({ 20,20 });
       // INDENT_NEXT

        ImGui::EndTable();
    }

    if (isChanged) {
        parameterChanged.store(true);
    }

}

void Gain::renderInterface(float nodeW) {

    ImGui::Dummy({ 20,30 });
    ImGui::SameLine();

    if(ImGuiKnobs::Knob("Gain", &gainValueDB, -60, 60, 0.1f, "%.1f dB", ImGuiKnobVariant_WiperOnly)) parameterChanged.store(true);

    ImGui::SameLine();

    guiMtx.lock();
    tools::drawGainMonitorVertic(GUIbuffer, nodeW, ID);
    guiMtx.unlock();

}

void Reverb::renderInterface(float nodeW) {
  
    bool isChanged = false;   

   // ImGui::NewLine();
    //ImGui::Dummy({ 132,20 });   // 2 knobs

    //ImGui::GetWindowDrawList()->AddLine(ImGui::GetCursorScreenPos(), (ImGui::GetCursorScreenPos() + ImVec2(270, 0)), IM_COL32(120, 120, 120, 255));

    if(ImGuiKnobs::Knob("Size", &roomSize, 0, 1, 0.0f, "%.2f", ImGuiKnobVariant_WiperOnly)) isChanged = true;
    ImGui::SameLine();
    if(ImGuiKnobs::Knob("Damping", &damping, 0, 1, 0.0f, "%.2f", ImGuiKnobVariant_WiperOnly)) isChanged = true;
    ImGui::SameLine();
    if(ImGuiKnobs::Knob("Width", &width, 0, 1, 0.0f, "%.2f", ImGuiKnobVariant_WiperOnly)) isChanged = true;
    ImGui::SameLine();
    if(ImGuiKnobs::Knob("Hold", &freezeMode, 0, 1, 0.0f, "%.2f", ImGuiKnobVariant_WiperOnly)) isChanged = true;
    ImGui::SameLine(); 
    if(ImGuiKnobs::Knob("Dry Level", &dryLevel, 0, 1, 0.0f, "%.2f", ImGuiKnobVariant_WiperOnly)) isChanged = true;
    ImGui::SameLine();
    if(ImGuiKnobs::Knob("Wet Level", &wetLevel, 0, 1, 0.0f, "%.2f", ImGuiKnobVariant_WiperOnly)) isChanged = true;

    if(isChanged)
    parameterChanged.store(true);

}

void Saturator::renderInterface(float nodeW) {

    bool isChanged = false;

    if (ImGui::BeginTable("d_tab", 4, ImGuiTableFlags_BordersInner | ImGuiTableFlags_Resizable, { 500,100 })) {


        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        INDENT_NEXT

        if (ImGuiKnobs::Knob("Input Gain", &inputGainDB, -24, 48, 0.2f, "%.2f", ImGuiKnobVariant_WiperOnly)) isChanged = true;
        ifDoubleClicked{ (inputGainDB = 0.0f); isChanged = true; }
       
        ImGui::TableNextColumn();
        INDENT_NEXT
        if (ImGui::RadioButton("SoftClip", &distortionType, 0)) isChanged = true;
        INDENT_NEXT
        if (ImGui::RadioButton("HardClip", &distortionType, 1)) isChanged = true;
        INDENT_NEXT
        if (ImGui::RadioButton("SineFold", &distortionType, 2)) isChanged = true;
        INDENT_NEXT
        if (ImGui::RadioButton("Diode", &distortionType, 3)) isChanged = true;

        ImGui::TableNextColumn();
        INDENT_NEXT
        if (ImGuiKnobs::Knob("Out Gain", &outputGainDB, -36, 24, 0.2f, "%.2f", ImGuiKnobVariant_WiperOnly)) isChanged = true;
        ifDoubleClicked{ (outputGainDB = 0.0f); isChanged = true; }

        ImGui::SameLine();
        INDENT_NEXT
        if (ImGuiKnobs::Knob("Dry/Wet", &dryWetMix, 0, 1, 0.0f, "%.2f", ImGuiKnobVariant_WiperOnly)) isChanged = true;
        
        ImGui::TableNextColumn();
        
        guiMtx.lock();
        tools::drawGainMonitorVertic(GUIbuffer, 140, ID);
        guiMtx.unlock();

        ImGui::EndTable();
    }

    if (isChanged)
        parameterChanged.store(true);
}

void EffectRack::renderInterface(float nodeW) {

}