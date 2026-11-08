#ifndef AUDIO_FILE_NODE
#define AUDIO_FILE_NODE
#include "mainHeader.hpp"
#include <JuceHeader.h>
#include "AudioNodes.h"

class FileIO;

/*


TODO: 
fix general sizing of gui elements and node size
fix samplerate conversion
check file playback hiccup on gui button interaction
check crash on recording (?)
remove fileio pointer sharing, just use the static :: for function calls

soundboard feature check: 
    presets?
    add file browse slider similar to normal file thing
    set startpoint & length as parameters which can be assigned in float(?)
    Make interface larger, fix text
    fix dropdown menu for play mode
    remove load/clear button. replace with + on empty slot



AudioFileNode - file player / recorder node.

    Like DeviceNode, the direction is decided by the BlockType it is created with:

      BlockType::FileInput   -> PLAYER    : reads an audio file, has an OUTPUT pin
      BlockType::FileOutput  -> RECORDER  : writes incoming audio to a file, has an INPUT pin
                                            (also passes the audio through to outputBuffer for metering)

    Threading:
      - prepareOutput() runs on the engine thread and NEVER touches the disk.
      - Playback uses juce::AudioTransportSource with a read-ahead buffer filled by diskThread
        (also does the file-rate -> engine-rate resampling).
      - Recording uses juce::AudioFormatWriter::ThreadedWriter, which copies into a FIFO and
        lets diskThread do the actual writing.
      - All GUI actions (load / play / seek / record) are called from the GUI thread.

    Bypass:  player   = muted (playhead keeps running)
             recorder = recording paused (audio still passes through)
*/
class AudioFileNode final : public AudioNode
{
public:
    AudioFileNode(BlockType blocktype, juce::String initName, NodeID nodeID);
    ~AudioFileNode() override;

    // AudioNode
    void renderAsNode(float pinSize, float spacing) override;
    void prepareOutput() override;
    void onPresetLoaded() override;     // requires: virtual void onPresetLoaded() {} in AudioNode

    // Wiring (engine calls this after creating the node, for the file dialogs)
    void setFileIO(FileIO* io) { fileIO = io; }

    // ---- Player control (GUI thread) ----
    bool loadFile(const juce::File& file);
    void unloadFile();
    void play();
    void pause();
    void stopAndRewind();
    bool hasFile() const { return readerSource != nullptr; }
    bool isPlaying() const { return transport.isPlaying(); }

    // ---- Recorder control (GUI thread) ----
    bool startRecording(const juce::File& file);
    void stopRecording();
    bool isRecording() const { return recording.load(); }

    // ---- Settings (also exposed to presets) ----
    float gainDB  = 0.0f;
    bool  looping = false;

private:
    void renderPlayerInterface();
    void renderRecorderInterface();
    void pollPlaybackState();
    void runPendingDialogs();

    FileIO* fileIO = nullptr;

    // declared first -> destroyed last (writer / transport must be gone before the thread stops)
    juce::TimeSliceThread diskThread;
    juce::AudioFormatManager formatManager;

    // ---- Player ----
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    juce::AudioTransportSource transport;
    juce::File currentFile;
    std::string filePath;               // preset-linked mirror of currentFile (UTF-8 full path)
    double fileLengthSec = 0.0;
    std::atomic<int> fileChannels{ 2 };
    float seekPos = 0.0f;
    bool  seeking = false;

    // ---- Recorder ----
    juce::SpinLock writerLock;
    std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> writer;
    std::atomic<bool> recording{ false };
    std::atomic<juce::int64> samplesRecorded{ 0 };
    std::atomic<int> overruns{ 0 };
    juce::File recordFile;
    int bitDepthIdx = 1;    // 0 = 16, 1 = 24, 2 = 32 (float for wav)

    // dialogs must not open in the middle of the node draw, so we defer them
    bool pendingLoadDialog = false;
    bool pendingRecordDialog = false;
};

#endif
