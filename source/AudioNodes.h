#ifndef AUDIO_NODES
#define AUDIO_NODES
#include "MainHeader.h"
#include <JuceHeader.h>


// base class for any node in the network
class AudioNode {
public:

	AudioNode(BlockType blocktype, juce::String initDeviceName, NodeID nodeID);

	NodeID ID;
	juce::String Name;
	BlockType m_blockType;
	
	PinID inputPin;
	PinID outputPin;
	
	// double sampleRate = 48000;
	// int blockSize = BLOCKSIZE;
	// int numChannels = 2;



	void addPin(PinID pinID, pinType type) { 
		
		if (type == pinType::input) inputPin = pinID;
		if (type == pinType::output) outputPin = pinID;
	
	}
	pinType hasPin(PinID pinID) const {
		if (inputPin == pinID) return pinType::input;
		if (outputPin == pinID) return pinType::output;
		return pinType::null;
	}
	BlockType getBlockType() const { return m_blockType; }
	string getName() const { return Name.toStdString(); }
};


#endif