#include "AudioDSPNode.h"

DSPNode::DSPNode(BlockType blocktype, juce::String initDeviceName, NodeID nodeID) :
    AudioNode(blocktype, initDeviceName, nodeID),
    numInputs(2),
    numOutputs(2) {
}

void DSPNode::renderAsNode(float pinSize, float spacing) {
    node::BeginNode(ID);

    startPos = ImGui::GetCursorPos();
    const float nodeLeftX = ImGui::GetCursorScreenPos().x;

    // ======- title =======
    ImGui::Text(getBlockName().c_str());
    ImGui::SameLine();
    ImGui::Dummy({ 120, 5 });

    // ---- measure real width ----
    const float minW = 240.0f;                 // title + IN + OUT + buttons fit in this
    float contentW = minW;

    ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::Dummy(ImVec2(0, 6));

    if (showInterface) {
        renderInterface(contentW);             // pass stable value
        float used = ImGui::GetCurrentWindow()->DC.CursorMaxPos.x - nodeLeftX;
        contentW = std::max(contentW, used);
    }

    // separator line uses the stable width
    ImGui::GetWindowDrawList()->AddLine(p, ImVec2(p.x + contentW, p.y), IM_COL32(120, 120, 120, 255));

    // ---- pin row: IN on the left, OUT right-aligned to contentW ----
    const float rowX = ImGui::GetCursorPosX();
    const float r = ImGui::GetTextLineHeight() * 0.5f;

    // input
    node::BeginPin(inputPin, ed::PinKind::Input);
    {
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImGui::GetWindowDrawList()->AddCircleFilled({ pos.x + r, pos.y + r }, r, IM_COL32(30, 150, 230, 255));
        ImGui::Dummy({ 2 * r, 2 * r });
        ImGui::SameLine();
        ImGui::TextUnformatted("IN");
    }
    node::EndPin();

    // output
    const char* labelOut = "OUT";
    const float outW = ImGui::CalcTextSize(labelOut).x + spacing + 2 * r;
    ImGui::SameLine();
    ImGui::SetCursorPosX(rowX + contentW - outW);
    node::BeginPin(outputPin, ed::PinKind::Output);
    {
        ImGui::TextUnformatted(labelOut);
        ImGui::SameLine(0, spacing);
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImGui::GetWindowDrawList()->AddCircleFilled({ pos.x + r, pos.y + r }, r, IM_COL32(250, 150, 30, 255));
        ImGui::Dummy({ 2 * r, 2 * r });
    }
    node::EndPin();

    // ---- buttons, right-aligned to the same stable width ----
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 5, 3 });
    const float btnW = ImGui::CalcTextSize(">").x + 10;
    ImGui::SetCursorPos({ startPos.x + contentW - btnW, startPos.y - 3 });

    if (ImGui::Button(showInterface ? ADD_ID(">##") : ADD_ID("v##"))) toggle(showInterface);

    ImGui::SetCursorPos({ startPos.x + contentW - btnW - 65, startPos.y - 3 });
    tools::toggleButton(ADD_ID("bypass##"), bypass);
    ImGui::PopStyleVar();

    node::EndNode();
}

void DSPNode::prepareOutput() {
    outputBuffer.clear();
    outputBuffer.makeCopyOf(inputBuffer, false);    // output buf is the process context
    juce::MidiBuffer empt;
    
    if(!bypass)
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