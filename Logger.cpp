#include "pch.h"
#include "framework.h"
#include "Logger.h"
#include <chrono>
#include <ctime>
#include <shlobj.h>

Logger& Logger::Instance() {
    static Logger instance;
    return instance;
}

Logger::Logger() {
    std::wstring logPath = GetLogPath();
    size_t lastSlash = logPath.find_last_of(L'\\');
    if (lastSlash != std::wstring::npos) {
        CreateDirectoryW(logPath.substr(0, lastSlash).c_str(), nullptr);
    }
    m_file.open(logPath, std::ios::out | std::ios::app);
}

Logger::~Logger() {
    if (m_file.is_open()) m_file.close();
}

std::wstring Logger::GetLogPath() {
    wchar_t path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, path))) {
        return std::wstring(path) + L"\\DeskDisplay\\DeskDisplay.log";
    }
    return L"DeskDisplay.log";
}

void Logger::RotateIfNeeded() {
    m_file.seekp(0, std::ios::end);
    if (m_file.tellp() > MAX_SIZE) {
        m_file.close();
        m_file.open(GetLogPath(), std::ios::out | std::ios::trunc);
    }
}

const wchar_t* Logger::LevelPrefix(LogLevel level) {
    switch (level) {
    case LogLevel::Info:  return L"[INFO] ";
    case LogLevel::Warn:  return L"[WARN] ";
    case LogLevel::Error: return L"[ERROR]";
    }
    return L"";
}

void Logger::Log(LogLevel level, const std::wstring& message) {
    std::lock_guard<std::mutex> lock(m_mutex);
    RotateIfNeeded();

    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    tm local = {};
    localtime_s(&local, &t);

    wchar_t timeBuf[32];
    wcsftime(timeBuf, 32, L"%Y-%m-%d %H:%M:%S", &local);

    m_file << timeBuf << L" " << LevelPrefix(level) << L" " << message << std::endl;
}
