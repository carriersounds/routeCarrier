#include "AudioFileNode.h"
#include "FileIO.h"
#include "Logger.h"

namespace {
    constexpr int kBitDepths[3] = { 16, 24, 32 };

    std::string fmtTime(double s) {
        if (s < 0) s = 0;
        int m = (int)(s / 60.0);
        double r = s - m * 60.0;
        char b[32];
        snprintf(b, sizeof(b), "%d:%04.1f", m, r);
        return b;
    }
}

AudioFileNode::AudioFileNode(BlockType blocktype, juce::String initName, NodeID nodeID)
    : AudioNode(blocktype, initName, nodeID),
      diskThread("routeCarrier file I/O")
{
    formatManager.registerBasicFormats();   // wav, aiff, flac (+ mp3/m4a decoding if JUCE's Windows media format is enabled)
    diskThread.startThread();

    // Engine runs at a fixed internal rate / block size
    transport.prepareToPlay(BLOCKSIZE, sampleRate);

    // presets
    currentState.defineSetting("Gain", &gainDB);
    currentState.defineSetting("Loop", &looping);
    if (isPlayer())
        currentState.defineSetting("File", &filePath);      // std::string specialization
}

void AudioFileNode::onPresetLoaded()
{
    if (!isPlayer() || filePath.empty()) return;

    juce::File f(juce::String::fromUTF8(filePath.c_str()));
    if (f.existsAsFile())
        loadFile(filePath);
    else    // keep filePath so re-saving the preset doesn't lose it
        Logger::log("Player: file not found: " + filePath, level_WARNING, source_FILES);
}

AudioFileNode::~AudioFileNode()
{
    stopRecording();
    transport.stop();
    transport.setSource(nullptr);
    readerSource.reset();
    diskThread.stopThread(2000);
}

// ============================================================================
//  PLAYER
// ============================================================================
bool AudioFileNode::loadFile(string fpath)
{

    const juce::File file(fpath);

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    if (reader == nullptr) {
        Logger::log("Could not open audio file: " + file.getFullPathName().toStdString(), level_ERROR, source_FILES);
        return false;
    }

    // detach the old source before destroying it (setSource takes the transport's callback lock)
    transport.stop();
    transport.setSource(nullptr);
    readerSource.reset();

    const double fileRate = reader->sampleRate;
    fileChannels.store((int)reader->numChannels);
    fileLengthSec = fileRate > 0 ? (double)reader->lengthInSamples / fileRate : 0.0;

    readerSource = std::make_unique<juce::AudioFormatReaderSource>(reader.release(), true);
    readerSource->setLooping(looping);

    // 64k read-ahead on the disk thread; transport resamples fileRate -> engine rate
    transport.setSource(readerSource.get(), 65536, &diskThread, fileRate, 2);

    currentFile = file;
    filePath = file.getFullPathName().toStdString();
    Name = file.getFileName();      // shows up as the node title
    seekPos = 0.0f;

    Logger::log("Loaded audio file: " + file.getFullPathName().toStdString(), level_INFO, source_FILES);
    
    Logger::log("file rate " + to_string(fileRate) + " -> engine rate " + to_string(sampleRate), level_INFO, source_FILES);
    return true;
}

void AudioFileNode::unloadFile()
{
    transport.stop();
    transport.setSource(nullptr);
    readerSource.reset();
    currentFile = juce::File();
    filePath.clear();
    fileLengthSec = 0.0;
    Name = "Audio File Player";
}

void AudioFileNode::play()
{
    if (!hasFile()) return;
    if (transport.hasStreamFinished()) transport.setPosition(0.0);
    transport.start();
}

void AudioFileNode::pause()
{
    transport.stop();   // keeps position
}

void AudioFileNode::stopAndRewind()
{
    transport.stop();
    transport.setPosition(0.0);
}

void AudioFileNode::pollPlaybackState()
{
    // non-looping file reached its end -> rewind so Play starts again from 0
    if (isPlayer() && readerSource && transport.isPlaying() && transport.hasStreamFinished()) {
        transport.stop();
        transport.setPosition(0.0);
    }
}

