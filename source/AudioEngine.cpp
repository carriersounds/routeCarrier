#include "AudioEngine.h"
#include "Program.h"

AudioEngine::AudioEngine(Program* prog) : 
    prog(prog), nullDevice(BlockType::NullDevice,"",1000){
    enableRouting = true;
    mainOutputReady = false;
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

    Counter engineClock;
    Counter processClock;

    while (audio_engine_on) {
           
        engineClockTimeMs = engineClock.getDurationLoop() / 1000.0f;
        
        if (mainOutputReady)
            requestNewAudioBlock.wait();                // wait for main output fifo to empty below BLOCKSIZE
        else
            std::this_thread::sleep_for(10.60ms);       // simulate tickrate for routing previews. 48khz at 512 samples = 10.66ms (assuming 60us processing)

        nodeLock.lock();
        processClock.startTimer();

        if (enableRouting) {

            for (auto& node : sortedNodes) {
                nodes.at(node)->inputBuffer.clear();    // so they can be filled safely
            }

            for (auto& node : sortedNodes) {
                nodes.at(node)->prepareOutput();        // run processing or fifo read/write
                nodes.at(node)->sendAudioToNextNodes(); // mix audio into nodes linked to this->output
            }

            for (auto& [id, node] : nodes) {
                if (fifoLevels.contains(id)) {
                    auto* devNode = dynamic_cast<DeviceNode*>(node.get());
                    fifoLevels[id].avgFill = devNode->avgFill;
                    fifoLevels[id].totFill = devNode->currentFill;
                    fifoLevels[id].ratio = devNode->SRC_ratio * 1000.0f;
                }
            }
        }
        engineProcessTimeMs = processClock.getDuration() / 1000.0f;
        nodeLock.unlock();
    }
}

void AudioEngine::Load(FileIOFlags openDialog, string path) {

    std::string PresetFilePath;
    Logger::log("Loading preset...", level_INFO, source_FILES);

    if (openDialog == f_openDialog)
        PresetFilePath = FileIO::OpenFileDialog(f_preset);          // Check filepath
    else if (path.size())
        PresetFilePath = path;
    else
        PresetFilePath = FileIO::LoadLastPresetPath(f_preset);

    if (PresetFilePath.size() < 1) return;

    node::SetCurrentEditor(prog->gui.node_Context);         // Needed for node locations and link management
    clearAll();

    bool LocalCoInitialize = SUCCEEDED(CoInitializeEx(nullptr, 0));
    auto* xml = FileIO::XmlFromFile(PresetFilePath);
    if (!xml) {
        Logger::log("PRESET: could not load " + PresetFilePath + ", Incorrect file format", level_WARNING, source_FILES);
        if (LocalCoInitialize) CoUninitialize();
    }

    long countNode = 0;
    IXMLDOMNodeList* NodeList = FileIO::XmlSelectNodes(xml, L"/PRESET/NODES/NODE", countNode);

    // Load all nodes
    for (long i = 0; i < countNode; i++) {
        IXMLDOMNode* SingleNode = FileIO::XmlNodeAt(NodeList, i);
        if (!SingleNode) continue;

        BlockType type = (BlockType)FileIO::XmlAttrInt(SingleNode, L"type");            // Get type & ID
        NodeID ID = FileIO::XmlAttrInt(SingleNode, L"id");

        if (type == BlockType::InputDevice || type == BlockType::OutputDevice)
            addNewDeviceNode(type, FileIO::XmlAttr(SingleNode, L"name"), ID);    // Create in engine, explicit ID assignment    || Add check for missing devices!
        else if (type == BlockType::FileInput || type == BlockType::FileOutput || type == BlockType::Soundboard)
            addNewFileNode(type, ID);
        else
            addNewDSPNode(type, ID);

        ImVec2 SetPos = { FileIO::XmlAttrFloat(SingleNode, L"posX"),FileIO::XmlAttrFloat(SingleNode, L"posY") };
        node::SetNodePosition(ID, SetPos);

        nodes.at(ID)->currentState.AssignParameters(SingleNode);                // Set param values

        SingleNode->Release();
    }

    if (NodeList) NodeList->Release();

    long countLink = 0;
    IXMLDOMNodeList* LinkList = FileIO::XmlSelectNodes(xml, L"/PRESET/LINKS/LINK", countLink);

    // Load all links
    for (long i = 0; i < countLink; i++) {
        IXMLDOMNode* SingleLink = FileIO::XmlNodeAt(LinkList, i);
        if (!SingleLink) continue;

        LinkID Lid = FileIO::XmlAttrInt(SingleLink, L"ID");
        PinID LeftPin = FileIO::XmlAttrInt(SingleLink, L"Left");
        PinID RightPin = FileIO::XmlAttrInt(SingleLink, L"Right");
        createLink(LeftPin, RightPin, Lid);                                     // create link with explicit ID assignment
        SingleLink->Release();
    }
    if (LinkList) LinkList->Release();
    q21

    Logger::log("Loaded preset: " + PresetFilePath, level_INFO, source_FILES);
    node::SetCurrentEditor(nullptr);
    if (xml) xml->Release();
    FileIO::SaveLastPresetPath(f_preset, path);
    if (LocalCoInitialize) CoUninitialize();
}

