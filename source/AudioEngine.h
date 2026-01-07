#ifndef AUDIO_ENGINE
#define AUDIO_ENGINE
#include "MainHeader.h"
#include <JuceHeader.h>

#include "AudioDeviceNode.h"
#include "AudioSignalEffects.h"
#include "AudioNodes.h"


#define BLOCKSIZE 512
#define FIFOSIZE 2048

class Program;

// graph structure:
// gui elements in a list, drag & drop to insert. . default = mix (add)
// 

// input device
// output device

// set masterClock (device ID)
//

class AudioEngine {
public:
    AudioEngine(Program* prog);
    ~AudioEngine(){
        audio_engine_on = false;
        audiothread.join();    
    }

    void run();
 
    juce::WaitableEvent requestNewAudioBlock;       // from main output


    juce::AudioBuffer<float> mainBuffer;

    std::map<size_t, juce::AudioBuffer<float>> inputBuffers;
    std::map<size_t, juce::AudioBuffer<float>> outputBuffers;
    bool enableRouting = true;


    DeviceNode nullDevice;                      // no input, no output, just for namecheck
    
    // Node Identifiers
    std::map<size_t, unique_ptr<DeviceNode>> hardwareBlocks;    // node ID, which contains pins    
    std::map<size_t,BlockLink> links;                           // link ID + pins
    std::map<size_t, size_t> m_PinNodePairs;               // first = pinID, second = corresponding node ID


    int microSleep = 0; 

    // vector<juce::String> getDeviceNames (type ASIO / wasapi etc)

    //++ability to choose ASIO(like ddj RB for routing)
    size_t addNewDeviceBlock(BlockType blockType, juce::String initDeviceName);         // choose input or output, returns next ID
    void deleteDeviceBlock(size_t deviceID);                                            
    void selectDevice(const juce::String& nameToFind, bool isOutput);                   // also input the chosen deviceNode (ID/index) you want to change
    
    void addNewEffectBlock(int effect, std::pair<int, int> location);                   // effects = enum


    size_t getNewID(Identifier type);        // uniqueID++
    void createLink(node::PinId leftPin, node::PinId rightPin);
    void deleteLink(size_t linkID);

    // for copying / mixing buffers
    void copyBuffer(juce::AudioBuffer<float>& dest,const juce::AudioBuffer<float>& src);
    void mixInto(juce::AudioBuffer<float>& dest, const juce::AudioBuffer<float>& src);


    
    void setDSPParameter(int blockID,int effectID, float value);     // --> node indexing probably using some smartypants graph theory
    void modifyEffectBlock(int blockID);                             // add or remove effect from the chain         
    void editNode(int action, int nodeID);                           // enum action (remove/connect/split/merge), nodeID is how they connect

    

    void initDSP(double sr, int bs);




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
    size_t uniqueID;
};


#endif