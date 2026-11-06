#ifndef MAIN_HEADER
#define MAIN_HEADER     // MAIN INCLUDE HEADER FOR LIBRARIES AND GLOBAL DEFINES
                        //#pragma comment(lib, "Ws2_32.lib")
#include <string>
#include <sstream>
#include <d3d11.h>
#include <mutex>
#include <atomic>
#include <chrono>
#include <excpt.h>
#include <ShObjIdl.h>   // file dialogs
#include <shlobj.h>
#include <queue>
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
#include <map>
#include <memory>
#include <Psapi.h>
#include <comutil.h>

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"                      // modified for extra imvec2 math operators
#include "imgui_internal.h"
#include "imgui_impl_dx11.h"            
#include "imgui_impl_win32.h"
#include "implot\implot.h"              // modified to add extra piechart to dial function
#include "implot\implot_internal.h"
#include "imgui\node-editor\imgui_node_editor.h"
#include "imgui-knobs.h"                                                // modified to accept ID## without printing numbers
#include "external\libsamplerate-0.2.2-win64\include\samplerate.h"

#define JUCE_ASIO
#include <JuceHeader.h>
namespace ed = ax::NodeEditor;
namespace node = ax::NodeEditor;

#define BLOCKSIZE 512
#define FIFOSIZE 2048

#define INDENT_NEXT ImGui::Dummy({ 10,10 }); ImGui::SameLine();
#define ifDoubleClicked if (ImGui::IsItemActive() && ImGui::IsMouseDoubleClicked(0))
#define ADD_ID(x) ((x + to_string(ID)).c_str())
#define toggle(x) x = !x

#define tRight ImGui::TableNextColumn()
#define tDown ImGui::TableNextRow()
#define min(x,y) (x < y ? x : y);

namespace fs = std::filesystem;
using namespace std::literals::chrono_literals;

using std::vector;
using std::string;
using std::wstring;
using std::memcpy;
using std::to_string;
using std::thread;
using std::chrono::high_resolution_clock;
using std::atomic;
using std::unique_ptr;
using std::make_unique;


using timepoint = std::chrono::high_resolution_clock::time_point ;
using LinkID = size_t;
using BaseID = size_t;
using ParamID = size_t;
using NodeID = size_t;
using PinID = size_t;

enum class BlockType {
    NullDevice,
    InputDevice,
    OutputDevice,
    FileInput,
    FileOutput,
    Filter,
    Gain,
    Reverb,
    EQ,
    Saturator,
    ChannelUtil,
    Compressor,
    Phaser
};

enum ParamType {
    Float,
    Int,
    Bool,
    String
};


namespace tools {

    inline std::string WideToStdString(const wstring& w) {
        return { w.begin(),w.end() };
    }

    inline wstring StringToWide(const std::string& s) {
        return { s.begin(),s.end() };
    }

    inline HWND FindTopLevelWindowByProcessName(const std::string& targetProcessName) {
        HWND hwnd = GetTopWindow(NULL);  // Start with the topmost window

        while (hwnd) {
            // Get the process ID for this window
            DWORD processId;
            GetWindowThreadProcessId(hwnd, &processId);

            // Get the process name
            HANDLE processHandle = OpenProcess(
                PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
                FALSE,
                processId
            );

            if (processHandle) {
                char filename[MAX_PATH];
                if (GetModuleFileNameExA(processHandle, NULL, filename, MAX_PATH)) {
                    // Extract just the filename
                    std::string processName(filename);
                    size_t lastSlash = processName.find_last_of("\\");
                    if (lastSlash != std::string::npos) {
                        processName = processName.substr(lastSlash + 1);
                    }
                    // Check for exact match
                    if (processName == targetProcessName) {
                        CloseHandle(processHandle);
                        return hwnd;  // Found it!
                    }
                }
                CloseHandle(processHandle);
            }

            // Move to next window in Z-order
            hwnd = GetNextWindow(hwnd, GW_HWNDNEXT);
        }

        return NULL;  // Not found
    }

