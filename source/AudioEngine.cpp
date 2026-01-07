#include "AudioEngine.h"
#include "Program.h"

AudioEngine::AudioEngine(Program* prog) : prog(prog), nullDevice(BlockType::NullDevice,"",1000){

    uniqueID = 1000;    // null device starts at 0

    mainBuffer.setSize(2, BLOCKSIZE);  // 2 channel, 256 samples   


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

        Logger::log(threadTimer.getDurationLoopString() + " for all processing");
        
        requestNewAudioBlock.wait();            // wait for output fifo to empty below BLOCKSIZE

        Logger::log(threadTimer.getDurationLoopString() + " waited");   // wait ~10 ms, processing  = ~20-30 us

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

            size_t correspondingInputNode = m_PinNodePairs.at(link.second.ID_left.Get());

            
            size_t correspondingOutNode = m_PinNodePairs.at(link.second.ID_right.Get());




            // ONLY WORKS FOR HW-HW LINKS ! NO DSP BUFFERS YET
            mixInto(outputBuffers[correspondingOutNode], inputBuffers[correspondingInputNode]);  // copy inputs to outputs. 

        }

        // we now have all inputs

        // check links
        // resolve graph   
        // read the correct input buffers;
        // 
        // process audio
        // 
        // prepare / mix output buffers


        mainBuffer.applyGain(gain);


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


size_t AudioEngine::getNewID(Identifier type) {

    size_t value = uniqueID++;

   //  if (type == Identifier::link)
         
   //  if (type == Identifier::node)

   //  if (type == Identifier::pin)

    return value;
}

void AudioEngine::createLink(node::PinId leftPin, node::PinId rightPin) {

    size_t nextID = getNewID(Identifier::link);

    links.emplace(nextID, BlockLink(nextID, leftPin, rightPin));

}

void AudioEngine::deleteLink(size_t linkID) {

    links.erase(linkID);
}


//returns the next Block ID, also creates new pin ID
size_t AudioEngine::addNewDeviceBlock(BlockType blockType, juce::String initDeviceName) {

    // create 2 new IDs. 1 for device, 1 for pin
    size_t blockID = getNewID(Identifier::node);
    size_t pinID = getNewID(Identifier::pin);

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

void AudioEngine::deleteDeviceBlock(size_t deviceID) {

    vector<size_t> linksToDelete;

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

void AudioEngine::initDSP(double sr, int bs) {
   //inputDevice.initDSP(sr, bs);
}

void AudioEngine::selectDevice(const juce::String& nameToFind, bool isOutput) {
  // inputDevice.selectDevice(nameToFind, isOutput);
}
