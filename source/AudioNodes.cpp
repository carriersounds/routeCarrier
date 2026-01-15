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

    for (auto& [nextID, nextNode] : nextNodes) {
        mixInto(&nextNode->inputBuffer, &outputBuffer);
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

void AudioNode::copyBuffer(juce::AudioBuffer<float>* dest, const juce::AudioBuffer<float>* src)
{
    jassert(dest->getNumChannels() == src->getNumChannels());
    jassert(dest->getNumSamples() == src->getNumSamples());

    dest->makeCopyOf(*src, false);
}

bool AudioNode::ImGui_InvertedFloatSlider(const char* name, float& param, float min, float max, const char* format, ImGuiSliderFlags flags) {
    
    ImGui::Text(name);
    ImGui::SameLine(60);
    if (ImGui::SliderFloat(string("##" + (string)name + to_string(ID)).c_str(), &param, min, max, format, flags))
        return true;
    else
        return false;
    
    // "paramChanged = true" is not implemented here, as we might want to do some calculations or interpretation
    // on the parameters first before flagging the parameters as changed to the processor
}