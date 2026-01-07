#ifndef AUDIO_NODES
#define AUDIO_NODES
#include "MainHeader.h"
#include <JuceHeader.h>



// base class

struct AudioNode {
public:

	AudioNode(BlockType blocktype, juce::String initDeviceName, size_t nodeID);

	size_t ID;
	juce::String Name;
	BlockType m_blockType;
	
	size_t inputPin;
	size_t outputPin;

	vector<size_t> connections;

	

	void addPin(size_t pinID, pinType type) { 
		
		if (type == pinType::input) inputPin = pinID;
		if (type == pinType::output) outputPin = pinID;
	
	}

	pinType hasPin(size_t pinID) const {
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