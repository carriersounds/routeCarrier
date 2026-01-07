# routeCarrier: A Modular Hardware Audio Router for Windows (WIP)


This is an early work-in-progress modular audio routing tool.

It currently supports **hardware device --> hardware device** audio routing only.  
There is no DSP, no effects, and no processing beyond basic passthrough.

The UI is built around a node editor where audio input and output devices appear as nodes that can be visually connected.

Multiple inputs can be connected to the same output, and vice versa.


## Current State

- Hardware input ↔ hardware output routing
- Visual patching using ImGui node editor
- Multiple-in / multiple-out routing


## To be added

- DSP nodes
- accurate sample rate conversion
- preset saving/loading

This project is still WIP and should be considered a minimum "working" version
