#pragma conce

#include "Common.h"

#include <string>
#include <vector>

#ifdef _MSC_VER
#define strncasecmp _strnicmp
#define strcasecmp _stricmp
#else
#include <string.h>
#endif

std::string LineNumberString(const std::string& str);
std::string IndentString(const std::string& str, const std::string &sep, bool skipFirst = false);

inline bool startsWith(const std::string& str, const std::string& what) {
	if (str.size() < what.size())
		return false;
	return str.substr(0, what.size()) == what;
}

inline bool endsWith(const std::string& str, const std::string& what) {
	if (str.size() < what.size())
		return false;
	return str.substr(str.size() - what.size()) == what;
}

inline bool startsWithNoCase(const std::string& str, const std::string& what) {
	if (str.size() < what.size())
		return false;
	return strncasecmp(str.c_str(), what.c_str(), what.size()) == 0;
}

inline bool endsWithNoCase(const std::string& str, const std::string& what) {
	if (str.size() < what.size())
		return false;
	const size_t offset = str.size() - what.size();
	return strncasecmp(str.c_str() + offset, what.c_str(), what.size()) == 0;
}

void DataToHexString(const uint8_t* data, size_t size, std::string* output);
void DataToHexString(int indent, uint32_t startAddr, const uint8_t* data, size_t size, std::string* output);

std::string StringFromFormat(const char* format, ...);
std::string StringFromInt(int value);

std::string StripSpaces(const std::string& s);
std::string StripQuotes(const std::string& s);

void SplitString(const std::string& str, const char delim, std::vector<std::string>& output);

void GetQuotedStrings(const std::string& str, std::vector<std::string>& output);

std::string ReplaceAll(std::string input, const std::string& src, const std::string& dest);

void SkipSpace(const char **ptr);

void truncate_cpy(char *dest, size_t destSize, const char *src);
template <size_t Count>
inline void truncate_cpy(char(&out)[Count], const char* src) {
	truncate_cpy(out, Count, src);
}

long parseHexLong(std::string s);
long parseLong(std::string s);
std::string StringFromFormat(const char* format, ...);

bool CharArrayFromFormatV(char* out, int outsize, const char* format, va_list args);

template<size_t Count>
inline void CharArrayFromFormat(char(&out)[Count], const char* format, ...)
{
	va_list args;
	va_start(args, format);
	CharArrayFromFormatV(out, Count, format, args);
	va_end(args);
}

bool SplitPath(const std::string& full_path, std::string* _pPath, std::string* _pFilename, std::string* _pExtension);
