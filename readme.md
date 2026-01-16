# routeCarrier: A Modular Hardware Audio Router for Windows (WIP)

This is a work-in-progress modular audio routing tool.
This program is built around a node editor where audio devices can be connected by a graph structure.

## Who is it for?

- DJs, streamers, audio engineers
- Power users experimenting with audio pipelines
- People interested in PC routing media (VoiceMeeter, Discord, OBS, Voicemod, DAWs ,Virtual Audio Cable, etc.)

## Features

- Can stream to/from multiple hardware audio inputs/outputs simultaneously, represented as i/o nodes
- Routing is done via a visual node-based graph system, not limited by menus or buttons
- Audio effects as nodes for processing within the graph (VST support coming soon, hopefully)
- Click or drag & drop a node from the toolbar to add it to the graph, and connect pins accordingly (hint, hit F on the graph to zoom to your nodes)

![Interface Demo](screenshots/screenshot7%20big%20gui%20improvement.png)

## Current State

- Very barebones DSP blocks and toolbar (only 4 effects so far)
- Not a lot of metadata so Windows will probably still flag the app
- Samplerate conversion is not dynamic yet, so de-sync and crackling will probably creep in over time. This is a big feature and it will take time to flush out properly.

Other WIP features which are planned be implemented in the future

- Saving and loading presets 
- More utility fx: Graphic EQ, Dynamics, Multiband, Channel strips, etc..
- Soundboard - file input
- Recorder - file output
- Multichannel device support (3+ channels per device), currently it forces 2 channels per device
- VST node support, bring your own plugins!