    inline string to_string_float(float val, int afterComma = 2) {

        switch (afterComma) {
        case 0: return std::format("{:.0f}", val); break;
        case 1: return std::format("{:.1f}", val); break;
        case 2: return std::format("{:.2f}", val); break;
        case 3: return std::format("{:.3f}", val); break;
        case 4: return std::format("{:.4f}", val); break;
        case 5: return std::format("{:.5f}", val); break;
        case 6: return std::format("{:.6f}", val); break;
        case 7: return std::format("{:.7f}", val); break;
        default: return std::format("{:.2f}", val); break;
        }
    };


    inline std::string toString(ParamType t) {
        switch (t) {
        case ParamType::Float:  return "float";
        case ParamType::Int:    return "int";
        case ParamType::Bool:   return "bool";
        case ParamType::String: return "string";
        }
        return {};
    }

    inline ParamType fromString(const string& s) {
        if (s == "float")  return ParamType::Float;
        if (s == "int")    return ParamType::Int;
        if (s == "bool")   return ParamType::Bool;

        return ParamType::String;
    }


    inline void ImShiftCursor(float x, float y) {
        ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(x, y));
    }

    // returns 1 on both toggles
    inline bool toggleButton(const char* label, bool& var, const ImVec2& size = ImVec2(0, 0)) {

        bool clicked = false;

        if (var) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.75f, 0.95f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.15f, 0.75f, 0.95f, 1.0f));
        }

        if (ImGui::Button(label, size)) {
            clicked = true;
            var = !var;
            if (!var) ImGui::PopStyleColor(2);
        }
        else {
            if (var) ImGui::PopStyleColor(2);
        }

        return clicked;
    }

    inline void ImDragDropMacro(BlockType& toSet, BlockType type) {
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))        // triggers continuously when grabbing
        {
            toSet = type;
            ImGui::SetDragDropPayload("DND_DEMO_CELL", &toSet, sizeof(BlockType)); // Set payload to carry the index of our item (could be anything)
            ImGui::EndDragDropSource();
        }
    }

    template <typename T>
    void RemoveObjectFromVector(std::vector<T>& vec, const T& value)
    {
        vec.erase(std::remove(vec.begin(), vec.end(), value), vec.end());
    }
    
    template <typename T>
    void RemoveIndexFromVector(std::vector<T>& vec, int idx)
    {
        if (idx < vec.size())
            vec.erase(vec.begin() + idx);
    }
     
    inline float decibelsToGain(const float decibels)
    {
        const float minusInfinityDb = -100.0f;
        return decibels > minusInfinityDb ? std::pow(10.0f, decibels * 0.05f): 0;
    }
   
    inline float gainToDecibels(const float gain)
    {
        const float minusInfinityDb = -100.0f;
        if (gain > 0.0f) {
            return std::log10(gain) * 20.0f;
        }
        else {
            return minusInfinityDb;
        }
    }

    inline void drawGainMonitorHoriz(juce::AudioBuffer<float>& buffer, float drawWidth, NodeID ID) {

        if (drawWidth < 10) return;    // might glitch on the 1st frame

        float rmsDB = tools::gainToDecibels(buffer.getRMSLevel(0, 0, BLOCKSIZE));
        float peakDB = tools::gainToDecibels(buffer.getMagnitude(0, 0, BLOCKSIZE));
        float lowerLimit = 60;

        if (rmsDB > 12 || peakDB > 12) {
            rmsDB = -lowerLimit; peakDB = -lowerLimit;
        }

        ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, { 10,0 });
        ImGui::PushStyleColor(ImGuiCol_FrameBg, { 0,0,0,0 });

        static int mapID = 0;

        string lvlID = "lv" + to_string(ID);

        if (ImPlot::BeginPlot(lvlID.c_str(), ImVec2(drawWidth, 60), ImPlotFlags_NoLegend | ImPlotFlags_NoTitle)) {

            // Calculate the height of each segment (NOT POSITION!). position is calculated from 0 i suppose
            float dataBr[3];
            dataBr[0] = peakDB;
            dataBr[1] = rmsDB - peakDB;
            dataBr[2] = -(lowerLimit + rmsDB);

            const char* labels[] = { "Min", "med1", "med2"};
           // ImPlot::SetupAxisLimits(ImAxis_Y1, -1, 1, ImPlotCond_Always);
            ImPlot::SetupAxisLimits(ImAxis_X1, -lowerLimit, 0);
            ImPlot::SetupAxis(ImAxis_Y1, nullptr, ImPlotAxisFlags_NoLabel | ImPlotAxisFlags_NoGridLines | ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_NoTickMarks);


            ImPlot::PushColormap("gain");
            ImPlot::PlotBarGroups(labels, dataBr, 3, 1, 1.0f, 2.0f, ImPlotBarGroupsFlags_Stacked | ImPlotBarGroupsFlags_Horizontal);
            ImPlot::PopColormap();

            ImPlot::EndPlot();
        }

        ImGui::PopStyleColor();
        ImPlot::PopStyleVar();
    }

    inline void drawGainMonitorVertic(juce::AudioBuffer<float>& buffer, float drawLength, NodeID xtraID, float thick = 60, int channel = 0) {

        if (drawLength < 10) return;    // might glitch on the 1st frame

        float rmsDB = tools::gainToDecibels(buffer.getRMSLevel(channel, 0, BLOCKSIZE));
        float peakDB = tools::gainToDecibels(buffer.getMagnitude(channel, 0, BLOCKSIZE));
        float lowerLimit = 60;

        if (rmsDB > 12 || peakDB > 12) {
            rmsDB = -lowerLimit; peakDB = -lowerLimit;
        }

        ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, { 0,10 });
        ImGui::PushStyleColor(ImGuiCol_FrameBg, { 0,0,0,0 });

        string lvlID = "lv" + to_string(xtraID);

        if (ImPlot::BeginPlot(lvlID.c_str(), ImVec2(thick, drawLength), ImPlotFlags_NoLegend | ImPlotFlags_NoTitle)) {

            // Calculate the height of each segment (NOT POSITION!). position is calculated from 0 i suppose
            float dataBr[3] = {peakDB,rmsDB - peakDB, -(lowerLimit + rmsDB) };

            const char* labels[] = { "Min", "med1", "med2" };
            ImPlot::SetupAxisLimits(ImAxis_Y1, -lowerLimit, 0);
            ImPlot::SetupAxis(ImAxis_X1, nullptr, ImPlotAxisFlags_NoLabel | ImPlotAxisFlags_NoGridLines | ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_NoTickMarks);

            ImPlot::PushColormap("gain");
            ImPlot::PlotBarGroups(labels, dataBr, 3, 1, 1.0f, 2.0f, ImPlotBarGroupsFlags_Stacked);
            ImPlot::PopColormap();

            ImPlot::EndPlot();
        }

        ImGui::PopStyleColor();
        ImPlot::PopStyleVar();
    }

    inline void drawGainMonitorVertic_Stereo(juce::AudioBuffer<float>& buffer, float drawLength, NodeID ID, float thick = 30, bool isStereo = true) {

        if (drawLength < 10) return;    // might glitch on the 1st frame
        float lowerLimit = 60;
        

        float rmsDBL = tools::gainToDecibels(buffer.getRMSLevel(0, 0, BLOCKSIZE));
        float peakDBL = tools::gainToDecibels(buffer.getMagnitude(0, 0, BLOCKSIZE));

        float rmsDBR = tools::gainToDecibels(buffer.getRMSLevel(isStereo, 0, BLOCKSIZE));
        float peakDBR = tools::gainToDecibels(buffer.getMagnitude(isStereo, 0, BLOCKSIZE));


        if (rmsDBL > 12 || peakDBL > 12) {
            rmsDBL = -lowerLimit; peakDBL = -lowerLimit;
        }

        if (rmsDBR > 12 || peakDBR > 12) {
            rmsDBR = -lowerLimit; peakDBR = -lowerLimit;
        }

        ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, { 0,10 });
        ImGui::PushStyleColor(ImGuiCol_FrameBg, { 0,0,0,0 });

        if (ImPlot::BeginPlot(ADD_ID("lv"), ImVec2(2 * thick, drawLength), ImPlotFlags_NoLegend | ImPlotFlags_NoTitle)) {

            // Calculate the height of each segment (NOT POSITION!). position is calculated from 0 i suppose
            float dataL[3] = { peakDBL,rmsDBL - peakDBL, -(lowerLimit + rmsDBL) };
            float dataR[3] = { peakDBR,rmsDBR - peakDBR, -(lowerLimit + rmsDBR) };

            const char* labels[] = { "Min", "med1", "med2" };
            ImPlot::SetupAxisLimits(ImAxis_Y1, -lowerLimit, 0);
            ImPlot::SetupAxis(ImAxis_X1, nullptr, ImPlotAxisFlags_NoLabel | ImPlotAxisFlags_NoGridLines | ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_NoTickMarks);

            ImPlot::PushColormap("gain");
            ImPlot::PlotBarGroups(labels, dataL, 3, 1, 0.5f, 0.0f, ImPlotBarGroupsFlags_Stacked);
            ImPlot::PlotBarGroups(labels, dataR, 3, 1, 0.5f, 0.7f, ImPlotBarGroupsFlags_Stacked);
            ImPlot::PopColormap();

            ImPlot::EndPlot();
        }

        ImGui::PopStyleColor();
        ImPlot::PopStyleVar();
    }

    inline bool drawInvertedFloatSlider(const char* name, float* param, NodeID ID, float min, float max, const char* format = "%.2f", ImGuiSliderFlags flags = 0) {

        ImGui::Text(name);
        ImGui::SameLine(60);
        if (ImGui::SliderFloat(string("##" + (string)name + to_string(ID)).c_str(), param, min, max, format, flags))
            return true;
        else
            return false;

        // "paramChanged = true" is not implemented here, as we might want to do some calculations or interpretation
        // on the parameters first before flagging the parameters as changed to the processor
    }

}


