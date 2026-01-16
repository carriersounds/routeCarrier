// MAIN INCLUDE HEADER FOR LIBRARIES AND GLOBAL DEFINES
#ifndef VISION_OBJECTS
#define VISION_OBJECTS
#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS



#include <string>
#include <sstream>
#include <d3d11.h>
#include <mutex>
#include <atomic>
#include <chrono>
#include <excpt.h>
#include <ShObjIdl.h>
#include <deque>
#include <queue>
#include <cstdint>
#include <vector>
#include <cstring>
#include <stdio.h>
#include <iostream>
#include <thread>
#include <future>
#include <filesystem>
#include <fstream>
#include <shellapi.h>
#include <format>
#include <variant>
#include <map>
#include <memory>

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include "implot\implot.h"
#include "implot\implot_internal.h"
#include "imgui_node_editor.h"
#include "imgui-knobs.h"


namespace ed = ax::NodeEditor;
namespace node = ax::NodeEditor;

#define BLOCKSIZE 512
#define FIFOSIZE 2048

#pragma comment(lib, "Ws2_32.lib")

using std::vector;
using std::string;
using std::memcpy;
using std::to_string;
using std::thread;
using std::chrono::high_resolution_clock;
using namespace std::literals::chrono_literals;
using std::atomic;
namespace fs = std::filesystem;
typedef vector<BYTE> bytearray;
typedef vector<uint16_t> int16array;
typedef std::chrono::high_resolution_clock::time_point timepoint;

typedef size_t NodeID;
typedef size_t PinID;
typedef size_t LinkID;
typedef size_t BaseID;
typedef size_t ParamID;

using std::unique_ptr;
using std::make_unique;



enum class ParamType {
    Integer,
    Float,
    Bool
};

enum class EffectType {
    None,
    Filter,
    Phaser,
    Gain,
    Reverb,
    EQ
};

struct BlockLink {
    BlockLink(node::LinkId id , node::PinId left, node::PinId right):ID(id), ID_left(left), ID_right(right){}
    node::LinkId ID;
    node::PinId ID_left;
    node::PinId ID_right;
    // maybe name, color or volume level or something

};

enum class pinType {
    null,
    input,
    output
};

enum class Identifier {
    node = 0,
    link = 1,
    pin  = 2
};

enum class BlockType {
    NullDevice,
    InputDevice,
    OutputDevice,
    DSP,
    FileInput
};

enum class blockModifier {
    addNew,
    swapFor,
    remove
};

enum autoSaveModifiers {
    save_all = -1,
    save_defects = 1,
    save_disabled = 0
};

enum class UI {
    calibration_module,
    fullscreen_admin,
    hide,
    unhide,
    unlock,
    lock
};

enum viewMode {
    empty,
    normal,
    binarized,
    equalized,
    enhanced,
    edges
};

enum FileIOFlags {

    f_openDialog = 1,
    f_noDialog = 0,

    f_openFile = 0,
    f_openFolder = 1,

    f_current = 0,
    f_previous = 1,
    f_next = 2,

    f_image = 7,
    f_settings = 8
};

struct iconData {
    string name;
    ImU64 pixbuf;
    ImVec2 size;
};

struct Counter {

    timepoint startPoint;

    Counter() {
        startTimer();
    }

    Counter(timepoint& start) {
        startPoint = start;
    }

    void startTimer() {
        startPoint = std::chrono::high_resolution_clock::now();
    }

    size_t getDurationLoop() {
        timepoint checkNow = std::chrono::high_resolution_clock::now();
        size_t timeChunk = getTimeDiff_us(startPoint, checkNow);
        startPoint = checkNow;
        return (llabs(timeChunk));

    }

    size_t getDuration() {
        timepoint checkNow = std::chrono::high_resolution_clock::now();
        size_t timeChunk = getTimeDiff_us(startPoint, checkNow);
        return (llabs(timeChunk));

    }


    string getDurationLoopString() {
        timepoint checkNow = std::chrono::high_resolution_clock::now();
        float timeChunk = getTimeDiff_us(startPoint, checkNow);
        startPoint = checkNow;
        return (" " + to_string((int)abs(timeChunk)) + " us");

    }

    string getDurationString() {
        timepoint checkNow = std::chrono::high_resolution_clock::now();
        size_t timeChunk = getTimeDiff_us(startPoint, checkNow);
        return (" " + to_string(llabs(timeChunk)) + " us");

    }

    size_t getTimeDiff_us(timepoint start, timepoint stop) {
        return std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count();
    }

    size_t getTimeDiff_ms(timepoint start, timepoint stop) {
        return std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
    }
};

#endif