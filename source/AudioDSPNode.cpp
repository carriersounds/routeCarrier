#include "AudioDSPNode.h"


DSPNode::DSPNode(BlockType blocktype, juce::String initDeviceName, NodeID nodeID) :
    AudioNode(blocktype, initDeviceName, nodeID),
    inputBuffer(2, BLOCKSIZE),
    outputBuffer(2, BLOCKSIZE),
    numInputs(2),
    numOutputs(2) 
{
    inputBuffer.setSize(numChannels, BLOCKSIZE, false, true, true);
    outputBuffer.setSize(numChannels, BLOCKSIZE, false, true, true);
}