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

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include "implot\implot.h"
#include "implot\implot_internal.h"
#include "imgui\node-editor\imgui_node_editor.h"
#include "imgui-knobs.h"
#include "external\libsamplerate-0.2.2-win64\include\samplerate.h"
#include <JuceHeader.h>

namespace ed = ax::NodeEditor;
namespace node = ax::NodeEditor;

#define BLOCKSIZE 512
#define FIFOSIZE 2048

#define INDENT_NEXT ImGui::Dummy({ 10,10 }); ImGui::SameLine();
#define ifDoubleClicked if (ImGui::IsItemActive() && ImGui::IsMouseDoubleClicked(0))
#define ADD_ID(x) ((x + to_string(ID)).c_str())


using std::vector;
using std::string;
using std::memcpy;
using std::to_string;
using std::thread;
using std::chrono::high_resolution_clock;
using namespace std::literals::chrono_literals;
using std::atomic;
namespace fs = std::filesystem;
typedef std::chrono::high_resolution_clock::time_point timepoint;

typedef size_t NodeID;
typedef size_t PinID;
typedef size_t LinkID;
typedef size_t BaseID;
typedef size_t ParamID;

using std::unique_ptr;
using std::make_unique;

namespace tools {

    inline void ImShiftCursor(float x, float y) {
        ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(x, y));
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

    inline void drawGainMonitorVertic(juce::AudioBuffer<float>& buffer, float drawLength, NodeID xtraID) {

        if (drawLength < 10) return;    // might glitch on the 1st frame

        float rmsDB = tools::gainToDecibels(buffer.getRMSLevel(0, 0, BLOCKSIZE));
        float peakDB = tools::gainToDecibels(buffer.getMagnitude(0, 0, BLOCKSIZE));
        float lowerLimit = 60;

        if (rmsDB > 12 || peakDB > 12) {
            rmsDB = -lowerLimit; peakDB = -lowerLimit;
        }

        ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, { 0,10 });
        ImGui::PushStyleColor(ImGuiCol_FrameBg, { 0,0,0,0 });

        string lvlID = "lv" + to_string(xtraID);