// ============================================================================
//  RECORDER
// ============================================================================
bool AudioFileNode::startRecording(const juce::File& file)
{
    stopRecording();

    auto* format = formatManager.findFormatForFileExtension(file.getFileExtension());
    if (format == nullptr) {
        Logger::log("No audio format for extension: " + file.getFileExtension().toStdString(), level_ERROR, source_FILES);
        return false;
    }

    file.deleteFile();      // FileOutputStream appends to existing files; the save dialog already asked to overwrite
    std::unique_ptr<juce::FileOutputStream> stream = file.createOutputStream();
    if (stream == nullptr || stream->failedToOpen()) {
        Logger::log("Could not open file for writing: " + file.getFullPathName().toStdString(), level_ERROR, source_FILES);
        return false;
    }

    int bits = kBitDepths[juce::jlimit(0, 2, bitDepthIdx)];
    auto possible = format->getPossibleBitDepths();
    if (!possible.contains(bits))
        bits = possible.isEmpty() ? 24 : possible.getLast();       // e.g. flac has no 32-bit

    juce::AudioFormatWriter* rawWriter = format->createWriterFor(stream.get(),
                                                                 sampleRate,
                                                                 (unsigned int)outputBuffer.getNumChannels(),
                                                                 bits,
                                                                 juce::StringPairArray{},
                                                                 0);
    if (rawWriter == nullptr) {
        // e.g. mp3 - JUCE can decode it but has no encoder
        Logger::log("Cannot write this file format (" + file.getFileExtension().toStdString() + "), use wav / aif / flac", level_ERROR, source_FILES);
        return false;
    }
    stream.release();       // now owned by the writer

    auto threaded = std::make_unique<juce::AudioFormatWriter::ThreadedWriter>(rawWriter, diskThread, 65536);

    {
        const juce::SpinLock::ScopedLockType sl(writerLock);
        writer = std::move(threaded);
    }

    recordFile = file;
    Name = file.getFileName();
    samplesRecorded.store(0);
    overruns.store(0);
    recording.store(true);

    Logger::log("Recording to: " + file.getFullPathName().toStdString(), level_INFO, source_FILES);
    return true;
}

void AudioFileNode::stopRecording()
{
    recording.store(false);

    std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> old;
    {
        const juce::SpinLock::ScopedLockType sl(writerLock);
        old = std::move(writer);
    }
    old.reset();    // flushes the FIFO and finalizes the file header, on the GUI thread

    if (isRecorder() && recordFile != juce::File())
        Logger::log("Recording stopped: " + recordFile.getFullPathName().toStdString(), level_INFO, source_FILES);
}

// ============================================================================
//  AUDIO THREAD
// ============================================================================
void AudioFileNode::prepareOutput()
{
    outputBuffer.clear();
    const float gain = tools::decibelsToGain(gainDB);

    if (isPlayer())
    {
        juce::AudioSourceChannelInfo info(&outputBuffer, 0, BLOCKSIZE);
        transport.getNextAudioBlock(info);      // silence if stopped / no file

        // mono file -> make sure both channels carry signal
        if (fileChannels.load() == 1 && outputBuffer.getNumChannels() > 1)
            outputBuffer.copyFrom(1, 0, outputBuffer, 0, 0, BLOCKSIZE);

        outputBuffer.applyGain(gain);
        if (bypass) outputBuffer.applyGain(0);
    }
    else if (isRecorder())
    {
        copyBuffer(&outputBuffer, &inputBuffer);        // pass-through (also feeds the meter)
        outputBuffer.applyGain(gain);

        if (!bypass && recording.load())
        {
            const juce::SpinLock::ScopedTryLockType sl(writerLock);
            if (sl.isLocked() && writer != nullptr)
            {
                if (!writer->write(outputBuffer.getArrayOfReadPointers(), BLOCKSIZE))
                    overruns.fetch_add(1);          // disk can't keep up, block was dropped
                samplesRecorded.fetch_add(BLOCKSIZE);
            }
        }
    }
}

// ============================================================================
//  GUI
// ============================================================================
void AudioFileNode::renderAsNode(float pinSize, float spacing)
{
    pollPlaybackState();

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
    tools::toggleButton(isPlayer() ? ADD_ID("M##") : ADD_ID("P##"), bypass, { 0,0 });
    ImGui::SetItemTooltip(isPlayer() ? "Mute" : "Pause recording");
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
    ImGui::Dummy(ImVec2(300, 6));       // minimum node width

    // ============== INTERFACE ==============
    if (showInterface) {
        if (isPlayer()) renderPlayerInterface();
        else            renderRecorderInterface();
    }

    // ============== PIN ==============
    if (isPlayer() && outputPin)
    {
        const char* labelout = "FROM FILE        ";
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
    else if (isRecorder() && inputPin)
    {
        const char* labelin = "    TO FILE";
        ImVec2 textSizeIn = ImGui::CalcTextSize(labelin);

        node::BeginPin(inputPin, ed::PinKind::Input);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 2 * spacing);

        ImDrawList* dli = ImGui::GetWindowDrawList();
        ImVec2 posi = ImGui::GetCursorScreenPos();
        dli->AddCircleFilled(ImVec2(posi.x, posi.y + textSizeIn.y * 0.5f), textSizeIn.y * 0.5f, IM_COL32(30, 150, 230, 255));
        ImGui::TextUnformatted(labelin);
        node::EndPin();
    }

    node::EndNode();

    runPendingDialogs();
}

