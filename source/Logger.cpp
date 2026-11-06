#include "Logger.h"
#include "Program.h"
#define min(x,y) (x < y ? x : y);

void Logger::ThreadExecute() {
	if (!fullLogPath.empty()) {
		OpenLogFile();
	}

	while (!threadStop) {
		CompleteMessage msg;
		bool haveMsg = false;
		{
			std::lock_guard<std::mutex> lock(messageLock);
			if (!logMessageQueue.empty()) {
				msg = logMessageQueue.front();
				logMessageQueue.pop();
				haveMsg = true;
			}
		}

		if (!haveMsg) {
			std::this_thread::sleep_for(50ms);
			continue;
		}

		try {
			if (msg.message == lastMessageWithoutTime) {
				counter++;
				continue;
			}

			if (counter) {
				CompleteMessage repeatMsg = MakeMessage("Last message repeated (" + to_string(counter) + "x)", level_INFO, source_DEFAULT, 0);
				WriteAndNotify(repeatMsg);
				counter = 0;
			}

			lastMessageWithoutTime = msg.message;
			WriteAndNotify(msg);
		}
		catch (const std::exception& ex) {
			OutputDebugStringA(ex.what());
		}
	}
}

void Logger::WriteAndNotify(CompleteMessage& msg) {
	const time_t t = time(0);
	struct tm now;
	localtime_s(&now, &t);

	if (!(logfile.is_open() && currentDay == now.tm_yday)) {
		logfile.close();
		OpenLogFile();
	}

	const auto text = msg.SentenceBuilder();
	logfile << text;
	logfile.flush();

	if (!subscribers.empty()) {
		notifySubscribers(text, msg);
	}
}

void Logger::OpenLogFile() {
	try
	{
		fileLocation = fullLogPath + "\\" + GetTimestamp() + "_logs.txt";
		logfile.open(fileLocation);

		const time_t t = time(0);
		struct tm now;
		localtime_s(&now, &t);
		currentDay = now.tm_yday;
#ifdef _DEBUG
		Logger::log("The program is in DEBUG mode", level_INFO, source_DEFAULT);
#else
		Logger::log("The program is in RELEASE mode", level_INFO, source_DEFAULT);
#endif
	}
	catch (const std::exception& error)
	{
		std::cerr << "Unable to open logfile" << std::endl;
		std::cerr << error.what() << std::endl;
	}
}

void Logger::getFullDirPath() {
	char buffer[MAX_PATH];
	GetModuleFileNameA(NULL, buffer, MAX_PATH);
	auto workingDirPath = path(buffer).parent_path();

	fullLogPath = workingDirPath.string() + "\\..\\..\\assets\\Logs";
	if (!std::filesystem::is_directory(fullLogPath))
	{
		bool check = std::filesystem::create_directory(fullLogPath);
		if (!check) { std::cout << "Unable to make a new Log directory!" << std::endl; return; }
		else { std::cout << "Made a new Log directory!" << std::endl; }
	}
	else
	{
		std::cout << "Directory already existed!" << std::endl;
	}
}

void Logger::logMessage(const std::string& message, LogLevel level, LogSource source, int id)
{
	std::string time = GetTimestamp();
	CompleteMessage completeMessage; {
		completeMessage.time = time;
		completeMessage.message = message;
		completeMessage.level = level;
		completeMessage.source = source;
		completeMessage.id = id;
	}
	std::lock_guard<std::mutex> lock(messageLock);
	logMessageQueue.push(completeMessage);
}

CompleteMessage Logger::MakeMessage(const std::string& message, LogLevel level, LogSource source, int id)
{
	std::string time = GetTimestamp();
	CompleteMessage completeMessage; {
		completeMessage.time = time;
		completeMessage.message = message;
		completeMessage.level = level;
		completeMessage.source = source;
		completeMessage.id = id;
	}
	return completeMessage;
}


void Logger::notifySubscribers(std::string fullMessage, CompleteMessage msgStruct) {
	for (Subscriber* subscriber : subscribers) {
		subscriber->sendLogMessage(fullMessage, msgStruct);
	}
};

void Logger::DestroyLogger() {
	getInstance().destroyLog();
}

void Logger::destroyLog() {
	try {
		while (!threadStop) {
			vector<Subscriber*> subsToRemove;

			for (size_t i = 0; i < subscribers.size(); i++)
				removeSubscriber(subscribers[i]);

			if (logMessageQueue.size() == 0) {
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
	const time_t t = time(0);
	struct tm now;
	localtime_s(&now, &t);
	char timestamp[80];
	strftime(timestamp, 80, "%Y-%m-%d %H.%M.%S", &now);
	std::string strTimestamp = timestamp;
	return strTimestamp;
}
