#pragma once

#include <string>
#include <fstream>
#include <mutex>

enum class LogLevel { Info, Warn, Error };

class Logger {
public:
    static Logger& Instance();

    void Log(LogLevel level, const std::wstring& message);
    void Info(const std::wstring& msg) { Log(LogLevel::Info, msg); }
    void Warn(const std::wstring& msg) { Log(LogLevel::Warn, msg); }
    void Error(const std::wstring& msg) { Log(LogLevel::Error, msg); }

private:
    Logger();
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void RotateIfNeeded();
    std::wstring GetLogPath();
    const wchar_t* LevelPrefix(LogLevel level);

    std::wofstream m_file;
    std::mutex m_mutex;
    static constexpr long long MAX_SIZE = 1024 * 1024;
};
