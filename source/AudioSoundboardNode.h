#ifndef AUDIO_SOUNDBOARD_NODE
#define AUDIO_SOUNDBOARD_NODE
#include "mainHeader.hpp"
#include <JuceHeader.h>
#include "AudioNodes.h"
#include <array>

class FileIO;

/*
    AudioSoundboardNode (BlockType::Soundboard) - a grid of pads, each holding one audio file.
    Source node: it only has an OUTPUT pin, all pads are mixed into it.

    Pad modes:
      One-shot : press = (re)start from the beginning, plays to the end
      Loop     : press = toggle loop on / off
      Hold     : plays while the pad is held down

    Same threading model as AudioFileNode: the engine thread never touches the disk,
    every pad has its own AudioTransportSource (read-ahead on the shared diskThread).

    Presets: master gain + per pad  PadN_File (string), PadN_Gain, PadN_Mode.
    After the preset parser has filled the strings, the engine must call onPresetLoaded()
    so the files actually get loaded.
*/
class AudioSoundboardNode final : public AudioNode
{
public:
    static constexpr int kNumPads = 8;
    static constexpr int kColumns = 4;
    enum PadMode { OneShot = 0, Loop = 1, Hold = 2 };

    AudioSoundboardNode(BlockType blocktype, juce::String initName, NodeID nodeID);
    ~AudioSoundboardNode() override;

    // AudioNode
    void renderAsNode(float pinSize, float spacing) override;
    void prepareOutput() override;
    void onPresetLoaded() override;            

    void setFileIO(FileIO* io) { fileIO = io; }

    // ---- control (GUI thread) ----
    bool loadPad(int index, const juce::File& file);
    void clearPad(int index);
    void triggerPad(int index);                 // on mouse-down
    void releasePad(int index);                 // on mouse-up (only does something in Hold mode)
    void stopAll();

    float masterGainDB = 0.0f;

private:
    struct Pad {
        // ---- preset-linked (NodePreset holds raw pointers to these, so this object must never move) ----
        std::string path;
        float gainDB = 0.0f;
        int   mode = OneShot;

        // ---- runtime ----
        std::string displayName;
        bool   missing = false;                 // path from a preset that could not be found
        double lengthSec = 0.0;
        std::atomic<bool> loaded{ false };
        std::atomic<int>  channels{ 2 };
        std::unique_ptr<juce::AudioFormatReaderSource> reader;
        juce::AudioTransportSource transport;
    };

    void detachPad(Pad& pad);
    void pollPlayback();
    void renderPad(int index, const ImVec2& size);
    void renderPadEditor();
    void runPendingDialogs();

    FileIO* fileIO = nullptr;

    // declared before the pads -> destroyed after them
    juce::TimeSliceThread diskThread;
    juce::AudioFormatManager formatManager;

    std::array<Pad, kNumPads> pads;
    juce::AudioBuffer<float> padBuffer;         // scratch buffer, one pad at a time

    int selectedPad = 0;
    int pendingLoadPad = -1;
};

#endif
