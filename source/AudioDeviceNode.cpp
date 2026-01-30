#include "AudioDeviceNode.h"
#include "Program.h"

DeviceNode::DeviceNode(BlockType blocktype, juce::String initDeviceName, NodeID nodeID)
        : hardwareFIFO(FIFOSIZE),
        hardwareBuffer(2, FIFOSIZE),
        srcBuffer(2,1024),
        AudioNode(blocktype,initDeviceName,nodeID),
        devicePointer(nullptr),
        trigger(nullptr),
        trigAddress(nullptr)
{
    // attempt initial channelcount of 2 if the device allows it
    juce::String err = deviceManager.initialise(
        m_blockType == BlockType::InputDevice ? 2 : 0,
        m_blockType == BlockType::OutputDevice ? 2 : 0,
        nullptr,
        true,
        Name
    );

    sampleRateConverter = src_new(SRC_LINEAR, 2, &err4src);

    juce::AudioDeviceManager::AudioDeviceSetup setup;

    deviceManager.getAudioDeviceSetup(setup);
    setup.bufferSize = BLOCKSIZE;
    setup.sampleRate = 48e3;

    deviceManager.setAudioDeviceSetup(setup, true);
    blockSize = BLOCKSIZE;

    if (err.isNotEmpty())
        throw std::runtime_error(err.toStdString());

    if(m_blockType != BlockType::NullDevice)
    deviceManager.addAudioCallback(this);       // HERE it auto-calls "aboutToStart"
}
 
void DeviceNode::renderAsNode(float pinSize, float spacing) {
    // Adjust stereo/mono/channelcount split
    
    // ============== TITLE CARD ==============
    startPos = ImGui::GetCursorPos();
    node::BeginNode(ID);
    ImGui::Text(getBlockName().c_str()); 
    ImVec2 p = ImGui::GetCursorScreenPos();

    ImGui::SameLine(0,50);
    INDENT_NEXT
    tools::ImShiftCursor(0, -3);

    if (showInterface) {
        if (ImGui::Button(ADD_ID(">  ##"), { 40,20 })) { showInterface = !showInterface; }
        ImGui::SameLine(); ImGui::Dummy({ 1,1 });
    }
    else {
        if (ImGui::Button(ADD_ID("v  ##"), { 40,20 })) { showInterface = !showInterface; }
        ImGui::SameLine(); ImGui::Dummy({ 1,1 });
    }

    if (isMainOutput()) {    
        ImGui::SameLine();
        float radius = 8.0f;
        ImDrawList* dlm = ImGui::GetWindowDrawList();
        ImVec2 posm = ImGui::GetCursorScreenPos();
        posm = posm + radius;
        ImGui::Dummy({2 * radius + spacing,radius });
        dlm->AddCircleFilled(ImVec2(posm.x, posm.y), radius, IM_COL32(255, 50, 50, 200));
    }    
    float w = node::GetNodeSize(ID).x - pinSize - spacing - spacing;
    ImGui::GetWindowDrawList()->AddLine(p, ImVec2(p.x + w, p.y), IM_COL32(120, 120, 120, 255));

    ImGui::Dummy(ImVec2(0, 6));

    // beginpin is inside
    if (isInput()){

        if (showInterface) {
            guiMtx.lock();
            tools::drawGainMonitorVertic(GUIbuffer, 140, ID);
            guiMtx.unlock();
            ImGui::SameLine();
            INDENT_NEXT
                string knob = to_string(ID) + "##Input Gain";
            ImGui::PushID(knob.c_str());
            if (ImGuiKnobs::Knob("Gain", &deviceGain_dB, -24, 48, 0.2f, "%.2f dB", ImGuiKnobVariant_WiperOnly)) parameterChanged.store(true);
            ifDoubleClicked{ (deviceGain_dB = 0.0f); parameterChanged.store(true); }
            ImGui::PopID();
        }
       
        // ============== OUTPUT PIN ==============        
        const char* labelout = "FROM DEVICE        ";
        ImVec2 textSizeOut = ImGui::CalcTextSize(labelout);

        // Move cursor to the right edge,
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + spacing + w - (textSizeOut.x)); // to counter the 2 spacings for W
        node::BeginPin(outputPin, ed::PinKind::Output);
        ImGui::TextUnformatted(labelout);
        ImGui::SameLine();

        // Pin icon
        ImDrawList* dlo = ImGui::GetWindowDrawList();
        ImVec2 poso = ImGui::GetCursorScreenPos();
        poso.x = poso.x - textSizeOut.y - spacing;      // textSizeOut.y = circle width(spacing)
        dlo->AddCircleFilled(ImVec2(poso.x, poso.y + textSizeOut.y * 0.5f), textSizeOut.y * 0.5f, IM_COL32(250, 150, 30, 255));
        node::EndPin();
    }
    else if(isOutput())     // Draw output Node
    {
        string knob = to_string(ID) + "##Output Gain";
        const char* labelin = "    TO DEVICE";
        ImVec2 textSizeIn = ImGui::CalcTextSize(labelin);

        if (showInterface) {
            INDENT_NEXT     
            ImGui::PushID(knob.c_str());
            if (ImGuiKnobs::Knob("Gain", &deviceGain_dB, -24, 48, 0.2f, "%.2f dB", ImGuiKnobVariant_WiperOnly)) parameterChanged.store(true);
            ifDoubleClicked{ (deviceGain_dB = 0.0f); parameterChanged.store(true); }
            ImGui::PopID();
            ImGui::SameLine(0, 30);
            guiMtx.lock();
            tools::drawGainMonitorVertic(GUIbuffer, 140, ID);
            guiMtx.unlock();

        }
        // ============== PIN ==============
        node::BeginPin(inputPin, ed::PinKind::Input);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 2 * spacing);
        // Pin icon  
        ImDrawList* dli = ImGui::GetWindowDrawList();
        ImVec2 posi = ImGui::GetCursorScreenPos();
        dli->AddCircleFilled(ImVec2(posi.x, posi.y + textSizeIn.y * 0.5f), textSizeIn.y * 0.5f, IM_COL32(30, 150, 230, 255));
        ImGui::TextUnformatted(labelin);
        node::EndPin();
    }     
        
    node::EndNode();
}