void AudioEngine::Save(FileIOFlags openDialog, string path) {

    string savePath;

    if (openDialog == f_openDialog)
        savePath = FileIO::saveFileDialog(f_preset);
    else if (path.size())
        savePath = path;
    else
        savePath = FileIO::LastPreset.string();

    if (savePath.size() < 1) return;

    int VERSION = 1;
    string out = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    out += "<PRESET version=\" " + to_string(VERSION) + "\">\n";
    out += "   <GLOBAL>\n";
    out += "      <SETUP LastID=\"" + to_string(uniqueID) + "\"/>\n";
    out += "   </GLOBAL>\n";
    out += "   <NODES>\n";
    for (auto& [ID, node] : nodes)
        out += node->currentState.saveCurrentState(prog->gui.node_Context);       // save each node's own preset params. required engine context to get ax::nodecontext for location

    out += "   </NODES>\n";
    out += "   <LINKS>\n";
    for (auto& [ID, lp] : links)
        out += lp.ToXML();

    out += "   </LINKS>\n";
    out += "</PRESET>";

    Logger::log("Saved preset to : " + savePath, level_INFO, source_FILES);

    FileIO::SaveXML(out, savePath);
}

BaseID AudioEngine::getNewID(Identifier type, BaseID PresetNodeID) {

    string idtype;

    if (PresetNodeID) {
        uniqueID = PresetNodeID;
    }
    else {
        uniqueID++;
    }
  

    if (type == Identifier::link) idtype = "link";
    if (type == Identifier::node) idtype = "node";
    if (type == Identifier::pin)  idtype = "pin";

    Logger::log("new " + idtype +", ID: " + to_string(uniqueID), level_DEBUG);



    return uniqueID;
}

void AudioEngine::createLink(node::PinId leftPin, node::PinId rightPin, LinkID presetLinkID) {

    LinkID nextID = getNewID(Identifier::link, presetLinkID);
    links.emplace(nextID, BlockLink(nextID, leftPin, rightPin));

    calculateSends(nextID);
    topologicalSortNodes();

    // Detect feedback loops, will call toposort again if detected
    if (sortedNodes.size() != nodes.size()) {
        deleteLink(nextID);
    }
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
    sends.at(firstNode).push_back(nodes.at(secondNode)->getID());                   // update sends list

}

void AudioEngine::topologicalSortNodes() {

    int n = sends.size();
    std::map<NodeID, int> indegree;
    std::queue<NodeID> q;
    vector<NodeID> list;

    // Compute indegrees
    for (auto& outs : sends) {
        for (auto& singleOutput : outs.second)      // for all outgoing links of ID
            indegree[singleOutput]++;
    }

    // Add all nodes with indegree 0 into the queue
    for (auto& degNode : nodes)
        if (indegree[degNode.first] == 0)
            q.push(degNode.first);

    // Kahn’s Algorithm (BFS)
    while (!q.empty()) {
        int top = q.front();
        q.pop();
        list.push_back(top);

        for (auto& nextID : sends[top]) {
            indegree[nextID]--;
            if (indegree[nextID] == 0)      // nodes involved in feedback loops will never reach degree zero and will not be added to sortedNodes
                q.push(nextID);
        }
    } 
   
    sortedNodes = list;
}

void AudioEngine::deleteLink(LinkID linkID) {

    NodeID leftNode = m_PinNodePairs.at(links.at(linkID).ID_left.Get());
    NodeID rightNode = m_PinNodePairs.at(links.at(linkID).ID_right.Get());

    std::vector<NodeID>& conns = sends.at(leftNode);        // delete connection for left node from sends list

    tools::RemoveObjectFromVector(conns, rightNode);

    nodes.at(leftNode)->nextNodes.erase(rightNode);         // also delete device pointer from "nextNodes" list
    links.erase(linkID);                                    // finally, delete actual link

    topologicalSortNodes();
}

