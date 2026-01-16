#ifndef AUDIO_ENGINE
#define AUDIO_ENGINE
#include "MainHeader.h"
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
- CHECK FOR FEEDBACK LOOPS: any output to connect: disable all inputs "before" this signal chain, so it can't even be created (this maintains the non-loopness)
- proper fifo management
- Create full channel strip, modular effects, working effects, processdoubler?
    + filters (low / band / hi)
    + gain
    - EQ
    - compressor
    - reverb
    - saturation tanh() / digClip / sinefold
    - waveform visualizer per link
- drag link up/down to set gain for that copy stage, buffer write operations = gain included always

later: 
- save preset
- make sure you can't make the same link >1 times
- ability to choose ASIO (like pioneer ddj for routing)
- improve graphin GUI lmao

Node Features:

- right click node: options
- right-click pin: options?
- right-click links: options?
 
- monitor input or output (show waveform on graph)
- delete / disconnect
- set as main output

*/

class AudioEngine {
public:
    AudioEngine(Program* prog);
    ~AudioEngine();

    void run();
 
    juce::WaitableEvent requestNewAudioBlock;       // from main output
    bool enableRouting = true;
   
    
    // Identifiers
    DeviceNode nullDevice;                                      // no input, no output, just for namecheck 
    std::map<NodeID, unique_ptr<AudioNode>> nodes;              // Both DSP and device nodes
    std::map<NodeID, vector<NodeID>> sends;                     // as input for toposort
    vector<NodeID> sortedNodes;       
    std::map<LinkID, BlockLink> links;
    std::map<PinID, NodeID> m_PinNodePairs;                     // first = pinID, second = corresponding node ID
    std::map<NodeID,float> fifoLevels;                          // for metering buffers

    // Node management
    NodeID addNewDeviceNode(BlockType blockType, juce::String initDeviceName);          // choose input or output, returns next ID
    NodeID addNewDSPNode(EffectType typeOfEffect);                                 // effects = enum in function input, return is for GUI i think?   
    void deleteNode(NodeID deviceID);
    void changeAudioDevice(NodeID deviceID, const juce::String& nameToFind, bool isOutput);   
    void calculateSends(LinkID newlink);
    void topologicalSortNodes();
    BaseID getNewID(Identifier type);                                                   // uniqueID++
    void createLink(node::PinId leftPin, node::PinId rightPin);
    void deleteLink(LinkID linkID);
    void breakAllLinks(NodeID node);

    void selectMainOutput(NodeID id);
    vector<juce::String> getDeviceNames();          // uses nullDevice, (type ASIO / wasapi etc) 
    bool audio_engine_on;

private:
    int numDevices = 0;
    std::thread audiothread;
    std::mutex nodeLock;
    Program* prog;
    BaseID uniqueID;
};


#endif