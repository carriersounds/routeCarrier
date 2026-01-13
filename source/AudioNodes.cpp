#include "AudioEngine.h"
#include "AudioNodes.h"

AudioNode::AudioNode(BlockType blocktype, juce::String initDeviceName, NodeID nodeID) : 
	m_blockType(blocktype), 
	Name(initDeviceName),
	ID(nodeID),
	numChannels(2), 
	inputBuffer(2, BLOCKSIZE),
	outputBuffer(2, BLOCKSIZE)
{ 

	inputBuffer.setSize(numChannels, BLOCKSIZE, false, true, true);
	outputBuffer.setSize(numChannels, BLOCKSIZE, false, true, true);

	inputPin = 0;	// 0 means not present or connected
	outputPin = 0;
}

void AudioNode::sendAudioToNextNodes() {

    for (AudioNode* next : nextNodes) {
        mixInto(&next->inputBuffer, &outputBuffer);
    }
}

void AudioNode::mixInto(juce::AudioBuffer<float>* dest, const juce::AudioBuffer<float>* src)
{
    jassert(dest->getNumChannels() == src->getNumChannels());
    jassert(dest->getNumSamples() == src->getNumSamples());
    jassert(dest != nullptr);

    int sampilos = src->getNumSamples();

    for (int ch = 0; ch < dest->getNumChannels(); ++ch)
    {
        dest->addFrom(ch,          // dest channel
            0,           // dest start sample
            *src,
            ch,          // src channel
            0,           // src start sample
            src->getNumSamples());

        // ADD GAIN !!

    }
}