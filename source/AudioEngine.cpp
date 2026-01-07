#include "AudioEngine.h"
#include "Program.h"

AudioEngine::AudioEngine(Program* prog) : 
    prog(prog), nullDevice(BlockType::NullDevice,"",1000){

    uniqueID = 1000;    // null device starts at 1000

    audio_engine_on = true;
    audiothread = std::thread(&AudioEngine::run, this);


}
void AudioEngine::run() {

    unsigned long tickCounter = 0;

    Counter threadTimer;
    threadTimer.startTimer();


    while (audio_engine_on) {
        

        tickCounter++;

        // FIFO state is the only reliable clock in a decoupled audio engine.
        // condition variable, lock audio thread until output/input fifo are "ready"

       // Logger::log(threadTimer.getDurationLoopString() + " for all processing");
        
        requestNewAudioBlock.wait();            // wait for output fifo to empty below BLOCKSIZE

       // Logger::log(threadTimer.getDurationLoopString() + " waited");   // wait ~10 ms, processing  = ~20-30 us

        // 2. READ all input fifo's  once
        for (auto& device : hardwareBlocks)
        {
            if (device.second != nullptr && device.second->isInput())
            {
                levels[0] = device.second->hardwareFIFO.getNumReady();
                levels[1] = device.second->hardwareFIFO.getFreeSpace();

                device.second->readFromFifoTo(inputBuffers[device.first].getArrayOfWritePointers(), BLOCKSIZE);

              //  device.second->readFromFifoTo(mainBuffer.getArrayOfWritePointers(), BLOCKSIZE);
                
            }
        }

        // read file IO buffers as inputs

        for (auto& buf : outputBuffers) {
            buf.second.clear();             // fist, flush output buffers
        }

        for (auto& link : links) {

            NodeID nodeLeftOfLink = m_PinNodePairs.at(link.second.ID_left.Get()); 
            NodeID nodeRightOfLink = m_PinNodePairs.at(link.second.ID_right.Get());

            // check if input or output = DSPNode, input or output device node

            juce::AudioBuffer<float>* currentLinkInput = nullptr;
            juce::AudioBuffer<float>* currentLinkOutput = nullptr;


            // TODO: make sure audio always flows from left to right, so input devices first, then dsp, then output devices


            // HW input
            if (hardwareBlocks.contains(nodeLeftOfLink)) {
                currentLinkInput = &inputBuffers[nodeLeftOfLink];
                Logger::log("Engine: hardware input");
            }

            // DSP input
            if (DSPBlocks.contains(nodeRightOfLink)) {
                currentLinkOutput = &DSPBlocks[nodeRightOfLink]->inputBuffer;
                Logger::log("Engine: DSP input link");
            }

            // DSP output
            if (DSPBlocks.contains(nodeLeftOfLink)){

                DSPBlocks[nodeLeftOfLink]->process();
                currentLinkInput = &DSPBlocks[nodeLeftOfLink]->outputBuffer;    // output, since you can only read from a DSP output pin
                Logger::log("Engine: DSP output link (process)");
            }
               
            // HW output
            if (hardwareBlocks.contains(nodeRightOfLink)) {
                currentLinkOutput = &outputBuffers[nodeRightOfLink];
                Logger::log("Engine: hardware output");
            } 
            
            // copy inputs to outputs.
            if (currentLinkInput != nullptr && currentLinkOutput != nullptr)
                copyBuffer(*currentLinkOutput, *currentLinkInput);
            else
                Logger::log("Buffer was nullptr!");

        }

        // we now have all inputs

        // check links
        // resolve graph   
        // read the correct input buffers;
        // process audio
        // prepare / mix output buffers

      //  mainBuffer.applyGain(gain);


        // And send output buffers to HW FIFO's

        for (auto& device : hardwareBlocks)
        {
            if (device.second != nullptr && device.second->isOutput())
            {             
                levels[2] = device.second->hardwareFIFO.getNumReady();
                levels[3] = device.second->hardwareFIFO.getFreeSpace();

                device.second->writeToFifoFrom(outputBuffers[device.first].getArrayOfReadPointers(), BLOCKSIZE);
               // device.second->writeToFifoFrom(mainBuffer.getArrayOfReadPointers(), BLOCKSIZE);
            }
        }
    }
}

void AudioEngine::copyBuffer(juce::AudioBuffer<float>& dest,const juce::AudioBuffer<float>& src)
{
    jassert(dest.getNumChannels() == src.getNumChannels());
    jassert(dest.getNumSamples() == src.getNumSamples());

    dest.makeCopyOf(src, true);
}

