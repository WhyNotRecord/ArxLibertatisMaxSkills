/*
 * Copyright 2015-2021 Arx Libertatis Team (see the AUTHORS file)
 *
 * This file is part of Arx Libertatis.
 *
 * Arx Libertatis is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Arx Libertatis is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Arx Libertatis.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "platform/WindowsUtils.h"

#include <algorithm>
#include <sstream>

#include <wchar.h>

#include <boost/algorithm/string/trim.hpp>

#include "io/fs/FilePath.h"

namespace platform {

void WideString::assign(std::string_view utf8, size_t offset) {
	if(!dynamic() && offset + utf8.size() <= capacity()) {
		// Optimistically assume that the wide length is not longer than the utf8 length
		INT length = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), utf8.size(), m_static + offset, capacity() - offset);
		if(length || utf8.empty()) {
			m_static[offset + length] = L'\0';
			m_size = offset + length;
			return;
		}
	}
	INT length = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), utf8.size(), nullptr, 0);
	allocate(offset + length);
	MultiByteToWideChar(CP_UTF8, 0, utf8.data(), utf8.size(), data() + offset, length);
}

void WideString::assign(const WCHAR * text, size_t length, size_t offset) {
	allocate(offset + length);
	std::copy(text, text + length, data() + offset);
}

void WideString::allocateDynamic(size_t size) {
	if(dynamic()) {
		str().resize(size, L'\0');
	} else {
		WCHAR backup[ARRAY_SIZE(m_static)];
		std::copy(m_static, m_static + m_size, backup);
		new(reinterpret_cast<char *>(&m_dynamic)) DynamicType(size, L'\0');
		std::copy(backup, backup + m_size, str().begin());
		m_size = size_t(-1);
	}
}

void WideString::reserve(size_t futureSize) {
	if(futureSize > capacity()) {
		if(!dynamic()) {
			allocateDynamic(size());
		}
		str().reserve(futureSize);
	}
}

void WideString::allocate(size_t size) {
	if(!dynamic() && size <= capacity()) {
		m_static[size] = L'\0';
		m_size = size;
	} else {
		allocateDynamic(size);
	}
}

void WideString::resize(size_t newSize) {
	size_t oldSize = size();
	if(newSize != oldSize) {
		allocate(newSize);
		if(!dynamic() && newSize > oldSize) {
			std::fill(m_static + oldSize, m_static + newSize, L'\0');
		}
	}
}

void WideString::compact() {
	resize(wcslen(c_str()));
}

std::string WideString::toUTF8(const WCHAR * string, size_t length) {
	std::string utf8(length, '\0');
	// Optimistically assume that the utf8 length is not longer than the wide length
	INT utf8_length = WideCharToMultiByte(CP_UTF8, 0, string, length, utf8.data(), length, 0, 0);
	if(utf8_length || !length) {
		utf8.resize(utf8_length);
	} else {
		// Our assumption failed - /
		utf8_length = WideCharToMultiByte(CP_UTF8, 0, string, length, 0, 0, 0, 0);
		utf8.resize(utf8_length, '\0');
		WideCharToMultiByte(CP_UTF8, 0, string, length, utf8.data(), utf8_length, 0, 0);
	}
	return utf8;
}

std::string WideString::toUTF8(const WCHAR * string) {
	return toUTF8(string, wcslen(string));
}

std::string WideString::toUTF8() const {
	return toUTF8(c_str(), length());
}

void WinPath::assign(const fs::path & path) {
	
	const WCHAR * prefix = L"\\\\?\\";
	
	if(path.is_absolute() && path.string().length() > MAX_PATH) {
		reserve(path.string().length() + 4);
		WideString::assign(prefix);
		append(path.string());
	} else {
		WideString::assign(path.string());
		if(path.is_absolute() && size() > MAX_PATH) {
			allocate(size() + 4);
			std::memmove(data() + 4, data(), size() * sizeof(WCHAR));
			std::copy(prefix, prefix + 4, data());
		}
	}
	
}

std::string getErrorString(DWORD error, HMODULE module) {
	
	DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS;
	if(module) {
		flags |= FORMAT_MESSAGE_FROM_HMODULE;
	} else {
		flags |= FORMAT_MESSAGE_FROM_SYSTEM;
	}
	
	LPWSTR buffer = nullptr;
	DWORD n = FormatMessageW(flags, module, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
	                         reinterpret_cast<LPWSTR>(&buffer), 0, nullptr);
	if(n != 0) {
		std::string message = WideString::toUTF8(buffer, n);
		boost::trim(message);
		LocalFree(buffer);
		return message;
	} else {
		std::ostringstream oss;
		oss << "Unknown error (" << error << ").";
		return oss.str();
	}
	
}

std::optional<DWORD> getRegistryDWORD(HKEY key, const WCHAR * name) {
	
	DWORD buffer;
	
	DWORD type = 0;
	DWORD length = sizeof(buffer);
	long ret = RegQueryValueExW(key, name, nullptr, &type, reinterpret_cast<LPBYTE>(&buffer), &length);
	
	if(ret == ERROR_SUCCESS && type == REG_DWORD) {
		return buffer;
	} else {
		return { };
	}
	
}

std::optional<std::string> getRegistryString(HKEY key, const WCHAR * name) {
	
	platform::WideString buffer;
	buffer.allocate(buffer.capacity());
	
	DWORD type = 0;
	DWORD length = buffer.size() * sizeof(WCHAR);
	long ret = RegQueryValueExW(key, name, nullptr, &type, reinterpret_cast<LPBYTE>(buffer.data()), &length);
	if(ret == ERROR_MORE_DATA && length > 0) {
		buffer.resize(length / sizeof(WCHAR) + 1);
		ret = RegQueryValueExW(key, name, nullptr, &type, reinterpret_cast<LPBYTE>(buffer.data()), &length);
	}
	
	if(ret == ERROR_SUCCESS && type == REG_SZ) {
		buffer.resize(length / sizeof(WCHAR));
		buffer.compact();
		return buffer.toUTF8();
	} else {
		return { };
	}
	
}

std::optional<std::string> getRegistryValue(HKEY hive, const WCHAR * key, const WCHAR * name, REGSAM flags) {
	
	HKEY handle = 0;
	if(RegOpenKeyEx(hive, key, 0, KEY_QUERY_VALUE | flags, &handle) != ERROR_SUCCESS) {
		return { };
	}
	
	auto result = getRegistryString(handle, name);
	
	RegCloseKey(handle);
	
	return result;
}

WideString getModuleFileName(HMODULE module) {
	
	platform::WideString buffer;
	buffer.allocate(buffer.capacity());
	
	while(true) {
		DWORD size = GetModuleFileNameW(module, buffer.data(), buffer.size());
		if(size < buffer.size()) {
			buffer.resize(size);
			return buffer;
		}
		buffer.allocate(buffer.size() * 2);
	}
	
}

} // namespace platform
