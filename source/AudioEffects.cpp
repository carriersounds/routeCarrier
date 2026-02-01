#include "AudioEffects.h"
#include "Logger.h"


//========= PROCESSING ===========
void Equalizer::processDSP(juce::dsp::AudioBlock<float>& block) {

    if (parameterChanged.load()) {

        for (size_t i = 0; i < bandInterface.size(); i++) {
            bandSettings[i] = bandInterface[i].toFloats();
        }
        
        // apply EQ parameters
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

        drywet.setWetMixProportion(dryWetMix);
        parameterChanged.store(false);
    }

    drywet.pushDrySamples(block);

    for (int i = 0; i < chainVec.size(); i++) {
        auto& filter = chainVec[i];

        if(bandSettings[i].enabled)
        filter.process(juce::dsp::ProcessContextReplacing<float>(block));
    }

    drywet.mixWetSamples(block);
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
        chain[block]->prepareOutput();

    }
}

void Compressor::processDSP(juce::dsp::AudioBlock<float>& block) {

    if (parameterChanged.load()) {     
        compressor.get<0>().setAttack(attack);
        compressor.get<0>().setRelease(release);
        compressor.get<0>().setThreshold(threshold);
        compressor.get<0>().setRatio(ratio);
        compressor.get<1>().setGainDecibels(outputGainDB);
        drywet.setWetMixProportion(dryWetMix);
        parameterChanged.store(false);
    }

    drywet.pushDrySamples(block);
    compressor.process(juce::dsp::ProcessContextReplacing<float>(block));
    drywet.mixWetSamples(block);

}

void ChannelUtility::processDSP(juce::dsp::AudioBlock<float>& block){


    if (parameterChanged.load()) {
        gain.setGainDecibels(gainValueDB);
        pan.setPan(panValue);
   
        midside.setPan(midSide);
        parameterChanged.store(false);
    }

    gain.process(juce::dsp::ProcessContextReplacing<float>(block));
    pan.process(juce::dsp::ProcessContextReplacing<float>(block));


    midSideBuffer.clear();

    // M = L + R 
    juce::FloatVectorOperations::add(midSideBuffer.getWritePointer(0), outputBuffer.getReadPointer(0), outputBuffer.getReadPointer(1), BLOCKSIZE);

    // S = L - R 
    juce::FloatVectorOperations::subtract(midSideBuffer.getWritePointer(1), outputBuffer.getReadPointer(0), outputBuffer.getReadPointer(1), BLOCKSIZE);
    
    outputBuffer.clear();
    if (!mono) {
        juce::dsp::AudioBlock<float> midsideBlock(midSideBuffer);
        midside.process(juce::dsp::ProcessContextReplacing<float>(midsideBlock));

        // L = M + S
        juce::FloatVectorOperations::addWithMultiply(outputBuffer.getWritePointer(0), midSideBuffer.getReadPointer(0), 1.0f, BLOCKSIZE);
        juce::FloatVectorOperations::addWithMultiply(outputBuffer.getWritePointer(0), midSideBuffer.getReadPointer(1), 1.0f, BLOCKSIZE);

        // R = M - S
        juce::FloatVectorOperations::addWithMultiply(outputBuffer.getWritePointer(1), midSideBuffer.getReadPointer(0), 1.0f, BLOCKSIZE);
        juce::FloatVectorOperations::subtract(outputBuffer.getWritePointer(1), midSideBuffer.getReadPointer(1), BLOCKSIZE);

    }
    else {
        // (L,R) = M
        juce::FloatVectorOperations::addWithMultiply(outputBuffer.getWritePointer(0), midSideBuffer.getReadPointer(0), 1.0f, BLOCKSIZE);
        juce::FloatVectorOperations::addWithMultiply(outputBuffer.getWritePointer(1), midSideBuffer.getReadPointer(0), 1.0f, BLOCKSIZE);
    }
   
    if (invertLeft) {
        juce::FloatVectorOperations::multiply(outputBuffer.getWritePointer(0),-1.0f, BLOCKSIZE);
    }

    if (invertRight) {
        juce::FloatVectorOperations::multiply(outputBuffer.getWritePointer(1), -1.0f, BLOCKSIZE);
    }

}

