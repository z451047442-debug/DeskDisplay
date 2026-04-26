#include "pch.h"
#include "framework.h"
#include "OverlayRenderer.h"

constexpr int MARGIN = 10;
constexpr int LINE_HEIGHT = 18;

OverlayRenderer::OverlayRenderer(int width, int height)
    : m_width(width), m_height(height) {
    CreateBackBuffer();

    m_titleFont = new Gdiplus::Font(L"Microsoft YaHei UI", 11, Gdiplus::FontStyleBold, Gdiplus::UnitPoint);
    m_normalFont = new Gdiplus::Font(L"Microsoft YaHei UI", 9, Gdiplus::FontStyleRegular, Gdiplus::UnitPoint);
    m_smallFont = new Gdiplus::Font(L"Consolas", 8, Gdiplus::FontStyleRegular, Gdiplus::UnitPoint);

    m_bgBrush = new Gdiplus::SolidBrush(Gdiplus::Color(200, 20, 20, 30));
    m_whiteBrush = new Gdiplus::SolidBrush(Gdiplus::Color(255, 255, 255, 255));
    m_cyanBrush = new Gdiplus::SolidBrush(Gdiplus::Color(255, 0, 200, 255));
    m_greenBrush = new Gdiplus::SolidBrush(Gdiplus::Color(255, 0, 255, 100));
    m_yellowBrush = new Gdiplus::SolidBrush(Gdiplus::Color(255, 255, 200, 0));
    m_redBrush = new Gdiplus::SolidBrush(Gdiplus::Color(255, 255, 80, 80));
    m_dimBrush = new Gdiplus::SolidBrush(Gdiplus::Color(180, 180, 180, 180));

    m_borderPen = new Gdiplus::Pen(Gdiplus::Color(180, 0, 150, 255), 1.0f);
    m_sectionLinePen = new Gdiplus::Pen(Gdiplus::Color(100, 0, 150, 255), 1.0f);
    m_graphPen = new Gdiplus::Pen(Gdiplus::Color(255, 0, 200, 255), 1.5f);
    m_chartBorderPen = new Gdiplus::Pen(Gdiplus::Color(100, 100, 100, 100), 1.0f);
}

OverlayRenderer::~OverlayRenderer() {
    delete m_titleFont;
    delete m_normalFont;
    delete m_smallFont;
    delete m_bgBrush;
    delete m_whiteBrush;
    delete m_cyanBrush;
    delete m_greenBrush;
    delete m_yellowBrush;
    delete m_redBrush;
    delete m_dimBrush;
    delete m_borderPen;
    delete m_sectionLinePen;
    delete m_graphPen;
    delete m_chartBorderPen;
    ReleaseBackBuffer();
}

void OverlayRenderer::CreateBackBuffer() {
    HDC hdc = GetDC(nullptr);
    m_memDC = CreateCompatibleDC(hdc);
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = m_width;
    bmi.bmiHeader.biHeight = -m_height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    m_hBmp = CreateDIBSection(m_memDC, &bmi, DIB_RGB_COLORS, &m_bits, nullptr, 0);
    if (m_hBmp) {
        m_oldBmp = (HBITMAP)SelectObject(m_memDC, m_hBmp);
    }
    ReleaseDC(nullptr, hdc);
}

void OverlayRenderer::ReleaseBackBuffer() {
    if (m_oldBmp) {
        SelectObject(m_memDC, m_oldBmp);
        m_oldBmp = nullptr;
    }
    if (m_hBmp) {
        DeleteObject(m_hBmp);
        m_hBmp = nullptr;
    }
    if (m_memDC) {
        DeleteDC(m_memDC);
        m_memDC = nullptr;
    }
    m_bits = nullptr;
}

void OverlayRenderer::Resize(int width, int height) {
    if (width == m_width && height == m_height) return;
    m_width = width;
    m_height = height;
    ReleaseBackBuffer();
    CreateBackBuffer();
}

void OverlayRenderer::DrawSectionLine(Gdiplus::Graphics& g, int w, int& y, const wchar_t* title) {
    y += 4;
    g.DrawLine(m_sectionLinePen, MARGIN, y, w - MARGIN, y);
    y += 4;
    g.DrawString(title, -1, m_titleFont, Gdiplus::PointF((float)MARGIN, (float)y), m_cyanBrush);
    y += 22;
}

void OverlayRenderer::DrawTextLine(Gdiplus::Graphics& g, int& y, const std::wstring& text,
                                    Gdiplus::SolidBrush* brush) {
    g.DrawString(text.c_str(), -1, m_normalFont, Gdiplus::PointF((float)(MARGIN + 5), (float)y),
                 brush ? brush : m_whiteBrush);
    y += LINE_HEIGHT;
}

void OverlayRenderer::DrawTitle(Gdiplus::Graphics& g, int w, int& y) {
    g.DrawString(L"DeskDisplay", -1, m_titleFont, Gdiplus::PointF((float)(w / 2 - 45), (float)y), m_cyanBrush);
    y += 26;
}

