#include "AudioSoundboardNode.h"
#include "FileIO.h"
#include "Logger.h"

AudioSoundboardNode::AudioSoundboardNode(BlockType blocktype, juce::String initName, NodeID nodeID)
    : AudioNode(blocktype, initName, nodeID),
      diskThread("routeCarrier soundboard I/O"),
      padBuffer(2, BLOCKSIZE)
{
    formatManager.registerBasicFormats();
    diskThread.startThread();

    for (auto& pad : pads)
        pad.transport.prepareToPlay(BLOCKSIZE, sampleRate);

    // presets: names become XML attributes, so keep them to letters / digits / underscore
    currentState.defineSetting("Gain", &masterGainDB);
    for (int i = 0; i < kNumPads; ++i) {
        const string p = "Pad" + to_string(i + 1) + "_";
        currentState.defineSetting(p + "File", &pads[i].path);      // std::string specialization
        currentState.defineSetting(p + "Gain", &pads[i].gainDB);
        currentState.defineSetting(p + "Mode", &pads[i].mode);
    }
}

AudioSoundboardNode::~AudioSoundboardNode()
{
    for (auto& pad : pads) detachPad(pad);
    diskThread.stopThread(2000);
}

// ============================================================================
//  CONTROL
// ============================================================================
void AudioSoundboardNode::detachPad(Pad& pad)
{
    pad.loaded.store(false);
    pad.transport.stop();
    pad.transport.setSource(nullptr);       // takes the transport lock, audio thread is safe
    pad.reader.reset();
}

bool AudioSoundboardNode::loadPad(int index, const juce::File& file)
{
    if (index < 0 || index >= kNumPads) return false;
    auto& pad = pads[index];

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    if (reader == nullptr) {
        Logger::log("Soundboard: could not open " + file.getFullPathName().toStdString(), level_ERROR, source_FILES);
        return false;
    }

    detachPad(pad);

    const double fileRate = reader->sampleRate;
    pad.channels.store((int)reader->numChannels);
    pad.lengthSec = fileRate > 0 ? (double)reader->lengthInSamples / fileRate : 0.0;

    pad.reader = std::make_unique<juce::AudioFormatReaderSource>(reader.release(), true);
    pad.reader->setLooping(pad.mode == Loop);
    pad.transport.setSource(pad.reader.get(), 32768, &diskThread, fileRate, 2);

    pad.path = file.getFullPathName().toStdString();
    pad.displayName = file.getFileNameWithoutExtension().toStdString();
    pad.missing = false;
    pad.loaded.store(true);
    return true;
}

void AudioSoundboardNode::clearPad(int index)
{
    if (index < 0 || index >= kNumPads) return;
    auto& pad = pads[index];
    detachPad(pad);
    pad.path.clear();
    pad.displayName.clear();
    pad.missing = false;
    pad.lengthSec = 0.0;
}

void AudioSoundboardNode::triggerPad(int index)
{
    if (index < 0 || index >= kNumPads) return;
    auto& pad = pads[index];
    if (!pad.loaded.load()) return;

    if (pad.mode == Loop && pad.transport.isPlaying()) {
        pad.transport.stop();
        pad.transport.setPosition(0.0);
        return;
    }

    pad.transport.setPosition(0.0);
    pad.transport.start();
}

void AudioSoundboardNode::releasePad(int index)
{
    if (index < 0 || index >= kNumPads) return;
    auto& pad = pads[index];
    if (pad.mode == Hold && pad.loaded.load()) {
        pad.transport.stop();
        pad.transport.setPosition(0.0);
    }
}

void AudioSoundboardNode::stopAll()
{
    for (auto& pad : pads) {
        if (!pad.loaded.load()) continue;
        pad.transport.stop();
        pad.transport.setPosition(0.0);
    }
}

void AudioSoundboardNode::pollPlayback()
{
    // one-shots that reached the end: rewind, so the start is already buffered for the next trigger
    for (auto& pad : pads) {
        if (pad.loaded.load() && pad.transport.isPlaying() && pad.transport.hasStreamFinished()) {
            pad.transport.stop();
            pad.transport.setPosition(0.0);
        }
    }
}