//========= GRAPHICAL INTERFACE ===========
void Equalizer::renderInterface(float nodeW) {

    ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, { 10,0 });
    ImGui::PushStyleColor(ImGuiCol_FrameBg, { 0,0,0,0 });

    if (ImGui::BeginTable(ADD_ID("eq##"), 2, ImGuiTableFlags_Resizable, ImVec2(670,200))){

        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        bool changed = false;

        if (ImPlot::BeginPlot(ADD_ID("EQ GRAF##"), { 500,200 }, ImPlotFlags_CanvasOnly | ImPlotFlags_NoTitle)) {

            ImPlot::SetupAxisLimits(ImAxis_X1, 20, 20000, ImGuiCond_Always);
            ImPlot::SetupAxisLimits(ImAxis_Y1, -1 * plotRangeY, plotRangeY, ImGuiCond_Always);
            ImPlot::SetupAxisScale(ImAxis_X1, ImPlotScale_Log10);
            ImPlot::SetupAxis(ImAxis_X1, "Frequency", ImPlotAxisFlags_NoLabel | ImPlotAxisFlags_NoTickLabels);
            ImPlot::SetupAxis(ImAxis_Y1, "Gain", ImPlotAxisFlags_NoLabel | ImPlotAxisFlags_NoTickLabels);

            ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL, 0.5f);

            // if parameter changed
            guiMtx.lock();
            generateFrequencyResponse(chainVec, sampleRate, X_frequencies, Y_responseDB);
            guiMtx.unlock();

            ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(0.15f, 0.75f, 0.95f, 1.0f));
            ImPlot::PlotLine("respons", X_frequencies.data(), Y_responseDB.data(), X_frequencies.size());
            ImPlot::PopStyleColor();

            for (int i = 0; i < (int)bandInterface.size(); i++) {
                auto& band = bandInterface[i];
                bool hov = false;
                bool clicked = false;
                bool dragged = false;

                // make ID space for up to 16 points ( ID << 4)
                if (bandInterface[i].typeToCtrl == FilterType::lowShelf || bandInterface[i].typeToCtrl == FilterType::highShelf || bandInterface[i].typeToCtrl == FilterType::peak) {

                    if (ImPlot::DragPoint((ID << 4) + i, &band.freq, &band.gainDB, ImVec4(0.9f, 0.6f, 0.0f, 1.0f), 7.0f, 0, &clicked, &hov)) {
                        dragged = true;
                        changed = true;
                    }

                    ImGui::PushFont(NULL, 12.0f);
                    ImPlot::PushStyleColor(ImPlotCol_InlayText, ImVec4(0, 0, 0, 1));

                    ImPlot::PlotText(to_string(i).c_str(), band.freq, band.gainDB);
                    ImPlot::PopStyleColor();
                    ImGui::PopFont();

                }
                else {
                  
                    fakeQ = 3 * log(2 * band.Q);    // convert for GUI
                    if (ImPlot::DragPoint((ID << 4) + i, &band.freq, &fakeQ, ImVec4(0.9f, 0.6f, 0.0f, 1.0f), 7.0f, 0, &clicked, &hov)) {

                        fakeQ = juce::jlimit(-12.0, 12.0, fakeQ);
                        band.Q = 0.5 * exp(0.333 * fakeQ);
                        dragged = true;
                        changed = true;
                    }

                    ImGui::PushFont(NULL, 12.0f);
                    ImPlot::PushStyleColor(ImPlotCol_InlayText, ImVec4(0, 0, 0, 1));

                    ImPlot::PlotText(to_string(i).c_str(), band.freq, fakeQ);
                    ImPlot::PopStyleColor();
                    ImGui::PopFont();
                }

                if (clicked || dragged) {
                    lastClicked = i;
                }     

                if (hov && ImGui::IsKeyDown(ImGuiMod_Alt)) {
                    band.Q += 0.1 * ImGui::GetIO().MouseDelta.y;
                    changed = true;
                }

                band.Q = std::clamp(band.Q, 0.01, 50.0);
                band.freq = std::clamp(band.freq, 10.0, 20000.0);
                band.gainDB = std::clamp(band.gainDB, -30.0, 30.0);
            }

            ImPlot::EndPlot();      // fix Q slider!!!
        }

        ImGui::TableNextColumn();

        float lastF = bandInterface[lastClicked].freq;
        float lastG = bandInterface[lastClicked].gainDB;
        float lastQ = bandInterface[lastClicked].Q;

        bool lastEnabled = bandInterface[lastClicked].enabled;
        int lastType = (int)bandInterface[lastClicked].typeToCtrl;



        plotRangeY = 12;

        for (auto& ee : bandInterface) {
            plotRangeY = std::max(abs(ee.gainDB), (double)plotRangeY);
        }
        
        plotRangeY = std::max((double)plotRangeY, abs(fakeQ));


        ImGui::PushItemWidth(100);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.11f, 0.23f, 0.34f, 1.0f));

        if (ImGui::Checkbox(("Enabled##" + to_string(ID)).c_str(), &lastEnabled)) {
            bandInterface[lastClicked].enabled = lastEnabled;
            changed = true;
        }    

        if (ImGui::SliderInt(("Type##" + to_string(ID)).c_str(), &lastType, 0, 7, FilterTypeString[lastType].c_str())) {
            bandInterface[lastClicked].typeToCtrl = (FilterType)lastType;
            changed = true;
        }

        ImGui::Dummy({ 0,5 });

        if (ImGui::SliderFloat(("Freq##" + to_string(ID)).c_str(), &lastF, 10, 20000, "%.2f", ImGuiSliderFlags_Logarithmic)) {
            bandInterface[lastClicked].freq = (double)lastF;
            changed = true;
        }

        if (lastType == (int)FilterType::lowShelf || lastType == (int)FilterType::highShelf || lastType == (int)FilterType::peak) {
            if (ImGui::SliderFloat(("Gain##" + to_string(ID)).c_str(), &lastG, -30.0f, 30.0f, "%.2f")) {
                bandInterface[lastClicked].gainDB = (double)lastG;
                changed = true;          
            }
            ifDoubleClicked{bandInterface[lastClicked].gainDB = 0.0; changed = true; }
        }
        else {
            ImGui::Dummy({20,23});
        }

        if (ImGui::SliderFloat(("Q##" + to_string(ID)).c_str(), &lastQ, 0.01, 30.0f, "%.2f", ImGuiSliderFlags_Logarithmic)) {
            bandInterface[lastClicked].Q = (double)lastQ;
            changed = true;
        }

        ImGui::Dummy({ 20,20 });

        if (ImGui::SliderFloat(("Dry/Wet##" + to_string(ID)).c_str(), &dryWetMix, 0.0f, 1.0f, "%.2f")) {
            changed = true;
        }

        ImGui::PopItemWidth();
        ImGui::PopStyleColor();

        ImGui::Dummy({ 10,10 });
        if (changed)
            parameterChanged.store(true);

        ImGui::EndTable();
    }

    ImGui::PopStyleColor();
    ImPlot::PopStyleVar();

}