void AudioEngine::breakAllLinks(NodeID node){
    // Break all connected links, including sends list and nextNodes list
    AudioNode* nodePointer = nodes.at(node).get();
    vector<LinkID> linksToDelete;
    std::unique_lock<std::mutex> lock(nodeLock);

    if (node::HasAnyLinks((node::NodeId)node)) {
        for (auto& [linkID, toDelete] : links) {
            if (nodePointer->hasPin(toDelete.ID_left.Get()) != pinType::null ||
                nodePointer->hasPin(toDelete.ID_right.Get()) != pinType::null) {
                linksToDelete.push_back(linkID);
            }
        }
        for (auto& linkID : linksToDelete)   // so it doesn't crash mid-for loop
            deleteLink(linkID);              // also removes left->nextNodes[right] & sends[left][it(right)] 
    }
}

NodeID AudioEngine::addNewDeviceNode(BlockType blockType, juce::String initDeviceName, NodeID presetNodeID) {

    // create 2 new IDs. 1 for device, 1 for pin
    NodeID blockID = getNewID(Identifier::node, presetNodeID);
    PinID pinID = getNewID(Identifier::pin);


    nodes[blockID] = make_unique<DeviceNode>(blockType, initDeviceName, blockID);        // add new block

    std::unique_lock<std::mutex> lock(nodeLock);

    if(blockType == BlockType::InputDevice)
        nodes[blockID]->addPin(pinID, pinType::output);     // add 1 pin for each device. input or output is decided by node type   
    
    if (blockType == BlockType::OutputDevice)
        nodes[blockID]->addPin(pinID, pinType::input);
    

    m_PinNodePairs.emplace(pinID, blockID);                 // make the parent node easier to find using a LUT                                                                     
    
   fifoLevels.emplace(blockID,FifoData());                          // add channel for fifo monitoring (debug)
    
    sends.emplace(blockID, vector<NodeID>());

    DeviceNode* devptr = dynamic_cast<DeviceNode*>(nodes[blockID].get());

    if (devptr->getBlockType() == BlockType::OutputDevice) {
        devptr->setTriggerAddress(&requestNewAudioBlock);
        selectMainOutput(blockID);
        mainOutputReady = true;
    }

    topologicalSortNodes();                                 // add to sortedNodes
    
    return blockID;
}

NodeID AudioEngine::addNewDSPNode(BlockType typeOfEffect, NodeID presetNodeID) {

    NodeID blockID = getNewID(Identifier::node, presetNodeID);                // get new IDs for node and pins. pins will follow pre-set ID
    PinID inputPinID = getNewID(Identifier::pin);
    PinID outputPinID = getNewID(Identifier::pin);

    std::unique_lock<std::mutex> lock(nodeLock);

    switch (typeOfEffect)
    {
    case BlockType::Filter:
        nodes.emplace(blockID, make_unique<Filter>(BlockType::Filter, "Filter", blockID));
        break;
    case BlockType::Gain:
        nodes.emplace(blockID, make_unique<Gain>(BlockType::Gain, "Gain", blockID));
        break;
    case BlockType::Reverb:
        nodes.emplace(blockID, make_unique<Reverb>(BlockType::Reverb, "Reverb", blockID));
        break;
    case BlockType::EQ:
        nodes.emplace(blockID, make_unique<Equalizer>(BlockType::EQ, "Graphic Equalizer", blockID));
        break;
    case BlockType::Saturator:
        nodes.emplace(blockID, make_unique<Saturator>(BlockType::Saturator, "Saturator", blockID));
        break;
    case BlockType::ChannelUtil:
        nodes.emplace(blockID, make_unique<ChannelUtility>(BlockType::ChannelUtil, "Channel Utility", blockID));
        break;
    case BlockType::Compressor:
        nodes.emplace(blockID, make_unique<Compressor>(BlockType::Compressor, "Compressor", blockID));
        break;
    case BlockType::Phaser:
       // nodes.emplace(blockID, make_unique<Phaser>(BlockType::DSP, "Saturator", blockID));
        break;
    default:
        break;
    }   
    
    

    sends.emplace(blockID, vector<NodeID>());

    DSPNode* dspptr = dynamic_cast<DSPNode*>(nodes.at(blockID).get());
    Logger::log(("Added " + dspptr->getBlockName()).c_str());

    dspptr->prepareToPlay(48000, BLOCKSIZE);                                 // initialize samplerates 
    nodes.at(blockID)->addPin(inputPinID, pinType::input);                   // assign pins
    nodes.at(blockID)->addPin(outputPinID, pinType::output);

    m_PinNodePairs.emplace(inputPinID, blockID);                             // save pin assignment in LUT
    m_PinNodePairs.emplace(outputPinID, blockID);

    topologicalSortNodes();                                                  // to add to sortedNodes

    return blockID;
}