void AudioSoundboardNode::onPresetLoaded()
{
    for (int i = 0; i < kNumPads; ++i) {
        auto& pad = pads[i];
        pad.mode = juce::jlimit(0, 2, pad.mode);

        if (pad.path.empty()) continue;

        juce::File f(juce::String::fromUTF8(pad.path.c_str()));
        if (f.existsAsFile() && loadPad(i, f)) continue;

        // keep the path (so saving the preset again doesn't lose it), just flag the pad
        pad.missing = true;
        pad.displayName = f.getFileNameWithoutExtension().toStdString();
        Logger::log("Soundboard: pad " + to_string(i + 1) + " file not found: " + pad.path, level_WARNING, source_FILES);
    }
}

// ============================================================================
//  AUDIO THREAD
// ============================================================================
void AudioSoundboardNode::prepareOutput()
{
    outputBuffer.clear();

    for (auto& pad : pads)
    {
        if (!pad.loaded.load()) continue;

        padBuffer.clear();
        juce::AudioSourceChannelInfo info(&padBuffer, 0, BLOCKSIZE);
        pad.transport.getNextAudioBlock(info);      // silence when idle (also finishes the stop fade-out)

        if (pad.channels.load() == 1 && padBuffer.getNumChannels() > 1)
            padBuffer.copyFrom(1, 0, padBuffer, 0, 0, BLOCKSIZE);

        const float g = tools::decibelsToGain(pad.gainDB);
        const int chs = min(outputBuffer.getNumChannels(), padBuffer.getNumChannels());
        for (int ch = 0; ch < chs; ++ch)
            outputBuffer.addFrom(ch, 0, padBuffer, ch, 0, BLOCKSIZE, g);
    }

    outputBuffer.applyGain(tools::decibelsToGain(masterGainDB));
    if (bypass) outputBuffer.applyGain(0);
}

// ============================================================================
//  GUI
// ============================================================================
void AudioSoundboardNode::renderAsNode(float pinSize, float spacing)
{
    pollPlayback();

    // ============== TITLE CARD ==============
    startPos = ImGui::GetCursorPos();
    node::BeginNode(ID);

    ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + 150);
    ImGui::TextUnformatted(getBlockName().c_str());
    ImGui::PopTextWrapPos();
    ImVec2 p = ImGui::GetCursorScreenPos();

    ImGui::SameLine(0, 50);
    tools::ImShiftCursor(0, -3);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 5,3 });
    tools::toggleButton(ADD_ID("M##"), bypass, { 0,0 });
    ImGui::SetItemTooltip("Mute");
    ImGui::SameLine();
    tools::ImShiftCursor(0, -3);

    if (showInterface) {
        if (ImGui::Button(ADD_ID(">##"), { 0,0 })) { toggle(showInterface); }
    } else {
        if (ImGui::Button(ADD_ID("v##"), { 0,0 })) { toggle(showInterface); }
    }
    ImGui::SameLine(); ImGui::Dummy({ 1,1 });
    ImGui::PopStyleVar();

    float w = node::GetNodeSize(ID).x - pinSize - spacing - spacing;
    ImGui::GetWindowDrawList()->AddLine(p, ImVec2(p.x + w, p.y), IM_COL32(120, 120, 120, 255));
    ImGui::Dummy(ImVec2(310, 6));       // minimum node width (4 pads)

    // ============== INTERFACE ==============
    if (showInterface) {
        const ImVec2 padSize(72, 46);
        for (int i = 0; i < kNumPads; ++i) {
            if (i % kColumns) ImGui::SameLine();
            renderPad(i, padSize);
        }

        ImGui::Dummy({ 0, 4 });
        renderPadEditor();
        ImGui::Dummy({ 0, 4 });

        // master gain + meter + stop all
        INDENT_NEXT
        string knob = to_string(ID) + "##Board Gain";
        ImGui::PushID(knob.c_str());
        if (ImGuiKnobs::Knob("Gain", &masterGainDB, -48, 24, 0.2f, "%.2f dB", ImGuiKnobVariant_WiperOnly)) parameterChanged.store(true);
        ifDoubleClicked{ masterGainDB = 0.0f; parameterChanged.store(true); }
        ImGui::PopID();
        ImGui::SameLine(0, 30);

        guiMtx.lock();
        if (numChannels > 1) tools::drawGainMonitorVertic_Stereo(GUIbuffer, 140, ID);
        else                 tools::drawGainMonitorVertic(GUIbuffer, 140, ID);
        guiMtx.unlock();

        ImGui::SameLine(0, 20);
        if (ImGui::Button(ADD_ID("Stop All##"))) stopAll();
    }

    // ============== OUTPUT PIN ==============
    if (outputPin)
    {
        const char* labelout = "SOUNDBOARD        ";
        ImVec2 textSizeOut = ImGui::CalcTextSize(labelout);

        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + spacing + w - textSizeOut.x);
        node::BeginPin(outputPin, ed::PinKind::Output);
        ImGui::TextUnformatted(labelout);
        ImGui::SameLine();

        ImDrawList* dlo = ImGui::GetWindowDrawList();
        ImVec2 poso = ImGui::GetCursorScreenPos();
        poso.x = poso.x - textSizeOut.y - spacing;
        dlo->AddCircleFilled(ImVec2(poso.x, poso.y + textSizeOut.y * 0.5f), textSizeOut.y * 0.5f, IM_COL32(250, 150, 30, 255));
        node::EndPin();
    }

    node::EndNode();

    runPendingDialogs();
}

