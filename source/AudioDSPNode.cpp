#include "AudioDSPNode.h"


DSPNode::DSPNode(BlockType blocktype, juce::String initDeviceName, NodeID nodeID) :
    AudioNode(blocktype, initDeviceName, nodeID),
    inputBuffer(2, BLOCKSIZE),
    outputBuffer(2, BLOCKSIZE),
    numInputs(2),
    numOutputs(2) {

    inputBuffer.setSize(numChannels, BLOCKSIZE, false, true, true);
    outputBuffer.setSize(numChannels, BLOCKSIZE, false, true, true);
}

void DSPNode::render() {


    // render name, if icon, needs to be base member or set elsewhere

    renderInterface();  // implemented in derived FX classes

    // render i/o pins

}

void DSPNode::process() {
    outputBuffer.makeCopyOf(inputBuffer, false);    // output buf is the process context
    juce::MidiBuffer empt;                  // function needs it RIP
    processBlock(outputBuffer, empt);
}

void DSPNode::prepareToPlay(double sampleRate, int blockSize)
{
    juce::dsp::ProcessSpec spec{
        sampleRate,
        static_cast<int>(blockSize),
        static_cast<int>(numOutputs)
    };

    prepareDSP(spec);
}

void DSPNode::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::dsp::AudioBlock<float> block(buffer);

    //   if (!bypassed) {
    processDSP(block);
    // } else {
    //     outputBuffer.makeCopyOf(inputBuffer, false);
   //  }


}

bool DSPNode::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet().size() == numInputs &&
        layouts.getMainOutputChannelSet().size() == numOutputs;
}