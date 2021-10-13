#pragma once

#undef _WIN32_WINNT

#if defined(_MSC_VER) && _MSC_VER < 1700
#error You need a newer version of Visual Studio
#else
#define _WIN32_WINNT 0x601
#endif

#undef WINVER
#define WINVER _WIN32_WINNT
#ifndef _WIN32_IE
#define _WIN32_IE 0x0600
#endif

#undef _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES
#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES 1

#ifndef __clang__
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <WinSock2.h>

#include "Common/CommonWindows.h"

#include <windowsx.h>
#include <process.h>
#include <tchar.h>
#include <stdio.h>

#define _USE_MATH_DEFINES
#include <math.h>
#include <time.h>
#include <vector>
#include <map>
#include <string>

#include "Common/Log.h"
