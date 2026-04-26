#pragma once

#include <string>
#include <vector>
#include <deque>
#include <unordered_map>
#include <windows.h>

struct ProcessInfo {
    DWORD pid = 0;
    std::wstring name;
    double cpuPercent = 0.0;
    SIZE_T memBytes = 0;
};

struct AdapterStats {
    std::wstring name;
    ULONG64 bytesRecv = 0;
    ULONG64 bytesSent = 0;
    ULONG64 prevBytesRecv = 0;
    ULONG64 prevBytesSent = 0;
    double downloadSpeed = 0.0;
    double uploadSpeed = 0.0;
};

struct NetworkStats {
    double downloadSpeed = 0.0;
    double uploadSpeed = 0.0;
    std::vector<AdapterStats> adapters;
};

struct DiskInfo {
    std::wstring drive;
    ULONGLONG totalGB = 0;
    ULONGLONG usedGB = 0;
};

struct SysInfo {
    double cpuUsage = 0.0;
    std::vector<double> perCoreCpu;
    int batteryPercent = 0;
    bool batteryPercentValid = false;
    bool batteryCharging = false;
    bool hasBattery = false;
    DWORDLONG totalPhysMem = 0;
    DWORDLONG usedPhysMem = 0;
    std::wstring ipAddress;
    std::wstring macAddress;
    std::wstring computerName;
    std::wstring domainStatus;
    std::wstring systemUptime;
    std::wstring osVersion;
    std::vector<ProcessInfo> topMemProcs;
    std::vector<ProcessInfo> topCpuProcs;
    std::vector<DiskInfo> disks;
};

class SystemMonitor {
public:
    SystemMonitor();
    ~SystemMonitor();

    SystemMonitor(const SystemMonitor&) = delete;
    SystemMonitor& operator=(const SystemMonitor&) = delete;

    void Collect();
    void SetRefreshIntervalMs(int ms) { m_refreshIntervalMs = ms; }

    const SysInfo& GetSysInfo() const { return m_sysInfo; }
    const NetworkStats& GetNetStats() const { return m_netStats; }
    double GetCpuMax() const { return m_cpuMax; }
    double GetCpuAvg() const { return m_cpuAvg; }
    const std::deque<double>& GetCpuHistory() const { return m_cpuHistory; }

    std::vector<ProcessInfo> GetTopProcessesByCpu(int topN);

    static std::wstring FormatBytes(double bytes);
    static std::wstring FormatSpeed(double bytesPerSec);

private:
    void InitCpuQuery();
    double GetCpuUsage();
    void GetPerCoreCpu();
    void GetBatteryInfo();
    void GetMemoryInfo();
    std::wstring GetIPAddress();
    std::wstring GetMACAddress();
    std::wstring GetComputerNameStr();
    std::wstring GetDomainStatus();
    std::wstring GetSystemUptime();
    std::wstring GetOSVersion();
    void GetNetworkSpeed();
    std::vector<ProcessInfo> GetTopProcessesByMem(int topN);
    std::vector<DiskInfo> GetDiskInfo();

    static void GetAdaptersWithRetry(ULONG family, ULONG flags,
        std::vector<BYTE>& buf, PIP_ADAPTER_ADDRESSES& addrs);

    PDH_HQUERY m_cpuQuery = nullptr;
    PDH_HCOUNTER m_cpuCounter = nullptr;
    PDH_HQUERY m_perCoreQuery = nullptr;
    PDH_HCOUNTER m_perCoreCounter = nullptr;
    std::deque<double> m_cpuHistory;
    int m_collectCount = 0;
    int m_refreshIntervalMs = 2000;
    double m_cpuMax = 0.0;
    double m_cpuAvg = 0.0;
    int m_coreCount = 1;
    std::unordered_map<DWORD, ULONGLONG> m_prevProcCpuTimes;
    ULONGLONG m_lastProcCpuTick = 0;
    NetworkStats m_netStats;
    SysInfo m_sysInfo;
};
