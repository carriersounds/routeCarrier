#include "AudioEngine.h"
#include "AudioNodes.h"


AudioNode::AudioNode(BlockType blocktype, juce::String initDeviceName, size_t nodeID) : m_blockType(blocktype), Name(initDeviceName),ID(nodeID) {


	inputPin = 0;	// 0 means not present or connected
	outputPin = 0;
}

