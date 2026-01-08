# routeCarrier: A Modular Hardware Audio Router for Windows (WIP)


This is an early work-in-progress modular audio routing tool.

The UI is built around a node editor where audio input and output devices appear as nodes that can be visually connected.

Multiple inputs can be connected to the same output, and vice versa.


## Current State

- Hardware input ↔ hardware output routing
- Very barebones DSP blocks
- Visual patching using ImGui node editor
- Multiple-in / multiple-out routing


This project is still WIP and should be considered a minimum "working" version...

## TODO:

- make sure mixing into output actually works... now only the last copy is valid
- Actual sample rate conversion
- Handle node order, topological sort
- this means re-work the loop in the main run() function for links
- +make sure audio always flows from left to right, so input devices first, then dsp, then output devices
- CHECK FOR FEEDBACK LOOPS: any output to connect: disable all inputs "before" this signal chain, so it can't even be created (this maintains the non-loopness)
- proper fifo management
- Create full channel strip, modular effects, working effects, processdoubler?
    - filters (low / band / hi)
    - EQ
    - compressor
    - reverb
    - saturation tanh() / digClip / sinefold
    - waveform visualizer per link
- Editable parameters through GUI
    - parameter ID / effect ID? created with ID++, depending on # of params
- drag link up/down to set gain for that copy stage, buffer write operations = gain included always
- delete input/output buffers when a device node gets deleted
- make samplerate, blocksize and numchannels part of the base class. they all need it anyways. determined by engine params

Later: 
- drag & drop DSP and device nodes
- save preset
- make sure you can't make the same link >1 times
- ability to choose ASIO (like pioneer ddj for routing)
- improve graphin GUI lmao