void OverlayRenderer::DrawComputerSection(Gdiplus::Graphics& g, int w, int& y, const SysInfo& sysInfo) {
    DrawSectionLine(g, w, y, L"计算机");
    DrawTextLine(g, y, L"计算机名: " + sysInfo.computerName);
    DrawTextLine(g, y, sysInfo.domainStatus);
    DrawTextLine(g, y, L"运行时间: " + sysInfo.systemUptime, m_dimBrush);
    DrawTextLine(g, y, sysInfo.osVersion, m_dimBrush);
}

void OverlayRenderer::DrawCpuSection(Gdiplus::Graphics& g, int w, int& y, const SysInfo& sysInfo,
                                      const std::deque<double>& cpuHistory, double cpuMax, double cpuAvg,
                                      int cpuHistorySize) {
    DrawSectionLine(g, w, y, L"CPU");

    wchar_t cpuBuf[64];
    swprintf_s(cpuBuf, L"使用率: %.1f%%", sysInfo.cpuUsage);
    Gdiplus::SolidBrush* cpuColor = sysInfo.cpuUsage > 80 ? m_redBrush
        : (sysInfo.cpuUsage > 50 ? m_yellowBrush : m_greenBrush);
    DrawTextLine(g, y, cpuBuf, cpuColor);

    swprintf_s(cpuBuf, L"最高: %.1f%%  平均: %.1f%%", cpuMax, cpuAvg);
    DrawTextLine(g, y, cpuBuf, m_dimBrush);

    if (!sysInfo.perCoreCpu.empty() && sysInfo.perCoreCpu.size() > 1) {
        std::wstring coreLine = L"核心:";
        for (size_t i = 0; i < sysInfo.perCoreCpu.size(); i++) {
            wchar_t c[16];
            swprintf_s(c, L" %.0f", sysInfo.perCoreCpu[i]);
            coreLine += c;
        }
        g.DrawString(coreLine.c_str(), -1, m_smallFont,
                     Gdiplus::PointF((float)(MARGIN + 10), (float)y), m_dimBrush);
        y += LINE_HEIGHT - 2;
    }

    int chartX = MARGIN + 5;
    int chartY = y;
    int chartW = w - 2 * MARGIN - 10;
    int chartH = 50;
    g.DrawRectangle(m_chartBorderPen, chartX, chartY, chartW, chartH);

    if (cpuHistory.size() > 1) {
        int count = (int)cpuHistory.size();
        float step = (float)chartW / (cpuHistorySize - 1);
        std::vector<Gdiplus::PointF> pts;
        for (int i = 0; i < count; i++) {
            float px = chartX + (cpuHistorySize - count + i) * step;
            float py = chartY + chartH - (float)(cpuHistory[i] / 100.0 * chartH);
            pts.push_back(Gdiplus::PointF(px, py));
        }
        g.DrawLines(m_graphPen, pts.data(), (int)pts.size());
    }
    y += chartH + 8;
}

void OverlayRenderer::DrawBatterySection(Gdiplus::Graphics& g, int w, int& y, const SysInfo& sysInfo) {
    if (!sysInfo.hasBattery) return;
    DrawSectionLine(g, w, y, L"电池");
    wchar_t batBuf[64];
    if (sysInfo.batteryPercentValid) {
        swprintf_s(batBuf, L"电量: %d%% %s", sysInfo.batteryPercent,
                   sysInfo.batteryCharging ? L"(充电中)" : L"");
    } else {
        swprintf_s(batBuf, L"电量: 未知 %s", sysInfo.batteryCharging ? L"(充电中)" : L"");
    }
    Gdiplus::SolidBrush* batColor = (!sysInfo.batteryPercentValid || sysInfo.batteryPercent < 20)
        ? m_redBrush : (sysInfo.batteryPercent < 50 ? m_yellowBrush : m_greenBrush);
    DrawTextLine(g, y, batBuf, batColor);
}

void OverlayRenderer::DrawMemorySection(Gdiplus::Graphics& g, int w, int& y, const SysInfo& sysInfo) {
    DrawSectionLine(g, w, y, L"内存");

    double totalMB = sysInfo.totalPhysMem / (1024.0 * 1024);
    double usedMB = sysInfo.usedPhysMem / (1024.0 * 1024);
    double memPercent = (totalMB > 0) ? (usedMB / totalMB * 100.0) : 0;
    wchar_t memBuf[128];
    swprintf_s(memBuf, L"%.0f MB / %.0f MB (%.1f%%)", usedMB, totalMB, memPercent);
    Gdiplus::SolidBrush* memColor = memPercent > 80 ? m_redBrush
        : (memPercent > 60 ? m_yellowBrush : m_greenBrush);
    DrawTextLine(g, y, memBuf, memColor);

    int barX = MARGIN + 5;
    int barY = y;
    int barW = w - 2 * MARGIN - 10;
    int barH = 8;
    g.FillRectangle(m_dimBrush, barX, barY, barW, barH);
    int fillW = (int)(barW * memPercent / 100.0);
    Gdiplus::Color barFillColor = memPercent > 80
        ? Gdiplus::Color(255, 255, 80, 80)
        : (memPercent > 60 ? Gdiplus::Color(255, 255, 200, 0) : Gdiplus::Color(255, 0, 255, 100));
    Gdiplus::SolidBrush barFill(barFillColor);
    g.FillRectangle(&barFill, barX, barY, fillW, barH);
    y += barH + 8;

    DrawTextLine(g, y, L"内存占用 Top 5:", m_dimBrush);
    for (auto& p : sysInfo.topMemProcs) {
        std::wstring line = p.name + L"  " + SystemMonitor::FormatBytes((double)p.memBytes);
        g.DrawString(line.c_str(), -1, m_smallFont,
                     Gdiplus::PointF((float)(MARGIN + 10), (float)y), m_dimBrush);
        y += LINE_HEIGHT - 2;
    }
}

