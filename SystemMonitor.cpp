#include "pch.h"
#include "framework.h"
#include "SystemMonitor.h"
#include "Logger.h"
#include <chrono>

constexpr int CPU_HISTORY_SIZE = 60;
constexpr int TIMER_INTERVAL = 2000;

SystemMonitor::SystemMonitor() {
    InitCpuQuery();
}

SystemMonitor::~SystemMonitor() {
    if (m_perCoreQuery) PdhCloseQuery(m_perCoreQuery);
    if (m_cpuQuery) PdhCloseQuery(m_cpuQuery);
}

void SystemMonitor::InitCpuQuery() {
    PdhOpenQuery(nullptr, 0, &m_cpuQuery);
    PdhAddEnglishCounterW(m_cpuQuery, L"\\Processor(_Total)\\% Processor Time", 0, &m_cpuCounter);
    PdhCollectQueryData(m_cpuQuery);

    PdhOpenQuery(nullptr, 0, &m_perCoreQuery);
    PdhAddEnglishCounterW(m_perCoreQuery, L"\\Processor(*)\\% Processor Time", 0, &m_perCoreCounter);
    PdhCollectQueryData(m_perCoreQuery);

    Sleep(100);
    PdhCollectQueryData(m_cpuQuery);
    PdhCollectQueryData(m_perCoreQuery);
}

void SystemMonitor::GetPerCoreCpu() {
    PdhCollectQueryData(m_perCoreQuery);
    DWORD bufSize = 0;
    DWORD itemCount = 0;
    PdhGetFormattedCounterArrayW(m_perCoreCounter, PDH_FMT_DOUBLE, &bufSize, &itemCount, nullptr);
    if (bufSize == 0) return;

    std::vector<BYTE> buf(bufSize);
    auto* arr = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM_W*>(buf.data());
    if (PdhGetFormattedCounterArrayW(m_perCoreCounter, PDH_FMT_DOUBLE, &bufSize, &itemCount, arr) != ERROR_SUCCESS)
        return;

    std::vector<std::pair<std::wstring, double>> cores;
    for (DWORD i = 0; i < itemCount; i++) {
        std::wstring name(arr[i].szName);
        if (name.find(L"_Total") != std::wstring::npos) continue;
        cores.push_back({ name, arr[i].FmtValue.doubleValue });
    }
    std::sort(cores.begin(), cores.end());

    m_sysInfo.perCoreCpu.clear();
    for (auto& c : cores) {
        m_sysInfo.perCoreCpu.push_back(c.second);
    }
}

double SystemMonitor::GetCpuUsage() {
    PdhCollectQueryData(m_cpuQuery);
    PDH_FMT_COUNTERVALUE val = {};
    DWORD status = PdhGetFormattedCounterValue(m_cpuCounter, PDH_FMT_DOUBLE, nullptr, &val);
    if (status != ERROR_SUCCESS) {
        Logger::Instance().Warn(L"PdhGetFormattedCounterValue failed: " + std::to_wstring(status));
        return 0.0;
    }
    return val.doubleValue;
}

void SystemMonitor::GetBatteryInfo() {
    SYSTEM_POWER_STATUS ps;
    if (GetSystemPowerStatus(&ps)) {
        m_sysInfo.hasBattery = (ps.BatteryFlag != 128 && ps.BatteryFlag != 255);
        m_sysInfo.batteryPercentValid = (ps.BatteryLifePercent != 255);
        m_sysInfo.batteryPercent = m_sysInfo.batteryPercentValid ? ps.BatteryLifePercent : 0;
        m_sysInfo.batteryCharging = (ps.ACLineStatus == 1);
    } else {
        m_sysInfo.hasBattery = false;
        m_sysInfo.batteryPercentValid = false;
        m_sysInfo.batteryPercent = 0;
        m_sysInfo.batteryCharging = false;
    }
}

void SystemMonitor::GetMemoryInfo() {
    MEMORYSTATUSEX ms = {};
    ms.dwLength = sizeof(ms);
    GlobalMemoryStatusEx(&ms);
    m_sysInfo.totalPhysMem = ms.ullTotalPhys;
    m_sysInfo.usedPhysMem = ms.ullTotalPhys - ms.ullAvailPhys;
}

void SystemMonitor::GetAdaptersWithRetry(ULONG family, ULONG flags,
    std::vector<BYTE>& buf, PIP_ADAPTER_ADDRESSES& addrs) {
    ULONG bufLen = 15000;
    DWORD ret;
    do {
        buf.resize(bufLen);
        addrs = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buf.data());
        ret = GetAdaptersAddresses(family, flags, nullptr, addrs, &bufLen);
        if (ret == ERROR_BUFFER_OVERFLOW) {
            bufLen *= 2;
        }
    } while (ret == ERROR_BUFFER_OVERFLOW);
}

