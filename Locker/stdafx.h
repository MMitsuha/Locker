#pragma once

#ifdef _DEBUG
#define SPDLOG_ACTIVE_LEVEL 0
#endif

#define _USE_SCSI
#define _NTSCSI_USER_MODE_
#define _CRT_SECURE_NO_WARNINGS 1
#include <Windows.h>
#include <spdlog/spdlog.h>
#include <lazy_importer.hpp>
#include "XorString.h"
#include <winioctl.h>
#include <scsi.h>
#include <ntddscsi.h>
#include <winternl.h>

#ifndef _DEBUG
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
#endif
