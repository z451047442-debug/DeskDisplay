#pragma once

#include <string>
#include <windows.h>

class Config {
public:
    Config();

    int GetWindowX(int defaultX) const;
    int GetWindowY(int defaultY) const;
    int GetRefreshMs(int defaultMs) const;
    int GetOpacity(int defaultOpacity) const;
    bool GetClickThrough(bool defaultVal) const;

    void SetWindowPos(int x, int y);
    void SetRefreshMs(int ms);
    void SetOpacity(int opacity);
    void SetClickThrough(bool enabled);

private:
    std::wstring m_iniPath;
    int ReadInt(const wchar_t* section, const wchar_t* key, int defaultVal) const;
    void WriteInt(const wchar_t* section, const wchar_t* key, int value);
};
