#ifndef AUDIO_VISUAL
#define AUDIO_VISUAL
#include "MainHeader.h"
#include <JuceHeader.h>
#include "AudioNodes.h"


class VisualizerNode : public AudioNode
{
public:
    VisualizerNode(BlockType blocktype, juce::String initDeviceName, NodeID nodeID);

    ~VisualizerNode(){

    }



};




#endif