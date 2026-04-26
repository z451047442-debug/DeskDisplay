#include "pch.h"
#include "framework.h"
#include "Config.h"
#include <shlobj.h>

Config::Config() {
    wchar_t path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, path))) {
        m_iniPath = std::wstring(path) + L"\\DeskDisplay";
        CreateDirectoryW(m_iniPath.c_str(), nullptr);
        m_iniPath += L"\\settings.ini";
    } else {
        m_iniPath = L"settings.ini";
    }
}

int Config::ReadInt(const wchar_t* section, const wchar_t* key, int defaultVal) const {
    return GetPrivateProfileIntW(section, key, defaultVal, m_iniPath.c_str());
}

void Config::WriteInt(const wchar_t* section, const wchar_t* key, int value) {
    WritePrivateProfileStringW(section, key, std::to_wstring(value).c_str(), m_iniPath.c_str());
}

int Config::GetWindowX(int defaultX) const { return ReadInt(L"Window", L"X", defaultX); }
int Config::GetWindowY(int defaultY) const { return ReadInt(L"Window", L"Y", defaultY); }
int Config::GetRefreshMs(int defaultMs) const { return ReadInt(L"General", L"RefreshMs", defaultMs); }
int Config::GetOpacity(int defaultOpacity) const { return ReadInt(L"General", L"Opacity", defaultOpacity); }
bool Config::GetClickThrough(bool defaultVal) const { return ReadInt(L"General", L"ClickThrough", defaultVal ? 1 : 0) != 0; }

void Config::SetWindowPos(int x, int y) { WriteInt(L"Window", L"X", x); WriteInt(L"Window", L"Y", y); }
void Config::SetRefreshMs(int ms) { WriteInt(L"General", L"RefreshMs", ms); }
void Config::SetOpacity(int opacity) { WriteInt(L"General", L"Opacity", opacity); }
void Config::SetClickThrough(bool enabled) { WriteInt(L"General", L"ClickThrough", enabled ? 1 : 0); }