void OverlayRenderer::DrawNetworkSection(Gdiplus::Graphics& g, int w, int& y, const SysInfo& sysInfo,
                                          const NetworkStats& netStats) {
    DrawSectionLine(g, w, y, L"网络");
    DrawTextLine(g, y, L"IP: " + sysInfo.ipAddress);
    DrawTextLine(g, y, L"MAC: " + sysInfo.macAddress);

    if (netStats.adapters.size() <= 1) {
        DrawTextLine(g, y, L"下载: " + SystemMonitor::FormatSpeed(netStats.downloadSpeed), m_greenBrush);
        DrawTextLine(g, y, L"上传: " + SystemMonitor::FormatSpeed(netStats.uploadSpeed), m_cyanBrush);
    } else {
        DrawTextLine(g, y, L"合计 下载: " + SystemMonitor::FormatSpeed(netStats.downloadSpeed), m_greenBrush);
        DrawTextLine(g, y, L"合计 上传: " + SystemMonitor::FormatSpeed(netStats.uploadSpeed), m_cyanBrush);
        for (auto& ad : netStats.adapters) {
            wchar_t buf[128];
            swprintf_s(buf, L"  %s v %.1f/%.1f", ad.name.c_str(),
                       ad.downloadSpeed / 1024.0, ad.uploadSpeed / 1024.0);
            g.DrawString(buf, -1, m_smallFont,
                         Gdiplus::PointF((float)(MARGIN + 10), (float)y), m_dimBrush);
            y += LINE_HEIGHT - 2;
        }
    }
}

void OverlayRenderer::DrawDiskSection(Gdiplus::Graphics& g, int w, int& y, const SysInfo& sysInfo) {
    DrawSectionLine(g, w, y, L"磁盘");
    for (auto& d : sysInfo.disks) {
        wchar_t diskBuf[128];
        double diskPercent = d.totalGB > 0 ? (double)d.usedGB / d.totalGB * 100.0 : 0;
        swprintf_s(diskBuf, L"%s  %llu/%llu GB (%.0f%%)", d.drive.c_str(), d.usedGB, d.totalGB, diskPercent);
        Gdiplus::SolidBrush* diskColor = diskPercent > 90 ? m_redBrush
            : (diskPercent > 70 ? m_yellowBrush : m_whiteBrush);
        DrawTextLine(g, y, diskBuf, diskColor);
    }
}

void OverlayRenderer::Render(HWND hWnd, const SysInfo& sysInfo, const NetworkStats& netStats,
                              const std::deque<double>& cpuHistory, double cpuMax, double cpuAvg,
                              int cpuHistorySize) {
    RECT rc;
    GetClientRect(hWnd, &rc);
    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;

    if (w != m_width || h != m_height) {
        Resize(w, h);
    }

    Gdiplus::Graphics g(m_memDC);
    g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    g.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);

    g.FillRectangle(m_bgBrush, 0, 0, w, h);
    g.DrawRectangle(m_borderPen, 0, 0, w - 1, h - 1);

    int y = MARGIN;
    DrawTitle(g, w, y);
    DrawComputerSection(g, w, y, sysInfo);
    DrawCpuSection(g, w, y, sysInfo, cpuHistory, cpuMax, cpuAvg, cpuHistorySize);
    DrawBatterySection(g, w, y, sysInfo);
    DrawMemorySection(g, w, y, sysInfo);
    DrawNetworkSection(g, w, y, sysInfo, netStats);
    DrawDiskSection(g, w, y, sysInfo);

    HDC hdc = GetDC(hWnd);
    SIZE sz = { w, h };
    POINT ptSrc = { 0, 0 };
    BLENDFUNCTION blend = {};
    blend.BlendOp = AC_SRC_OVER;
    blend.SourceConstantAlpha = 255;
    blend.AlphaFormat = AC_SRC_ALPHA;
    UpdateLayeredWindow(hWnd, hdc, nullptr, &sz, m_memDC, &ptSrc, 0, &blend, ULW_ALPHA);
    ReleaseDC(hWnd, hdc);
}
