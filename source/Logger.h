#ifndef LOGGER_H
#define LOGGER_H

#include "mainHeader.hpp"
#include <iostream>
#include <fstream>
#include <ctime>
#include <thread>
#include <sstream>
#include <iomanip>
#include <functional>
#include <direct.h>
#include <sys/types.h>
#include <sys/stat.h>

enum LogLevel
{
    level_CRITICAL,
    level_ERROR,
    level_MCERROR,
    level_VISIONERROR,
    level_WARNING,
    level_INFO,
    level_DEBUG,
    level_DEFAULT
};

enum LogSource
{
    source_GUI,
    source_AUDIO,
    source_DEVICE,
    source_DEBUG,
    source_SETTINGS,
    source_PROGRAM,
    source_FILES,
    source_NODE,
    source_DEFAULT
};

// Interface to subscribe to the Logger messages from sub-class below
class Subscriber {
public:
    virtual void sendLogMessage(std::string message) = 0;
};

class Logger {
public:
    static void log(const std::string& message) {log(message, level_DEFAULT,  source_DEFAULT); }
    static void log(const std::string& message, LogLevel level) { log(message, level,  source_DEFAULT); }
    static void log(const std::string& message, LogLevel level,  LogSource source){ getInstance().logMessage(level, message, source);}

    static void setLogLevel(LogLevel level) { getInstance().currentLogLevel_ = level; }
    static void DestroyLogger();
    static void registerSubscriber(Subscriber* subscriber) { getInstance().subscribers.push_back(subscriber); }
    static void removeSubscriber(Subscriber* subscriber) { getInstance().subscribers.erase(std::remove(getInstance().subscribers.begin(), getInstance().subscribers.end(), subscriber)); }

private:
    Logger()
    {
        getFullDirPath();
        logThread = std::thread(&Logger::ThreadExecute, this);
    }
    ~Logger() { DestroyLogger(); }

    void destroyLog();
    void notifySubscribers(std::string message);
    static std::string GetTimestamp();
    static std::string LevelName(LogLevel level);
    static std::string SourceName(LogSource source);

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }
    void getFullDirPath();
    void logMessage(LogLevel level, const std::string& message, LogSource source);    // Proces incoming message and put it into queues
    void ThreadExecute();   // Processing incoming messages and sending them to the appropriate places. Thread sleeps for 500 ms to allow the program to catch up
    

    struct CompleteMessage
    {
        std::string message, time;
        LogLevel level;
        LogSource source;
        std::string SentenceBuilder() const
        {
            return (std::string)(time + "[" + LevelName(level) + "] : " + message + "\n"); // [" + SourceName(source) + "]
        }
    };
    std::queue<std::string> messageQueue;
    std::queue<LogLevel> levelQueue;
    std::queue<std::string> timeQueue;
    std::queue<LogSource> sourceQueue;
    std::ofstream logfile;
    std::mutex messageLock;
    std::thread logThread;
    std::string fullLogPath;
    bool threadStop = false;
    std::vector<Subscriber*> subscribers;
    LogLevel currentLogLevel_ = level_DEBUG;
};

// GUI Log (Subscriber)
struct AppLog : public Subscriber
{
    ImGuiTextBuffer     Buf;
    ImGuiTextFilter     Filter;
    vector<int>       LineOffsets; // Index to lines offset. We maintain this with AddLog() calls.
    bool                AutoScroll;  // Keep scrolling if already at the bottom.

    AppLog()
    {
        Logger::registerSubscriber(this);
        AutoScroll = true;
        Clear();
    }

    void sendLogMessage(string message) {
        AddString(message);
    }

    void Clear()
    {
        Buf.clear();
        LineOffsets.clear();
        LineOffsets.push_back(0);
    }

    void AddString(string inputstr, ...) IM_FMTARGS(2)
    {
        if (inputstr.find("<") == 0) return;    // to block <server>, <client> and <fileAck> messages from displaying

        const char* fmt = inputstr.c_str();
        int old_size = Buf.size();

        va_list args;
        va_start(args, fmt);
        Buf.appendfv(fmt, args);
        va_end(args);

        for (int new_size = Buf.size(); old_size < new_size; old_size++)
            if (Buf[old_size] == '\n')
                LineOffsets.push_back(old_size + 1);
    }

    void Draw(const char* title, bool* p_open = NULL)
    {
        ImGui::Begin(title, p_open, ImGuiWindowFlags_NoFocusOnAppearing);

        // Options menu

        if (ImGui::Checkbox("Auto-scroll", &AutoScroll)) ImGui::SetScrollHereY(1.0f);

        // Main window
        ImGui::SameLine();
        bool clear = ImGui::Button("Clear");
        ImGui::SameLine();
        bool copy = ImGui::Button("Copy");
        ImGui::SameLine();
        Filter.Draw("Filter", -100.0f);

        ImGui::Separator();

        if (ImGui::BeginChild("scrolling", ImVec2(0, 0), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar))
        {
            if (clear)
                Clear();
            if (copy)
                ImGui::LogToClipboard();

            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
            const char* buf = Buf.begin();
            const char* buf_end = Buf.end();
            if (Filter.IsActive())
            {
                // In this example we don't use the clipper when Filter is enabled.
                // This is because we don't have random access to the result of our filter.
                // A real application processing logs with ten of thousands of entries may want to store the result of
                // search/filter.. especially if the filtering function is not trivial (e.g. reg-exp).
                for (int line_no = 0; line_no < LineOffsets.size(); line_no++)
                {
                    const char* line_start = buf + LineOffsets[line_no];
                    const char* line_end = (line_no + 1 < LineOffsets.size()) ? (buf + LineOffsets[line_no + 1] - 1) : buf_end;
                    if (Filter.PassFilter(line_start, line_end))
                        ImGui::TextUnformatted(line_start, line_end);
                }
            }
            else
            {
                // The simplest and easy way to display the entire buffer:
                //   ImGui::TextUnformatted(buf_begin, buf_end);
                // And it'll just work. TextUnformatted() has specialization for large blob of text and will fast-forward
                // to skip non-visible lines. Here we instead demonstrate using the clipper to only process lines that are
                // within the visible area.
                // If you have tens of thousands of items and their processing cost is non-negligible, coarse clipping them
                // on your side is recommended. Using ImGuiListClipper requires
                // - A) random access into your data
                // - B) items all being the  same height,
                // both of which we can handle since we have an array pointing to the beginning of each line of text.
                // When using the filter (in the block of code above) we don't have random access into the data to display
                // anymore, which is why we don't use the clipper. Storing or skimming through the search result would make
                // it possible (and would be recommended if you want to search through tens of thousands of entries).

                ImGuiListClipper clipper;
                clipper.Begin(LineOffsets.size()); 
                while (clipper.Step())
                {
                    for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++)
                    {
                        const char* line_start = buf + LineOffsets[line_no];
                        const char* line_end = (line_no + 1 < LineOffsets.size()) ? (buf + LineOffsets[line_no + 1] - 1) : buf_end;
                       
                        ImGui::TextUnformatted(line_start, line_end);
                    }
                }
                clipper.End();

            }
            ImGui::PopStyleVar();

            // Keep up at the bottom of the scroll region if we were already at the bottom at the beginning of the frame.
            // Using a scrollbar or mouse-wheel will take away from the bottom edge.

            if (AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
                ImGui::SetScrollHereY(1.0f);
        }
        ImGui::EndChild();
        ImGui::End();

    }
};

#endif