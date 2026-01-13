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
    juce::WASAPIDeviceMode::exclusive;

   // juce::AudioClientProperties prop;


    while (audio_engine_on) {
        tickCounter++;
       // Logger::log(threadTimer.getDurationLoopString() + " for all processing");
           
        requestNewAudioBlock.wait();  // wait for main output fifo to empty below BLOCKSIZE

        // for each I/O device: 
        // calculate ratio of fifo reading. use PID or similar to modulate ratio to aim at target fill average
        // 
        // think: normally, this ratio should be 1, but can oscillate around 1 (shows that its working)


        // 1. READ all input fifo's  once
        for (auto& device : deviceNodes) {     
            if (device.second != nullptr && device.second->isInput()) {


                // instead of BLOCKSIZE, calculate numSamples based on fifo state & average

                // SRC
#if (0) {

                    juce::AudioBuffer<float> SRCbuf(2, FIFOSIZE);
                    juce::LagrangeInterpolator interpolator;
                    double ratio = device.second->deviceManager.getAudioDeviceSetup().sampleRate / 48000;
                    
                    interpolator.process(ratio, inputChannelData[0], SRCbuf.getWritePointer(0), (double)BLOCKSIZE / ratio);
                    interpolator.process(ratio, inputChannelData[1], SRCbuf.getWritePointer(1), (double)BLOCKSIZE / ratio);

                    samplesWritten = writeToFifoFrom(SRCbuf.getArrayOfReadPointers(), (double)BLOCKSIZE / ratio);

                    //   samplesWritten = writeToFifoFrom(inputChannelData, numSamples);

                    return;
#endif//               }



                fifoLevels[device.first] = device.second->hardwareFIFO.getNumReady();
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
        for (auto& linkID : sortedLinks) {

            if (!links.contains(linkID)) continue;

            NodeID startNode = m_PinNodePairs.at(links.at(linkID).ID_left.Get());
            NodeID endNode = m_PinNodePairs.at(links.at(linkID).ID_right.Get());

            PinID startPin = links.at(linkID).ID_left.Get();
            PinID endPin = links.at(linkID).ID_right.Get();

            juce::AudioBuffer<float>* currentLinkInputBuffer = nullptr;
            juce::AudioBuffer<float>* currentLinkOutputBuffer = nullptr;

            // HW Inputs
            if(deviceNodes.contains(startNode) && deviceNodes.at(startNode)->isInput())  
                currentLinkInputBuffer = &inputBuffers[startNode];
        
            // DSP I/O 
            if (DSPNodes.contains(endNode) && DSPNodes.at(endNode)->hasPin(endPin) == pinType::input) 
                currentLinkOutputBuffer = &DSPNodes[endNode]->inputBuffer;
            
            if (DSPNodes.contains(startNode) && DSPNodes.at(startNode)->hasPin(startPin) == pinType::output) {
                DSPNodes[startNode]->process();
                currentLinkInputBuffer = &DSPNodes[startNode]->outputBuffer;
            }

            // HW Outputs
            if (deviceNodes.contains(endNode) && deviceNodes.at(endNode)->isOutput()) 
                currentLinkOutputBuffer = &outputBuffers[endNode];

            // copy buffer
            if (currentLinkInputBuffer != nullptr && currentLinkOutputBuffer != nullptr)
                mixInto(currentLinkOutputBuffer, currentLinkInputBuffer);
        }

        // 6. send output buffers to HW FIFO's
        for (auto& device : deviceNodes) {    
            if (device.second != nullptr && device.second->isOutput()) {
                      
#if 0
                // Main output = sync source. everything else follows
                if (!device.second->isMainOutput()) {


                    // SRC
                    juce::AudioBuffer<float> SRCbuf(2, FIFOSIZE);
                    juce::LagrangeInterpolator interpolator;
                    double ratio = device.second->deviceManager.getAudioDeviceSetup().sampleRate / 48000;

                    interpolator.process(ratio, inputChannelData[0], SRCbuf.getWritePointer(0), (double)BLOCKSIZE / ratio);
                    interpolator.process(ratio, inputChannelData[1], SRCbuf.getWritePointer(1), (double)BLOCKSIZE / ratio);

                    samplesWritten = writeToFifoFrom(SRCbuf.getArrayOfReadPointers(), (double)BLOCKSIZE / ratio);

                    //   samplesWritten = writeToFifoFrom(inputChannelData, numSamples);
                    
                    
                   continue;

                }

#endif
                device.second->writeToFifoFrom(outputBuffers[device.first].getArrayOfReadPointers(), BLOCKSIZE);


                fifoLevels[device.first] = device.second->hardwareFIFO.getNumReady();

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

    sortedLinks.clear();

   // convert links into nodes for sort algorithm
   // expects vector<vector<int>>& adj = nodes it points to
   //  linksAfterNextNode


    calculateLinkAdjacents();

    // assume we KNOW the outNodes
    topologicalSortLinks();


}

void AudioEngine::calculateLinkAdjacents() {

    linksAfterNextNode.clear();

    // make sure I/O and O/I connections are corrected if connected in reverse in the GUI

    for (auto& link : links) {
        NodeID firstNode = m_PinNodePairs.at(link.second.ID_left.Get());
        NodeID secondNode = m_PinNodePairs.at(link.second.ID_right.Get());
        PinID firstPin = link.second.ID_left.Get();
        PinID secondPin = link.second.ID_right.Get();
    
        // if either order is wrong
        if ((deviceNodes.contains(secondNode) && deviceNodes.at(secondNode)->isInput()) || (
                DSPNodes.contains(firstNode)  &&    DSPNodes.at(firstNode)->hasPin(firstPin) == pinType::input) || (
                DSPNodes.contains(secondNode) &&    DSPNodes.at(secondNode)->hasPin(secondPin) == pinType::output) || (
             deviceNodes.contains(firstNode)  && deviceNodes.at(firstNode)->isOutput())) 
        {
            node::PinId temp = link.second.ID_left;
            link.second.ID_left = link.second.ID_right; // invert left and right pin
            link.second.ID_right = temp;
        }
    }

    // get adjacent links

    for (auto& currentLink : links) {    
        NodeID rightNodeOfCurrentLink = m_PinNodePairs.at(currentLink.second.ID_right.Get());

        for (auto& nextLink : links) {
            NodeID leftNodeOfNextLink = m_PinNodePairs.at(nextLink.second.ID_left.Get());
            if (leftNodeOfNextLink == rightNodeOfCurrentLink) {
                linksAfterNextNode[currentLink.first].push_back(nextLink.first);
            }
        }
        if (!linksAfterNextNode.contains(currentLink.first))
            linksAfterNextNode.emplace(currentLink.first,vector<NodeID>());
    }
}

// input = map where the input
void AudioEngine::topologicalSortLinks() {

    std::map<LinkID, vector<NodeID>>& adj = linksAfterNextNode;

    int n = adj.size();
    std::map<LinkID, int> indegree;
    std::queue<LinkID> q;
    vector<LinkID> list;


    // Compute indegrees
    for (auto& link : adj) {
        for (auto& next : link.second)      // for all outgoing links of ID
            indegree[next]++;
    }

    // Add all nodes with indegree 0 
    // into the queue
    for (auto& degLink : links)
        if (indegree[degLink.first] == 0)
            q.push(degLink.first);

    // Kahn’s Algorithm (BFS)
    while (!q.empty()) {
        int top = q.front();
        q.pop();
        list.push_back(top);

        for (auto& nextID : adj[top]) {
            indegree[nextID]--;
            if (indegree[nextID] == 0)
                q.push(nextID);
        }
    }
    // nodes involved in cycles will never reach degree zero

    sortedLinks = list;
}

void AudioEngine::deleteLink(LinkID linkID) {

    links.erase(linkID);
}

NodeID AudioEngine::addNewDeviceNode(BlockType blockType, juce::String initDeviceName) {

    // create 2 new IDs. 1 for device, 1 for pin
    NodeID blockID = getNewID(Identifier::node);
    PinID pinID = getNewID(Identifier::pin);
    deviceNodes[blockID] = make_unique<DeviceNode>(blockType, initDeviceName, blockID);        // add new block

    if(blockType == BlockType::InputDevice)
        deviceNodes[blockID]->addPin(pinID, pinType::output);        // add 1 pin for each device. input or output is decided by node type   
    
    if (blockType == BlockType::OutputDevice)
        deviceNodes[blockID]->addPin(pinID, pinType::input);
    

    m_PinNodePairs.emplace(pinID, blockID);                             // make the parent node easier to find using a LUT
                                                                       
    fifoLevels.emplace(blockID,0); // add channel for fifo monitoring (debug)
    
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
        DSPNodes.emplace(blockID, make_unique<LowpassNode>(BlockType::DSP, "Filter", blockID));
        break;
    case EffectType::Gain:
        DSPNodes.emplace(blockID, make_unique<GainNode>(BlockType::DSP, "Gain", blockID));
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
    fifoLevels.erase(deviceID);
    

}

void AudioEngine::changeAudioDevice(NodeID deviceID, const juce::String& nameToFind, bool isOutput) {

    if (deviceNodes.contains(deviceID))
        deviceNodes[deviceID]->selectDevice(nameToFind, isOutput);
    else
        Logger::log("Invalid audio device node selected", level_ERROR, source_AUDIO);

}
