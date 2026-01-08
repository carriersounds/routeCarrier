#include "AudioDeviceNode.h"
#include "Program.h"

DeviceNode::DeviceNode(BlockType blocktype, juce::String initDeviceName, NodeID nodeID)
        : hardwareFIFO(FIFOSIZE), 
        m_isMainOutput(false), 
        data(2, FIFOSIZE),
        AudioNode(blocktype,initDeviceName,nodeID)
    {

    trigger = nullptr;


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
            juce::FloatVectorOperations::copy(data.getWritePointer(ch, start1), input[ch], size1);
    }

    if (size2 > 0) {
        for (int ch = 0; ch < numChannels; ++ch)
            juce::FloatVectorOperations::copy(data.getWritePointer(ch, start2), input[ch] + size1, size2);
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
            juce::FloatVectorOperations::copy(output[ch], data.getReadPointer(ch, start1),
                size1);
    }

    if (size2 > 0) {
        for (int ch = 0; ch < numChannels; ++ch)
            juce::FloatVectorOperations::copy(output[ch], data.getReadPointer(ch, start2), size2);

    }

    hardwareFIFO.finishedRead(size1 + size2);



    return size1 + size2;
}

void DeviceNode::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    sampleRate = device->getCurrentSampleRate();
    blockSize = device->getCurrentBufferSizeSamples();

    Logger::log("Device About To Start: " + device->getName().toStdString());

    Logger::log("SampleRate = " + to_string(device->getCurrentSampleRate()));
    Logger::log("BlockSize = " + to_string(device->getCurrentBufferSizeSamples()));

    // Prepare
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = blockSize;
    spec.numChannels = 2; // If mono, else 2 for stereo

    gainProcessor.prepare(spec);
    gainProcessor.setGainLinear(gainValue.load());
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
            const float* data = this->data.getWritePointer(0);

            // SRC, not working currently
            if (0 && sampleRate < 47900) {

                juce::AudioBuffer<float> SRCbuf(1, BLOCKSIZE);
                juce::LagrangeInterpolator interpolator;
                double ratio = deviceManager.getAudioDeviceSetup().sampleRate / 48000;
                interpolator.process(ratio, inputChannelData[1], SRCbuf.getWritePointer(0), BLOCKSIZE);
                samplesWritten = writeToFifoFrom(SRCbuf.getArrayOfReadPointers(), numSamples);
                
                if (samplesWritten > numSamples) {
         //           Logger::log("OVERFLOW IN INPUT: SamplesWritten(" + to_string(samplesWritten) + ") > numSamples(" + to_string(numSamples) + ")");
                }
                return;
            }

            samplesWritten = writeToFifoFrom(inputChannelData, numSamples);
            
            if (samplesWritten > numSamples) {
     //           Logger::log("OVERFLOW IN INPUT: SamplesWritten(" + to_string(samplesWritten) + ") > numSamples(" + to_string(numSamples) + ")");
            }
        } else {
//            Logger::log("INPUT FIFO TOO FULL: fifoFill(" + to_string(fifoFill) + ") > fifoCapacity(" + to_string(fifoCapacity) + ")");
        }
    }

    if (m_blockType == BlockType::OutputDevice) {

        const float* data = this->data.getReadPointer(0);
        int samplesRead = readFromFifoTo(outputChannelData, numSamples);
        if (samplesRead < numSamples)
        {
   //         Logger::log("UNDERRUN IN OUTPUT: SamplesRead(" + to_string(samplesRead) + ") < numSamples(" + to_string(numSamples) + ")");

            // Underrun → zero remaining output
            for (int ch = 0; ch < numOutputChannels; ++ch)      // only triggered once i believe, at the start?
            {
                juce::FloatVectorOperations::clear(
                    outputChannelData[ch] + samplesRead,
                    numSamples - samplesRead);
            }
        }

       if(isMainOutput() && hardwareFIFO.getNumReady() < BLOCKSIZE)
        trigger->signal();                                                  // trigger engine that the output fifo is getting empty! needs a refill 
       // fifo should be filled within 1 output sample period

    }


    juce::AudioBuffer<float> buffer;

    if (isInput())
        buffer = juce::AudioBuffer<float>(const_cast<float**>(inputChannelData), 1, numSamples);
    else if(isOutput())
        buffer = juce::AudioBuffer<float>(const_cast<float**>(outputChannelData),1, numSamples);

    float* src = buffer.getWritePointer(0);
    updateLevel(src, numSamples);
   

}

void DeviceNode::setAsMainOutput(juce::WaitableEvent* trigger) {

    m_isMainOutput = true;
    this->trigger = trigger;
}

float DeviceNode::getLevel() const noexcept {

    // Logger::log("samplin thread, level = " + to_string(currentLevel.load()));

    return currentLevel.load();
}

void DeviceNode::initDSP(double sr, int bs)
{
    //   dspChain.sampleRate = sr;
   //    dspChain.blockSize = bs;

       // default: add lowpass filter
   //    dspChain.addEffect<juce::dsp::IIR::Filter<float>>();

       // Configure filter
    //   auto* filter = dynamic_cast<juce::dsp::IIR::Filter<float>*>(dspChain.chain[0].get());
    //   *filter = juce::dsp::IIR::Coefficients<float>::makeLowPass(sr, 2000.0f);
}