void AudioSoundboardNode::renderPad(int index, const ImVec2& size)
{
    auto& pad = pads[index];
    const bool loaded = pad.loaded.load();
    const bool playing = loaded && pad.transport.isPlaying();

    string label;
    if (loaded || pad.missing) label = pad.displayName;
    else                       label = "(empty)";
    label += "##pad" + to_string(index) + "_" + to_string(ID);

    int colors = 0;
    if (playing) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.75f, 0.95f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.15f, 0.75f, 0.95f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 0, 0, 1));
        colors = 3;
    } else if (pad.missing) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.45f, 0.12f, 0.12f, 1.0f));
        colors = 1;
    }

    ImGui::Button(label.c_str(), size);
    if (ImGui::IsItemActivated()) { selectedPad = index; triggerPad(index); }       // fire on mouse-down, not release
    if (ImGui::IsItemDeactivated()) releasePad(index);

    if (colors) ImGui::PopStyleColor(colors);

    // selection outline + play progress along the bottom edge
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 mn = ImGui::GetItemRectMin();
    const ImVec2 mx = ImGui::GetItemRectMax();

    if (index == selectedPad)
        dl->AddRect(mn, mx, IM_COL32(230, 230, 230, 200), 4.0f, 0, 1.5f);

    if (playing && pad.lengthSec > 0.0) {
        const float frac = juce::jlimit(0.0f, 1.0f, (float)(pad.transport.getCurrentPosition() / pad.lengthSec));
        dl->AddRectFilled(ImVec2(mn.x, mx.y - 3), ImVec2(mn.x + (mx.x - mn.x) * frac, mx.y), IM_COL32(255, 255, 255, 220));
    }
}

void AudioSoundboardNode::renderPadEditor()
{
    auto& pad = pads[selectedPad];

    ImGui::Text("Pad %d:  %s", selectedPad + 1,
        (pad.loaded.load() || pad.missing) ? pad.displayName.c_str() : "(empty)");

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 10, 3 });
    if (ImGui::Button(ADD_ID("Load...##"))) pendingLoadPad = selectedPad;
    ImGui::SameLine();
    if (ImGui::Button(ADD_ID("Clear##"))) clearPad(selectedPad);
    ImGui::PopStyleVar();

    ImGui::SameLine();
    ImGui::SetNextItemWidth(90);
    if (ImGui::Combo(ADD_ID("##mode"), &pad.mode, "One-shot\0Loop\0Hold\0")) {
        if (pad.reader) pad.reader->setLooping(pad.mode == Loop);
    }

    ImGui::SetNextItemWidth(260);
    ImGui::SliderFloat(ADD_ID("##padgain"), &pad.gainDB, -48.0f, 12.0f, "Pad gain %.1f dB");
    ifDoubleClicked{ pad.gainDB = 0.0f; }

    if (pad.missing)
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "File not found: %s", pad.path.c_str());
}

void AudioSoundboardNode::runPendingDialogs()
{
    if (pendingLoadPad >= 0) {
        const int idx = pendingLoadPad;
        pendingLoadPad = -1;
        if (fileIO) {
            string path = fileIO->OpenFileDialog(f_audio);
            if (!path.empty()) loadPad(idx, juce::File(juce::String(path)));
        }
    }
}