std::wstring SystemMonitor::GetIPAddress() {
    std::vector<BYTE> buf;
    PIP_ADAPTER_ADDRESSES addrs = nullptr;
    GetAdaptersWithRetry(AF_INET, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST, buf, addrs);
    if (!addrs) return L"N/A";
    for (auto a = addrs; a; a = a->Next) {
        if (a->OperStatus != IfOperStatusUp) continue;
        if (a->IfType == IF_TYPE_SOFTWARE_LOOPBACK) continue;
        auto ua = a->FirstUnicastAddress;
        if (ua) {
            wchar_t ip[64] = {};
            DWORD ipLen = 64;
            if (WSAAddressToStringW(ua->Address.lpSockaddr, ua->Address.iSockaddrLength,
                                    nullptr, ip, &ipLen) == 0) {
                return ip;
            }
        }
    }
    return L"N/A";
}

std::wstring SystemMonitor::GetMACAddress() {
    std::vector<BYTE> buf;
    PIP_ADAPTER_ADDRESSES addrs = nullptr;
    GetAdaptersWithRetry(AF_INET, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST, buf, addrs);
    if (!addrs) return L"N/A";
    for (auto a = addrs; a; a = a->Next) {
        if (a->OperStatus != IfOperStatusUp) continue;
        if (a->IfType == IF_TYPE_SOFTWARE_LOOPBACK) continue;
        if (a->PhysicalAddressLength == 6) {
            wchar_t mac[32];
            swprintf_s(mac, L"%02X:%02X:%02X:%02X:%02X:%02X",
                a->PhysicalAddress[0], a->PhysicalAddress[1], a->PhysicalAddress[2],
                a->PhysicalAddress[3], a->PhysicalAddress[4], a->PhysicalAddress[5]);
            return mac;
        }
    }
    return L"N/A";
}

std::wstring SystemMonitor::GetComputerNameStr() {
    wchar_t buf[MAX_COMPUTERNAME_LENGTH + 1] = {};
    DWORD size = MAX_COMPUTERNAME_LENGTH + 1;
    GetComputerNameW(buf, &size);
    return buf;
}

std::wstring SystemMonitor::GetDomainStatus() {
    LPWSTR nameBuf = nullptr;
    NETSETUP_JOIN_STATUS status;
    if (NetGetJoinInformation(nullptr, &nameBuf, &status) == NERR_Success) {
        std::wstring result;
        switch (status) {
        case NetSetupDomainName:
            result = L"域: " + std::wstring(nameBuf);
            break;
        case NetSetupWorkgroupName:
            result = L"工作组: " + std::wstring(nameBuf);
            break;
        default:
            result = L"未加入域";
        }
        if (nameBuf) NetApiBufferFree(nameBuf);
        return result;
    }
    return L"未知";
}

void SystemMonitor::GetNetworkSpeed() {
    std::vector<BYTE> buf;
    PIP_ADAPTER_ADDRESSES addrs = nullptr;
    GetAdaptersWithRetry(AF_INET, 0, buf, addrs);
    if (!addrs) return;

    double totalDownload = 0.0;
    double totalUpload = 0.0;
    std::vector<AdapterStats> newAdapters;

    for (auto a = addrs; a; a = a->Next) {
        if (a->OperStatus != IfOperStatusUp) continue;
        if (a->IfType == IF_TYPE_SOFTWARE_LOOPBACK) continue;
        MIB_IF_ROW2 row = {};
        row.InterfaceIndex = a->IfIndex;
        if (GetIfEntry2(&row) != NO_ERROR) continue;

        ULONG64 recv = row.InOctets;
        ULONG64 sent = row.OutOctets;

        AdapterStats ad;
        ad.name = a->FriendlyName ? a->FriendlyName : a->Description;
        ad.bytesRecv = recv;
        ad.bytesSent = sent;

        for (auto& prev : m_netStats.adapters) {
            if (prev.name == ad.name && prev.prevBytesRecv > 0) {
                ad.prevBytesRecv = prev.bytesRecv;
                ad.prevBytesSent = prev.bytesSent;
                double interval = TIMER_INTERVAL / 1000.0;
                ad.downloadSpeed = (recv > prev.bytesRecv ? recv - prev.bytesRecv : 0) / interval;
                ad.uploadSpeed = (sent > prev.bytesSent ? sent - prev.bytesSent : 0) / interval;
                break;
            }
        }
        ad.prevBytesRecv = recv;
        ad.prevBytesSent = sent;
        totalDownload += ad.downloadSpeed;
        totalUpload += ad.uploadSpeed;
        newAdapters.push_back(ad);
    }

    m_netStats.downloadSpeed = totalDownload;
    m_netStats.uploadSpeed = totalUpload;
    m_netStats.adapters = std::move(newAdapters);
}

