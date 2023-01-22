#pragma once

#define _CRT_SECURE_NO_WARNINGS 1
//#include <iostream>
#include <Windows.h>
//#include <conio.h>
//#include <spdlog/spdlog.h>
//#include <fmt/xchar.h>
#include <lazy_importer.hpp>
#include "XorString.h"

void* __cdecl memset(
	_Out_writes_bytes_all_(_Size) void* _Dst,
	_In_                          int    _Val,
	_In_                          size_t _Size
)
{
	register unsigned char* ptr = (unsigned char*)_Dst;
	while (_Size-- > 0)
		*ptr++ = _Val;
	return _Dst;
}