void DeviceNode::selectDevice(const juce::String& nameToFind, bool isOutput)
{
    juce::OwnedArray<juce::AudioIODeviceType> types;
    deviceManager.createAudioDeviceTypes(types);

    Logger::log("Clicked Audio Device: " + nameToFind.toStdString(), level_INFO);

    for (auto* type : types)
    {

        string typeString = type->getTypeName().toStdString();
       // Logger::log("TYPE: " + typeString);

        type->scanForDevices();

        juce::StringArray deviceInputs = type->getDeviceNames(true);   // true = input
        juce::StringArray deviceOutputs = type->getDeviceNames(false);  // false = output


        // Always use Low Latency Mode
        if (typeString.find("Latency") != string::npos) {

            if (!isOutput) {
                if (deviceInputs.contains(nameToFind, false)) {
                    Logger::log("Selected Audio Input: " + nameToFind.toStdString(),level_INFO,source_DEVICE);
                    juce::AudioDeviceManager::AudioDeviceSetup setup;
                    deviceManager.getAudioDeviceSetup(setup);
                    setup.inputDeviceName = nameToFind;
                    setup.bufferSize = BLOCKSIZE;
                    deviceManager.setAudioDeviceSetup(setup, true);
                    Name = nameToFind.toStdString();
                    m_blockType = BlockType::InputDevice;

                    return;
                }
            } else {
                if (deviceOutputs.contains(nameToFind, false)) {
                    Logger::log("Selected Audio Output: " + nameToFind.toStdString(), level_INFO, source_DEVICE);
                    juce::AudioDeviceManager::AudioDeviceSetup setup;
                    deviceManager.getAudioDeviceSetup(setup);
                    setup.outputDeviceName = nameToFind;
                    setup.bufferSize = BLOCKSIZE;
                    deviceManager.setAudioDeviceSetup(setup, true);
                    Name = nameToFind.toStdString();
                    m_blockType = BlockType::OutputDevice;

                    return;
                }
            }
            Logger::log("Audio Device Could not be found",level_ERROR, source_DEVICE);
        }

    }

    audioDeviceAboutToStart(deviceManager.getCurrentAudioDevice());
}