struct BlockLink {
    BlockLink(node::LinkId id , node::PinId left, node::PinId right):ID(id), ID_left(left), ID_right(right){}
    node::LinkId ID;
    node::PinId ID_left;
    node::PinId ID_right;

    // maybe name, color or volume level or something
    string ToXML() {
        string builder;
        builder += "    <LINK ID=\"" + to_string(ID.Get()) + "\" Left=\"" + to_string(ID_left.Get()) + "\" Right=\"" + to_string(ID_right.Get()) + "\"/>\n";
        return builder;
    }
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


enum class blockModifier {
    addNew,
    swapFor,
    remove
};

enum class UI {
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

    f_audio = 7,
    f_preset = 8
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
        return (" " + to_string((size_t)abs(timeChunk)) + " us");

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

// UNUSED GUI TOOLS from old project
#if 0


// Small histogram

if (advancedView)
if (ImPlot::BeginPlot("Histogramme", ImVec2(-1, 180), ImPlotFlags_NoTitle | ImPlotFlags_NoLegend)) {

    float maxPeak = 0;
    float maxPeakIndex = 0;

    for (size_t i = 0; i < 256; i++)
    {
        histogramContent[i] = (float)guiBuffer.histogram[i];

        if (maxPeak < histogramContent[i]) {
            maxPeak = histogramContent[i];
            maxPeakIndex = i;
        }
    }
    ImPlot::SetupAxes(nullptr, nullptr, 0, ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_NoTickLabels);
    ImPlot::SetupAxisScale(ImAxis_Y1, ImPlotScale_Log10);
    ImPlot::SetupLegend(ImPlotLocation_East | ImPlotLocation_North, 0);
    ImPlot::PlotBars("Pixel Histogram", histogramContent.data(), 255);
    ImPlot::PlotInfLines("Effective Threshold", &prog->calibrationRegister[caliParamIndex::threshold].value, 1);
    ImPlot::EndPlot();
}


// BRIGHTNESS RANGE

// Variance and deviation 
            // Calculate the height of each segment
if (ImPlot::BeginPlot("Data Variance", ImVec2(-1, -1), ImPlotFlags_NoLegend | ImPlotFlags_NoTitle)) {

    const char* labels[] = { "Min", "Range", "Max" };
    static float dataBr[3];
    dataBr[0] = minBright;
    dataBr[1] = maxBright - minBright;
    dataBr[2] = 255 - maxBright;
    ImPlot::SetupAxis(ImAxis_X1, nullptr, ImPlotAxisFlags_NoLabel | ImPlotAxisFlags_NoTickLabels);
    ImPlot::SetupAxisLimits(ImAxis_X1, 1, 3);
    ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 300);

