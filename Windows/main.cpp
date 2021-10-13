#include "stdafx.h"
#include <algorithm>
#include <cmath>
#include <functional>

#include "Common/CommonWindow.h"
#include "Common/File/FileUtil.h"
#include "Common/OSVersion.h"
#include "Common/GPU/Vulkan/VulkanLoader.h"
#include "ppsspp_config.h"

#include <Wbemidl.h>
#include <shellapi.h>
#include <ShlObj.h>
#include <mmsystem.h>

#include "Common/System/Display.h"
#include "Common/System/NativeApp.h"
#include "Common/System/System.h"
#include "Common/File/VFS/VFS.h"
#include "Common/File/VFS/AssetReader.h"
#include "Common/Data/Text/I18n.h"
#include "Common/Profiler/Profiler.h"
#include "Common/Thread/ThreadUtil.h"
#include "Common/Data/Encoding/Utf8.h"
#include "Common/Net/Resolve.h"

#include "Core/Config.h"
#include "Core/ConfigValues.h"
#include "Core/SaveState.h"
#include "Windows/EmuThread.h"
#include "Windows/WindowsAudio.h"
#include "ext/disarm.h"

#include "Common/LogManager.h"
#include "Common/ConsoleListener.h"
#include "Common/StringUtils.h"

#include "Commctrl.h"

#include "UI/GameInfoCache.h"
#include "Windows/resource.h"

#include "Windows/MainWindow.h"
#include "Windows/Debugger/Debugger_Disasm.h"
#include "Windows/Debugger/Debugger_MemoryDlg.h"
#include "Windows/Debugger/Debugger_VFPUDlg.h"
#if PPSSPP_API(ANY_GL)
#include "WIndows/GEDebugger/GEDebugger.h"
#endif
#include "Windows/W32Util/DialogManager.h"
#include "WIndows/W32Util/ShellUtil.h"

#include "Windows/Debugger/CtrlDisAsmView.h"
#include "Windows/Debugger/CtrlMemView.h"
#include "Windows/Debugger/CtrlRegisterList.h"
#include "Windows/InputBox.h"

#include "Windows/WindowsHost.h"
#include "Windows/main.h"

extern "C" {
	__declspec(dllexport) DWORD NvOptimusEnablement = 1;
}

