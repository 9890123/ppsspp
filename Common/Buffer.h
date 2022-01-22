#pragma once

#include <string>
#include <vector>

#include "Common/Common.h"

class Path;

class Buffer {
public:
	Buffer();
	Buffer(Buffer&&) = default;
	~Buffer();

	static Buffer Void() {
		Buffer buf;
		buf.void_ = true;
		return buf;
	}

	char* Append(size_t length);

	void Append(const char* str);
	void Append(const std::string& str);
	void Append(const Buffer& other);

	void AppendValue(int value);

	int OffsetToAfterNextCRLF();

	void Take(size_t length, std::string* dest);
	void Take(size_t length, char* dest);
	void TakeAll(std::string* dest) { Take(size(), dest); }

	int TakeLineCRLF(std::string * dest);

	void Skip(size_t length);

	int SkipLineCRLF();

	void Printf(const char* fmt, ...);

	void PeekAll(std::string * dest);

	bool FlushToFile(const Path & filename);

	size_t size() const { return data_.size(); }
	bool empty() const { return size() == 0; }
	void clear() { data_.resize(0); }
	bool IsVoid() { return void_; }

protected:
	std::vector<char> data_;
	bool void_ = false;

private:
	DISALLOW_COPY_AND_ASSIGN(Buffer);
};