    // Brightness variance
    ImPlot::PushColormap("ranges");
    ImPlot::PlotBarGroups(labels, dataBr, 3, 1, 1.0f, 2.0f, ImPlotBarGroupsFlags_Stacked);
    ImPlot::PopColormap();
    ImPlot::PlotScatter("a", &two, &prog->calibrationRegister[caliParamIndex::brightness].value, 1);              // brightPoint 
    string currentBrighttxt = std::format("{:.2f}", prog->calibrationRegister[caliParamIndex::brightness].value);
    string brightVarianceText = (string)"Variance\n = " + std::format("{:2.2f}", prog->calibrationRegister[caliParamIndex::br_variance].value);
    ImPlot::PlotText("Brightness", two, 290.0f);
    ImPlot::PlotText(currentBrighttxt.data(), two, std::clamp(prog->calibrationRegister[caliParamIndex::brightness].value, 0.0f, 150.0f) + 5.0f);  // brightText
    ImPlot::PlotText(brightVarianceText.data(), two, 265.0f, { 0,0 });

    ImPlot::EndPlot();
}

SHADED HISTOGRAM
if (calibrationMode) {

    static float y_data[256], x_data[256];
    for (size_t i = 0; i < 256; i++)
    {
        x_data[i] = (float)i;
        y_data[i] = (i > 15 && i < 50) ? maxPeak : 0;
    }

    if (maxPeakIndex > 15 && maxPeakIndex < 50) {
        ImPlot::PushStyleColor(ImPlotCol_Fill, IM_COL32(20, 200, 20, 100));
        ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.8f);
    }
    else {
        ImPlot::PushStyleColor(ImPlotCol_Fill, IM_COL32(200, 20, 20, 60));
        ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.3f);
    }
    ImPlot::PlotShaded("Highlight", x_data, y_data, 256, 0);
    ImPlot::PopStyleColor(1);
    ImPlot::PopStyleVar();
}



