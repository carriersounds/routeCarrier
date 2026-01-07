#ifndef AUDIO_NODES
#define AUDIO_NODES
#include "MainHeader.h"
#include <JuceHeader.h>



// base class

class AudioNode {
public:

	AudioNode(BlockType blocktype, juce::String initDeviceName, NodeID nodeID);

	NodeID ID;
	juce::String Name;
	BlockType m_blockType;
	
	PinID inputPin;
	PinID outputPin;
	

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


	// right click node: options
	// right-click pin: options?
	// right-click links: options?
	// 
	// if (is hw device block) select (i/o) device (default = null) 
	// 
	// monitor input or output (show waveform on graph)
	// delete / disconnect
	// set as main output


#endif