extern "C" {
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#if PPSSPP_API(ANY_GL)
CGEDebugger* geDebuggerWindow = nullptr;
#endif

CDisam *disasmWindow = nullptr;
CMemoryDlg *memoryWindow = nullptr;

static std::string langRegion;
static std::string osName;
static std::string gpuDriverVersion;

static std::string restartArgs;

HMENU g_hPopupMenus;
int g_activeWindow = 0;

static std::thread inputBoxThread;
static bool inputBoxRunning = false;

void OpenDirectory(const char* path) {
	PIDLIST_ABSOLUTE pidl = ILCreateFromPath(ConvertUTF8ToWString(ReplaceAll(path, "/", "\\")).c_str());
	if (pidl) {
		SHOpenFolderAndSelectItems(pidl, 0, NULL, 0);
		ILFree(pidl);
	}
}

void LaunchBrowser(const char* url) {
	ShellExecute(NULL, L"open", ConvertUTF8ToWString(url).c_str(), NULL, NULL, SW_SHOWNORMAL);
}

void Vibrate(int length_ms) {

}

std::string GetVideoCardDriverVersion() {
	std::string retvalue = "";

	HRESULT hr;
	hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (FAILED(hr)) {
		return retvalue;
	}

	IWbemLocator* pIWbemLocator = NULL;
	hr = CoCreateInstance(__uuidof(WbemLocator), NULL, CLSCTX_INPROC_SERVER,
		__uuidof(IWbemLocator), (LPVOID *)&pIWbemLocator);
	if (FAILED(hr)) {
		CoUninitialize();
		return retvalue;
	}

	BSTR bstrServer = SysAllocString(L"\\\\.\\root\\cimv2");
	IWbemServices *pIWbemServices;
	hr = pIWbemLocator->ConnectServer(bstrServer, NULL, NULL, 0L, 0L, NULL, NULL, &pIWbemServices);
	if (FAILED(hr)) {
		pIWbemLocator->Release();
		SysFreeString(bstrServer);
		CoUninitialize();
		return retvalue;
	}

	hr = CoSetProxyBlanket(pIWbemServices, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE,
		NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_DEFAULT);

	BSTR bstrWQL = SysAllocString(L"WQL");
	BSTR bstrPath = SysAllocString(L"select * from Win32_VideoController");
	IEnumWbemClassObject* pEnum;
	hr = pIWbemServices->ExecQuery(bstrWQL, bstrPath, WBEM_FLAG_FORWARD_ONLY, NULL, &pEnum);

	ULONG uReturned;
	VARIANT var;
	IWbemClassObject* pObj = NULL;
	if (!FAILED(hr)) {
		hr = pEnum->Next(WBEM_INFINITE, 1, &pObj, &uReturned);
	}

	if (!FAILED(hr) && uReturned) {
		hr = pObj->Get(L"DriverVersion", 0, &var, NULL, NULL);
		if (SUCCEEDED(hr)) {
			char str[MAX_PATH];
			WideCharToMultiByte(CP_ACP, 0, var.bstrVal, -1, str, sizeof(str), NULL, NULL);
			retvalue = str;
		}
	}

	pEnum->Release();
	SysFreeString(bstrPath);
	SysFreeString(bstrWQL);
	pIWbemServices->Release();
	pIWbemLocator->Release();
	SysFreeString(bstrServer);
	CoUninitialize();
	return retvalue;
}

std::string System_GetProperty(SystemProperty prop) {
	static bool hasCheckedGPUDriverVersion = false;
	switch (prop) {
	case SYSPROP_NAME:
		return osName;
	case SYSPROP_LANGREGION:
		return langRegion;
	case SYSPROP_CLIPBOARD_TEXT:
		{
			std::string retval;
			if (OpenClipboard(MainWindow::GetDisplayHWND())) {
				HANDLE handle = GetClipboardData(CF_UNICODETEXT);
				const wchar_t* wstr = (const wchar_t*)GlobalLock(handle);
				if (wstr)
					retval = ConvertWStringToUTF8(wstr);
				else
					retval = "";
				GlobalUnlock(handle);
				CloseClipboard();
			}
			return retval;
		}
	case SYSPROP_GPUDRIVER_VERSION:
		if (!hasCheckedGPUDriverVersion) {
			hasCheckedGPUDriverVersion = true;
			gpuDriverVersion = GetVideoCardDriverVersion();
		}
		return gpuDriverVersion;
	default:
		return "";
	}
}

std::vector<std::string> System_GetPropertyStringVec(SystemProperty prop) {
	std::vector<std::string> result;
	switch (prop) {
	case SYSPROP_TEMP_DIRS:
	{
		std::wstring tempPath(MAX_PATH, '\0');
		size_t sz = GetTempPath((DWORD)tempPath.size(), &tempPath[0]);
		if (sz >= tempPath.size()) {
			tempPath.resize(sz);
			sz = GetTempPath((DWORD)tempPath.size(), &tempPath[0]);
		}

		tempPath.resize(sz);
		result.push_back(ConvertWStringToUTF8(tempPath));

		if (getenv("TMPDIR") && strlen(getenv("TMPDIR")) != 0)
			result.push_back(getenv("TMPDIR"));
		if (getenv("TMP") && strlen(getenv("TMP")) != 0)
			result.push_back(getenv("TMP"));
		if (getenv("TEMP") && strlen(getenv("TEMP")) != 0)
			result.push_back(getenv("TEMP"));
		return result;
	}

	default:
		return result;
	}
}

extern WindowsAudioBackend *winAudioBackend;

#ifdef _WIN32
#if PPSSPP_PLATFORM(UWP)
static int ScreenDPI() {
	return 96;
}
#else
static int ScreenDPI() {
	HDC screenDC = GetDC(nullptr);
	int dotsPerInch = GetDeviceCaps(screenDC, LOGPIXELSY);
	ReleaseDC(nullptr, screenDC);
	return dotsPerInch ? dotsPerInch : 96;
}
#endif
#endif

int System_GetPropertyInt(SystemProperty prop) {
	switch (prop) {
	case SYSPROP_AUDIO_SAMPLE_RATE:
		return winAudioBackend ? winAudioBackend->GetSampleRate() : -1;
	case SYSPROP_DEVICE_TYPE:
		return DEVICE_TYPE_DESKTOP;
	case SYSPROP_DISPLAY_COUNT:
		return GetSystemMetrics(SM_CMONITORS);
	default:
		return -1;
	}
}

float System_GetPropertyFloat(SystemProperty prop) {
	switch (prop) {
	case SYSPROP_DISPLAY_REFRESH_RATE:
		return 60.f;
	case SYSPROP_DISPLAY_DPI:
		return (float)ScreenDPI();
	case SYSPROP_DISPLAY_SAFE_INSET_LEFT:
	case SYSPROP_DISPLAY_SAFE_INSET_RIGHT:
	case SYSPROP_DISPLAY_SAFE_INSET_TOP:
	case SYSPROP_DISPLAY_SAFE_INSET_BOTTOM:
		return 0.0f;
	default:
		return -1;
	}
}

bool System_GetPropertyBool(SystemProperty prop) {
	switch (prop) {
	case SYSPROP_HAS_FILE_BROWSER:
	case SYSPROP_HAS_FOLDER_BROWSER:
		return true;
	case SYSPROP_HAS_IMAGE_BROWSER:
		return true;
	case SYSPROP_HAS_BACK_BUTTON:
		return true;
	case SYSPROP_APP_GOLD:
#ifdef GOLD
		return true;
#else
		return false;
#endif
	case SYSPROP_CAN_JIT:
		return true;
	default:
		return false;
	}
}

void System_SendMessage(const char* command, const char* parameter) {
	if (!strcmp(command, "finish")) {
		if (!NativeIsRestarting()) {
			PostMessage(MainWindow::GetHWND(), WM_CLOSE, 0, 0);
		}
	}
	else if (!strcmp(command, "graphics_restart")) {
		restartArgs = parameter == nullptr ? "" : parameter;
		if (IsDebuggerPresent()) {
			PostMessage(MainWindow::GetHWND(), MainWindow:WM_USER_RESTART_EMUTHREAD, 0, 0);
		} else {
			g_Config.bRestartRequired = true;
			PostMessage(MainWindow::GetHWND(), WM_CLOSE, 0, 0);
		}
	} else if (!strcmp(command, "graphics_failedBackend")) {
		auto err = GetI18NCategory("Error");
		const char* backendSwitchError = err->T("GenericBackendSwitchCrash", "PPSSPP crashed while starting. This usually means a graphics driver problem. Try upgrading your graphics drivers.\n\nGraphics backend has been switched:");
		std::wstring full_error = ConvertUTF8ToWString(StringFromFormat("%s %s", backendSwitchError, parameter));
		std::wstring title = ConvertUTF8ToWString(err->T("GenericGraphicsError", "Graphics Error"));
		MessageBox(MainWindow::GetHWND(), full_error.c_str(), title.c_str(), MB_OK);
	} else if (!strcmp(command, "setclipboardtext")) {
		if (OpenClipboard(MainWindow::GetDisplayHWND())) {
			std::wstring data = ConvertUTF8ToWString(parameter);
			HANDLE handle = GlobalAlloc(GMEM_MOVEABLE, (data.size() + 1) * sizeof(wchar_t));
			wchar_t* wstr = (wchar_t*)GlobalLock(handle);
			memcpy(wstr, data.c_str(), (data.size() + 1) * sizeof(wchar_t));
			GlobalUnlock(wstr);
			SetClipboardData(CF_UNICODETEXT, handle);
			GlobalFree(handle);
			CloseClipboard();
		}
	} else if (!strcmp(command, "browse_title")) {
		MainWindow::BrowseAndBoot("");
	} else if (!strcmp(command, "browse_folder")) {
		auto mm = GetI18NCategory("MainMenu");
		std::string folder = W32Util::BrowseForFolder(MainWindow::GetHWND(), mm->T("Choose folder"));
		if (folder.size())
			NativeMessageReceived("browse_folderSelect", folder.c_str());
	} else if (!strcmp(command, "bgImage_browse")) {
		MainWindow::BrowseBackground();
	} else if (!strcmp(command, "toggle_fullscreen")) {
		bool flag = !MainWindow::IsFullscreen();
		if (strcmp(parameter, "0") == 0) {
			flag = false;
		} else if (strcmp(parameter, "1") == 0) {
			flag = true;
		}
		MainWindow::SendToggleFullscreen(flag);
	}
}

void System_AskForPermission(SystemPermission permission) {}
PermissionStatus System_GetPermissionStatus(SystemPermission permission) { return PERMISSION_STATUS_GRANTED; }

void EnableCrashingOnCrashes() {
	typedef BOOL (WINAPI *tGetPolicy)(LPDWORD lpFlags);
	typedef BOOL (WINAPI *tSetPolicy)(DWORD dwFlags);
	const DWORD EXCEPTION_SWALLOWING = 0x1;

	HMODULE kernel32 = LoadLibrary(L"kernel32.dll");
	tGetPolicy pGetPolicy = (tGetPolicy)GetProcAddress(kernel32,
		"GetProcessUserModeExceptionPolicy");
	tSetPolicy pSetPolicy = (tSetPolicy)GetProcAddress(kernel32,
		"SetProcessUserModeExceptionPolicy");
	if (pGetPolicy && pSetPolicy) {
		DWORD dwFlags;
		if (pGetPolicy(&dwFlags)) {
			pSetPolicy(dwFlags & ~EXCEPTION_SWALLOWING);
		}
	}
	FreeLibrary(kernel32);
}

void System_InputBoxGetString(const std::string &title, const std::string &defaultValue, std::function<void(bool, const std::string &)> cb) {
	if (inputBoxRunning) {
		inputBoxThread.join();
	}

	inputBoxRunning = true;
	inputBoxThread = std::thread([=] {
		std::string out;
		if (InputBox_GetString(MainWindow::GetHInstance(), MainWindow::GetHWND(), ConvertUTF8ToWString(title).c_str(), defaultValue, out)) {
			NativeInputBoxReceived(cb, true, out);
		} else {
			NativeInputBoxReceived(cb, false, "");
		}
	});
}

static std::string GetDefaultLangRegion() {
	wchar_t lcLangName[256] = {};

	if (0 != GetLocaleInfo(LOCALE_NAME_USER_DEFAULT, LOCALE_SNAME, lcLangName, ARRAY_SIZE(lcLangName))) {
		std::string result = ConvertWStringToUTF8(lcLangName);
		std::replace(result.begin(), result.end(), '-', '_');
		return result;
	} else {
		if (0 != GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SISO639LANGNAME, lcLangName, ARRAY_SIZE(lcLangName))) {
			wchar_t lcRegion[256] = {};
			if (0 != GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SISO3166CTRYNAME, lcRegion, ARRAY_SIZE(lcRegion))) {
				return ConvertWStringToUTF8(lcLangName) + "_" + ConvertWStringToUTF8(lcRegion);
			}
		}
	}

	return "en_US";
}

