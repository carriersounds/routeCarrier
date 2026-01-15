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
           
        requestNewAudioBlock.wait();                // wait for main output fifo to empty below BLOCKSIZE
        nodeLock.lock();

        for (auto& node : sortedNodes) {
            nodes.at(node)->inputBuffer.clear();    // so they can be filled
        }

        for (auto& node : sortedNodes) {
            nodes.at(node)->prepareOutput();
            nodes.at(node)->sendAudioToNextNodes();
        }

        nodeLock.unlock();
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


    calculateSends(nextID);
    topologicalSortNodes();

}

void AudioEngine::calculateSends(LinkID newlink) {

    // make sure I/O and O/I connections are corrected if connected in reverse in the GUI
    NodeID firstNode = m_PinNodePairs.at(links.at(newlink).ID_left.Get());
    NodeID secondNode = m_PinNodePairs.at(links.at(newlink).ID_right.Get());
    PinID firstPin = links.at(newlink).ID_left.Get();
    PinID secondPin = links.at(newlink).ID_right.Get();
    
    if (nodes.at(firstNode)->hasPin(firstPin) == pinType::input || nodes.at(secondNode)->hasPin(secondPin) == pinType::output)
    {
        node::PinId temp = links.at(newlink).ID_left;
        links.at(newlink).ID_left = links.at(newlink).ID_right; // invert left and right pin
        links.at(newlink).ID_right = temp;

        NodeID tempNode;
        tempNode = firstNode;
        firstNode = secondNode;
        secondNode = tempNode;
    }
 
    nodes.at(firstNode)->nextNodes.emplace(secondNode,nodes[secondNode].get());     // add the node right of the link to the left node's "nextNodes" list
    sends.at(firstNode).push_back(nodes.at(secondNode)->getID());       // update sends list

}

void AudioEngine::topologicalSortNodes() {

    std::map<NodeID, vector<NodeID>>& adj = sends;

    int n = adj.size();
    std::map<NodeID, int> indegree;
    std::queue<NodeID> q;
    vector<NodeID> list;

    // Compute indegrees
    for (auto& outs : adj) {
        for (auto& singleOutput : outs.second)      // for all outgoing links of ID
            indegree[singleOutput]++;
    }

    // Add all nodes with indegree 0 
    // into the queue
    for (auto& degNode : nodes)
        if (indegree[degNode.first] == 0)
            q.push(degNode.first);

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

    sortedNodes = list;


    // calculate adjacent nodes

    // then sort and push to sortedNodes



}

void AudioEngine::deleteLink(LinkID linkID) {

    NodeID leftNode = m_PinNodePairs.at(links.at(linkID).ID_left.Get());
    NodeID rightNode = m_PinNodePairs.at(links.at(linkID).ID_right.Get());

    std::vector<NodeID>& conns = sends.at(leftNode);        // delete connection for left node from sends list

    for (std::vector<NodeID>::iterator it = conns.begin(); it != conns.end();)
    {
        if (*it == rightNode) {
            it = conns.erase(it);
        } else {
            ++it;
        }
    }

    nodes.at(leftNode)->nextNodes.erase(rightNode);         // also delete device pointer from "nextNodes" list
    links.erase(linkID);

    topologicalSortNodes();
}

NodeID AudioEngine::addNewDeviceNode(BlockType blockType, juce::String initDeviceName) {

    // create 2 new IDs. 1 for device, 1 for pin
    NodeID blockID = getNewID(Identifier::node);
    PinID pinID = getNewID(Identifier::pin);
    nodes[blockID] = make_unique<DeviceNode>(blockType, initDeviceName, blockID);        // add new block

    std::unique_lock<std::mutex> lock(nodeLock);

    if(blockType == BlockType::InputDevice)
        nodes[blockID]->addPin(pinID, pinType::output);        // add 1 pin for each device. input or output is decided by node type   
    
    if (blockType == BlockType::OutputDevice)
        nodes[blockID]->addPin(pinID, pinType::input);
    

    m_PinNodePairs.emplace(pinID, blockID);                             // make the parent node easier to find using a LUT                                                                     
    fifoLevels.emplace(blockID,0); // add channel for fifo monitoring (debug)
    
    sends.emplace(blockID, vector<NodeID>());

    DeviceNode* devptr = dynamic_cast<DeviceNode*>(nodes[blockID].get());

  //  if (blockType == BlockType::InputDevice) inputBuffers.emplace(blockID,juce::AudioSampleBuffer(2, 512));        // add a buffer for any corresponding HW block
  //  if (blockType == BlockType::OutputDevice) outputBuffers.emplace(blockID, juce::AudioSampleBuffer(2, 512));


    if (devptr->getBlockType() == BlockType::OutputDevice)
        devptr->setAsMainOutput(&requestNewAudioBlock);
    
    return blockID;
}

