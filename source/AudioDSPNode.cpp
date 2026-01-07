#include "AudioDSPNode.h"


DSPNode::DSPNode(BlockType blocktype, juce::String initDeviceName, NodeID nodeID) : 
	AudioNode( blocktype, initDeviceName, nodeID),inputBuffer(2, BLOCKSIZE),outputBuffer(2, BLOCKSIZE){


		// 2 channel, 256 samples 
	


	addEffect(EffectType::Phaser);

	//addEffect

//	auto* filter = dynamic_cast<juce::dsp::IIR::Filter<float>*>(chain[0].get());
//	*filter = juce::dsp::IIR::Coefficients<float>::makeLowPass(48000, 500.0f);


	// empty route, in = out
	// also call when routing has changed maybe?

}

