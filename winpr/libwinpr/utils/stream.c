/*
 * WinPR: Windows Portable Runtime
 * Stream Utils
 *
 * Copyright 2011 Vic Lee
 * Copyright 2012 Marc-Andre Moreau <marcandre.moreau@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <winpr/config.h>

#include <winpr/assert.h>
#include <winpr/crt.h>
#include <winpr/stream.h>

#include "stream.h"
#include "../log.h"

#define STREAM_TAG WINPR_TAG("wStream")

#define STREAM_ASSERT(cond)                                                            \
	do                                                                                 \
	{                                                                                  \
		if (!(cond))                                                                   \
		{                                                                              \
			WLog_FATAL(STREAM_TAG, "%s [%s:%s:%" PRIuz "]", #cond, __FILE__, __func__, \
			           (size_t)__LINE__);                                              \
			winpr_log_backtrace(STREAM_TAG, WLOG_FATAL, 20);                           \
			abort();                                                                   \
		}                                                                              \
	} while (0)

BOOL Stream_EnsureCapacity(wStream* s, size_t size)
{
	WINPR_ASSERT(s);
	if (s->capacity < size)
	{
		size_t position = 0;
		size_t old_capacity = 0;
		size_t new_capacity = 0;
		BYTE* new_buf = NULL;

		old_capacity = s->capacity;
		new_capacity = old_capacity;

		do
		{
			new_capacity *= 2;
		} while (new_capacity < size);

		position = Stream_GetPosition(s);

		if (!s->isOwner)
		{
			new_buf = (BYTE*)malloc(new_capacity);
			CopyMemory(new_buf, s->buffer, s->capacity);
			s->isOwner = TRUE;
		}
		else
		{
			new_buf = (BYTE*)realloc(s->buffer, new_capacity);
		}

		if (!new_buf)
			return FALSE;
		s->buffer = new_buf;
		s->capacity = new_capacity;
		s->length = new_capacity;
		ZeroMemory(&s->buffer[old_capacity], s->capacity - old_capacity);

		Stream_SetPosition(s, position);
	}
	return TRUE;
}

BOOL Stream_EnsureRemainingCapacity(wStream* s, size_t size)
{
	if (Stream_GetPosition(s) + size > Stream_Capacity(s))
		return Stream_EnsureCapacity(s, Stream_Capacity(s) + size);
	return TRUE;
}

wStream* Stream_New(BYTE* buffer, size_t size)
{
	wStream* s = NULL;

	if (!buffer && !size)
		return NULL;

	s = calloc(1, sizeof(wStream));
	if (!s)
		return NULL;

	if (buffer)
		s->buffer = buffer;
	else
		s->buffer = (BYTE*)malloc(size);

	if (!s->buffer)
	{
		free(s);
		return NULL;
	}

	s->pointer = s->buffer;
	s->capacity = size;
	s->length = size;

	s->pool = NULL;
	s->count = 1;
	s->isAllocatedStream = TRUE;
	s->isOwner = TRUE;
	return s;
}

wStream* Stream_StaticConstInit(wStream* s, const BYTE* buffer, size_t size)
{
	union
	{
		BYTE* b;
		const BYTE* cb;
	} cnv;

	cnv.cb = buffer;
	return Stream_StaticInit(s, cnv.b, size);
}

wStream* Stream_StaticInit(wStream* s, BYTE* buffer, size_t size)
{
	const wStream empty = { 0 };

	WINPR_ASSERT(s);
	WINPR_ASSERT(buffer);

	*s = empty;
	s->buffer = s->pointer = buffer;
	s->capacity = s->length = size;
	s->pool = NULL;
	s->count = 1;
	s->isAllocatedStream = FALSE;
	s->isOwner = FALSE;
	return s;
}

void Stream_EnsureValidity(wStream* s)
{
	size_t cur = 0;

	STREAM_ASSERT(s);
	STREAM_ASSERT(s->pointer >= s->buffer);

	cur = (size_t)(s->pointer - s->buffer);
	STREAM_ASSERT(cur <= s->capacity);
	STREAM_ASSERT(s->length <= s->capacity);
}

void Stream_Free(wStream* s, BOOL bFreeBuffer)
{
	if (s)
	{
		Stream_EnsureValidity(s);
		if (bFreeBuffer && s->isOwner)
			free(s->buffer);

		if (s->isAllocatedStream)
			free(s);
	}
}

BOOL Stream_SetLength(wStream* _s, size_t _l)
{
	if ((_l) > Stream_Capacity(_s))
	{
		_s->length = 0;
		return FALSE;
	}
	_s->length = _l;
	return TRUE;
}

BOOL Stream_SetPosition(wStream* _s, size_t _p)
{
	if ((_p) > Stream_Capacity(_s))
	{
		_s->pointer = _s->buffer;
		return FALSE;
	}
	_s->pointer = _s->buffer + (_p);
	return TRUE;
}

void Stream_SealLength(wStream* _s)
{
	size_t cur = 0;
	WINPR_ASSERT(_s);
	WINPR_ASSERT(_s->buffer <= _s->pointer);
	cur = (size_t)(_s->pointer - _s->buffer);
	WINPR_ASSERT(cur <= _s->capacity);
	if (cur <= _s->capacity)
		_s->length = cur;
	else
	{
		WLog_FATAL(STREAM_TAG, "wStream API misuse: stream was written out of bounds");
		winpr_log_backtrace(STREAM_TAG, WLOG_FATAL, 20);
		_s->length = 0;
	}
}

#if defined(WITH_WINPR_DEPRECATED)
BOOL Stream_SetPointer(wStream* _s, BYTE* _p)
{
	WINPR_ASSERT(_s);
	if (!_p || (_s->buffer > _p) || (_s->buffer + _s->capacity < _p))
	{
		_s->pointer = _s->buffer;
		return FALSE;
	}
	_s->pointer = _p;
	return TRUE;
}

BOOL Stream_SetBuffer(wStream* _s, BYTE* _b)
{
	WINPR_ASSERT(_s);
	WINPR_ASSERT(_b);

	_s->buffer = _b;
	_s->pointer = _b;
	return _s->buffer != NULL;
}

void Stream_SetCapacity(wStream* _s, size_t _c)
{
	WINPR_ASSERT(_s);
	_s->capacity = _c;
}

#endif

size_t Stream_GetRemainingCapacity(const wStream* _s)
{
	size_t cur = 0;
	WINPR_ASSERT(_s);
	WINPR_ASSERT(_s->buffer <= _s->pointer);
	cur = (size_t)(_s->pointer - _s->buffer);
	WINPR_ASSERT(cur <= _s->capacity);
	if (cur > _s->capacity)
	{
		WLog_FATAL(STREAM_TAG, "wStream API misuse: stream was written out of bounds");
		winpr_log_backtrace(STREAM_TAG, WLOG_FATAL, 20);
		return 0;
	}
	return (_s->capacity - cur);
}

size_t Stream_GetRemainingLength(const wStream* _s)
{
	size_t cur = 0;
	WINPR_ASSERT(_s);
	WINPR_ASSERT(_s->buffer <= _s->pointer);
	WINPR_ASSERT(_s->length <= _s->capacity);
	cur = (size_t)(_s->pointer - _s->buffer);
	WINPR_ASSERT(cur <= _s->length);
	if (cur > _s->length)
	{
		WLog_FATAL(STREAM_TAG, "wStream API misuse: stream was read out of bounds");
		winpr_log_backtrace(STREAM_TAG, WLOG_FATAL, 20);
		return 0;
	}
	return (_s->length - cur);
}

BOOL Stream_Write_UTF16_String(wStream* s, const WCHAR* src, size_t length)
{
	WINPR_ASSERT(s);
	WINPR_ASSERT(src || (length == 0));
	if (!s || !src)
		return FALSE;

	if (!Stream_CheckAndLogRequiredCapacityOfSize(STREAM_TAG, (s), length, sizeof(WCHAR)))
		return FALSE;

	for (size_t x = 0; x < length; x++)
		Stream_Write_UINT16(s, src[x]);

	return TRUE;
}

BOOL Stream_Read_UTF16_String(wStream* s, WCHAR* dst, size_t length)
{
	WINPR_ASSERT(s);
	WINPR_ASSERT(dst);

	if (!Stream_CheckAndLogRequiredLengthOfSize(STREAM_TAG, s, length, sizeof(WCHAR)))
		return FALSE;

	for (size_t x = 0; x < length; x++)
		Stream_Read_UINT16(s, dst[x]);

	return TRUE;
}

BOOL Stream_CheckAndLogRequiredCapacityEx(const char* tag, DWORD level, wStream* s, size_t nmemb,
                                          size_t size, const char* fmt, ...)
{
	WINPR_ASSERT(size != 0);
	const size_t actual = Stream_GetRemainingCapacity(s) / size;

	if (actual < nmemb)
	{
		va_list args;

		va_start(args, fmt);
		Stream_CheckAndLogRequiredCapacityExVa(tag, level, s, nmemb, size, fmt, args);
		va_end(args);

		return FALSE;
	}
	return TRUE;
}

BOOL Stream_CheckAndLogRequiredCapacityExVa(const char* tag, DWORD level, wStream* s, size_t nmemb,
                                            size_t size, const char* fmt, va_list args)
{
	WINPR_ASSERT(size != 0);
	const size_t actual = Stream_GetRemainingCapacity(s) / size;

	if (actual < nmemb)
		return Stream_CheckAndLogRequiredCapacityWLogExVa(WLog_Get(tag), level, s, nmemb, size, fmt,
		                                                  args);
	return TRUE;
}

WINPR_ATTR_FORMAT_ARG(6, 0)
BOOL Stream_CheckAndLogRequiredCapacityWLogExVa(wLog* log, DWORD level, wStream* s, size_t nmemb,
                                                size_t size, WINPR_FORMAT_ARG const char* fmt,
                                                va_list args)
{

	WINPR_ASSERT(size != 0);
	const size_t actual = Stream_GetRemainingCapacity(s) / size;

	if (actual < nmemb)
	{
		char prefix[1024] = { 0 };

		(void)vsnprintf(prefix, sizeof(prefix), fmt, args);

		WLog_Print(log, level,
		           "[%s] invalid remaining capacity, got %" PRIuz ", require at least %" PRIu64
		           " [element size=%" PRIuz "]",
		           prefix, actual, nmemb, size);
		winpr_log_backtrace_ex(log, level, 20);
		return FALSE;
	}
	return TRUE;
}

WINPR_ATTR_FORMAT_ARG(6, 7)
BOOL Stream_CheckAndLogRequiredCapacityWLogEx(wLog* log, DWORD level, wStream* s, size_t nmemb,
                                              size_t size, WINPR_FORMAT_ARG const char* fmt, ...)
{

	WINPR_ASSERT(size != 0);
	const size_t actual = Stream_GetRemainingCapacity(s) / size;

	if (actual < nmemb)
	{
		va_list args;

		va_start(args, fmt);
		Stream_CheckAndLogRequiredCapacityWLogExVa(log, level, s, nmemb, size, fmt, args);
		va_end(args);

		return FALSE;
	}
	return TRUE;
}

WINPR_ATTR_FORMAT_ARG(6, 7)
BOOL Stream_CheckAndLogRequiredLengthEx(const char* tag, DWORD level, wStream* s, size_t nmemb,
                                        size_t size, WINPR_FORMAT_ARG const char* fmt, ...)
{
	WINPR_ASSERT(size > 0);
	const size_t actual = Stream_GetRemainingLength(s) / size;

	if (actual < nmemb)
	{
		va_list args;

		va_start(args, fmt);
		Stream_CheckAndLogRequiredLengthExVa(tag, level, s, nmemb, size, fmt, args);
		va_end(args);

		return FALSE;
	}
	return TRUE;
}

BOOL Stream_CheckAndLogRequiredLengthExVa(const char* tag, DWORD level, wStream* s, size_t nmemb,
                                          size_t size, const char* fmt, va_list args)
{
	WINPR_ASSERT(size > 0);
	const size_t actual = Stream_GetRemainingLength(s) / size;

	if (actual < nmemb)
		return Stream_CheckAndLogRequiredLengthWLogExVa(WLog_Get(tag), level, s, nmemb, size, fmt,
		                                                args);
	return TRUE;
}

BOOL Stream_CheckAndLogRequiredLengthWLogEx(wLog* log, DWORD level, wStream* s, size_t nmemb,
                                            size_t size, const char* fmt, ...)
{
	WINPR_ASSERT(size > 0);
	const size_t actual = Stream_GetRemainingLength(s) / size;

	if (actual < nmemb)
	{
		va_list args;

		va_start(args, fmt);
		Stream_CheckAndLogRequiredLengthWLogExVa(log, level, s, nmemb, size, fmt, args);
		va_end(args);

		return FALSE;
	}
	return TRUE;
}

WINPR_ATTR_FORMAT_ARG(6, 0)
BOOL Stream_CheckAndLogRequiredLengthWLogExVa(wLog* log, DWORD level, wStream* s, size_t nmemb,
                                              size_t size, WINPR_FORMAT_ARG const char* fmt,
                                              va_list args)
{
	WINPR_ASSERT(size > 0);
	const size_t actual = Stream_GetRemainingLength(s) / size;

	if (actual < nmemb)
	{
		char prefix[1024] = { 0 };

		(void)vsnprintf(prefix, sizeof(prefix), fmt, args);

		WLog_Print(log, level,
		           "[%s] invalid length, got %" PRIuz ", require at least %" PRIuz
		           " [element size=%" PRIuz "]",
		           prefix, actual, nmemb, size);
		winpr_log_backtrace_ex(log, level, 20);
		return FALSE;
	}
	return TRUE;
}

SSIZE_T Stream_Write_UTF16_String_From_UTF8(wStream* s, size_t wcharLength, const char* src,
                                            size_t length, BOOL fill)
{
	SSIZE_T rc = 0;
	WCHAR* str = Stream_PointerAs(s, WCHAR);

	if (length != 0)
	{
		if (!Stream_CheckAndLogRequiredCapacityOfSize(STREAM_TAG, s, wcharLength, sizeof(WCHAR)))
			return -1;

		rc = ConvertUtf8NToWChar(src, length, str, wcharLength);
		if (rc < 0)
			return -1;

		Stream_Seek(s, (size_t)rc * sizeof(WCHAR));
	}

	if (fill)
		Stream_Zero(s, (wcharLength - (size_t)rc) * sizeof(WCHAR));
	return rc;
}

char* Stream_Read_UTF16_String_As_UTF8(wStream* s, size_t wcharLength, size_t* pUtfCharLength)
{
	const WCHAR* str = Stream_ConstPointer(s);
	if (wcharLength > SIZE_MAX / sizeof(WCHAR))
		return NULL;

	if (!Stream_CheckAndLogRequiredLength(STREAM_TAG, s, wcharLength * sizeof(WCHAR)))
		return NULL;

	Stream_Seek(s, wcharLength * sizeof(WCHAR));
	return ConvertWCharNToUtf8Alloc(str, wcharLength, pUtfCharLength);
}

SSIZE_T Stream_Read_UTF16_String_As_UTF8_Buffer(wStream* s, size_t wcharLength, char* utfBuffer,
                                                size_t utfBufferCharLength)
{
	const WCHAR* ptr = Stream_ConstPointer(s);
	if (wcharLength > SIZE_MAX / sizeof(WCHAR))
		return -1;

	if (!Stream_CheckAndLogRequiredLength(STREAM_TAG, s, wcharLength * sizeof(WCHAR)))
		return -1;

	Stream_Seek(s, wcharLength * sizeof(WCHAR));
	return ConvertWCharNToUtf8(ptr, wcharLength, utfBuffer, utfBufferCharLength);
}

BOOL Stream_SafeSeekEx(wStream* s, size_t size, const char* file, size_t line, const char* fkt)
{
	if (!Stream_CheckAndLogRequiredLengthEx(STREAM_TAG, WLOG_WARN, s, size, 1, "%s(%s:%" PRIuz ")",
	                                        fkt, file, line))
		return FALSE;

	Stream_Seek(s, size);
	return TRUE;
}
