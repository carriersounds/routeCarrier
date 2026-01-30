#include "AudioDSPNode.h"


DSPNode::DSPNode(BlockType blocktype, juce::String initDeviceName, NodeID nodeID) :
    AudioNode(blocktype, initDeviceName, nodeID),
    numInputs(2),
    numOutputs(2) {

}

void DSPNode::renderAsNode(float pinSize, float spacing) {

    node::BeginNode(ID);

    // ============== TITLE ============== 
    startPos = ImGui::GetCursorPos();
    ImGui::Text(getBlockName().c_str());
    ImGui::SameLine(); 
    ImGui::Dummy({ 100,5 });

    ImVec2 p = ImGui::GetCursorScreenPos();
    float w = node::GetNodeSize(ID).x - pinSize - spacing - spacing;
    ImGui::GetWindowDrawList()->AddLine(p, ImVec2(p.x + w, p.y), IM_COL32(120, 120, 120, 255));
    ImGui::Dummy(ImVec2(0, 6));

    // ============== PARAMETERS / IMPLEMENTED IN DERIVED FX =====

    if(showInterface)
    renderInterface(w);     

    // ============== INPUT PIN ============== 
    const char* labelin = "    IN";
    ImVec2 textSizeIn = ImGui::CalcTextSize(labelin);
    node::BeginPin(inputPin, ed::PinKind::Input);
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 2 * spacing);

    // Input Pin icon   
    ImDrawList* dli = ImGui::GetWindowDrawList();
    ImVec2 posi = ImGui::GetCursorScreenPos();
    dli->AddCircleFilled(ImVec2(posi.x, posi.y + textSizeIn.y * 0.5f), textSizeIn.y * 0.5f, IM_COL32(30, 150, 230, 255));
    ImGui::TextUnformatted(labelin);
    node::EndPin();

    // ============== OUTPUT PIN ==============        
    const char* labelout = "OUT       ";
    ImVec2 textSizeOut = ImGui::CalcTextSize(labelout);
    ImGui::SameLine();

    // Move cursor to the right edge.. different in debug/release for some reason
    float preciseOffset = 55.0f;
#ifdef DEBUG
    preciseOffset = 53.0f
#endif // DEBUG

    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + node::GetNodeSize(ID).x - (textSizeOut.x + spacing + preciseOffset));
    node::BeginPin(outputPin, ed::PinKind::Output);
    ImGui::TextUnformatted(labelout);                   // this determines the feedback loop for the node size for some reason
    ImGui::SameLine();

    // Output Pin icon
    ImDrawList* dlo = ImGui::GetWindowDrawList();
    ImVec2 poso = ImGui::GetCursorScreenPos() + ImVec2(spacing, 0);
    poso.x = poso.x - textSizeOut.y - spacing;
    dlo->AddCircleFilled(ImVec2(poso.x, poso.y + textSizeOut.y * 0.5f), textSizeOut.y * 0.5f, IM_COL32(250, 150, 30, 255));
    node::EndPin();


    // collapse button
    ImGui::SetNextItemWidth(100);
    ImGui::SetCursorPosX(startPos.x + node::GetNodeSize(ID).x - (spacing + preciseOffset));
    ImGui::SetCursorPosY(startPos.y - 3);
    if (showInterface) {
        if (ImGui::Button(ADD_ID(">  ##"),{40,20})) { showInterface = !showInterface; }
    } else {
        if (ImGui::Button(ADD_ID("v  ##"), { 40,20 })) { showInterface = !showInterface; }
    }



    node::EndNode();
}

void DSPNode::prepareOutput() {
    outputBuffer.clear();
    outputBuffer.makeCopyOf(inputBuffer, false);    // output buf is the process context
    juce::MidiBuffer empt;                          // function needs it RIP
    processBlock(outputBuffer, empt);
}

void DSPNode::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::dsp::AudioBlock<float> block(buffer);
    processDSP(block);
}

void DSPNode::prepareToPlay(double sampleRate, int blockSize)
{
    juce::dsp::ProcessSpec spec{sampleRate,blockSize,numOutputs};

    prepareDSP(spec);
}

bool DSPNode::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet().size() == numInputs &&
        layouts.getMainOutputChannelSet().size() == numOutputs;
}