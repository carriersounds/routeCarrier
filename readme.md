# routeCarrier: A Modular Hardware Audio Router for Windows (WIP)

This is a work-in-progress modular audio routing tool.
This program is built around a node editor where audio devices can be connected by a graph structure.

![Interface Demo](screenshots/screenshot7%20big%20gui%20improvement.png)

## Who is it for?

- DJs, streamers, audio engineers
- Power users experimenting with audio pipelines
- People interested in PC routing media (VoiceMeeter, Discord, OBS, Voicemod, DAWs ,Virtual Audio Cable, etc.)

## Platform

- Runs on Windows
- Based on DirectX11 / Win32 implementation of ImGui
- No external dependencies needed to run
- Currently only supports WASAPI driver

## Features

- Can stream to/from multiple hardware audio inputs/outputs simultaneously, represented as i/o nodes
- Routing is done via a visual node-based graph system, not limited by menus or buttons
- Audio effects as nodes for processing within the graph (VST support coming soon, hopefully)
- Click or drag & drop a node from the toolbar to add it to the graph, and connect pins accordingly (hint, hit F on the graph to zoom to your nodes)

## Build & Dependencies
- Compiled using Visual Studio 2022
- The VS project expects the library modules for the [JUCE API](https://github.com/juce-framework/JUCE) to be located at `..\..\..\Libraries\JUCE\modules`. Change this to where ever you have copied the repo to.
- Apart from that, no other change should be needed within Visual Studio. Build from there
- Source files for [ImGui](https://github.com/ocornut/imgui), [ImPlot](https://github.com/epezent/implot), [ImGui Node Editor](https://github.com/thedmd/imgui-node-editor) and [ImGui Knobs](https://github.com/altschuler/imgui-knobs) are all integrated in the `source/imgui` directory and referenced locally.


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



