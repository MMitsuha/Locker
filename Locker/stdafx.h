#pragma once
#define _NTSCSI_USER_MODE_ TRUE
#include <iostream>
#include <Windows.h>
#include <conio.h>
#include <spdlog/spdlog.h>
#include <fmt/xchar.h>
#include <scsi.h>
#include <ntddscsi.h>
#include <winioctl.h>