// OLD FOCUS BAR
static float maxFocus = 1.0f;
maxFocus = max(guiImagestats.focus, maxFocus);
double edge = 1.5f * (double)maxFocus;
if (!holdValues && EVERY_5_SECONDS) maxFocus = 0;

if (ImPlot::BeginPlot("e", ImVec2(-1, 250), ImPlotFlags_NoTitle)) {

    ImPlot::SetupLegend(ImPlotLocation_North, ImPlotLegendFlags_Outside | ImPlotLegendFlags_Horizontal);
    ImPlot::SetupAxes(nullptr, nullptr, 0 | ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_NoTickMarks | ImPlotAxisFlags_NoGridLines);
    ImPlot::SetupAxisLimits(ImAxis_X1, -1, 1);
    ImPlot::SetupAxisLimits(ImAxis_Y1, 0, edge, ImPlotCond_Always);


    // GOOD FOCUS THING
    ImPlot::SetNextFillStyle(ImVec4(1, 1, 1, 0.2), 1);
    ImPlot::SetNextLineStyle(ImVec4(1, 1, 1, 0.5), 1);

    if (localStatCopy.focus > 4.8f) {
        ImPlot::SetNextFillStyle(ImVec4(0.80, 1, 1, 0.7), 1);
        ImPlot::SetNextLineStyle(ImVec4(0.40, 0.4, 1, 0.8), 4);
    }

    ImPlot::PlotBars("Focus Quality", &zero, &localStatCopy.focus, 1, 1);
    string focusTxt = std::format("{:.2f}", localStatCopy.focus);
    ImPlot::PlotText(focusTxt.data(), zero, localStatCopy.focus + 0.5f);

    // target focus
    ImPlot::SetNextLineStyle(ImVec4(0.40, 1, 0.4, 1), 2);
    ImPlot::PlotInfLines("Target", &targetFocus, 1, ImPlotInfLinesFlags_Horizontal);


    //  ImPlot::PlotInfLines("Max focus", &maxFocus, 1, ImPlotInfLinesFlags_Horizontal);
    ImPlot::EndPlot();




}


