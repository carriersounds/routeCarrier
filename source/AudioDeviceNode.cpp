#include "AudioDeviceNode.h"
#include "Program.h"

DeviceNode::DeviceNode(BlockType blocktype, juce::String initDeviceName, NodeID nodeID)
        : hardwareFIFO(FIFOSIZE),
        hardwareBuffer(2, FIFOSIZE),
        AudioNode(blocktype,initDeviceName,nodeID),
        devicePointer(nullptr),
        trigger(nullptr),trigAddress(nullptr)
{
    // attempt initial channelcount of 2 if the device allows it
    juce::String err = deviceManager.initialise(
        m_blockType == BlockType::InputDevice ? 2 : 0,
        m_blockType == BlockType::OutputDevice ? 2 : 0,
        nullptr,
        true,
        Name
    );

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

    // ++ ADD LEVEL METER + GAIN


         // Draw input Node
    
    // ============== TITLE CARD ==============
    node::BeginNode(ID);

    ImGui::Text(getBlockName().c_str()); 
    ImVec2 p = ImGui::GetCursorScreenPos();

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

    guiMtx.lock();
    tools::drawGainMonitorHoriz(GUIbuffer, w, ID);
    guiMtx.unlock();

    ImGui::Dummy(ImVec2(0, 10));

    if (isInput()){

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

        // ============== PIN ==============
        const char* labelin = "    TO DEVICE";
        ImVec2 textSizeIn = ImGui::CalcTextSize(labelin);
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

void DeviceNode::prepareOutput() {

    outputBuffer.clear();

    if (m_blockType == BlockType::InputDevice) {       
        readFromFifoTo(outputBuffer.getArrayOfWritePointers(), BLOCKSIZE);
    }
    else if (m_blockType == BlockType::OutputDevice) {
        copyBuffer(&outputBuffer, &inputBuffer);    
        writeToFifoFrom(inputBuffer.getArrayOfReadPointers(), BLOCKSIZE);
    }

    // if output, send to hardware & virtual out (gui), since these will always be last after sorting
}

void DeviceNode::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    sampleRate = device->getCurrentSampleRate();
    blockSize = device->getCurrentBufferSizeSamples();

    if(isInput())
        numChannels = device->getActiveInputChannels().countNumberOfSetBits();

    if(isOutput())
        numChannels = device->getActiveOutputChannels().countNumberOfSetBits();

    if (numChannels != 2) {
        inputBuffer.setSize(numChannels, BLOCKSIZE, false, true, true);
        outputBuffer.setSize(numChannels, BLOCKSIZE, false, true, true);
        GUIbuffer.setSize(numChannels, BLOCKSIZE, false, true, true);
        hardwareBuffer.setSize(numChannels, FIFOSIZE, false, true, true);
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
            
            if (samplesWritten > numSamples) {
            //    Logger::log("OVERFLOW IN INPUT: SamplesWritten(" + to_string(samplesWritten) + ") > numSamples(" + to_string(numSamples) + ")");
            }
            if (samplesWritten < numSamples) {
             //   Logger::log("UNDERRUN IN INPUT (not enough samples): SamplesWritten(" + to_string(samplesWritten) + ") > numSamples(" + to_string(numSamples) + ")");
            }
        } else {
         //   Logger::log("INPUT FIFO TOO FULL: fifoFill(" + to_string(fifoFill) + ") > fifoCapacity(" + to_string(fifoCapacity) + ")");
        }
    }


    // for each I/O device: 
    // calculate ratio of fifo reading. use PID or similar to modulate ratio to aim at target fill average
    // 
    // think: normally, this ratio should be 1, but can oscillate around 1 (shows that its working)

    if (m_blockType == BlockType::OutputDevice) {

        const float* data = this->hardwareBuffer.getReadPointer(0);
        int samplesRead = readFromFifoTo(outputChannelData, numSamples);
        if (samplesRead < numSamples)
        {
        //    Logger::log("UNDERRUN IN OUTPUT: SamplesRead(" + to_string(samplesRead) + ") < numSamples(" + to_string(numSamples) + ")");

            // Underrun → zero remaining output
            for (int ch = 0; ch < numOutputChannels; ++ch)      // only triggered once i believe, at the start?
            {
                juce::FloatVectorOperations::clear(
                    outputChannelData[ch] + samplesRead,
                    numSamples - samplesRead);
            }
        }

       if(trigger != nullptr && hardwareFIFO.getNumReady() < BLOCKSIZE)
        trigger->signal();                                                  // trigger engine that the output fifo is getting empty! needs a refill 
       // fifo should be filled within 1 output sample period
       // get timestampPair?
     //  IAudioClock::GetPosition()


    }
}
void DeviceNode::setAsMainOutput(bool set) {   
    trigger = set ? trigAddress : nullptr;
}
void DeviceNode::setTriggerAddress(juce::WaitableEvent* trigger) {
    this->trigAddress = trigger; 
}