        if (ImPlot::BeginPlot(lvlID.c_str(), ImVec2(60, drawLength), ImPlotFlags_NoLegend | ImPlotFlags_NoTitle)) {

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

    // Custom GUI Elements - helper functions

    inline void drawGradientBackground(ImDrawList* drwList, const char* label, ImRect bb, bool button, bool pressed, bool hovered) {
        // Call directly after "begin" to set as background
        ImU32 colTop;
        ImU32 colBottom;

        // button
        const ImVec4 outlineColorBase = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
        ImU32 colOutline = ImGui::GetColorU32(outlineColorBase);
        float rounding = 4.0f;
        float thickness = 2.0f; // 2-pixel thickness for the border


        if (!button) {
            colTop = IM_COL32(4, 16, 7, 255);
            colBottom = IM_COL32(72, 76, 74, 255);
        }
        else {
            colTop = IM_COL32(72, 76, 74, 255);
            colBottom = IM_COL32(4, 16, 7, 255);
        }

        // hover and press handler
        if (button) {
            float pressFactor = 1.8f; // Slightly darker when pressed
            float hoverFactor = 1.2f; // Slightly brighter when hovered

            // adjust the outline color based on hover/press state if desired
            if (pressed) {
                // Example: Make the outline BRIGHT YELLOW when pressed
                colOutline = ImGui::GetColorU32(ImVec4(0.9f, 0.9f, 0.7f, 1.0f));
            }
            else if (hovered) {
                // Example: Make the outline YELLOW when hovered
                colOutline = ImGui::GetColorU32(ImVec4(0.8f, 0.8f, 0.0f, 1.0f));
            }
        }

        // 4. Draw Background
        drwList->AddRectFilledMultiColor(
            bb.Min,
            bb.Max,
            colBottom, // Color Top-Left (actually the bottom of the gradient)
            colBottom, // Color Top-Right
            colTop,    // Color Bottom-Right (actually the top of the gradient)
            colTop    // Color Bottom-Left
        );

        // 4.1 Draw Outline (if button)
        if (button) {
            drwList->AddRect(
                bb.Min,
                bb.Max,
                colOutline,
                rounding,
                ImDrawFlags_None, // Flags (can be used to specify sides)
                thickness
            );
        }

    }

    inline void drawAnalogDial_3sections(const char* name, float ranges[], float value, ImVec2 plotSize, bool reverseColors) {

        const char* labels1[] = { "Section 1","Section 2","Section 3" };
        static ImPlotColormap stopLight = -1;
        static ImPlotColormap blackBG;
        static float radius = 0.4f;
        static ImVec2 center = { 0.5f,0.5f };
        static float angle0 = -135;

        // Color map
        if (stopLight == -1) {
            ImU32 stopLight_data[3] = { IM_COL32(0, 255, 0, 255), IM_COL32(200, 200, 0, 255), IM_COL32(255, 0, 0, 255) };
            if (reverseColors) {
                ImU32 temp = stopLight_data[0];
                stopLight_data[0] = stopLight_data[2];
                stopLight_data[2] = temp;
            }
            stopLight = ImPlot::AddColormap("stoplight", stopLight_data, 3);

            ImU32 blegh[] = { IM_COL32(10, 10, 10, 255),IM_COL32(10, 10, 10, 255) };

            blackBG = ImPlot::AddColormap("black", blegh, 2);

        }

        if (ImPlot::BeginPlot(name, plotSize, ImPlotFlags_Equal | ImPlotFlags_CanvasOnly | ImPlotFlags_NoTitle | ImPlotFlags_NoMouseText | ImPlotFlags_NoLegend)) {
            ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_NoDecorations, ImPlotAxisFlags_NoDecorations);
            ImPlot::SetupAxesLimits(0, 1, 0, 1, ImPlotCond_Always);
            float plot_converted_angle0 = angle0 + 90;

            ImPlot::PushColormap("stoplight");
            ImPlot::PlotPieChartAsDial(labels1, ranges, 3, center.x, center.y, radius, "%.2f", plot_converted_angle0);
            ImPlot::PopColormap();

            ImPlot::PushColormap("black");
            const char* dumby[] = { "1","2" };
            static float dumval[] = { 0.5f,0.5f };
            ImPlot::PlotPieChart(dumby, dumval, 2, center.x, center.y, radius * 0.8f, " ", 0, ImPlotPieChartFlags_Normalize);
            ImPlot::PopColormap();

            float start_angle_radians = angle0 * 2.0f * IM_PI / 360.0f;

            // Text markers
            for (int i = 0; i < 4; ++i) {
                const float percent = ranges[i] / ranges[3]; // value over max   (float 0 - 1)
                float angle = start_angle_radians - (IM_PI * 1.5f * percent); // MINUS BECAUSE ITS INVERTED                   
                ImVec2 pos = { center.x + radius * 1.15f * cos(angle), center.y + radius * 1.15f * sin(angle) };   // polar to cartesian
                string plottertext = std::format("{:.0f}", ranges[i]);
                ImPlot::PlotText(plottertext.data(), pos.x, pos.y);
            }
            ImVec2 titlePos = { center.x,center.y - (radius * 0.5f) };
            ImPlot::PlotText(name, titlePos.x, titlePos.y);

            ImVec2 lineVectorPoint;
            float v_Percent = value / ranges[3];
            float v_Angle = start_angle_radians - (IM_PI * 1.5f * v_Percent);
            lineVectorPoint = { center.x + radius * cos(v_Angle), center.y + radius * sin(v_Angle) };
            float Xs[] = { center.x,lineVectorPoint.x };
            float Ys[] = { center.y,lineVectorPoint.y };
            ImPlot::SetNextLineStyle(ImVec4(255, 255, 255, 255), 4.0f);
            ImPlot::PlotLine("CV", Xs, Ys, 2, ImPlotLineFlags_Segments);
            ImPlot::EndPlot();
        }

    }

