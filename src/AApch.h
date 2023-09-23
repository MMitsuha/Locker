#pragma once

#define _NTSCSI_USER_MODE_
#define _CRT_SECURE_NO_WARNINGS 1
#include "XorString.h"
#include <Windows.h>
#include <lazy_importer.hpp>
#include <ntddscsi.h>
#include <scsi.h>
#include <spdlog/spdlog.h>
#include <winioctl.h>
#include <winternl.h>