static const int EXIT_CODE_VULKAN_WORKS = 42;

#ifndef _DEBUG
static bool DetectVulkanInExternalProcess() {
	std::wstring workingDirectory;
	std::wstring moduleFilename;
	W32Util::GetSelfExecuteParams(workingDirectory, moduleFileName);

	const wchar_t* cmdline = L"--vulkan-available-check";

	SHELLEXECUTEINFO info{ sizeof(SHELLEXECUTEINFO) };
	info.fMask = SEE_MASK_NOCLOSEPROCESS;
	info.lpFile = moduleFilename.c_str();
	info.lpParameters = cmdline;
	info.lpDirectory = workingDirectory.c_str();
	info.nShow = SW_HIDE;
	if (ShellExecuteEx(&info) != TRUE) {
		return false;
	}
	if (info.hProcess == nullptr) {
		return false;
	}

	DWORD result = WaitForSingleObject(info.hProcess, 10000);
	DWORD exitCode = 0;
	if (result == WAIT_FAILED || GetExitCodeProcess(info.hProcess, &exitCode) == 0) {
		CloseHandle(info.hProcess);
		return false;
	}
	CloseHandle(info.hProcess);

	return exitCode == EXIT_CODE_VULKAN_WORKS;
}
#endif

std::vector<std::wstring> GetWideCmdLine() {
	wchar_t** wargv;
	int wargc = -1;

	if (!restartArgs.empty()) {
		std::wstring wargs = ConvertUTF8ToWString("PPSSPP " + restartArgs);
		wargv = CommandLineToArgvW(wargs.c_str(), &wargc);
		restartArgs.clear();
	} else {
		wargv = CommandLineToArgvW(GetCommandLineW(), &wargc);
	}

	std::vector<std::wstring> wideArgs(wargv, wargv + wargc);
	LocalFree(wargv);

	return wideArgs;
}