// HISTOGRAM COPY

// Histogram copy
if (ImPlot::BeginPlot("Histogramme", ImVec2(-1, -1), ImPlotFlags_NoTitle)) {

    float maxPeak = 0;
    float maxPeakIndex = 0;

    for (size_t i = 0; i < 256; i++)
    {
        histogramContent[i] = (float)guiBuffer.histogram[i];

        if (maxPeak < histogramContent[i]) {
            maxPeak = histogramContent[i];
            maxPeakIndex = i;
        }
    }


    ImPlot::SetupAxes(nullptr, nullptr, 0, ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_NoTickLabels);
    ImPlot::SetupAxisScale(ImAxis_Y1, ImPlotScale_Log10);
    ImPlot::SetupLegend(ImPlotLocation_East | ImPlotLocation_North, 0);
    ImPlot::PlotBars("Pixel Histogram", histogramContent.data(), 255);
    ImPlot::PlotInfLines("Effective Threshold", &tre, 1);
    //ImPlot::PlotInfLines("Average brightness", &guiImagestats.pixelAverage, 1);

    ImPlot::EndPlot();
}

tRight;



// SPECTROGRAM

// Histogram History
if (ImPlot::BeginPlot("Histogram history", ImVec2(-1, 275), ImPlotFlags_NoLegend | ImPlotFlags_NoMouseText)) {

    static HistBuffer histMap;
    histMap.addHist(histogramContent);
    ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_NoLabel | ImPlotAxisFlags_NoTickLabels, ImPlotAxisFlags_NoLabel | ImPlotAxisFlags_NoTickLabels);
    ImPlot::SetupAxisLimits(ImAxis_Y1, 0, HIST_DEPTH, ImGuiCond_Always);
    ImPlot::SetupAxisLimits(ImAxis_X1, 0, HIST_WIDTH, ImGuiCond_Always);

    // ( Jet, Plasma, Viridis)
    ImPlot::PushColormap(ImPlotColormap_Plasma);

    ImPlot::PlotHeatmap(
        "Histogram-gram",
        histMap.flatData.data(),       // Pointer to the flattened data
        HIST_DEPTH,                    // Number of rows (time slices currently in buffer)
        HIST_WIDTH,                    // Number of columns (256 bins per slice)
        0, 5.5f,                       // color range
        nullptr,                       // Value format string (e.g., "%.2f")
        ImPlotPoint(0, 0),         // Bottom-left corner of the heatmap
        ImPlotPoint(HIST_WIDTH - 1, HIST_DEPTH) // Top-right corner of the heatmap
    );

    ImPlot::PopColormap(); // Pop the colormap after plotting
    ImPlot::EndPlot();
}
#endif


#endif