void Filter::renderInterface(float nodeW) {

    bool isChanged = false;

    // INDENT_NEXT
    if (ImGui::RadioButton(("Lowpass##" + to_string(ID)).c_str(), &filterType, 0))isChanged = true;
    ImVec2 nextLine = ImGui::GetCursorPos();

    ImGui::SameLine();
    INDENT_NEXT
    if (ImGuiKnobs::Knob(("Freq##" + to_string(ID)).c_str(), &cutoffHz, 20.0f, 20000, 0.0f, "%.1f Hz", ImGuiKnobVariant_WiperOnly, 0.0f, ImGuiKnobFlags_Logarithmic)) {
        isChanged = true;
    }   
    ImGui::SameLine();
    INDENT_NEXT
    if (ImGuiKnobs::Knob(("Reso##" + to_string(ID)).c_str(), &resonance, 0.1f, 20.0f, 0.0f, "%.1f", ImGuiKnobVariant_WiperOnly)) {
        isChanged = true;
    }

    ImGui::SameLine();
    INDENT_NEXT

    guiMtx.lock();
    tools::drawGainMonitorVertic_Stereo(GUIbuffer, 120, ID);
    guiMtx.unlock();
    ImGui::NewLine();

    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.11f, 0.23f, 0.34f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.11f, 0.43f, 0.54f, 1.0f));


    ImGui::SetCursorPos(nextLine);
    //  INDENT_NEXT
    if (ImGui::RadioButton(("Highpass##" + to_string(ID)).c_str(), &filterType, 1))isChanged = true;
    //  INDENT_NEXT
    if (ImGui::RadioButton(("Bandpass##" + to_string(ID)).c_str(), &filterType, 2))isChanged = true;

    if (ImGui::RadioButton(("Notch##" + to_string(ID)).c_str(), &filterType, 3))isChanged = true;

    ImGui::PopStyleColor(2);

    ImGui::NewLine();

    if (isChanged) {
        parameterChanged.store(true);
    }

}

