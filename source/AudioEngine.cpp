#include "AudioEngine.h"
#include "Program.h"

AudioEngine::AudioEngine(Program* prog) : 
    prog(prog), nullDevice(BlockType::NullDevice,"",1000){

    uniqueID = 1000;    // null device starts at ID = 1000
    audio_engine_on = true;
    audiothread = std::thread(&AudioEngine::run, this);
}

AudioEngine::~AudioEngine() {
    
    audio_engine_on = false;
    requestNewAudioBlock.signal();  // break free from wait loop if app is closed without audio running
    audiothread.join();
    
}

void AudioEngine::run() {

    unsigned long tickCounter = 0;

    Counter threadTimer;
    threadTimer.startTimer();


    while (audio_engine_on) {

        tickCounter++;
  
       // Logger::log(threadTimer.getDurationLoopString() + " for all processing");
           
        requestNewAudioBlock.wait();                                        // wait for main output fifo to empty below BLOCKSIZE

        // 1. READ all input fifo's  once
        for (auto& device : deviceNodes) {     
            if (device.second != nullptr && device.second->isInput()) {
                device.second->readFromFifoTo(inputBuffers[device.first].getArrayOfWritePointers(), BLOCKSIZE);            
            }
        }

        // 2. TODO: read file IO buffers as inputs

        // 3. flush output buffers
        for (auto& buf : outputBuffers) buf.second.clear();
                         
        // 4. clear DSP buffers
        for (auto& DSPblock : DSPNodes) {          
            DSPblock.second->outputBuffer.clear();  
            DSPblock.second->inputBuffer.clear();
        }

        // 5. check links and transfer audio
        for (auto& link : links) {

            NodeID nodeLeftOfLink = m_PinNodePairs.at(link.second.ID_left.Get());
            NodeID nodeRightOfLink = m_PinNodePairs.at(link.second.ID_right.Get());

            juce::AudioBuffer<float>* currentLinkInput = nullptr;
            juce::AudioBuffer<float>* currentLinkOutput = nullptr;

            // HW input
            if (deviceNodes.contains(nodeLeftOfLink)) {
                currentLinkInput = &inputBuffers[nodeLeftOfLink];
            }

            // DSP input
            if (DSPNodes.contains(nodeRightOfLink)) {
                currentLinkOutput = &DSPNodes[nodeRightOfLink]->inputBuffer;
            }


            // DSP output
            if (DSPNodes.contains(nodeLeftOfLink) && DSPNodes[nodeLeftOfLink]->sampleRate > 5000.0){
                DSPNodes[nodeLeftOfLink]->process();
                currentLinkInput = &DSPNodes[nodeLeftOfLink]->outputBuffer;    // output, since you can only read from a DSP output pin
            }
               
            // HW output
            if (deviceNodes.contains(nodeRightOfLink)) {
                currentLinkOutput = &outputBuffers[nodeRightOfLink];
            } 

            // copy HW inputs to their
            if (currentLinkInput != nullptr && currentLinkOutput != nullptr)
                mixInto(currentLinkOutput, currentLinkInput);
        }

        // 6. send output buffers to HW FIFO's
        for (auto& device : deviceNodes) {    
            if (device.second != nullptr && device.second->isOutput()) {
                

                if (device.second->isMainOutput()) {
                    fifoLevels[0] = device.second->hardwareFIFO.getNumReady();
                    fifoLevels[1] = device.second->hardwareFIFO.getFreeSpace();
                }
                
                device.second->writeToFifoFrom(outputBuffers[device.first].getArrayOfReadPointers(), BLOCKSIZE);
            }
        }
    }
}

void AudioEngine::copyBuffer(juce::AudioBuffer<float>* dest,const juce::AudioBuffer<float>* src)
{
    jassert(dest->getNumChannels() == src->getNumChannels());
    jassert(dest->getNumSamples() == src->getNumSamples());

    dest->makeCopyOf(*src, false);
}

void AudioEngine::mixInto(juce::AudioBuffer<float>* dest,const juce::AudioBuffer<float>* src)
{
    jassert(dest->getNumChannels() == src->getNumChannels());
    jassert(dest->getNumSamples() == src->getNumSamples());
    jassert(dest != nullptr);


    int sampilos = src->getNumSamples();

    for (int ch = 0; ch < dest->getNumChannels(); ++ch)
    {
        dest->addFrom(ch,          // dest channel
            0,           // dest start sample
            *src,
            ch,          // src channel
            0,           // src start sample
            src->getNumSamples());

        // ADD GAIN !!

    }
}

BaseID AudioEngine::getNewID(Identifier type) {

    uniqueID++;
    
    string idtype;

    if (type == Identifier::link) idtype = "link";
    if (type == Identifier::node) idtype = "node";
    if (type == Identifier::pin)  idtype = "pin";

   //  Logger::log("new " + idtype +" ID: " + to_string(uniqueID), level_DEBUG);

    return uniqueID;
}

void AudioEngine::createLink(node::PinId leftPin, node::PinId rightPin) {

    LinkID nextID = getNewID(Identifier::link);

    links.emplace(nextID, BlockLink(nextID, leftPin, rightPin));

}