static void WinMainInit() {
	CoInitializeEx(NULL, COINIT_MULTITHREADED);
	net::Init();

	INITCOMMONCONTROLSEX comm;
	comm.dwSize = sizeof(comm);
	comm.dwICC = ICC_BAR_CLASSES | ICC_LISTVIEW_CLASSES | ICC_TAB_CLASSES;
	InitCommonControlsEx(&comm);

	EnableCrashingOnCrashes();

#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
	PROFILE_INIT();

#if PPSSPP_ARCH(AMD64) && defined(_MSC_VER) && _MSC_VER < 1900
	_set_FMA3_enable(0);
#endif
}

static void WinMainCleanup() {
	if (inputBoxRunning) {
		inputBoxThread.join();
		inputBoxRunning = false;
	}
	net::Shutdown();
	CoUninitialize();

	if (g_Config.bRestartRequired) {
		W32Util::ExitAndRestart(!restartArgs.empty(), restartArgs);
	}
}

int WINAPI WinMain(HINSTANCE _hInstance, HINSTANCE hPrevInstance, LPSTR szCmdLine, int iCmdShow) {
	SetCurrentThreadName("Main");

	WinMainInit();

#ifndef _DEBUG
	bool showLog = false;
#else
	bool showLog = true;
#endif

	const Path& exePath = File::GetExeDirectory();
	VFSRegister("", new DirectoryAssetReader(exePath / "assets"));
	VFSRegister("", new DirectoryAssetReader(exePath));

	langRegion = GetDefaultLangRegion();
	osName = GetWindowsVersion() + " " + GetWindowsSystemArchitecture();

	std::string configFilename = "";
	const std::wstring configOption = L"--config=";

	std::string controlsConfigFilename = "";
	const std::wstring controlsOption = L"--controlconfig=";

	std::vector<std::wstring> wideArgs = GetWideCmdLine();

	for (size_t i = 1; i < wideArgs.size(); ++i) {
		if (wideArgs[i][0] == L'\0')
			continue;
		if (wideArgs[i][0] == L'-') {
			if (wideArgs[i].find(configOption) != std::wstring::npos && wideArgs[i].size() > configOption.size()) {
				const std::wstring tempWide = wideArgs[i].substr(configOption.size());
				configFilename = ConvertWStringToUTF8(tempWide);
			}

			if (wideArgs[i].find(controlsOption) != std::wstring::npos && wideArgs[i].size() > controlsOption.size()) {
				const std::wstring tempWide = wideArgs[i].substr(controlsOption.size());
				controlsConfigFilename = ConvertWStringToUTF8(tempWide);
			}
		}
	}

	LogManager::Init(&g_Config.bEnableLogging);

	g_Config.internalDataDirectory = Path(W32Util::UserDocumentsPath());
	InitSysDirectories();

	for (size_t i = 1; i < wideArgs.size(); ++i) {
		if (wideArgs[i][0] == L'-') {
			if (wideArgs[i] == L"--vulkan-available-check") {
				bool result = VulkanMayBeAvailable();

				LogManager::Shutdown();
				WinMainCleanup();
				return result ? EXIT_CODE_VULKAN_WORKS : EXIT_FAILURE;
			}
		}
	}

	g_Config.SetSearchPath(GetSysDirectory(DIRECTORY_SYSTEM));
	g_Config.load(configFilename.c_str(), controlsConfigFilename.c_str());

	bool debugLogLevel = false;

	const std::wstring gpuBackend = L"--graphics=";

	for (size_t i = 1; i < wideArgs.size(); ++i) {
		if (wideArgs[i][0] == L'\0')
			continue;

		if (wideArgs[i][0] == L'-') {
			switch (wideArgs[i][1]) {
			case L'l':
				showLog = true;
				g_Config.bEnableLogging = true;
				break;
			case L's':
				g_Config.bAutoRun = false;
				g_Config.bSaveSettings = false;
				break;
			case L'd':
				debugLogLevel = true;
				break;
			}

			if (wideArgs[i].find(gpuBackend) != std::wstring::npos && wideArgs[i].size() > gpuBackend.size()) {
				const std::wstring restOfOption = wideArgs[i].substr(gpuBackend.size());

				if (restOfOption == L"directx9") {
					g_Config.iGPUBackend = (int)GPUBackend::DIRECT3D9;
					g_Config.bSoftwareRendering = false;
				} else if (restOfOption == L"directx11") {
					g_Config.iGPUBackend = (int)GPUBackend::DIRECT3D11;
					g_Config.bSoftwareRendering = false;
				} else if (restOfOption == L"gles") {
					g_Config.iGPUBackend = (int)GPUBackend::OPENGL;
					g_Config.bSoftwareRendering = false;
				} else if (restOfOption == L"vulkan") {
					g_Config.iGPUBackend = (int)GPUBackend::VULKAN;
					g_Config.bSoftwareRendering = false;
				} else if (restOfOption == L"software") {
					g_Config.iGPUBackend = (int)GPUBackend::OPENGL;
					g_Config.bSoftwareRendering = true;
				}
			}
		}
	}
#ifdef _DEBUG
	g_Config.bEnableLogging = true;
#endif

#ifndef _DEBUG
	if (g_Config.IsBackendEnabled(GPUBackend::VULKAN, false)) {
		VulkanSetAvailable(DetectVulkanInExternalProcess());
	}
#endif

	if (iCmdShow == SW_MAXIMIZE) {
		g_Config.bFullScreen = true;
	}

	LogManager::GetInstance()->GetConsoleListener()->Init(showLog, 150, 120, "PPSSPP Debug Console");

	if (debugLogLevel) {
		LogManager::GetInstance()->SetAllLogLevels(LogTypes::LDEBUG);
	}

	timeBeginPeriod(1);

	MainWindow::Init(_hInstance);

	g_hPopupMenus = LoadMenu(_hInstance, (LPCWSTR)IDR_POPUPMENUS);

	MainWindow::Show(_hInstance);

	HWND hwndMain = MainWindow::GetHWND();

	CtrlDisAsmView::init();
	CtrlMemView::init();
	CtrlRegisterList::init();
#if PPSSPP_API(ANY_GL)
	CGEDebugger::Init();
#endif
	DialogManager::AddDlg(vfpudlg = new CVFPUDlg(_hInstance, hwndMain, currentDebugMIPS));

	MainWindow::CreateDebugWindows();

	const bool minimized = iCmdShow == SW_MINIMIZE || iCmdShow == SW_SHOWMINIMIZED || iCmdShow == SW_SHOWMINNOACTIVE;
	if (minimized) {
		MainWindow::Minimize();
	}

	MainThread_Start(g_Config.iGPUBackend == (int)GPUBackend::OPENGL);
	InputDevice::BeginPolling();

	HACCEL hAccelTable = LoadAccelerators(_hInstance, (LPCTSTR)IDR_ACCELS);
	HACCEL hDebugAccelTable = LoadAccelerators(_hInstance, (LPCTSTR)IDR_DEBUGACCELS);

	for (Msg msg; GetMessage(&msg, NULL, 0, 0); ) {
		if (msg.message == WM_KEYDOWN) {
			MainWindow::UpdateCommands();

			if (msg.hwnd != hwndMain && msg.wParam == VK_ESCAPE)
				BringWindowToTop(hwndMain);
		}

		HWND wnd;
		HACCEL accel;
		switch (g_activeWindow) {
		case WINDOW_MAINWINDOW:
			wnd = hwndMain;
			accel = g_Config.bSystemControls ? hAccelTable : NULL;
			break;
		case WINDOW_CPUDEBUGGER:
			wnd = disasmWindow ? disasmWindow->GetDlgHandle() : NULL;
			accel = g_Config.bSystemControls ? hDebugAccelTable : NULL;
			break;
		case WINDOW_GEDEBUGGER:
		default:
			wnd = NULL;
			accel = NULL;
			break;
		}

		if (!TranslateAccelerator(wnd, accel, &msg)) {
			if (!DialogManager::IsDialogMessage(&msg)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}

	MainThread_Stop();

	VFSShutdown();

	MainWindow::DestroyDebugWindows();
	DialogManager::DestroyAll();
	timeEndPeriod(1);

	LogManager::Shutdown();
	WinMainCleanup();

	return 0;
}
