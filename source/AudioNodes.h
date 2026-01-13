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
	vector<AudioNode*> nextNodes;
	
	double sampleRate = 48000;
	int blockSize = BLOCKSIZE;
	int numChannels = 2;

	juce::AudioBuffer<float> inputBuffer;
	juce::AudioBuffer<float> outputBuffer;

	NodeID getID() const {return ID;}
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
	string getBlockName() const { return Name.toStdString(); }
	virtual void prepareOutput() = 0;		// can only run if all it's inputs have been mixed into its inputbuffer
	void sendAudioToNextNodes();
	static void mixInto(juce::AudioBuffer<float>* dest, const juce::AudioBuffer<float>* src);

	bool isInput() const { return (m_blockType == BlockType::InputDevice); }
	bool isOutput() const { return (m_blockType == BlockType::OutputDevice); }
	bool isDSP() const { return (m_blockType == BlockType::DSP); }
protected:
	virtual void render() = 0;
	
	

	// if dsp, already done
	// if input hw, read from hardware fifo to outputBuffer
	// if output, 
};


#endif