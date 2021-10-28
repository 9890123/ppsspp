#include <string>

#include "ppsspp_config.h"

#include "Common.h"
#include "Common/Log.h"
#include "StringUtils.h"
#include "Common/Data/Encoding/Utf8.h"

#if PPSSPP_PLATFORM(ANDROID)
#include <android/log.h>
#elif PPSSPP_PLATFORM(WINDOWS)
#include "CommonWindows.h"
#endif

#define LOG_BUF_SIZE 2048

bool HandleAssert(const char* function, const char* file, int line, const char* expression, const char* format, ...) {
	char text[LOG_BUF_SIZE];
	const char* caption = "Critical";
	va_list args;
	va_start(args, format);
	vsnprintf(text, sizeof(text), format, args);
	va_end(args);

	char formatted[LOG_BUF_SIZE + 128];
	snprintf(formatted, sizeof(formatted), "(%s:%s:%d) %s: [%s] %s", file, function, line, caption, expression, text);

	ERROR_LOG(SYSTEM, "%s", formatted);

	printf("%s\n", formatted);

#if defined(USING_WIN_UI)
	int msgBoxStyle = MB_ICONINFORMATION | MB_YESNO;
	std::wstring wtext = ConvertUTF8ToWString(formatted) + L"\n\nTry to continue?";
	std::wstring wcaption = ConvertUTF8ToWString(caption);
	OutputDebugString(wtext.c_str());
	if (IDYES != MessageBox(0, wtext.c_str(), wcaption.c_str(), msgBoxStyle)) {
		return false;
	} else {
		return true;
	}
#elif PPSSPP_PLATFORM(ANDROID)s
	__android_log_assert(expression, "PPSSPP", "%s", formatted);

	return false;
#else
	OutputDebugStringUTF8(text);
	return false;
#endif
}
