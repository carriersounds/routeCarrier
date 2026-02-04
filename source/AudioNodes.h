#ifndef AUDIO_NODES
#define AUDIO_NODES
#include "mainHeader.hpp"
#include <JuceHeader.h>
#include "FileIO.h"

// base class for any node in the network
class AudioNode {
public:

	AudioNode(BlockType blocktype, juce::String initDeviceName, NodeID nodeID, bool asPreset = false);
	virtual ~AudioNode() = default;
	
	// Main Interface
	bool isInput() const { return (m_blockType == BlockType::InputDevice); }
	bool isOutput() const { return (m_blockType == BlockType::OutputDevice); }
	bool isDSP() const { return ((int)m_blockType > 4); }
	NodeID getID() const { return ID; }
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

	// Called from main engine loop
	virtual void prepareOutput() = 0;		// can only run if all it's inputs have been mixed into its inputbuffer (requires topo sort)
	void sendAudioToNextNodes();			// also sends it to its gui buffer

	// GUI
	virtual void renderAsNode(float pinSize, float spacing) = 0;
	bool showInterface = 1;
	ImVec2 startPos;

	// Presets
	virtual void saveCurrentState() {}
	virtual bool loadPreset(NodePreset loaded) { return 0; }


	// Data
	NodePreset currentState;
	PinID inputPin;
	PinID outputPin;
	std::map<NodeID,AudioNode*> nextNodes;
	juce::AudioBuffer<float> inputBuffer;		// can be filled by other members
	double sampleRate = 48000;
	int blockSize = BLOCKSIZE;
	int numChannels = 2;
	bool bypass = false;                      // simple toggle 

protected:
	juce::AudioBuffer<float> outputBuffer;		// can only be filled by itself
	NodeID ID;
	juce::String Name;
	BlockType m_blockType;
	static void mixInto(juce::AudioBuffer<float>* dest, const juce::AudioBuffer<float>* src);
	static void copyBuffer(juce::AudioBuffer<float>* dest, const juce::AudioBuffer<float>* src);

	// Decouple gui controls from audio using a single flag. currently only used by dsp, but will add more gui to device nodes as well prolly
	atomic<bool> parameterChanged = false;
	std::mutex guiMtx;
	juce::AudioBuffer<float> GUIbuffer;		// for displaying output level. 
};


#endif