void Gain::renderInterface(float nodeW) {


    if(ImGuiKnobs::Knob(("Gain##" + to_string(ID)).c_str(), &gainValueDB, -60, 60, 0.1f, "%.1f dB", ImGuiKnobVariant_WiperOnly)) parameterChanged.store(true);

    ImGui::SameLine();

    guiMtx.lock();
    tools::drawGainMonitorVertic_Stereo(GUIbuffer, nodeW, ID);
    guiMtx.unlock();

    ImGui::NewLine(); // spacing

}

void Reverb::renderInterface(float nodeW) {
  
    bool isChanged = false;   

   // ImGui::NewLine();
    //ImGui::Dummy({ 132,20 });   // 2 knobs

    //ImGui::GetWindowDrawList()->AddLine(ImGui::GetCursorScreenPos(), (ImGui::GetCursorScreenPos() + ImVec2(270, 0)), IM_COL32(120, 120, 120, 255));

    if(ImGuiKnobs::Knob(("Size##" + to_string(ID)).c_str(), &roomSize, 0, 1, 0.0f, "%.2f", ImGuiKnobVariant_WiperOnly)) isChanged = true;
    ImGui::SameLine();
    if(ImGuiKnobs::Knob(("Damping##" + to_string(ID)).c_str(), &damping, 0, 1, 0.0f, "%.2f", ImGuiKnobVariant_WiperOnly)) isChanged = true;
    ImGui::SameLine();
    if(ImGuiKnobs::Knob(("Width##" + to_string(ID)).c_str(), &width, 0, 1, 0.0f, "%.2f", ImGuiKnobVariant_WiperOnly)) isChanged = true;
    ImGui::SameLine();
    if(ImGuiKnobs::Knob(("Hold##" + to_string(ID)).c_str(), &freezeMode, 0, 1, 0.0f, "%.2f", ImGuiKnobVariant_WiperOnly)) isChanged = true;
    ImGui::SameLine(); 
    if(ImGuiKnobs::Knob(("Dry Level##" + to_string(ID)).c_str(), &dryLevel, 0, 1, 0.0f, "%.2f", ImGuiKnobVariant_WiperOnly)) isChanged = true;
    ImGui::SameLine();
    if(ImGuiKnobs::Knob(("Wet Level##" + to_string(ID)).c_str(), &wetLevel, 0, 1, 0.0f, "%.2f", ImGuiKnobVariant_WiperOnly)) isChanged = true;

    if(isChanged)
    parameterChanged.store(true);

}