void AudioEngine::deleteLink(LinkID linkID) {

    links.erase(linkID);
}

NodeID AudioEngine::addNewDeviceNode(BlockType blockType, juce::String initDeviceName) {

    // create 2 new IDs. 1 for device, 1 for pin
    NodeID blockID = getNewID(Identifier::node);
    PinID pinID = getNewID(Identifier::pin);
    deviceNodes[blockID] = std::make_unique<DeviceNode>(blockType, initDeviceName, blockID);        // add new block

    if(blockType == BlockType::InputDevice)
        deviceNodes[blockID]->addPin(pinID, pinType::output);        // add 1 pin for each device. input or output is decided by node type   
    
    if (blockType == BlockType::OutputDevice)
        deviceNodes[blockID]->addPin(pinID, pinType::input);
    

    m_PinNodePairs.emplace(pinID, blockID);                             // make the parent node easier to find using a LUT
                                                                        // add 2 channels for fifo monitoring (debug)
   
    fifoLevels.emplace(pinID, blockID);
    
    if (blockType == BlockType::InputDevice) inputBuffers.emplace(blockID,juce::AudioSampleBuffer(2, 512));        // add a buffer for any corresponding HW block
    if (blockType == BlockType::OutputDevice) outputBuffers.emplace(blockID, juce::AudioSampleBuffer(2, 512));

    if (deviceNodes[blockID]->getBlockType() == BlockType::OutputDevice)
        deviceNodes[blockID]->setAsMainOutput(&requestNewAudioBlock);
    
    return blockID;
}

NodeID AudioEngine::addNewDSPNode(EffectType typeOfEffect) {

    NodeID blockID = getNewID(Identifier::node);                                        // get new IDs for node and pins
    PinID inputPinID = getNewID(Identifier::pin);
    PinID outputPinID = getNewID(Identifier::pin);

    switch (typeOfEffect)
    {
    case EffectType::Filter:
        DSPNodes.emplace(blockID, std::make_unique<LowpassNode>(BlockType::DSP, "Filter", blockID));
        break;
    case EffectType::Gain:
        DSPNodes.emplace(blockID, std::make_unique<GainNode>(BlockType::DSP, "Gain", blockID));
        break;
    default:
        break;
    }   
    
    DSPNodes.at(blockID)->prepareToPlay(48000, BLOCKSIZE);                             // initialize samplerates   
    DSPNodes.at(blockID)->addPin(inputPinID, pinType::input);                             // assign pins
    DSPNodes.at(blockID)->addPin(outputPinID, pinType::output);

    m_PinNodePairs.emplace(inputPinID, blockID);                                        // save pin assignment in LUT
    m_PinNodePairs.emplace(outputPinID, blockID);

    return blockID;
}

void AudioEngine::deleteDSPNode(NodeID blockID) {

    if (!DSPNodes.contains(blockID)) {
        Logger::log("Invalid Device queued for deletion", level_ERROR);
        return;
    }

    vector<LinkID> linksToDelete;

    // Break all connected links
    if (node::HasAnyLinks((node::NodeId)blockID)) {
        for (auto& toDelete : links) {

            if (DSPNodes[blockID]->hasPin(toDelete.second.ID_left.Get()) != pinType::null ||        // ONLY WORKS FOR DEVICES WITH 1 PIN RIGHT NOW !!
                DSPNodes[blockID]->hasPin(toDelete.second.ID_right.Get()) != pinType::null) {

                linksToDelete.push_back(toDelete.first);
            }
        }

        for (auto& linkID : linksToDelete)   // so it doesn't crash mid-for loop
            links.erase(linkID);
    }

    DSPNodes.erase(blockID);


}

void AudioEngine::deleteDeviceNode(NodeID deviceID) {


    if (!deviceNodes.contains(deviceID)) {
        Logger::log("Invalid Device queued for deletion", level_ERROR);
        return;
    }

    vector<LinkID> linksToDelete;

    // Break all connected links
    if (node::HasAnyLinks((node::NodeId)deviceID)) {
        for (auto& toDelete : links) {

            if (deviceNodes[deviceID]->hasPin(toDelete.second.ID_left.Get()) != pinType::null ||        // ONLY WORKS FOR DEVICES WITH 1 PIN RIGHT NOW !!
                deviceNodes[deviceID]->hasPin(toDelete.second.ID_right.Get()) != pinType::null) {
                
                linksToDelete.push_back(toDelete.first);
            }
        }

        for(auto& linkID : linksToDelete)   // so it doesn't crash mid-for loop
            links.erase(linkID);
    }
  

    // delete corresponding audio buffer
    if (deviceNodes[deviceID]->isInput()) {
        inputBuffers[deviceID].clear();
    }
    else if (deviceNodes[deviceID]->isOutput()) {
        outputBuffers[deviceID].clear();
    }

    deviceNodes.erase(deviceID);

    

}

void AudioEngine::changeAudioDevice(NodeID deviceID, const juce::String& nameToFind, bool isOutput) {

    if (deviceNodes.contains(deviceID))
        deviceNodes[deviceID]->selectDevice(nameToFind, isOutput);
    else
        Logger::log("Invalid audio device node selected", level_ERROR, source_AUDIO);

}
