#ifndef AUDIO_ENGINE
#define AUDIO_ENGINE
#include "MainHeader.h"
#include <JuceHeader.h>

#include "AudioDeviceNode.h"
#include "AudioDSPNode.h"
#include "AudioNodes.h"


class Program;

// graph structure:
// drag & drop to insert. . default = mix (add)


// with block = 256: output fifo depth = 3 geeft 768 frames, 16ms latency

/*
TODO:

- make sure mixing into output actually works... now only the last copy is valid
- Actual sample rate conversion
- Handle node order, topological sort
- this means re-work the loop in the main run() function for links
- +make sure audio always flows from left to right, so input devices first, then dsp, then output devices
- CHECK FOR FEEDBACK LOOPS: any output to connect: disable all inputs "before" this signal chain, so it can't even be created (this maintains the non-loopness)
- proper fifo management
- Create full channel strip, modular effects, working effects, processdoubler?
    - filters (low / band / hi)
    - EQ
    - compressor
    - reverb
    - saturation tanh() / digClip / sinefold
    - waveform visualizer per link
- Editable parameters through GUI
    - parameter ID / effect ID? created with ID++, depending on # of params
- drag link up/down to set gain for that copy stage, buffer write operations = gain included always
- delete input/output buffers when a device node gets deleted
- make samplerate, blocksize and numchannels part of the base class. they all need it anyways. determined by engine params

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
    ~AudioEngine(){
        audio_engine_on = false;
        audiothread.join();    
    }

    void run();
 
    juce::WaitableEvent requestNewAudioBlock;       // from main output
    bool enableRouting = true;
   
    
    // Identifiers
    DeviceNode nullDevice;                                      // no input, no output, just for namecheck
    std::map<NodeID, unique_ptr<DeviceNode>> hardwareBlocks;    // node ID, which contains pins   
    std::map<NodeID, unique_ptr<DSPNode>> DSPBlocks;
    std::map<LinkID,BlockLink> links;                           // link ID + pins
    std::map<PinID, NodeID> m_PinNodePairs;                     // first = pinID, second = corresponding node ID
    std::map<NodeID, juce::AudioBuffer<float>> inputBuffers;
    std::map<NodeID, juce::AudioBuffer<float>> outputBuffers;



    // vector<juce::String> getDeviceNames (type ASIO / wasapi etc)
    NodeID addNewDeviceBlock(BlockType blockType, juce::String initDeviceName);         // choose input or output, returns next ID
    NodeID addNewDSPNode(const juce::String& NodeName);                                 // effects = enum in function input, return is for GUI i think?
    
    void deleteDeviceBlock(NodeID deviceID);
    void deleteEffectBlock(NodeID blockID);
    void changeAudioDevice(NodeID deviceID, const juce::String& nameToFind, bool isOutput);   
   

    BaseID getNewID(Identifier type);                                                   // uniqueID++
    void createLink(node::PinId leftPin, node::PinId rightPin);
    void deleteLink(LinkID linkID);

    // for copying / mixing buffers
    void copyBuffer(juce::AudioBuffer<float>& dest,const juce::AudioBuffer<float>& src);
    void mixInto(juce::AudioBuffer<float>& dest, const juce::AudioBuffer<float>& src);


    // TBA
    void setDSPParameter(NodeID blockID,int effectID, float value);     // --> node indexing probably using some smartypants graph theory
    void modifyEffectBlock(NodeID blockID);                             // add or remove effect from the chain         
    void editNode(int action, NodeID nodeID);                           // enum action (remove/connect/split/merge), nodeID is how they connect
    void topologicalSortNodes();            // to make sure the processing order / graph is actually correct

  
    void setGain(float g)
    {
        gain = g;
    }

    vector<float> levels;

    bool audio_engine_on;

private:
    float gain;
    int numDevices = 0;
    std::thread audiothread;
    Program* prog;
    BaseID uniqueID;
};


#endif