void Saturator::renderInterface(float nodeW) {

    bool isChanged = false;

    if (ImGui::BeginTable(ADD_ID("d_tab"), 4, ImGuiTableFlags_BordersInner | ImGuiTableFlags_SizingFixedFit, { 480,100 })) {


        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        INDENT_NEXT



        if (ImGuiKnobs::Knob(("Input Gain##" + to_string(ID)).c_str(), &inputGainDB, -24, 48, 0.2f, "%.2f dB", ImGuiKnobVariant_WiperOnly)) isChanged = true;
        ifDoubleClicked{ (inputGainDB = 0.0f); isChanged = true; }
       
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.11f, 0.23f, 0.34f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.11f, 0.43f, 0.54f, 1.0f));

        ImGui::TableNextColumn();
        INDENT_NEXT
        if (ImGui::RadioButton(("SoftClip##" + to_string(ID)).c_str(), &distortionType, 0)) isChanged = true;
        INDENT_NEXT
        if (ImGui::RadioButton(("HardClip##" + to_string(ID)).c_str(), &distortionType, 1)) isChanged = true;
        INDENT_NEXT
        if (ImGui::RadioButton(("SineFold##" + to_string(ID)).c_str(), &distortionType, 2)) isChanged = true;
        INDENT_NEXT
        if (ImGui::RadioButton(("Diode##" + to_string(ID)).c_str(), &distortionType, 3)) isChanged = true;
        INDENT_NEXT
        if (ImGui::RadioButton(("Cubed Sine##" + to_string(ID)).c_str(), &distortionType, 4)) isChanged = true;

        ImGui::PopStyleColor(2);

        ImGui::TableNextColumn();
        INDENT_NEXT
        if (ImGuiKnobs::Knob(("Out Gain##" + to_string(ID)).c_str(), &outputGainDB, -36, 24, 0.2f, "%.2f dB", ImGuiKnobVariant_WiperOnly)) isChanged = true;
        ifDoubleClicked{ (outputGainDB = 0.0f); isChanged = true; }

        ImGui::SameLine();
        INDENT_NEXT
        if (ImGuiKnobs::Knob(ADD_ID("Dry/Wet##"), &dryWetMix, 0, 1, 0.0f, "%.2f", ImGuiKnobVariant_WiperOnly)) isChanged = true;
        
        ImGui::TableNextColumn();
        
        INDENT_NEXT
        guiMtx.lock();
        tools::drawGainMonitorVertic_Stereo(GUIbuffer, 140, ID);
        guiMtx.unlock();

        ImGui::EndTable();
    }

    if (isChanged)
        parameterChanged.store(true);
}

void EffectRack::renderInterface(float nodeW) {

}

