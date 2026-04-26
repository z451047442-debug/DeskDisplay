#pragma once

#include "targetver.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <objidl.h>
#include <stdlib.h>
#include <tchar.h>

#include <string>
#include <vector>
#include <algorithm>
#include <cstdio>
#include <deque>

#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")

#include <pdh.h>
#pragma comment(lib, "pdh.lib")

#include <psapi.h>
#pragma comment(lib, "psapi.lib")

#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

#include <tlhelp32.h>
#include <powrprof.h>
#pragma comment(lib, "powrprof.lib")

#define _LMERRLOG_
#define _LMAUDIT_
#include <lm.h>
#pragma comment(lib, "netapi32.lib")
