#ifndef AUDIO_ENGINE
#define AUDIO_ENGINE
#include "mainHeader.hpp"
#include <JuceHeader.h>

#include "AudioDeviceNode.h"
#include "AudioDSPNode.h"
#include "AudioNodes.h"
#include "AudioEffects.h"
#include "AudioVisualizerNode.h"
#include "AudioPresets.h"
#include "AudioApplicationNode.h"
#include "AudioFileNode.h"
#include "AudioSoundboardNode.h"
class Program;

/*
* 
NEW TODO 2026:

- NodeType: Application audio stream input
- NodeType: Loopback capture source
- NodeType: Visualizer (scope / FFT)
- NodeType: Soundboard/file node (wav/mp3 file handler).

-------------
New Toolbar:
    Categories:

    Sources:
        Soundboard manager
        Application stream
        Hardware inputs  
        Output Loopback   

    Transform:
        DSP     
        VST Plugin

    Receivers:
        Visualizer
        Hardware Outputs
        Recorder 

Soundboard manager features:

-- soundboard = saved local wav files, file cutter GUI with waveform? Zoom with scroll
-- Record soundboard sample from any input/source. for live soundboard creation

- XML preset format (Advanced FileIO)


TODO:
- Create full channel strip, modular effects, working effects, processdoubler?
   - drag link up/down to set gain for that copy stage, buffer write operations = gain included always
    !! just like OBS

later: 
- make sure you can't make the same link >1 times
- ability to choose ASIO (like pioneer ddj for routing)
- move log of block creation to node constructor
- only switch case is the template for creation, rest can do with get blocktype
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
       
    AppCaptureNode application;

    struct FifoData {
        FifoData() { avgFill = 0; totFill = 0; ratio = 1; }
        float avgFill;
        float totFill;
        float ratio;
    };
    std::map<NodeID, FifoData> fifoLevels;                          // for metering buffers

    // Node Interface
    NodeID addNewDeviceNode(BlockType blockType, juce::String initDeviceName, NodeID PresetNodeID = 0);  // preset    // choose input or output, returns next ID
    NodeID addNewDSPNode(BlockType typeOfEffect, NodeID PresetNodeID = 0);                               // preset    // effects = enum in function input, return is for GUI i think?   
    
    NodeID addNewFileNode(BlockType type, NodeID presetID = 0);
    
    void createLink(node::PinId leftPin, node::PinId rightPin, LinkID presetID = 0);                     // preset  
    void deleteNode(NodeID deviceID);                                             
    void deleteLink(LinkID linkID);                                                                                           
    void breakAllLinks(NodeID node);                                            
    void changeAudioDevice(NodeID deviceID, const juce::String& nameToFind, bool isOutput); 
    void selectMainOutput(NodeID id);
    void clearAll();
    float engineClockTimeMs = 0;
    float engineProcessTimeMs = 0;

    // Loading / Saving as a preset (XML)
    void Load(FileIOFlags openDialog = f_noDialog, string path = "");
    void Save(FileIOFlags openDialog, string path = "");

    Program* prog;
    BaseID uniqueID;
private:

    float mainOutSampleRate = 44100;

    BaseID getNewID(Identifier type, BaseID presetComponentID = 0);
    void calculateSends(LinkID newlink);
    void topologicalSortNodes();
    bool mainOutputReady;
    int numDevices = 0;
    std::thread audiothread;
    std::mutex nodeLock;

    bool audio_engine_on;
};


#endif