void Compressor::renderInterface(float nodeW) {

    bool isChanged = false;

    if (ImGui::BeginTable(ADD_ID("d_tab"), 3, ImGuiTableFlags_BordersInner | ImGuiTableFlags_SizingFixedFit, { 450,80 })) {

        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.11f, 0.23f, 0.34f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.11f, 0.43f, 0.54f, 1.0f));

        INDENT_NEXT
        ImGui::PopStyleColor(2);

        ImGui::PushItemWidth(100);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.11f, 0.23f, 0.34f, 1.0f));

        ImGui::Dummy({ 0,5 });

        if (ImGui::DragFloat(("Attack##" + to_string(ID)).c_str(), &attack,0, 0, 20000, "%.2f ms", ImGuiSliderFlags_Logarithmic)) {       
            isChanged = true;
        }

        if (ImGui::DragFloat(("Release##" + to_string(ID)).c_str(), &release,0, 0, 20000, "%.2f ms", ImGuiSliderFlags_Logarithmic)) {
            isChanged = true;
        }
        ImGui::Dummy({ 20,20 });

        if (ImGui::DragFloat(("Threshold##" + to_string(ID)).c_str(), &threshold,0, -100, 6, "%.2f dB", ImGuiSliderFlags_Logarithmic)) {
            isChanged = true;
        }

        if (ImGui::DragFloat(("Ratio##" + to_string(ID)).c_str(), &ratio,0, 1, 10000, "%.2f", ImGuiSliderFlags_Logarithmic)) {
            isChanged = true;
        }

        ImGui::PopItemWidth();
        ImGui::PopStyleColor();

        ImGui::Dummy({ 10,10 });


        ImGui::TableNextColumn();
        INDENT_NEXT
            if (ImGuiKnobs::Knob(("Out Gain##" + to_string(ID)).c_str(), &outputGainDB, -36, 24, 0.2f, "%.2f dB", ImGuiKnobVariant_WiperOnly)) isChanged = true;
        ifDoubleClicked{ (outputGainDB = 0.0f); isChanged = true; }

        ImGui::SameLine();
        INDENT_NEXT
            if (ImGuiKnobs::Knob(("Dry/Wet##" + to_string(ID)).c_str(), &dryWetMix, 0, 1, 0.0f, "%.2f", ImGuiKnobVariant_WiperOnly)) isChanged = true;

        ImGui::TableNextColumn();

        INDENT_NEXT
        guiMtx.lock();
        tools::drawGainMonitorVertic_Stereo(GUIbuffer, 140, ID);
        guiMtx.unlock();

        ImGui::EndTable();
    }

    if (isChanged)
        parameterChanged.store(true);
}

void ChannelUtility::renderInterface(float nodeW) {

    if (ImGui::BeginTable(ADD_ID("tab"), 2, ImGuiTableFlags_SizingFixedFit, { 230, 200 })) {

        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        INDENT_NEXT

        if (ImGuiKnobs::Knob(ADD_ID("Gain##"), &gainValueDB, -60, 60, 0.2f, "%.1f dB", ImGuiKnobVariant_WiperOnly)) parameterChanged.store(true);
        ifDoubleClicked(gainValueDB = 0);
        ImGui::SameLine();

        if (MidSideModeGUI) {
            if (ImGuiKnobs::Knob(ADD_ID("M/S##"), &midSide, -1, 1, 0.002f, "%.2f", ImGuiKnobVariant_WiperOnly)) parameterChanged.store(true);
            ifDoubleClicked(midSide = 0);
        }
        else {
            if (ImGuiKnobs::Knob(ADD_ID("L/R Pan##"), &panValue, -1, 1, 0.002f, "%.2f", ImGuiKnobVariant_WiperOnly)) parameterChanged.store(true);
            ifDoubleClicked(panValue = 0);
        }
        ImGui::SameLine(); ImGui::Dummy({ 5, 10 });


        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 10,3 });

        ImGui::Dummy({ 7, 7 });
        tools::ImShiftCursor(37, 0);
        tools::toggleButton(ADD_ID("M/S Mode##"), MidSideModeGUI);


       tools::ImShiftCursor(53, 0);
        if(tools::toggleButton(ADD_ID("Mono##"), mono))parameterChanged.store(true);
 
        ImGui::Dummy({ 7, 7 });

        tools::ImShiftCursor(26, 0);
        tools::toggleButton(ADD_ID("inv L##"), invertLeft);
        ImGui::SameLine(0,10);
        tools::toggleButton(ADD_ID("inv R##"), invertRight);

        ImGui::PopStyleVar();
      

        ImGui::TableNextColumn();

        guiMtx.lock();
        tools::drawGainMonitorVertic_Stereo(GUIbuffer, nodeW, ID, 30);
        guiMtx.unlock();


        ImGui::EndTable();
    }

     // spacing

}