    inline void drawAnalogDial_5sections(const char* name, float ranges[], float value, ImVec2 plotSize) {

        const char* labels5[] = { "Section 1","Section 2","Section 3", "Section 4", "Section 5" };
        static ImPlotColormap stopLight5 = -1;
        static ImPlotColormap blackBG5;
        static float radius = 0.4f;
        static ImVec2 center = { 0.5f,0.5f };
        static float angle0 = -135;

        // Color map
        if (stopLight5 == -1) {
            ImU32 stopLight_data[5] = { IM_COL32(255, 0, 0, 255),IM_COL32(200, 200, 0, 255),IM_COL32(0, 255, 0, 255), IM_COL32(200, 200, 0, 255), IM_COL32(255, 0, 0, 255) };

            stopLight5 = ImPlot::AddColormap("stoplight5", stopLight_data, 5);

            ImU32 blegh[] = { IM_COL32(10, 10, 10, 255),IM_COL32(10, 10, 10, 255) };
            blackBG5 = ImPlot::AddColormap("black5", blegh, 2);

        }

        if (ImPlot::BeginPlot(name, plotSize, ImPlotFlags_Equal | ImPlotFlags_CanvasOnly | ImPlotFlags_NoTitle | ImPlotFlags_NoMouseText | ImPlotFlags_NoLegend)) {
            ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_NoDecorations, ImPlotAxisFlags_NoDecorations);
            ImPlot::SetupAxesLimits(0, 1, 0, 1, ImPlotCond_Always);
            float plot_converted_angle0 = angle0 + 90;

            ImPlot::PushColormap("stoplight5");
            ImPlot::PlotPieChartAsDial(labels5, ranges, 5, center.x, center.y, radius, "%.2f", plot_converted_angle0);
            ImPlot::PopColormap();

            ImPlot::PushColormap("black5");
            const char* dumby[] = { "1","2" };
            static float dumval[] = { 0.5f,0.5f };
            ImPlot::PlotPieChart(dumby, dumval, 2, center.x, center.y, radius * 0.8f, " ", 0, ImPlotPieChartFlags_Normalize);
            ImPlot::PopColormap();

            float start_angle_radians = angle0 * 2.0f * IM_PI / 360.0f;

            // Text markers
            for (int i = 0; i < 6; ++i) {
                const float percent = ranges[i] / ranges[5]; // value over max   (float 0 - 1)
                float angle = start_angle_radians - (IM_PI * 1.5f * percent); // MINUS BECAUSE ITS INVERTED                   
                ImVec2 pos = { center.x + radius * 1.15f * cos(angle), center.y + radius * 1.15f * sin(angle) };   // polar to cartesian
                string plottertext = std::format("{:.0f}", ranges[i]);
                ImPlot::PlotText(plottertext.data(), pos.x, pos.y);
            }
            ImVec2 titlePos = { center.x,center.y - (radius * 0.5f) };
            ImPlot::PlotText(name, titlePos.x, titlePos.y);

            ImVec2 lineVectorPoint;
            float v_Percent = value / ranges[5];
            float v_Angle = start_angle_radians - (IM_PI * 1.5f * v_Percent);
            lineVectorPoint = { center.x + radius * cos(v_Angle), center.y + radius * sin(v_Angle) };
            float Xs[] = { center.x,lineVectorPoint.x };
            float Ys[] = { center.y,lineVectorPoint.y };
            ImPlot::SetNextLineStyle(ImVec4(255, 255, 255, 255), 4.0f);
            ImPlot::PlotLine("CV", Xs, Ys, 2, ImPlotLineFlags_Segments);
            ImPlot::EndPlot();
        }

    }
}

enum class DragDropBlock {
    None,
    Device,
    Filter,
    Gain,
    Reverb,
    EQ,
    Saturator,
    ChannelUtil,
    Compressor
};

enum class EffectType {
    None,
    Filter,
    Phaser,
    Gain,
    Reverb,
    EQ,
    Saturator,
    ChannelUtil,
    Compressor
};

enum class BlockType {
    NullDevice,
    InputDevice,
    OutputDevice,
    DSP,
    FileInput
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