NodeID AudioEngine::addNewFileNode(BlockType blockType, NodeID PresetNodeID)
{
    if (blockType != BlockType::FileInput &&
        blockType != BlockType::FileOutput &&
        blockType != BlockType::Soundboard) {
        Logger::log("addNewFileNode: unsupported block type", level_ERROR, source_AUDIO);
        return 0;
    }

    std::lock_guard<std::mutex> lock(nodeLock);

    NodeID newID = getNewID(Identifier::node, PresetNodeID);    // PresetNodeID == 0 -> fresh ID

    // ---- create ----
    unique_ptr<AudioNode> node;
    switch (blockType)
    {
    case BlockType::FileInput: {
        auto n = make_unique<AudioFileNode>(blockType, "Audio File Player", newID);
        n->setFileIO(&prog->fileIO);                // for the open dialog
        node = std::move(n);
        break;
    }
    case BlockType::FileOutput: {
        auto n = make_unique<AudioFileNode>(blockType, "Audio File Recorder", newID);
        n->setFileIO(&prog->fileIO);                // for the save dialog
        node = std::move(n);
        break;
    }
    case BlockType::Soundboard: {
        auto n = make_unique<AudioSoundboardNode>(blockType, "Soundboard", newID);
        n->setFileIO(&prog->fileIO);
        node = std::move(n);
        break;
    }
    default: break;
    }

    // ---- pin: players / soundboard are sources (output pin), recorder is a sink (input pin) ----
    const pinType direction = (blockType == BlockType::FileOutput) ? pinType::input
        : pinType::output;
    const PinID pinID = getNewID(Identifier::pin);
    node->addPin(pinID, direction);
    m_PinNodePairs[pinID] = newID;

    // ---- register ----
    sends[newID] = {};                              // GUI link validation does sends.at(node), so it must exist
    nodes[newID] = std::move(node);

    Logger::log("Added " + nodes[newID]->getBlockName() + " (ID " + to_string(newID) + ")",
        level_INFO, source_AUDIO);

    // topologicalSortNodes();                      // only if addNewDeviceNode does this on creation

    return newID;
}

void AudioEngine::deleteNode(NodeID nodeToDelete) {

    if (!nodes.contains(nodeToDelete)) {
        Logger::log("Invalid Node queued for deletion", level_ERROR);
        return;
    }

    BlockType type = nodes.at(nodeToDelete)->getBlockType();
    AudioNode* nodePointer = nodes.at(nodeToDelete).get();
    vector<LinkID> linksToDelete;

    breakAllLinks(nodeToDelete);    // has its own mutex, but can be called separately

    std::unique_lock<std::mutex> lock(nodeLock);

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
    case BlockType::FileInput:
        break;
    default:
        m_PinNodePairs.erase(nodePointer->inputPin);
        m_PinNodePairs.erase(nodePointer->outputPin);
        break;
    }

    sends.erase(nodeToDelete);
    nodes.erase(nodeToDelete);

    topologicalSortNodes();     // to update nodecount of sortedNodes

    
}   // releases mtx

void AudioEngine::changeAudioDevice(NodeID deviceID, const juce::String& nameToFind, bool isOutput) {

    if (nodes.contains(deviceID)) {
        if (nodes.at(deviceID)->isInput() || nodes.at(deviceID)->isOutput()) {
            DeviceNode* devptr = dynamic_cast<DeviceNode*>(nodes.at(deviceID).get());
            devptr->selectDevice(nameToFind, isOutput);
        }        
    } else
        Logger::log("Invalid audio device selected", level_ERROR, source_AUDIO);

}

void AudioEngine::selectMainOutput(NodeID nodeID) {

    if (!nodes.at(nodeID)->isOutput()) {
        Logger::log("Invalid Device selected to set as main output", level_WARNING);
        return;
    }

    DeviceNode* devptr;


    for (auto& [id, device] : nodes) {
        if(devptr = dynamic_cast<DeviceNode*>(device.get()))
        devptr->setAsMainOutput(false);
    }

    devptr = dynamic_cast<DeviceNode*>(nodes.at(nodeID).get());
    devptr->setAsMainOutput(true);

   // mainOutSampleRate = devptr->sampleRate;
}

void AudioEngine::clearAll() {

    vector<NodeID> IDsToDelete;

    for (auto& [id, node] : nodes) {
        IDsToDelete.push_back(id);
    }
 
    for (auto& id : IDsToDelete) {
        deleteNode(id);             // also removes links, pins, sends, pairs etc...
    }

    Logger::log("Cleared all nodes", level_INFO);
        
    uniqueID = 1000;

}