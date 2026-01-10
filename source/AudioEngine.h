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


+ make sure mixing into output actually works... now only the last copy is valid
- BUG: multiple outputs will still drift -> small crackling will build up
- Actual sample rate conversion
- Handle node order, topological sort
- this means re-work the loop in the main run() function for links
- +make sure audio always flows from left to right, so input devices first, then dsp, then output devices
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
- Editable parameters through GUI
    - parameter ID / effect ID? created with ID++, depending on # of params
- drag link up/down to set gain for that copy stage, buffer write operations = gain included always
+ delete input/output buffers when a device node gets deleted
- make samplerate, blocksize and numchannels part of the base class. they all need it anyways. determined by engine params
- BUG: sometimes when re-connecting nodes to new links, the audio doesn't pass. need to delete DSP block and create new one to solve...
  ^^ probably due to link order in for(link : links)

later: 
- drag & drop DSP and device nodes
- save preset
- make sure you can't make the same link >1 times
- ability to choose ASIO (like pioneer ddj for routing)
- improve graphin GUI lmao


Node Features:

- right click node: options
- right-click pin: options?
- right-click links: options?
- 
- if (is hw device block) select (i/o) device (default = null)
- 
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
    std::map<NodeID, unique_ptr<DeviceNode>> deviceNodes;       // node ID, which contains pins   
    std::map<NodeID, unique_ptr<DSPNode>> DSPNodes;             // node ID, which contains pins   
    std::map<LinkID,BlockLink> links;                           // link ID + pins
    std::map<PinID, NodeID> m_PinNodePairs;                     // first = pinID, second = corresponding node ID
    std::map<NodeID, juce::AudioBuffer<float>> inputBuffers;
    std::map<NodeID, juce::AudioBuffer<float>> outputBuffers;
    std::map<NodeID,float> fifoLevels;                              // for metering buffers
   // juce::AudioProcessorGraph DSPGRaph;

    // Node management
    NodeID addNewDeviceNode(BlockType blockType, juce::String initDeviceName);          // choose input or output, returns next ID
    NodeID addNewDSPNode(EffectType typeOfEffect);                                 // effects = enum in function input, return is for GUI i think?   
    void deleteDeviceNode(NodeID deviceID);
    void deleteDSPNode(NodeID blockID);
    void changeAudioDevice(NodeID deviceID, const juce::String& nameToFind, bool isOutput);   
   
    BaseID getNewID(Identifier type);                                                   // uniqueID++
    void createLink(node::PinId leftPin, node::PinId rightPin);
    void deleteLink(LinkID linkID);

    // copying / mixing buffers
    void copyBuffer(juce::AudioBuffer<float>* dest,const juce::AudioBuffer<float>* src);
    void mixInto(juce::AudioBuffer<float>* dest, const juce::AudioBuffer<float>* src);


    vector<juce::String> getDeviceNames();          // uses nullDevice, (type ASIO / wasapi etc)


    // TBA                              
    void setDSPParameter(NodeID blockID,int effectID, float value);     // --> node indexing probably using some smartypants graph theory
    void modifyEffectBlock(NodeID blockID);                             // add or remove effect from the chain         
    void editNode(int action, NodeID nodeID);                           // enum action (remove/connect/split/merge), nodeID is how they connect
    void topologicalSortNodes();                                        // to make sure the processing order / graph is actually correct

    

    bool audio_engine_on;

private:
    int numDevices = 0;
    std::thread audiothread;
    Program* prog;
    BaseID uniqueID;
};


#endif