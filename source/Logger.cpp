#include "Logger.h"

void Logger::ThreadExecute(){
    try
    {
        if (!fullLogPath.empty()) { 

            string inputFileLoc = fullLogPath + "\\" + GetTimestamp() + "_logs.txt";

            logfile.open(inputFileLoc);

           // std::cout << "INput file = " << inputFileLoc << std::endl;
        }
        else { 
            std::cerr << "Full path was empty, unable to open log file" << std::endl; 
            return;
        }
    }
    catch (const std::exception& error)
    {
        std::cerr << "Unable to open logfile" << std::endl;
        std::cerr << error.what() << std::endl;
    }
    while (threadStop == false) {
        if (!messageQueue.empty())
        {
            try
            {
                CompleteMessage completeMes;
                {
                    std::lock_guard<std::mutex> lock(messageLock);
                    completeMes.message = messageQueue.front(); messageQueue.pop();
                    completeMes.time = timeQueue.front(); timeQueue.pop();
                    completeMes.level = levelQueue.front(); levelQueue.pop();
                    completeMes.source = sourceQueue.front(); sourceQueue.pop();
                }
                if (logfile.is_open())
                {
                    logfile << completeMes.SentenceBuilder();
                    logfile.flush();
                  //  std::cout << "Complete Sentence: " + completeMes.SentenceBuilder() << std::endl;
                }
                if (!subscribers.empty()) { notifySubscribers(completeMes.SentenceBuilder()); }
            }
            catch (const std::exception& ex)
            {
                std::cerr << ex.what() << std::endl;
            }
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
}

void Logger::getFullDirPath() {
    
        char buffer[MAX_PATH];
        GetModuleFileNameA(NULL, buffer, MAX_PATH);
        auto workingDirPath = std::filesystem::path(buffer).parent_path();

        fullLogPath = workingDirPath.string() + "\\assets\\Logs";

        string tempPath = fullLogPath;

        if (!std::filesystem::is_directory(tempPath))
        {

            try {

            

            bool check = std::filesystem::create_directory(tempPath);
            if (!check) { std::cout << "Unable to make a new directory!" << std::endl; return; }
            else { std::cout << "Made a new directory! Yay!!!!" << std::endl; }


            }
            catch (std::exception e) {
                std::cout << "FILE DIR PATH ERROR: " << e.what() << std::endl;
            }


        }
        else
        {
            std::cout << "Directory already existed!" << std::endl;
        }
    
}

void Logger::logMessage(LogLevel level, const std::string& message, LogSource source)
{
    //if (level >= currentLogLevel_) {
    std::string time = GetTimestamp();
    std::lock_guard<std::mutex> lock(messageLock);
    timeQueue.push(time);
    messageQueue.push(message);
    levelQueue.push(level);
    sourceQueue.push(source);
}

void Logger::notifySubscribers(std::string message) { 
    for (Subscriber* subscriber : subscribers) { 
        subscriber->sendLogMessage(message);
    } 
};

void Logger::DestroyLogger() {

    getInstance().destroyLog();
}

void Logger::destroyLog() {


    try {
        while (!threadStop) {

            for (Subscriber* subscriber : subscribers) { removeSubscriber(subscriber); }

            if (messageQueue.size() == 0) {
                threadStop = true;
                logfile.close();
                if (logThread.joinable()) { logThread.join(); }
            }
        }
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << std::endl;
    }
  
    
}

string Logger::GetTimestamp()
{
    const time_t t = time(0);   // get time now
    struct tm now;
    localtime_s(&now, &t);
    char timestamp[80];
    strftime(timestamp, 80, "%Y-%m-%d %H.%M.%S - ", &now);
    std::string strTimestamp = timestamp;
    return strTimestamp;
}

string Logger::LevelName(LogLevel level){
    switch (level)
    {
    case level_CRITICAL:
        return "Critical";
    case level_ERROR:
        return "Error";
    case level_MCERROR:
        return "MCError";
    case level_VISIONERROR:
        return "VisionError";
    case level_WARNING:
        return "Warning";
    case level_INFO:
        return "Info";
    case level_DEBUG:
        return "Debug";
    case level_DEFAULT:
        return "Default";
    default:
        return "Default";
    }
}

string Logger::SourceName(LogSource source) {
    switch (source)
    {
    case source_GUI:
        return "GUI";
    case source_AUDIO:
        return "Audio";
    case source_DEVICE:
        return "Device";
    case source_DEBUG:
        return "Debug";
    case source_SETTINGS:
        return "Settings";
    case source_PROGRAM:
        return "Program";
    case source_NODE:
        return "SignalNode";
    case source_FILES:
        return "File IO";
    case source_DEFAULT:
        return "Default";
    default:
        return "Default";
    }
}
