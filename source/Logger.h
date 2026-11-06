#ifndef LOGGER_H
#define LOGGER_H

#include "mainHeader.hpp"
#include <ctime>
#include <sstream>
#include <iomanip>
#include <functional>
#include <direct.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ranges>

class Program;
class LRIcomm;

enum LogLevel
{
    level_CRITICAL,
    level_ERROR,
    level_WARNING,
    level_INFO,
    level_DEBUG,
};

enum LogSource
{
    source_GUI,
    source_AUDIO,
    source_DEVICE,
    source_SETTINGS,
    source_PROGRAM,   
    source_FILES,
    source_DEFAULT,
    source_NODES
};

// Struct to combine the queues into a single string/message
struct CompleteMessage
{
    std::string message, time;
    LogLevel level;
    LogSource source;
    int id;
    std::string SentenceBuilder()
    {
        return (std::string)(time + "[" + LevelName(level) + "] [" + SourceName(source) + "] : " + message + "\n");
    }

    std::string LevelName(LogLevel level) {
        switch (level)
        {
        case level_CRITICAL:
            return "Critical";
        case level_ERROR:
            return "Error";
        case level_WARNING:
            return "Warning";
        case level_INFO:
            return "Info";
        case level_DEBUG:
            return "Debug";
        default:
            return "Default";
        }
    }

    std::string SourceName(LogSource source) {
        switch (source)
        {
        case source_GUI:
            return "GUI";
        case source_AUDIO:
            return "Audio";
        case source_DEVICE:
            return "Device";
        case source_NODES:
            return "Nodes";
        case source_SETTINGS:
            return "Settings";
        case source_PROGRAM:
            return "Program";     
        case source_FILES:
            return "Files";
        case source_DEFAULT:
            return "Default";
        default:
            return "Default";
        }
    }
};

// Interface to subscribe to the Logger messages from sub-class below
class Subscriber {
public:
    virtual void sendLogMessage(std::string fullMessage, CompleteMessage msgStruct) = 0;
};

class Logger {
public:
    static void log(const std::string& message) { log(message, level_INFO, source_DEFAULT, 0); }
    static void log(const std::string& message, LogLevel level) { log(message, level, source_DEFAULT, 0); }
    static void log(const std::exception& error, LogSource source) { log(error.what(), level_ERROR, source, 0); };
    static void log(const std::string& message, LogLevel level, LogSource source) { log(message, level, source, 0); }
    static void log(const std::string& message, LogLevel level, LogSource source, int id) { getInstance().logMessage(message, level, source, id); }
    //   static void log(const std::string& message, LogLevel level, LogSource source, int id, ...) { getInstance().logMessage(message, level, source, id); }

    static void setLogLevel(LogLevel level) { getInstance().currentLogLevel_ = level; }
    static void DestroyLogger();
    static void registerSubscriber(Subscriber* subscriber) { getInstance().subscribers.push_back(subscriber); }
    static void removeSubscriber(Subscriber* subscriber) { getInstance().subscribers.erase(std::remove(getInstance().subscribers.begin(), getInstance().subscribers.end(), subscriber)); }
    static std::string GetTimestamp();
    static std::string getFileLoc() { return getInstance().fileLocation; }
    static int GetMessageQueueFill() { return getInstance().logMessageQueue.size(); }

private:
    Logger()
    {
        getFullDirPath();
        logThread = std::thread(&Logger::ThreadExecute, this);
    }
    ~Logger() { DestroyLogger(); }

    void destroyLog();
    void notifySubscribers(std::string fullMessage, CompleteMessage msgStruct);
    void OpenLogFile();

    Logger(const Logger&) = delete;
    Logger& operator = (const Logger&) = delete;
    static Logger& getInstance() { static Logger instance; return instance; }
    void getFullDirPath();
    void logMessage(const std::string& message, LogLevel level, LogSource source, int id);    // Proces incoming message and put it into queues
    CompleteMessage MakeMessage(const std::string& message, LogLevel level, LogSource source, int id);   // for repeat messages in thread
    void ThreadExecute();   // Processing incoming messages and sending them to the appropriate places. Thread sleeps for 500 ms to allow the program to catch up
    void WriteAndNotify(CompleteMessage& msg);

    //Variables of the class
    //Queues for the message
    std::queue<std::string> messageQueue;
    std::queue<LogLevel> levelQueue;
    std::queue<std::string> timeQueue;
    std::queue<LogSource> sourceQueue;
    std::queue<CompleteMessage> logMessageQueue;

    //Variables used to write to files
    std::ofstream logfile;
    std::string fullLogPath;
    int currentDay;
    std::string fileLocation;
    string lastMessageWithoutTime;
    int counter = 0;

    //Threading variables
    std::mutex messageLock;
    std::thread logThread;
    bool threadStop = false;

    std::vector<Subscriber*> subscribers;
    LogLevel currentLogLevel_ = level_DEBUG;
};