void AudioFileNode::renderPlayerInterface()
{
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 10, 3 });

    // ---- transport row ----
    if (ImGui::Button(ADD_ID("Load...##"))) pendingLoadDialog = true;
    ImGui::SameLine();

    ImGui::BeginDisabled(!hasFile());
    const bool playing = isPlaying();
    const string playLabel = string(playing ? "Pause" : "Play") + "##pp" + to_string(ID);
    if (ImGui::Button(playLabel.c_str())) { if (playing) pause(); else play(); }
    ImGui::SameLine();
    if (ImGui::Button(ADD_ID("Stop##"))) stopAndRewind();
    ImGui::SameLine();
    if (tools::toggleButton(ADD_ID("Loop##"), looping) && readerSource)
        readerSource->setLooping(looping);
    ImGui::EndDisabled();
    ImGui::PopStyleVar();

    // ---- seek bar ----
    ImGui::BeginDisabled(!hasFile());
    if (!seeking) seekPos = (float)transport.getCurrentPosition();
    ImGui::SetNextItemWidth(260);
    ImGui::SliderFloat(ADD_ID("##seek"), &seekPos, 0.0f, (float)std::max(fileLengthSec, 0.001), "");
    seeking = ImGui::IsItemActive();
    if (ImGui::IsItemDeactivatedAfterEdit()) transport.setPosition((double)seekPos);
    ImGui::EndDisabled();

    ImGui::Text("%s / %s", fmtTime(seekPos).c_str(), fmtTime(fileLengthSec).c_str());
    ImGui::Dummy({ 0, 4 });

    // ---- gain + meter ----
    INDENT_NEXT
    string knob = to_string(ID) + "##File Gain";
    ImGui::PushID(knob.c_str());
    if (ImGuiKnobs::Knob("Gain", &gainDB, -48, 48, 0.2f, "%.2f dB", ImGuiKnobVariant_WiperOnly)) parameterChanged.store(true);
    ifDoubleClicked{ gainDB = 0.0f; parameterChanged.store(true); }
    ImGui::PopID();
    ImGui::SameLine(0, 30);

    guiMtx.lock();
    if (numChannels > 1) tools::drawGainMonitorVertic_Stereo(GUIbuffer, 140, ID);
    else                 tools::drawGainMonitorVertic(GUIbuffer, 140, ID);
    guiMtx.unlock();
}

void AudioFileNode::renderRecorderInterface()
{
    const bool rec = isRecording();

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 10, 3 });
    if (!rec) {
        if (ImGui::Button(ADD_ID("Record...##"))) pendingRecordDialog = true;
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.1f, 0.1f, 1.0f));
        if (ImGui::Button(ADD_ID("Stop##"))) stopRecording();
        ImGui::PopStyleColor();
    }
    ImGui::PopStyleVar();

    ImGui::SameLine();
    ImGui::BeginDisabled(rec);
    ImGui::SetNextItemWidth(130);
    static const char* depths[] = { "16-bit", "24-bit", "32-bit float" };
    ImGui::Combo(ADD_ID("##bits"), &bitDepthIdx, depths, 3);
    ImGui::EndDisabled();

    const double secs = (double)samplesRecorded.load() / sampleRate;
    if (rec) {
        ImGui::TextColored(ImVec4(1.0f, 0.25f, 0.25f, 1.0f), bypass ? "PAUSED  %s" : "REC  %s", fmtTime(secs).c_str());
        if (overruns.load() > 0)
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Disk too slow, %d blocks dropped", overruns.load());
    } else {
        ImGui::TextUnformatted(recordFile == juce::File() ? "Not recording" : ("Last: " + recordFile.getFileName().toStdString()).c_str());
    }
    ImGui::Dummy({ 0, 4 });

    // ---- gain + meter ----
    INDENT_NEXT
    string knob = to_string(ID) + "##Rec Gain";
    ImGui::PushID(knob.c_str());
    if (ImGuiKnobs::Knob("Gain", &gainDB, -48, 48, 0.2f, "%.2f dB", ImGuiKnobVariant_WiperOnly)) parameterChanged.store(true);
    ifDoubleClicked{ gainDB = 0.0f; parameterChanged.store(true); }
    ImGui::PopID();
    ImGui::SameLine(0, 30);

    guiMtx.lock();
    if (numChannels > 1) tools::drawGainMonitorVertic_Stereo(GUIbuffer, 140, ID);
    else                 tools::drawGainMonitorVertic(GUIbuffer, 140, ID);
    guiMtx.unlock();
}

void AudioFileNode::runPendingDialogs()
{
    if (pendingLoadDialog) {
        pendingLoadDialog = false;
    
        string path = FileIO::OpenFileDialog(f_audio);
        if (!path.empty()) loadFile(path);
        
    }

    if (pendingRecordDialog) {
        pendingRecordDialog = false;

        string path;
        path = FileIO::saveFileDialog(f_audio);

        if(path.empty()) path = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                        .getChildFile("routeCarrier_" + juce::Time::getCurrentTime().formatted("%Y-%m-%d_%H.%M.%S") + ".wav")
                        .getFullPathName().toStdString();

        if (!path.empty()) startRecording(juce::File(juce::String(path)));
    }
}