std::vector<ProcessInfo> SystemMonitor::GetTopProcessesByMem(int topN) {
    std::vector<ProcessInfo> procs;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return procs;
    PROCESSENTRY32W pe = {};
    pe.dwSize = sizeof(pe);
    if (Process32FirstW(snap, &pe)) {
        do {
            HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe.th32ProcessID);
            if (hProc) {
                PROCESS_MEMORY_COUNTERS pmc = {};
                if (GetProcessMemoryInfo(hProc, &pmc, sizeof(pmc))) {
                    ProcessInfo pi;
                    pi.name = pe.szExeFile;
                    pi.memBytes = pmc.WorkingSetSize;
                    pi.cpuPercent = 0;
                    procs.push_back(pi);
                }
                CloseHandle(hProc);
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    std::sort(procs.begin(), procs.end(), [](const ProcessInfo& a, const ProcessInfo& b) {
        return a.memBytes > b.memBytes;
    });
    if ((int)procs.size() > topN) procs.resize(topN);
    return procs;
}

std::vector<DiskInfo> SystemMonitor::GetDiskInfo() {
    std::vector<DiskInfo> disks;
    DWORD drives = GetLogicalDrives();
    for (int i = 0; i < 26; i++) {
        if (!(drives & (1 << i))) continue;
        wchar_t root[4] = { (wchar_t)('A' + i), L':', L'\\', 0 };
        UINT type = GetDriveTypeW(root);
        if (type != DRIVE_FIXED && type != DRIVE_REMOVABLE) continue;
        ULARGE_INTEGER freeBytesAvail, totalBytes, totalFreeBytes;
        if (GetDiskFreeSpaceExW(root, &freeBytesAvail, &totalBytes, &totalFreeBytes)) {
            DiskInfo di;
            di.drive = root;
            di.drive.pop_back();
            di.totalGB = totalBytes.QuadPart / (1024ULL * 1024 * 1024);
            di.usedGB = (totalBytes.QuadPart - totalFreeBytes.QuadPart) / (1024ULL * 1024 * 1024);
            disks.push_back(di);
        }
    }
    return disks;
}

std::wstring SystemMonitor::GetSystemUptime() {
    ULONGLONG ms = GetTickCount64();
    int days = (int)(ms / 86400000);
    ms %= 86400000;
    int hours = (int)(ms / 3600000);
    ms %= 3600000;
    int minutes = (int)(ms / 60000);
    wchar_t buf[64];
    swprintf_s(buf, L"%d 天 %d 小时 %d 分钟", days, hours, minutes);
    return buf;
}

std::wstring SystemMonitor::GetOSVersion() {
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    if (!ntdll) return L"未知";
    typedef LONG(WINAPI * RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);
    auto RtlGetVersion = (RtlGetVersionPtr)GetProcAddress(ntdll, "RtlGetVersion");
    if (!RtlGetVersion) return L"未知";
    RTL_OSVERSIONINFOW osvi = {};
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    if (RtlGetVersion(&osvi) != 0) return L"未知";
    wchar_t buf[64];
    swprintf_s(buf, L"Windows %lu.%lu Build %lu", osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber);
    return buf;
}

void SystemMonitor::Collect() {
    auto start = std::chrono::steady_clock::now();

    m_sysInfo.cpuUsage = GetCpuUsage();
    GetPerCoreCpu();
    m_cpuHistory.push_back(m_sysInfo.cpuUsage);
    if ((int)m_cpuHistory.size() > CPU_HISTORY_SIZE) m_cpuHistory.pop_front();
    m_cpuMax = *std::max_element(m_cpuHistory.begin(), m_cpuHistory.end());
    double sum = 0;
    for (auto v : m_cpuHistory) sum += v;
    m_cpuAvg = m_cpuHistory.empty() ? 0 : sum / m_cpuHistory.size();

    GetBatteryInfo();
    GetMemoryInfo();
    m_sysInfo.ipAddress = GetIPAddress();
    m_sysInfo.macAddress = GetMACAddress();
    m_sysInfo.computerName = GetComputerNameStr();
    m_sysInfo.domainStatus = GetDomainStatus();
    m_sysInfo.systemUptime = GetSystemUptime();
    m_sysInfo.osVersion = GetOSVersion();
    GetNetworkSpeed();
    m_collectCount++;
    if (m_collectCount % 3 == 1) {
        m_sysInfo.topMemProcs = GetTopProcessesByMem(5);
    }
    m_sysInfo.disks = GetDiskInfo();

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start).count();
    if (elapsed > 500) {
        Logger::Instance().Warn(L"Collect() took " + std::to_wstring(elapsed) + L"ms");
    }
}

std::wstring SystemMonitor::FormatBytes(double bytes) {
    const wchar_t* units[] = { L"B", L"KB", L"MB", L"GB", L"TB" };
    int idx = 0;
    while (bytes >= 1024.0 && idx < 4) { bytes /= 1024.0; idx++; }
    wchar_t buf[64];
    swprintf_s(buf, L"%.1f %s", bytes, units[idx]);
    return buf;
}

std::wstring SystemMonitor::FormatSpeed(double bytesPerSec) {
    return FormatBytes(bytesPerSec) + L"/s";
}