int DeviceNode::writeToFifoFrom(const float* const* input, int numSamples) {

    int start1, size1, start2, size2;
    hardwareFIFO.prepareToWrite(numSamples, start1, size1, start2, size2);

    if (size1 > 0)
    {
        for (int ch = 0; ch < numChannels; ++ch)
            juce::FloatVectorOperations::copy(hardwareBuffer.getWritePointer(ch, start1), input[ch], size1);
    }

    if (size2 > 0) {
        for (int ch = 0; ch < numChannels; ++ch)
            juce::FloatVectorOperations::copy(hardwareBuffer.getWritePointer(ch, start2), input[ch] + size1, size2);
    }

    hardwareFIFO.finishedWrite(size1 + size2);
    return size1 + size2;
}

int DeviceNode::readFromFifoTo(float* const* output, int numSamples)
{
    int start1, size1, start2, size2;   
    fillCountBuf[next_fillCountIndex()] = hardwareFIFO.getNumReady();

    // Handle non-SRC path
    if (isMainOutput()) {
       
        hardwareFIFO.prepareToRead(numSamples, start1, size1, start2, size2);

        if (size1 > 0) {
            for (int ch = 0; ch < numChannels; ++ch)
                juce::FloatVectorOperations::copy(output[ch], hardwareBuffer.getReadPointer(ch, start1),
                    size1);
        }

        if (size2 > 0) {
            for (int ch = 0; ch < numChannels; ++ch)
                juce::FloatVectorOperations::copy(output[ch], hardwareBuffer.getReadPointer(ch, start2), size2);

        }
        hardwareFIFO.finishedRead(size1 + size2);

        return size1 + size2;
    }

    // Update PID / Ratio
    double diff = getAvgFill() - 1024.0;

    diff *= abs(tanh(0.1 * diff));
    SRC_ratio = SRC_base - (recoverStrength * diff);
    SRC_ratio = juce::jlimit(SRC_base*0.95, SRC_base*1.05, SRC_ratio);      // should not hit, more for safety
    
    // Calculate how many input samples we need to guarantee 'numSamples' out
    // add a small safety margin so the resampler doesn't run dry.
    int inputNeeded = (int)std::ceil(numSamples / SRC_ratio) + 10;
    hardwareFIFO.prepareToRead(inputNeeded, start1, size1, start2, size2);

    // Copy from ring buffer to contiguous planar work buffer
    for (int ch = 0; ch < numChannels; ++ch) {
        if (size1 > 0) srcBuffer.copyFrom(ch, 0, hardwareBuffer, ch, start1, size1);
        if (size2 > 0) srcBuffer.copyFrom(ch, size1, hardwareBuffer, ch, start2, size2);
    }

    // need a flat float array for interleaved input and output
    std::vector<float> interleavedIn(srcBuffer.getNumSamples() * numChannels);          // try to pre-allocate this, NO MEMORY ALLOCATION IN AUDIO THREAD
    std::vector<float> interleavedOut(numSamples * numChannels);

    // Planar -> Interleaved
    for (int s = 0; s < srcBuffer.getNumSamples(); ++s) {
        for (int ch = 0; ch < numChannels; ++ch) {
            interleavedIn[s * numChannels + ch] = srcBuffer.getSample(ch, s);
        }
    }

    SRC_DATA data;
    data.data_in = interleavedIn.data();
    data.input_frames = srcBuffer.getNumSamples();
    data.data_out = interleavedOut.data();
    data.output_frames = numSamples; // We want exactly this many out
    data.src_ratio = SRC_ratio;
    data.end_of_input = 0;
    

    // Perform SRC
    int error = src_process(sampleRateConverter, &data);
    if (error) {
        Logger::log("SRC Error: " + string(src_strerror(error)));
        return 0;
    }

    // Interleaved -> Planar (Back to Engine Output)
    for (int s = 0; s < data.output_frames_gen; ++s) {
        for (int ch = 0; ch < numChannels; ++ch) {
            output[ch][s] = interleavedOut[s * numChannels + ch];
        }
    }

    hardwareFIFO.finishedRead(data.input_frames_used);

    return data.output_frames_gen;
}