// LIKE AND SUBSCRIBE :D

struct AppLog : public Subscriber
{
    ImGuiTextBuffer     Buf;
    ImGuiTextFilter     Filter;
    vector<int>         LineOffsets;                // Index to lines offset. We maintain this with AddLog() calls. Line = vector entry, val = total chars?
    vector<LogLevel>    LineLevels;                 // for color coding
    vector<LogSource>    LineSources;                 // for color coding
    bool                AutoScroll;                 // Keep scrolling if already at the bottom.
    bool                hideTCP = false;
    string              lastMessageWithoutTime;
    int                 counter = 0;                // checking repeat messages

    AppLog()
    {
        Logger::registerSubscriber(this);
        AutoScroll = true;
        Clear();
    }

    void sendLogMessage(string fullMessage, CompleteMessage msgStruct) {
        int pos = 0;
        int count = 0;
        while ((pos = fullMessage.find("\n", pos)) != std::string::npos) {
            pos += 2;
            count++;
        }
        do {
            LineLevels.push_back(msgStruct.level);
            LineSources.push_back(msgStruct.source);    // add entries for >1 of line chars
            count--;
        } while (count > 0);


        AddString(fullMessage);
    }

    void Clear()
    {
        Buf.clear();
        LineOffsets.clear();
        LineOffsets.push_back(0);
        LineSources.clear();
        LineSources.push_back(source_DEFAULT);
        LineLevels.clear();
        LineLevels.push_back(level_INFO);
    }

    void AddString(string inputstr)
    {
        //  if (inputstr.find("<") == 0) return;    // to block <server>, <client> and <fileAck> messages from displaying

        const char* fmt = inputstr.c_str();
        int old_size = Buf.size();

        va_list args = nullptr;
        //va_start(args, fmt);
        Buf.appendfv(fmt, args);
        // va_end(args);

        for (int new_size = Buf.size(); old_size < new_size; old_size++)
            if (Buf[old_size] == '\n')
                LineOffsets.push_back(old_size + 1);
    }


    void Draw(const char* title, bool* p_open = NULL)
    {
        ImGui::Begin(title, p_open, ImGuiWindowFlags_NoFocusOnAppearing);

        // Options menu

        if (ImGui::Checkbox("Auto-scroll", &AutoScroll)) ImGui::SetScrollHereY(1.0f);

        ImGui::SameLine();
        ImGui::Checkbox("Hide TCP debug", &hideTCP);

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
            int pushedCol = 0;

            if (Filter.IsActive())
            {
                // In this example we don't use the clipper when Filter is enabled.
                // This is because we don't have random access to the result of our filter.
                // A real application processing logs with ten of thousands of entries may want to store the result of
                // search/filter.. especially if the filtering function is not trivial (e.g. reg-exp).
                for (int line_no = 0; line_no < LineOffsets.size(); line_no++)
                {
                    pushedCol = 0;

                    int idx = min(LineOffsets.size() - 1, line_no + 1);

                    if (LineSources[idx] == source_NODES) {
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(150, 220, 255, 255));
                        pushedCol++;
                    }

                    switch (LineLevels[idx]) {

                    case level_CRITICAL:ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(250, 0, 250, 255)); pushedCol++;
                        break;
                    case level_ERROR:ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(250, 50, 0, 255)); pushedCol++;
                        break;
                    case level_WARNING:ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(250, 200, 0, 255)); pushedCol++;
                        break;
                    default:
                        break;
                    }

                    const char* line_start = buf + LineOffsets[line_no];
                    const char* line_end = (line_no + 1 < LineOffsets.size()) ? (buf + LineOffsets[line_no + 1] - 1) : buf_end;
                    if (Filter.PassFilter(line_start, line_end))
                        ImGui::TextUnformatted(line_start, line_end);

                    if (pushedCol) ImGui::PopStyleColor(pushedCol);

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
                        pushedCol = 0;

                        pushedCol = 0;

                        int idx = min(LineOffsets.size() - 1, line_no + 1);

                        if (LineSources[idx] == source_NODES) {
                            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(150, 220, 255, 255));
                            pushedCol++;
                        }

                        switch (LineLevels[idx]) {

                        case level_CRITICAL:ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(250, 0, 250, 255)); pushedCol++;
                            break;
                        case level_ERROR:ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(250, 50, 0, 255)); pushedCol++;
                            break;
                        case level_WARNING:ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(250, 200, 0, 255)); pushedCol++;
                            break;
                        default:
                            break;
                        }

                        const char* line_start = buf + LineOffsets[line_no];
                        const char* line_end = (line_no + 1 < LineOffsets.size()) ? (buf + LineOffsets[line_no + 1] - 1) : buf_end;

                        ImGui::TextUnformatted(line_start, line_end);

                        if (pushedCol) ImGui::PopStyleColor(pushedCol);
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