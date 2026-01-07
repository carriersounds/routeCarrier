# Modular Audio Router (WIP)

---

This is an early **work-in-progress** modular audio routing tool.

It currently supports **hardware device → hardware device** audio routing only.  
There is **no DSP**, no effects, and no processing beyond basic passthrough.

The UI is built around an **ImGui node editor**, where audio input and output devices appear as nodes that can be visually connected.

Multiple inputs can be connected to the same output — these are **automatically mixed** at the output stage.

---

## Current State

- Hardware input ↔ hardware output routing
- Visual patching using ImGui node editor
- Multiple-in / multiple-out routing
- Automatic mixing on outputs

---

## Limitations

- No DSP nodes
- No accurate sample rate conversion
- No preset saving/loading
- Very much a minimum working version

---

This project is under active development and should be considered experimental.