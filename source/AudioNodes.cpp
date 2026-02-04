#include "AudioEngine.h"
#include "AudioNodes.h"

AudioNode::AudioNode(BlockType blocktype, juce::String initDeviceName, NodeID nodeID, bool asPreset) :
	m_blockType(blocktype), 
	Name(initDeviceName),
	ID(nodeID),
	numChannels(2), 
	inputBuffer(2, BLOCKSIZE),
	outputBuffer(2, BLOCKSIZE),
    GUIbuffer(2,BLOCKSIZE),
    currentState(this)
{ 
	inputBuffer.setSize(numChannels, BLOCKSIZE, false, true, true);
	outputBuffer.setSize(numChannels, BLOCKSIZE, false, true, true);
    GUIbuffer.setSize(numChannels, BLOCKSIZE, false, true, true);

	inputPin = 0;	// 0 means not present or connected
	outputPin = 0;
}

void AudioNode::sendAudioToNextNodes() {

    // Send to adjacent nodes
    for (auto& [nextID, nextNode] : nextNodes) {
        mixInto(&nextNode->inputBuffer, &outputBuffer);    
    }

    // Pass buffer safely to GUI
    if (guiMtx.try_lock()) {
        copyBuffer(&GUIbuffer, &outputBuffer);
        guiMtx.unlock();
    }
}

void AudioNode::mixInto(juce::AudioBuffer<float>* dest, const juce::AudioBuffer<float>* src)
{
    // instead of jassert
    int lowestChannelCount = std::min(dest->getNumChannels(), src->getNumChannels());

    jassert(dest->getNumSamples() == src->getNumSamples());
    jassert(dest != nullptr);

    for (int ch = 0; ch < lowestChannelCount; ++ch)
    {
        dest->addFrom(ch,   // dest channel
            0,              // dest start sample
            *src,
            ch,             // src channel
            0,              // src start sample
            src->getNumSamples());

        // ADD GAIN !!

    }
}

void AudioNode::copyBuffer(juce::AudioBuffer<float>* dest, const juce::AudioBuffer<float>* src)
{
    jassert(dest->getNumChannels() == src->getNumChannels());
    jassert(dest->getNumSamples() == src->getNumSamples());

    dest->makeCopyOf(*src, false);
}

