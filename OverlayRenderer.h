#pragma once

#include <windows.h>
#include <gdiplus.h>
#include <deque>
#include "SystemMonitor.h"

class OverlayRenderer {
public:
    OverlayRenderer(int width, int height);
    ~OverlayRenderer();

    OverlayRenderer(const OverlayRenderer&) = delete;
    OverlayRenderer& operator=(const OverlayRenderer&) = delete;

    void Render(HWND hWnd, const SysInfo& sysInfo, const NetworkStats& netStats,
                const std::deque<double>& cpuHistory, double cpuMax, double cpuAvg,
                int cpuHistorySize);
    void Resize(int width, int height);

private:
    void DrawTitle(Gdiplus::Graphics& g, int w, int& y);
    void DrawComputerSection(Gdiplus::Graphics& g, int w, int& y, const SysInfo& sysInfo);
    void DrawCpuSection(Gdiplus::Graphics& g, int w, int& y, const SysInfo& sysInfo,
                        const std::deque<double>& cpuHistory, double cpuMax, double cpuAvg,
                        int cpuHistorySize);
    void DrawBatterySection(Gdiplus::Graphics& g, int w, int& y, const SysInfo& sysInfo);
    void DrawMemorySection(Gdiplus::Graphics& g, int w, int& y, const SysInfo& sysInfo);
    void DrawNetworkSection(Gdiplus::Graphics& g, int w, int& y, const SysInfo& sysInfo,
                            const NetworkStats& netStats);
    void DrawDiskSection(Gdiplus::Graphics& g, int w, int& y, const SysInfo& sysInfo);

    void DrawSectionLine(Gdiplus::Graphics& g, int w, int& y, const wchar_t* title);
    void DrawTextLine(Gdiplus::Graphics& g, int& y, const std::wstring& text,
                      Gdiplus::SolidBrush* brush = nullptr);

    void CreateBackBuffer();
    void ReleaseBackBuffer();

    int m_width;
    int m_height;

    HDC m_memDC = nullptr;
    HBITMAP m_hBmp = nullptr;
    HBITMAP m_oldBmp = nullptr;
    void* m_bits = nullptr;

    Gdiplus::Font* m_titleFont = nullptr;
    Gdiplus::Font* m_normalFont = nullptr;
    Gdiplus::Font* m_smallFont = nullptr;

    Gdiplus::SolidBrush* m_bgBrush = nullptr;
    Gdiplus::SolidBrush* m_whiteBrush = nullptr;
    Gdiplus::SolidBrush* m_cyanBrush = nullptr;
    Gdiplus::SolidBrush* m_greenBrush = nullptr;
    Gdiplus::SolidBrush* m_yellowBrush = nullptr;
    Gdiplus::SolidBrush* m_redBrush = nullptr;
    Gdiplus::SolidBrush* m_dimBrush = nullptr;

    Gdiplus::Pen* m_borderPen = nullptr;
    Gdiplus::Pen* m_sectionLinePen = nullptr;
    Gdiplus::Pen* m_graphPen = nullptr;
    Gdiplus::Pen* m_chartBorderPen = nullptr;
};