void AudioEngine::mixInto(juce::AudioBuffer<float>& dest,const juce::AudioBuffer<float>& src)
{
    jassert(dest.getNumChannels() == src.getNumChannels());
    jassert(dest.getNumSamples() == src.getNumSamples());

    for (int ch = 0; ch < dest.getNumChannels(); ++ch)
    {
        dest.addFrom(ch,          // dest channel
            0,           // dest start sample
            src,
            ch,          // src channel
            0,           // src start sample
            src.getNumSamples());

        // ADD GAIN !!

    }
}


BaseID AudioEngine::getNewID(Identifier type) {


    uniqueID++;
    

   //  if (type == Identifier::link)
   //  if (type == Identifier::node)
   //  if (type == Identifier::pin)

    return uniqueID++;
}

// TODO: CHECK FOR FEEDBACK LOOPS: any output to connect: disable all inputs "before" this signal chain, so it can't even be created (this maintains the non-loopness)
void AudioEngine::createLink(node::PinId leftPin, node::PinId rightPin) {

    LinkID nextID = getNewID(Identifier::link);

    links.emplace(nextID, BlockLink(nextID, leftPin, rightPin));

}

void AudioEngine::deleteLink(LinkID linkID) {

    links.erase(linkID);
}

//returns the next Block ID, also creates new pin ID
NodeID AudioEngine::addNewDeviceBlock(BlockType blockType, juce::String initDeviceName) {

    // create 2 new IDs. 1 for device, 1 for pin
    NodeID blockID = getNewID(Identifier::node);
    PinID pinID = getNewID(Identifier::pin);

    hardwareBlocks[blockID] = std::make_unique<DeviceNode>(blockType, initDeviceName, blockID);        // add new block


    if(blockType == BlockType::InputDevice)
        hardwareBlocks[blockID]->addPin(pinID, pinType::output);  // add 1 pin for each device. input or output is decided by node type   
    
    if (blockType == BlockType::OutputDevice)
        hardwareBlocks[blockID]->addPin(pinID, pinType::input);
    

    m_PinNodePairs.emplace(pinID, blockID);  // make the parent node easier to find using a LUT

    levels.push_back(0.0f); // add 2 channels for fifo monitoring
    levels.push_back(0.0f);


    // add a buffer for any corresponding HW block
    if (blockType == BlockType::InputDevice) inputBuffers.emplace(blockID,juce::AudioSampleBuffer(2, 512));
    if (blockType == BlockType::OutputDevice) outputBuffers.emplace(blockID, juce::AudioSampleBuffer(2, 512));


    if (hardwareBlocks[blockID]->getBlockType() == BlockType::OutputDevice)
        hardwareBlocks[blockID]->setAsMainOutput(&requestNewAudioBlock);
    
    return blockID;

}

NodeID AudioEngine::addNewDSPNode(const juce::String& name) {

    NodeID blockID = getNewID(Identifier::node);            // get new IDs for node and pins
    PinID inputPinID = getNewID(Identifier::pin);
    PinID outputPinID = getNewID(Identifier::pin);

    DSPBlocks[blockID] = std::make_unique<DSPNode>(BlockType::DSP, name, blockID);      // create new DSPNode
    DSPBlocks[blockID]->addPin(inputPinID, pinType::input);                             // assign pins
    DSPBlocks[blockID]->addPin(outputPinID, pinType::output);

    m_PinNodePairs.emplace(inputPinID, blockID);                                        // save pin assignment in LUT
    m_PinNodePairs.emplace(outputPinID, blockID);

    return blockID;
}

void AudioEngine::deleteDeviceBlock(NodeID deviceID) {

    vector<LinkID> linksToDelete;

    // Break all connected links
    if (node::HasAnyLinks((node::NodeId)deviceID)) {
        for (auto& toDelete : links) {

            if (hardwareBlocks[deviceID]->hasPin(toDelete.second.ID_left.Get()) != pinType::null ||        // ONLY WORKS FOR DEVICES WITH 1 PIN RIGHT NOW !!
                hardwareBlocks[deviceID]->hasPin(toDelete.second.ID_right.Get()) != pinType::null) {
                
                linksToDelete.push_back(toDelete.first);
            }
        }

        for(auto& linkID : linksToDelete)   // so it doesn't crash mid-for loop
            links.erase(linkID);
    }
  
    hardwareBlocks.erase(deviceID);

}

void AudioEngine::selectAudioDevice(NodeID deviceID, const juce::String& nameToFind, bool isOutput) {

    if (hardwareBlocks.contains(deviceID))
        hardwareBlocks[deviceID]->selectDevice(nameToFind, isOutput);
    else
        Logger::log("Invalid audio device node selected", level_ERROR, source_AUDIO);

}
