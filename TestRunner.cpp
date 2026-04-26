// Test harness for SystemMonitor
// Build: cl /EHsc /std:c++20 TestRunner.cpp SystemMonitor.cpp Logger.cpp Config.cpp
//        /I. /link pdh.lib psapi.lib iphlpapi.lib ws2_32.lib netapi32.lib powrprof.lib gdiplus.lib
//
// Or add as a separate Console project in Visual Studio with pch.h disabled.

#include <windows.h>
#include <iostream>
#include <cassert>
#include "SystemMonitor.h"

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::wcerr << L"FAIL: WSAStartup failed" << std::endl;
        return 1;
    }

    int failures = 0;

    // Test 1: CPU usage in range
    {
        SystemMonitor m;
        m.Collect();
        const auto& info = m.GetSysInfo();
        if (info.cpuUsage < 0.0 || info.cpuUsage > 100.0) {
            std::wcerr << L"FAIL: cpuUsage out of range: " << info.cpuUsage << std::endl;
            failures++;
        } else {
            std::wcout << L"PASS: CPU usage = " << info.cpuUsage << L"%" << std::endl;
        }
    }

    // Test 2: Memory info is positive
    {
        SystemMonitor m;
        m.Collect();
        const auto& info = m.GetSysInfo();
        if (info.totalPhysMem == 0 || info.usedPhysMem == 0) {
            std::wcerr << L"FAIL: Memory info is zero" << std::endl;
            failures++;
        } else {
            std::wcout << L"PASS: Memory = " << (info.usedPhysMem / (1024 * 1024))
                       << L" MB / " << (info.totalPhysMem / (1024 * 1024)) << L" MB" << std::endl;
        }
    }

    // Test 3: Computer name is not empty
    {
        SystemMonitor m;
        m.Collect();
        const auto& info = m.GetSysInfo();
        if (info.computerName.empty()) {
            std::wcerr << L"FAIL: Computer name is empty" << std::endl;
            failures++;
        } else {
            std::wcout << L"PASS: Computer = " << info.computerName << std::endl;
        }
    }

    // Test 4: IP address is present
    {
        SystemMonitor m;
        m.Collect();
        const auto& info = m.GetSysInfo();
        if (info.ipAddress.empty() || info.ipAddress == L"N/A") {
            std::wcout << L"WARN: IP address is " << info.ipAddress << L" (might be OK offline)" << std::endl;
        } else {
            std::wcout << L"PASS: IP = " << info.ipAddress << std::endl;
        }
    }

    // Test 5: Disk list is non-empty
    {
        SystemMonitor m;
        m.Collect();
        const auto& info = m.GetSysInfo();
        if (info.disks.empty()) {
            std::wcerr << L"FAIL: No disks found" << std::endl;
            failures++;
        } else {
            std::wcout << L"PASS: " << info.disks.size() << L" disk(s) found" << std::endl;
            for (auto& d : info.disks) {
                std::wcout << L"  " << d.drive << L" " << d.usedGB << L"/" << d.totalGB << L" GB" << std::endl;
            }
        }
    }

    // Test 6: Network speed delta calculation
    {
        SystemMonitor m;
        m.Collect();
        Sleep(2100);
        m.Collect();
        const auto& net = m.GetNetStats();
        std::wcout << L"PASS: Download = " << SystemMonitor::FormatSpeed(net.downloadSpeed)
                   << L", Upload = " << SystemMonitor::FormatSpeed(net.uploadSpeed) << std::endl;
    }

    // Test 7: Uptime and OS version
    {
        SystemMonitor m;
        m.Collect();
        const auto& info = m.GetSysInfo();
        std::wcout << L"PASS: Uptime = " << info.systemUptime << std::endl;
        std::wcout << L"PASS: OS = " << info.osVersion << std::endl;
    }

    // Test 8: FormatBytes
    {
        auto fmt = SystemMonitor::FormatBytes(1536.0);
        if (fmt.find(L"KB") == std::wstring::npos) {
            std::wcerr << L"FAIL: FormatBytes(1536) = " << fmt << std::endl;
            failures++;
        } else {
            std::wcout << L"PASS: FormatBytes(1536) = " << fmt << std::endl;
        }
    }

    WSACleanup();

    if (failures > 0) {
        std::wcerr << L"\n" << failures << L" test(s) FAILED" << std::endl;
        return 1;
    }
    std::wcout << L"\nAll tests passed." << std::endl;
    return 0;
}