void DeviceNode::prepareOutput() {

    outputBuffer.clear();

    if (m_blockType == BlockType::InputDevice) {       
        readFromFifoTo(outputBuffer.getArrayOfWritePointers(), BLOCKSIZE);
        outputBuffer.applyGain(tools::decibelsToGain(deviceGain_dB));          
    }
    else if (m_blockType == BlockType::OutputDevice) {
        inputBuffer.applyGain(tools::decibelsToGain(deviceGain_dB));
        copyBuffer(&outputBuffer, &inputBuffer);    
        writeToFifoFrom(inputBuffer.getArrayOfReadPointers(), BLOCKSIZE);
    }
}

void DeviceNode::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    sampleRate = device->getCurrentSampleRate();
    blockSize = device->getCurrentBufferSizeSamples();

    SRC_base = 48000.0f / sampleRate;
    SRC_ratio = SRC_base;

    

    if(isInput())
        numChannels = device->getActiveInputChannels().countNumberOfSetBits();

    if(isOutput())
        numChannels = device->getActiveOutputChannels().countNumberOfSetBits();

    if (numChannels != 2) {
        inputBuffer.setSize(numChannels, BLOCKSIZE, false, true, true);
        outputBuffer.setSize(numChannels, BLOCKSIZE, false, true, true);
        GUIbuffer.setSize(numChannels, BLOCKSIZE, false, true, true);

        hardwareBuffer.setSize(numChannels, FIFOSIZE, false, true, true);    
        srcBuffer.setSize(numChannels, FIFOSIZE, false, true, true);


        src_reset(sampleRateConverter);
        sampleRateConverter = src_new(SRC_LINEAR, numChannels, &err4src);

    }

    devicePointer = device;

    Logger::log("Device Starting: " + device->getName().toStdString(),level_INFO);
    Logger::log("SampleRate: " + to_string(sampleRate), level_INFO);
    Logger::log("BlockSize: " + to_string(blockSize), level_INFO);
    Logger::log("NumChannels: " + to_string(numChannels), level_INFO);
}

void DeviceNode::audioDeviceIOCallbackWithContext(const float* const* inputChannelData, int numInputChannels, 
                                                  float* const* outputChannelData, int numOutputChannels, 
                                                  int numSamples, const juce::AudioIODeviceCallbackContext&) {

    if (m_blockType == BlockType::InputDevice) {

        const int fifoFill = hardwareFIFO.getNumReady();
        const int fifoCapacity = hardwareFIFO.getTotalSize();               // Drift mitigation: avoid FIFO runaway
        const bool fifoTooFull = fifoFill > (fifoCapacity * 3) / 4;         // Simple soft ceiling: drop input if FIFO is too full
        int samplesWritten = 0;                                             // Input overflow → intentionally drop this block
                                                                            // This protects output timing stability

        if (!fifoTooFull)
        {
            const float* data = this->hardwareBuffer.getWritePointer(0);

            samplesWritten = writeToFifoFrom(inputChannelData, numSamples);
        } else {

        }
    }

    if (m_blockType == BlockType::OutputDevice) {
        const float* data = this->hardwareBuffer.getReadPointer(0);
        int samplesRead = readFromFifoTo(outputChannelData, numSamples);
        if (samplesRead < numSamples)
        {
            // Underrun → zero remaining output
            for (int ch = 0; ch < numOutputChannels; ++ch)      // only triggered once i believe, at the start?
            {
                juce::FloatVectorOperations::clear(
                    outputChannelData[ch] + samplesRead,
                    numSamples - samplesRead);
            }
        }

       if(trigger != nullptr && hardwareFIFO.getNumReady() < BLOCKSIZE << 1)        // also aim at 1024
        trigger->signal();                                                  

    }
}
void DeviceNode::setAsMainOutput(bool set) {   
    trigger = set ? trigAddress : nullptr;
}
void DeviceNode::setTriggerAddress(juce::WaitableEvent* trigger) {
    this->trigAddress = trigger; 
}
