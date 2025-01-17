#include "Utils.h"
#include <cstdarg>

void Trace(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	char buf[10240];
	vsprintf_s(buf, fmt, args);
	va_end(args);
#if 0
	va_list args;
	va_start(args, fmt);

	// Determine the required buffer size
	int size = vsnprintf(nullptr, 0, fmt, args) + 1; // +1 for null terminator
	va_end(args);

	if (size <= 0) {
		return; // Error determining buffer size
	}

	// Allocate buffer
	std::vector<char> buf(size);

	// Format the string
	va_start(args, fmt);
	vsnprintf(buf.data(), buf.size(), fmt, args);
	va_end(args);

	// Output the formatted string
	printf("%s", buf.data());
#endif
}