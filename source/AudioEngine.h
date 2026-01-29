#ifndef AUDIO_ENGINE
#define AUDIO_ENGINE
#include "mainHeader.hpp"
#include <JuceHeader.h>

#include "AudioDeviceNode.h"
#include "AudioDSPNode.h"
#include "AudioNodes.h"
#include "AudioEffects.h"
#include "AudioVisualizerNode.h"

class Program;

/*
TODO:
- BUG: multiple outputs will still drift -> small crackling will build up
- Actual sample rate conversion
- proper fifo management
- Create full channel strip, modular effects, working effects, processdoubler?
    - compressor
    - simple channel strip
    - waveform visualizer per link
   ? - drag link up/down to set gain for that copy stage, buffer write operations = gain included always

later: 
- save preset
- make sure you can't make the same link >1 times
- ability to choose ASIO (like pioneer ddj for routing)
- improve graphin GUI lmao


*/



class AudioEngine {
public:
    AudioEngine(Program* prog);
    ~AudioEngine();

    void run();
 
    juce::WaitableEvent requestNewAudioBlock;       // from main output
    
    atomic<bool> enableRouting;
     


    // Identifiers
    DeviceNode nullDevice;                                      // no input, no output, just for namecheck 
    std::map<NodeID, unique_ptr<AudioNode>> nodes;              // Both DSP and device nodes
    std::map<NodeID, vector<NodeID>> sends;                     // where each node sends its audio to
    vector<NodeID> sortedNodes;                                 // input for topological sort
    std::map<LinkID, BlockLink> links;                          // main interface for generating and deleting links
    std::map<PinID, NodeID> m_PinNodePairs;                     // first = pinID, second = corresponding node ID
    

    struct FifoData {
        FifoData() { avgFill = 0; totFill = 0; ratio = 1; }
        float avgFill;
        float totFill;
        float ratio;
    };
    std::map<NodeID, FifoData> fifoLevels;                          // for metering buffers

    // Node Interface
    NodeID addNewDeviceNode(BlockType blockType, juce::String initDeviceName, NodeID PresetNodeID = 0);   // preset       // choose input or output, returns next ID
    NodeID addNewDSPNode(EffectType typeOfEffect, NodeID PresetNodeID = 0);                               // preset       // effects = enum in function input, return is for GUI i think?   
    void createLink(node::PinId leftPin, node::PinId rightPin, LinkID presetID = 0);                      // preset  
    void deleteNode(NodeID deviceID);                                             
    void deleteLink(LinkID linkID);                                                                                           
    void breakAllLinks(NodeID node);                                            
    void changeAudioDevice(NodeID deviceID, const juce::String& nameToFind, bool isOutput); 
    void selectMainOutput(NodeID id);
    void clearAll();
    float engineClockTimeMs = 0;
    float engineProcessTimeMs = 0;




private:
    BaseID uniqueID;
    BaseID getNewID(Identifier type, BaseID presetComponentID = 0);
    void calculateSends(LinkID newlink);
    void topologicalSortNodes();
    bool mainOutputReady;
    int numDevices = 0;
    std::thread audiothread;
    std::mutex nodeLock;
    Program* prog;
    bool audio_engine_on;
};


#endif