NodeID AudioEngine::addNewDSPNode(EffectType typeOfEffect) {

    NodeID blockID = getNewID(Identifier::node);                                        // get new IDs for node and pins
    PinID inputPinID = getNewID(Identifier::pin);
    PinID outputPinID = getNewID(Identifier::pin);

    std::unique_lock<std::mutex> lock(nodeLock);

    switch (typeOfEffect)
    {
    case EffectType::Filter:
        nodes.emplace(blockID, make_unique<FilterNode>(BlockType::DSP, "Filter", blockID));
        break;
    case EffectType::Gain:
        nodes.emplace(blockID, make_unique<GainNode>(BlockType::DSP, "Gain", blockID));
        break;
    case EffectType::Reverb:
        nodes.emplace(blockID, make_unique<ReverbNode>(BlockType::DSP, "Reverb", blockID));
        break;
    case EffectType::EQ:
        nodes.emplace(blockID, make_unique<EqualizerNode>(BlockType::DSP, "EQ 4", blockID));
    default:
        break;
    }   
    
    sends.emplace(blockID, vector<NodeID>());

    DSPNode* dspptr = dynamic_cast<DSPNode*>(nodes.at(blockID).get());
    dspptr->prepareToPlay(48000, BLOCKSIZE);                                 // initialize samplerates   // ONLY FOR DSPNODE


    nodes.at(blockID)->addPin(inputPinID, pinType::input);                   // assign pins
    nodes.at(blockID)->addPin(outputPinID, pinType::output);

    m_PinNodePairs.emplace(inputPinID, blockID);                             // save pin assignment in LUT
    m_PinNodePairs.emplace(outputPinID, blockID);

    return blockID;
}

void AudioEngine::deleteNode(NodeID nodeToDelete) {

    if (!nodes.contains(nodeToDelete)) {
        Logger::log("Invalid Device queued for deletion", level_ERROR);
        return;
    }

    BlockType type = nodes.at(nodeToDelete)->getBlockType();
    AudioNode* nodePointer = nodes.at(nodeToDelete).get();
    vector<LinkID> linksToDelete;
    std::unique_lock<std::mutex> lock(nodeLock);


    // Break all connected links, including sends list and nextNodes list
    if (node::HasAnyLinks((node::NodeId)nodeToDelete)) {
        for (auto& [linkID,toDelete] : links) {
            if (nodePointer->hasPin(toDelete.ID_left.Get()) != pinType::null ||
                nodePointer->hasPin(toDelete.ID_right.Get()) != pinType::null) {
                linksToDelete.push_back(linkID);
            }
        }
        for (auto& linkID : linksToDelete)   // so it doesn't crash mid-for loop
            deleteLink(linkID);              // also removes left->nextNodes[right] & sends[left][it(right)] 
    }

    switch (type)
    {
    case BlockType::NullDevice:
        break;
    case BlockType::InputDevice:
        m_PinNodePairs.erase(nodePointer->outputPin);
        fifoLevels.erase(nodeToDelete);
        break;
    case BlockType::OutputDevice:     
        m_PinNodePairs.erase(nodePointer->inputPin);
        fifoLevels.erase(nodeToDelete);
        break;
    case BlockType::DSP:
        m_PinNodePairs.erase(nodePointer->inputPin);
        m_PinNodePairs.erase(nodePointer->outputPin);
        break;
    case BlockType::FileInput:
        break;
    default:
        break;
    }

    sends.erase(nodeToDelete);
    nodes.erase(nodeToDelete);

    topologicalSortNodes();     // to update nodecount of sortedNodes

    
}   // releases mtx

void AudioEngine::changeAudioDevice(NodeID deviceID, const juce::String& nameToFind, bool isOutput) {

 //   if (nodes.contains(deviceID))
 //       nodes[deviceID]->selectDevice(nameToFind, isOutput);
 //   else
 //       Logger::log("Invalid audio device node selected", level_ERROR, source_AUDIO);

}
